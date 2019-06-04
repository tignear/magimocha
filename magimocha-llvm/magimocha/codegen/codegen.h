#pragma once
#include <unordered_set>
#include <utility>
#include "magimocha/ast.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
namespace tig::magimocha::codegen {
	using char_type = char32_t;
	using string_type = std::basic_string<char_type>;

	class Scope;
	class expression_visitor;
	class module_visitor;
	
	llvm::Value* process_expression(Scope*, std::shared_ptr<ast::expression>);
	llvm::Value* process_expression(expression_visitor& v,Scope*, std::shared_ptr<ast::expression> expr);
	void process_module(module_visitor& v, Scope*, std::shared_ptr<ast::module_member> mod);
	void process_module( Scope*, std::shared_ptr<ast::module_member> mod);
	void register_module_symbols(Scope* s, std::shared_ptr<ast::module_member> mod);
	void register_expression_symbols(Scope* s, std::shared_ptr<ast::expression> expr);
	class ScopeHolder {
		std::unordered_map<Scope*,std::shared_ptr<Scope>> scopes_;
	public:
		template<class Sc,class... Args>
		Sc* create(Args&&... args) {
			auto r= std::make_shared<Sc>(std::forward<Args>(args)...);
			auto ptr = r.get();
			scopes_.insert({ ptr,r });
			return ptr;
		}
		std::shared_ptr<Scope> get(Scope* s) {
			return scopes_.at(s);
		}
		void clear() {
			scopes_.clear();
		}
	};
	//global
	extern llvm::LLVMContext TheContext;
	extern llvm::IRBuilder<llvm::ConstantFolder> Builder;
	extern ScopeHolder scopes;
}
