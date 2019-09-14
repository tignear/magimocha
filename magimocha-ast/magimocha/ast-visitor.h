#pragma once
#include "ast.h"
#include <memory>
namespace tig::magimocha::ast {
	template<class R, class Visitor>
	static R visit(Visitor& v, std::shared_ptr<expression> node) {
		switch (node->type()) {
		case leaf_type::declaration_function:
			return v(std::static_pointer_cast<declaration_function>(node));
		case leaf_type::signed_number_literal:
			return v(std::static_pointer_cast<signed_number_literal>(node));
		case leaf_type::unsigned_number_literal:
			return v(std::static_pointer_cast<unsigned_number_literal>(node));
		case leaf_type::floating_literal:
			return v(std::static_pointer_cast<floating_literal>(node));
		case leaf_type::string_literal:
			return v(std::static_pointer_cast<string_literal>(node));
		case leaf_type::apply_function:
			return v(std::static_pointer_cast<apply_function>(node));
		case leaf_type::call_name:
			return v(std::static_pointer_cast<call_name>(node));
		case leaf_type::expression_block:
			return v(std::static_pointer_cast<expression_block>(node));
		case leaf_type::operation:
			return v(std::static_pointer_cast<operation>(node));
		case leaf_type::named_function:
			return v(std::static_pointer_cast<named_function>(node));
		case leaf_type::declaration_variable:
			return v(std::static_pointer_cast<declaration_variable>(node));
		case leaf_type::declaration_infix:
			return v(std::static_pointer_cast<declaration_infix>(node));
		}
		throw std::invalid_argument("");
	}
	template<class R, class Visitor>
	static R visit(Visitor& v, std::shared_ptr<module_member> node) {
		switch (node->type()) {
		case leaf_type::declaration_module:
			return v(std::static_pointer_cast<declaration_module>(node));
		case leaf_type::declaration_variable:
			return v(std::static_pointer_cast<declaration_variable>(node));
		case leaf_type::declaration_export:
			return v(std::static_pointer_cast<declaration_export>(node));
		case leaf_type::named_function:
			return v(std::static_pointer_cast<named_function>(node));
		case leaf_type::declaration_infix:
			return v(std::static_pointer_cast<declaration_infix>(node));

		}
		throw std::invalid_argument("");
	}
	template<class R, class Visitor>
	static R visit(Visitor& v, std::shared_ptr<typed_data> node) {
		switch (node->type()) {
		case leaf_type::declaration_function:
			return v(std::static_pointer_cast<declaration_function>(node));
		case leaf_type::signed_number_literal:
			return v(std::static_pointer_cast<signed_number_literal>(node));
		case leaf_type::unsigned_number_literal:
			return v(std::static_pointer_cast<unsigned_number_literal>(node));
		case leaf_type::floating_literal:
			return v(std::static_pointer_cast<floating_literal>(node));
		case leaf_type::string_literal:
			return v(std::static_pointer_cast<string_literal>(node));
		case leaf_type::apply_function:
			return v(std::static_pointer_cast<apply_function>(node));
		case leaf_type::call_name:
			return v(std::static_pointer_cast<call_name>(node));
		case leaf_type::expression_block:
			return v(std::static_pointer_cast<expression_block>(node));
		case leaf_type::operation:
			return v(std::static_pointer_cast<operation>(node));
		case leaf_type::named_function:
			return v(std::static_pointer_cast<named_function>(node));
		case leaf_type::declaration_variable:
			return v(std::static_pointer_cast<declaration_variable>(node));
		case leaf_type::declaration_parameter:
			return v(std::static_pointer_cast<declaration_parameter>(node));
		case leaf_type::declaration_infix:
			return v(std::static_pointer_cast<declaration_infix>(node));

		}
		throw std::invalid_argument("");
	}
	template<class R, class Visitor>
	static R visit(Visitor& v, std::shared_ptr<ast_leaf> node) {
		switch (node->type()) {
		case leaf_type::declaration_function:
			return v(std::static_pointer_cast<declaration_function>(node));
		case leaf_type::signed_number_literal:
			return v(std::static_pointer_cast<signed_number_literal>(node));
		case leaf_type::unsigned_number_literal:
			return v(std::static_pointer_cast<unsigned_number_literal>(node));
		case leaf_type::floating_literal:
			return v(std::static_pointer_cast<floating_literal>(node));
		case leaf_type::string_literal:
			return v(std::static_pointer_cast<string_literal>(node));
		case leaf_type::apply_function:
			return v(std::static_pointer_cast<apply_function>(node));
		case leaf_type::call_name:
			return v(std::static_pointer_cast<call_name>(node));
		case leaf_type::expression_block:
			return v(std::static_pointer_cast<expression_block>(node));
		case leaf_type::operation:
			return v(std::static_pointer_cast<operation>(node));
		case leaf_type::named_function:
			return v(std::static_pointer_cast<named_function>(node));
		case leaf_type::declaration_variable:
			return v(std::static_pointer_cast<declaration_variable>(node));
		case leaf_type::declaration_parameter:
			return v(std::static_pointer_cast<declaration_parameter>(node));
		case leaf_type::declaration_infix:
			return v(std::static_pointer_cast<declaration_infix>(node));
		case leaf_type::declaration_module:
			return v(std::static_pointer_cast<declaration_module>(node));
		case leaf_type::declaration_export:
			return v(std::static_pointer_cast<declaration_export>(node));
		}
		throw std::invalid_argument("");
	}
	struct to_ast_leaf {
		std::shared_ptr<ast_leaf> s;
		to_ast_leaf(std::shared_ptr<ast::expression> s) {
			ast::visit<std::monostate>(*this,s);
		}
		to_ast_leaf(std::shared_ptr<ast::module_member> s) {
			ast::visit<std::monostate>(*this, s);
		}
		to_ast_leaf(std::shared_ptr<ast::literal_> s) {
			ast::visit<std::monostate>(*this, std::static_pointer_cast<ast::expression>(s));
		}
		template<class T>
		to_ast_leaf(std::shared_ptr<T> s) :s(s) {
		}
		std::shared_ptr<ast_leaf> operator()()const {
			return s;
		}
		operator std::shared_ptr<ast_leaf>()const {
			return s;
		}

		template<class U>
		std::monostate operator()(std::shared_ptr<U> u) {
			s = u;
			return std::monostate{};
		}
	};
}

