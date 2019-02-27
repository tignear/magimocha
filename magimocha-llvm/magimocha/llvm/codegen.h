#pragma once
#include <unordered_set>
#include <utility>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
namespace tig::magimocha::codegen {
	using char_type = char32_t;
	using string_type = std::basic_string<char_type>;
	llvm::LLVMContext TheContext;
	llvm::IRBuilder<> Builder(TheContext);
	class Scope;
	class expression_visitor;
	class module_visitor;
	namespace ast = tig::magimocha::ast;

	llvm::Value* process_expression(Scope*, std::shared_ptr<ast::expression>);
	llvm::Value* process_expression(expression_visitor& v,Scope*, std::shared_ptr<ast::expression> expr);
	void process_module(module_visitor& v, Scope*, std::shared_ptr<ast::module_member> mod);
	void process_module( Scope*, std::shared_ptr<ast::module_member> mod);

	class ScopeHolder {
		std::unordered_set<std::unique_ptr<Scope>> scopes_;
	public:
		template<class Sc,class... Args>
		Sc* create(Args&&... args) {
			auto r= std::make_unique<Sc>(std::forward<Args>(args)...);
			auto ptr = r.get();
			scopes_.insert(std::move(r));
			return ptr;
		}
	};
	ScopeHolder scopes;
}
