#include "codegen.h"
#include "../unicode.h"
#include "magimocha/ast-walker.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include <stack>
namespace {
using namespace tig::magimocha;
using namespace tig::magimocha::codegen2;
struct codegen_visitor {
    using R = llvm::Value *;

    std::deque<context> context;
    std::shared_ptr<typing::type_table> types;
    std::shared_ptr<typing::type_schema_table> schemas;
    std::shared_ptr<typing::make_scope_2_variable_table_table>
        make_scope_2_variable_table_table;
    std::shared_ptr<llvm_values> values;

    llvm::LLVMContext &the_context;
    llvm::IRBuilder<> builder;
    llvm::Module *the_module;
    codegen_visitor(std::shared_ptr<typing::type_table> types,
                    std::shared_ptr<typing::type_schema_table> schemas,
                    std::shared_ptr<typing::make_scope_2_variable_table_table>
                        make_scope_2_variable_table_table,
                    std::shared_ptr<llvm_values> values,
                    llvm::LLVMContext &the_context, llvm::Module *the_module)
        : types(types), schemas(schemas),
          make_scope_2_variable_table_table(make_scope_2_variable_table_table),
          values(values), the_context(the_context), builder(the_context),
          the_module(the_module) {}

    std::string get_name() {
        return context.empty() ? "" : context.back().name;
    }
    std::string get_name(const std::u32string &s) { return to_string(s); }

    void scope_in_base(std::shared_ptr<ast::make_scope> s, std::string name) {
        auto vars = make_scope_2_variable_table_table->get(s);
        context.push_back(codegen2::context{
            s, vars ? vars : name::create_variable_table(context.back().vars), name});
    }
    void scope_in(std::shared_ptr<ast::declaration_function> df) {
        scope_in_base(df, get_name() + "." +
                              std::to_string(++(
                                  context.back().count_of_anonymous_function)));
    }
    void scope_in(std::shared_ptr<ast::expression_block> eb) {
        scope_in_base(eb, get_name() + "." +
                              std::to_string(++(
                                  context.back().count_of_expression_block)));
    }
    void scope_in(std::shared_ptr<ast::declaration_module> dm) {
        scope_in_base(dm, get_name() + "." + to_string(dm->name()));
    }

