#include "codegen.h"
#include <stack>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "magimocha/ast-walker.h"
#include "../unicode.h"
namespace tig::magimocha::codegen2
{
struct visitor
{
	using R = llvm::Value *;

	std::stack<context> context;

	llvm::LLVMContext &the_context;
	llvm::Module *the_module;
	void scope_in(std::shared_ptr<ast::make_scope> s)
	{
		//context.push(context{s,});
	}
	void scope_out(std::shared_ptr<ast::make_scope> s)
	{
		context.pop();
	}
	R operator()(std::shared_ptr<ast::signed_number_literal> l)
	{
		return llvm::ConstantInt::get(the_context, llvm::APInt(64, l->value(), true));
	}
	R operator()(std::shared_ptr<ast::unsigned_number_literal> l)
	{
		return llvm::ConstantInt::get(the_context, llvm::APInt(64, l->value(), false));
	}
	R operator()(std::shared_ptr<ast::floating_literal> l)
	{
		return llvm::ConstantFP::get(the_context, llvm::APFloat(l->value()));
	}
	R operator()(std::shared_ptr<ast::string_literal> l)
	{
		throw "NIMPL";
	}
	R operator()(std::shared_ptr<ast::call_name> cn)
	{
		auto r = context.top().values->get(context.top().vars->get_deep(cn->value()));
		if (!r)
		{
			throw "ILLEGAL STATE";
		}
		return r;
	}
	R operator()(std::shared_ptr<ast::declaration_variable> dv)
	{
		auto r = ast::walk<R>(*this, (dv->body()));
		r->setName(to_string(dv->name()));
		context.top().values->set(dv, r);
		return r;
	}
	R operator()(std::shared_ptr<ast::declaration_function> df)
	{
	}
	R operator()(std::shared_ptr<ast::apply_function> af)
	{
	}
	R operator()(std::shared_ptr<ast::operation>)
	{
		throw "ILLEGAL STATE";
	}
	R operator()(std::shared_ptr<ast::expression_block> exprs)
	{
	}
	R operator()(std::shared_ptr<ast::named_function> nf)
	{
	}
	R operator()(std::shared_ptr<ast::declaration_infix> di)
	{
		//do nothing
	}
};
} // namespace tig::magimocha::codegen2
int main()
{
}