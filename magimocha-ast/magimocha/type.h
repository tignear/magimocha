#pragma once
#include "ast.h"
namespace tig::magimocha {
	//http://d.hatena.ne.jp/takuto_h/20110401/unify
	void unify(std::shared_ptr<ast::type_data> a, std::shared_ptr<ast::type_data> b) {
		if (a == b) {
			return;
		}
		if (a->type() == ast::type_data_type::var) {
			auto&& var_a=std::static_pointer_cast<ast::var_type_data>(a);
			if (!var_a->type_data) {
				var_a->type_data = b;
			}
			else {
				unify(var_a->type_data, b);
			}
		}
		else if (b->type() == ast::type_data_type::var) {
			auto&& var_b = std::static_pointer_cast<ast::var_type_data>(b);
			if (!var_b->type_data) {
				var_b->type_data = a;
			}
			else {
				unify(a,var_b->type_data);
			}
		}
		else {
			throw "error:unify";
		}
	}
}