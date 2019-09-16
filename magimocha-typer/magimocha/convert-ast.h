#pragma once
#include "magimocha/ast-walker.h"
#include <queue>
namespace tig::magimocha::codegen2 {


/*std::shared_ptr<ast::typed_data>
convert_scope(std::shared_ptr<ast::typed_data> e) { auto vis =
convert_scope_visitor(); return
ast::visit<std::shared_ptr<ast::expression>>(vis, e);
}*/
std::shared_ptr<ast::ast_leaf> convert_scope(std::shared_ptr<ast::ast_leaf> e);
std::shared_ptr<ast::module_member>
convert_scope(std::shared_ptr<ast::module_member> e);
std::shared_ptr<ast::expression>
convert_scope(std::shared_ptr<ast::expression> e);
template <class T> std::shared_ptr<T> convert_scope(std::shared_ptr<T> e) {
    return convert_scope_visitor()(e);
}
struct convert_scope_operation_visitor {
    using R = ast::operator_token_type;
    template <class T> R operator()(std::shared_ptr<T> e) {
        return convert_scope_visitor()(e);
    }
    R operator()(std::shared_ptr<ast::literal_> e) { return e; }
    R operator()(std::shared_ptr<ast::expression> e) {
        return convert_scope(e);
    }
    R operator()(ast::op_token_single e) { return e; }
    R operator()(ast::op_token_double e) { return e; }
    R operator()(ast::op_token_function_applying e) {
        std::vector<std::shared_ptr<ast::expression>> nargs;
        for(auto arg : e.args()) {
            nargs.push_back(arg);
        }
        return ast::op_token_function_applying(nargs);
    }
};
struct convert_scope_visitor {

    std::shared_ptr<ast::declaration_function>
    operator()(std::shared_ptr<ast::declaration_function> e) {
        return std::make_shared<ast::declaration_function>(
            e->params(), e->return_type_func(), convert_scope(e->body()));
    }
    std::shared_ptr<ast::signed_number_literal>
    operator()(std::shared_ptr<ast::signed_number_literal> e) {
        return e;
    }
    std::shared_ptr<ast::unsigned_number_literal>
    operator()(std::shared_ptr<ast::unsigned_number_literal> e) {
        return e;
    }
    std::shared_ptr<ast::floating_literal>
    operator()(std::shared_ptr<ast::floating_literal> e) {
        return e;
    }
    std::shared_ptr<ast::string_literal>
    operator()(std::shared_ptr<ast::string_literal> e) {
        return e;
    }
    std::shared_ptr<ast::apply_function>
    operator()(std::shared_ptr<ast::apply_function> e) {
        std::vector<std::shared_ptr<ast::expression>> nargs;
        for(auto arg : e->args()) {
            nargs.push_back(convert_scope(arg));
        }
        return std::make_shared<ast::apply_function>(convert_scope(e->target()),
                                                     nargs);
    }
    std::shared_ptr<ast::call_name>
    operator()(std::shared_ptr<ast::call_name> e) {
        return e;
    }
    std::shared_ptr<ast::expression_block>
    operator()(std::shared_ptr<ast::expression_block> e) {
        std::vector<std::shared_ptr<ast::expression>> nvalue;
        const auto &value = e->value();
        process_expression_block(begin(value), end(value), nvalue);
        return std::make_shared<ast::expression_block>(nvalue);
    }
    void process_expression_block(
        std::vector<std::shared_ptr<ast::expression>>::const_iterator s,
        std::vector<std::shared_ptr<ast::expression>>::const_iterator e,
        std::vector<std::shared_ptr<ast::expression>> &r) {
        for(auto itr = s; itr != e; ++itr) {
            r.push_back(convert_scope(*itr));
            auto ty = (*itr)->type();
            if(ty == ast::leaf_type::declaration_variable ||
               ty == ast::leaf_type::declaration_infix) {
                std::vector<std::shared_ptr<ast::expression>> r2;
                process_expression_block(next(itr), e, r2);
                r.push_back(std::make_shared<ast::expression_block>(r2));
                return;
            }
        }
    }
    std::shared_ptr<ast::operation>
    operator()(std::shared_ptr<ast::operation> e) {
        std::list<ast::operator_token_type> nvalue;
        for(auto v : e->value()) {
            nvalue.push_back(std::visit(convert_scope_operation_visitor(), v));
        }
        return std::make_shared<ast::operation>(nvalue);
    }
    std::shared_ptr<ast::named_function>
    operator()(std::shared_ptr<ast::named_function> e) {
        return std::make_shared<ast::named_function>(e->name(),
                                                     (*this)(e->body()));
    }
    std::shared_ptr<ast::declaration_variable>
    operator()(std::shared_ptr<ast::declaration_variable> e) {
        return std::make_shared<ast::declaration_variable>(
            e->name(), e->return_type(), convert_scope(e->body()));
    }
    std::shared_ptr<ast::declaration_parameter>
    operator()(std::shared_ptr<ast::declaration_parameter> e) {
        return e;
    }
    std::shared_ptr<ast::declaration_infix>
    operator()(std::shared_ptr<ast::declaration_infix> e) {
        return e;
    }
    std::shared_ptr<ast::declaration_module>
    operator()(std::shared_ptr<ast::declaration_module> e) {
        std::vector<std::shared_ptr<ast::module_member>> nmembers;
        for(auto member : e->members()) {
            nmembers.push_back(convert_scope(member));
        }
        return std::make_shared<ast::declaration_module>(e->name(), nmembers);
    }
    std::shared_ptr<ast::declaration_export>
    operator()(std::shared_ptr<ast::declaration_export> e) {
        return e;
    }
};
std::shared_ptr<ast::ast_leaf> convert_scope(std::shared_ptr<ast::ast_leaf> e) {
    auto vis = convert_scope_visitor();
    return ast::visit<std::shared_ptr<ast::ast_leaf>>(vis, e);
}
std::shared_ptr<ast::module_member>
convert_scope(std::shared_ptr<ast::module_member> e) {
    auto vis = convert_scope_visitor();
    return ast::visit<std::shared_ptr<ast::module_member>>(vis, e);
}
std::shared_ptr<ast::expression>
convert_scope(std::shared_ptr<ast::expression> e) {
    auto vis = convert_scope_visitor();
    return ast::visit<std::shared_ptr<ast::expression>>(vis, e);
}
} // namespace tig::magimocha::codegen2