#pragma once
#include "scope.h"
#include "codegen.h"
namespace tig::magimocha::codegen {
	llvm::FunctionType* type2llvmType(Scope* s,std::shared_ptr<ast::function_type_data> ty);
	std::optional<std::vector<llvm::Type*>> type2llvmType(Scope* s,std::vector<std::shared_ptr<ast::type_data>> tys);
	llvm::Type* type2llvmType(Scope* s,std::shared_ptr<ast::type_data> ty);
	static llvm::Type* param2llvmType(Scope* s,std::shared_ptr<ast::declaration_parameter> param) {
		auto&& ty = param->return_type();
		return type2llvmType(s,ty);
	}
	static std::vector<llvm::Type*> param2llvmType(Scope* s,const std::vector<std::shared_ptr<ast::declaration_parameter>>& params){
		std::vector<llvm::Type*> ret;
		for (auto&& param : params) {
			ret.push_back(param2llvmType(s,param));
		}
		return ret;
	}
	//http://d.hatena.ne.jp/takuto_h/20110401/unify
	void unify(Scope* s, std::shared_ptr<ast::type_data> a, std::shared_ptr<ast::type_data> b);
	std::shared_ptr<ast::type_data> resolve_type(Scope* s, std::shared_ptr<ast::type_data> t);
}