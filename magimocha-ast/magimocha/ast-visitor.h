#pragma once
#include "ast.h"
#include <memory>
namespace tig::magimocha::ast {
	template<class R,class Visitor>
	static constexpr R visit(Visitor& v,std::shared_ptr<typename ast_base> node) {
		switch (node->type()) {
		case leaf_type::declaration_name:
			return v.visit(std::static_pointer_cast<declaration_name>(node));
		case leaf_type::declaration_parameter:
			return v.visit(std::static_pointer_cast<declaration_parameter>(node));
		case leaf_type::declaration_function:
			return v.visit(std::static_pointer_cast<declaration_function>(node));
		case leaf_type::signed_number_literal:
			return v.visit(std::static_pointer_cast<signed_number_literal>(node));
		case leaf_type::unsigned_number_literal:
			return v.visit(std::static_pointer_cast<unsigned_number_literal>(node));
		case leaf_type::floating_literal:
			return v.visit(std::static_pointer_cast<floating_literal>(node));
		case leaf_type::string_literal:
			return v.visit(std::static_pointer_cast<string_literal>(node));
		case leaf_type::apply_function:
			return v.visit(std::static_pointer_cast<apply_function>(node));
		case leaf_type::call_name:
			return v.visit(std::static_pointer_cast<call_name>(node));
		case leaf_type::expression_block:
			return v.visit(std::static_pointer_cast<expression_block>(node));
		case leaf_type::operation:
			return v.visit(std::static_pointer_cast<operation>(node));
		case leaf_type::named_function:
			return v.visit(std::static_pointer_cast<named_function>(node));
		}
		throw std::invalid_argument("");
	}
	template<class R, class Visitor>
	static constexpr R visit(Visitor& v, std::shared_ptr<typename expression> node) {
		switch (node->type()) {
		/*case leaf_type::declaration_name:
			return v.visit(std::static_pointer_cast<declaration_name>(node));*/
		/*case leaf_type::declaration_parameter:
			return v.visit(std::static_pointer_cast<declaration_parameter>(node));*/
		case leaf_type::declaration_function:
			return v.visit(std::static_pointer_cast<declaration_function>(node));
		case leaf_type::signed_number_literal:
			return v.visit(std::static_pointer_cast<signed_number_literal>(node));
		case leaf_type::unsigned_number_literal:
			return v.visit(std::static_pointer_cast<unsigned_number_literal>(node));
		case leaf_type::floating_literal:
			return v.visit(std::static_pointer_cast<floating_literal>(node));
		case leaf_type::string_literal:
			return v.visit(std::static_pointer_cast<string_literal>(node));
		case leaf_type::apply_function:
			return v.visit(std::static_pointer_cast<apply_function>(node));
		case leaf_type::call_name:
			return v.visit(std::static_pointer_cast<call_name>(node));
		case leaf_type::expression_block:
			return v.visit(std::static_pointer_cast<expression_block>(node));
		case leaf_type::operation:
			return v.visit(std::static_pointer_cast<operation>(node));
		case leaf_type::named_function:
			return v.visit(std::static_pointer_cast<named_function>(node));
		}
		throw std::invalid_argument("");
	}
}