    void scope_in(std::shared_ptr<ast::named_function> nf) {
        scope_in_base(nf, get_name() + "." + to_string(nf->name()));
    }
    template <class T> std::string mangling(const T &src) {}
    void scope_out(std::shared_ptr<ast::make_scope> s) { context.pop_back(); }
    llvm::FunctionType *
    get_llvm_type(std::shared_ptr<ast::function_type_data> t) {
        auto rt = std::static_pointer_cast<ast::function_type_data>(t);
        std::vector<llvm::Type *> nargs;
        for(auto arg : rt->args) {
            nargs.push_back(get_llvm_type(arg));
        }
        return llvm::FunctionType::get(get_llvm_type(rt->result_type), nargs,
                                       false);
    }
    llvm::Type *get_llvm_type(std::shared_ptr<ast::type_data> t) {
        switch(t->type()) {
        case ast::type_data_type::var:
            if(auto r = types->find(
                   std::static_pointer_cast<ast::var_type_data>(t))) {
                return get_llvm_type(r->get());
            }
            throw "undefined type";
        case ast::type_data_type::function:
            return get_llvm_type(
                std::static_pointer_cast<ast::function_type_data>(t));
        case ast::type_data_type::simple:
            const auto &v =
                std::static_pointer_cast<ast::simple_type_data>(t)->value();
            if(v == U"Int64") {
                return llvm::Type::getInt64Ty(the_context);
            } else if(v == U"UInt64") {
                return llvm::Type::getInt64Ty(the_context);
            } else if(v == U"Double") {
                return llvm::Type::getDoubleTy(the_context);
            }
        }
    }
    R operator()(std::shared_ptr<ast::signed_number_literal> l) {
        return llvm::ConstantInt::get(the_context,
                                      llvm::APInt(64, l->value(), true));
    }
    R operator()(std::shared_ptr<ast::unsigned_number_literal> l) {
        return llvm::ConstantInt::get(the_context,
                                      llvm::APInt(64, l->value(), false));
    }
    R operator()(std::shared_ptr<ast::floating_literal> l) {
        return llvm::ConstantFP::get(the_context, llvm::APFloat(l->value()));
    }
    R operator()(std::shared_ptr<ast::string_literal> l) { throw "NIMPL"; }
    R operator()(std::shared_ptr<ast::call_name> cn) {
        auto v = context.back().vars->get_deep(cn->value());
        auto r = values->get(v);
        if(!r) {
            throw std::exception("ILLEGAL STATE");
        }
        return r;
    }
    R operator()(std::shared_ptr<ast::declaration_variable> dv) {
        auto r = ast::walk<R>(*this, (dv->body()));
        r->setName(to_string(dv->name()));
        context.back().vars->find_shallow(dv->name())->set(dv);
        values->set(dv, r);
        return r;
    }
    R operator()(std::shared_ptr<ast::declaration_function> df) {
        auto ty = get_llvm_type(df->return_type_func());
        auto f = llvm::Function::Create(ty, llvm::Function::ExternalLinkage,
                                        get_name(), the_module);
        process_declaration_function(f, df);
        return f;
    }
    R compiler_magic(std::shared_ptr<ast::apply_function> af) {
        if(af->target()->type() != ast::leaf_type::call_name) {
            return nullptr;
        }
        auto tcn = std::static_pointer_cast<ast::call_name>(af->target());
        const auto &name = tcn->value();
        const auto &args = af->args();
        if(name == U"+") {
            return builder.CreateFAdd(ast::walk<R>(*this, args.at(0)),
                                      ast::walk<R>(*this, args.at(1)));
        }
        if(name == U"*") {
            return builder.CreateFMul(ast::walk<R>(*this, args.at(0)),
                                      ast::walk<R>(*this, args.at(1)));
        }
        return nullptr;
    }
    R operator()(std::shared_ptr<ast::apply_function> af) {
        if(auto v = compiler_magic(af)) {
            return v;
        }
        auto target = ast::walk<R>(*this, af->target());
        std::vector<llvm::Value *> cargs;
        for(auto arg : af->args()) {
            cargs.push_back(ast::walk<R>(*this, arg));
        }
        return builder.CreateCall(target, cargs);
    }
    R operator()(std::shared_ptr<ast::operation>) { throw "ILLEGAL STATE"; }
    R operator()(std::shared_ptr<ast::expression_block> exprs) {
        llvm::Value *last;
        for(auto expr : exprs->value()) {
            auto l = ast::walk<R>(*this, expr);
            last = l ? l : last;
        }
        return last;
    }
    R operator()(std::shared_ptr<ast::named_function> nf) {
        auto ty = get_llvm_type(nf->return_type_func());
        auto f = llvm::Function::Create(ty, llvm::Function::ExternalLinkage,
                                        get_name(), the_module);
        context.back().vars->find_shallow(nf->name())->set(nf);
        process_declaration_function(f, nf->body());
        values->set(nf,f);
        return f;
    }
    void process_declaration_function(
        llvm::Function *f, std::shared_ptr<ast::declaration_function> df) {
        const auto &params = df->params();
        auto i = 0;
        for(llvm::Function::arg_iterator ai = f->arg_begin();
            i != params.size(); ++ai, ++i) {
            auto param = params.at(i);
            values->set(param, ai);
            if(auto name = param->name()) {
                context.back().vars->find_shallow(*name)->set(param);
                ai->setName(get_name(*name));
            }
        }
        llvm::BasicBlock *bb =
            llvm::BasicBlock::Create(the_context, "alpha", f);
        builder.SetInsertPoint(bb);
        auto ret = ast::walk<R>(*this, df->body());
        builder.CreateRet(ret);
        llvm::verifyFunction(*f);
    }
    R operator()(std::shared_ptr<ast::declaration_infix> di) { return nullptr; }
    R operator()(std::shared_ptr<ast::declaration_export> de) {
        return nullptr;
    }
    R operator()(std::shared_ptr<ast::declaration_module> dm) {
        for(auto e : dm->members()) {
            ast::walk<R>(*this, e);
        }
        return nullptr;
    }
};

} // namespace
namespace tig::magimocha::codegen2 {
void codegen(std::shared_ptr<ast::declaration_module> mod,
             std::shared_ptr<typing::type_table> types,
             std::shared_ptr<typing::type_schema_table> schemas,
             std::shared_ptr<typing::make_scope_2_variable_table_table>
                 make_scope_2_variable_table_table,
             std::shared_ptr<llvm_values> values,
             llvm::LLVMContext &the_context, llvm::Module *the_module) {
    auto walker =
        codegen_visitor(types, schemas, make_scope_2_variable_table_table,
                        values, the_context, the_module);
    walker.scope_in(mod);
    auto r = walker(mod);
    walker.scope_out(mod);
}
} // namespace tig::magimocha::codegen2