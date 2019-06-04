#include "llvm-type.h"
namespace tig::magimocha::codegen {
	std::optional<std::vector<llvm::Type*>> type2llvmType(Scope* s, std::vector<std::shared_ptr<ast::type_data>> tys) {
		std::vector<llvm::Type*> r;
		for (auto&& ty : tys) {
			auto t = type2llvmType(s, ty);
			if (!t) {
				return std::nullopt;
			}
			r.push_back(t);
		}
		return r;
	}
	llvm::FunctionType* type2llvmType(Scope* s, std::shared_ptr<ast::function_type_data> ty) {
		auto rt = type2llvmType(s, ty->result_type);
		if (!rt) {
			return nullptr;
		}
		auto argst = type2llvmType(s, ty->args);
		if (!argst) {
			return nullptr;
		}
		return llvm::FunctionType::get(rt,argst.value() , false);

	}
	llvm::Type* type2llvmType(Scope* s, std::shared_ptr<ast::type_data> ty) {
		if (ty->type() == ast::type_data_type::simple) {
			auto&& ty_simple = std::static_pointer_cast<ast::simple_type_data>(ty);
			auto&& v = ty_simple->value();
			if (v == U"double") {
				return llvm::Type::getDoubleTy(TheContext);
			}
			else {
				throw "not impl";
			}
		}
		else if (ty->type() == ast::type_data_type::var) {
			auto&& ty_var = std::static_pointer_cast<ast::var_type_data>(ty);
			auto&& vv = s->getTypeData(ty_var);
			if (vv){
				return type2llvmType(s,vv);
			}
			else {
				return nullptr;
			}
		}
		else if (ty->type() == ast::type_data_type::function) {
			auto&& ty_func = std::static_pointer_cast<ast::function_type_data>(ty);
			return type2llvmType(s,ty_func);
		}
		else {
			throw "not impl";
		}
	}
	void unify(Scope* s, std::shared_ptr<ast::type_data> a, std::shared_ptr<ast::type_data> b) {
		if (a == b) {
			return;
		}
		if (a->type() == ast::type_data_type::simple&&
			b->type() == ast::type_data_type::simple&&
			std::static_pointer_cast<ast::simple_type_data>(a)->value() == std::static_pointer_cast<ast::simple_type_data>(b)->value()) {
			return;
		}
		if (a->type() == ast::type_data_type::var) {
			auto&& var_a = std::static_pointer_cast<ast::var_type_data>(a);
			auto&& tda = s->getTypeData(var_a);
			if (!tda) {
				s->addTypeData(var_a, b);
			}
			else {
				unify(s, tda, b);
			}
		}
		else if (b->type() == ast::type_data_type::var) {
			auto&& var_b = std::static_pointer_cast<ast::var_type_data>(b);
			auto&& tdb = s->getTypeData(var_b);

			if (!tdb) {
				s->addTypeData(var_b, tdb);
			}
			else {
				unify(s, a, tdb);
			}
		}
		else {
			throw "error:unify";
		}
	}
	std::shared_ptr<ast::type_data> resolve_type(Scope* s,std::shared_ptr<ast::type_data> td) {
		if (td->type() != ast::type_data_type::var) {
			return td;
		}
		auto vtd = std::static_pointer_cast<ast::var_type_data>(td);
		auto r = s->getTypeData(vtd);
		if (!r||r == td) {
			return td;
		}
		return resolve_type(s, r);
	}
}