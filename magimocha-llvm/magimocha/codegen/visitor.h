#pragma once
#include "llvm/IR/Value.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "magimocha/ast.h"
#include "../unicode.h"
#include "llvm-type.h"
#include "codegen.h"
#include "scope.h"

namespace tig::magimocha::codegen {

	class expression_visitor;
	class module_visitor;
	class Scope;
	namespace ast = tig::magimocha::ast;


	class expression_visitor {
		Scope* scope_;
	public:
		expression_visitor(Scope* scope) :scope_(scope) {

		}
		
		auto visit(std::shared_ptr<ast::signed_number_literal> n) {
			return llvm::ConstantInt::get(TheContext, llvm::APInt(64, n->value(), true));

		}
		auto visit(std::shared_ptr<ast::unsigned_number_literal> n) {
			return llvm::ConstantInt::get(TheContext, llvm::APInt(64, n->value()));

		}
		auto visit(std::shared_ptr<ast::floating_literal> n) {
			return llvm::ConstantFP::get(TheContext, llvm::APFloat(n->value()));

		}
		auto get_opinfo(const ast::op_token_double& token) {
			return scope_->getOperatorInfo(scope_,token.name());

		}
		
		template<typename T> struct is_shared_ptr : std::false_type {};
		template<typename T> struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};
		template<typename T> static constexpr bool is_shared_ptr_v = is_shared_ptr<T>::value;
		std::list<ast::operator_token_type> processing_apply_function(const std::list<typename ast::operator_token_type>& tokens);
		std::list<ast::operator_token_type> processing_single_operator(std::list<typename ast::operator_token_type>&& e);
		std::shared_ptr<ast::expression> processing_double_operator(std::list<typename ast::operator_token_type>&& tokens);
		auto visit(std::shared_ptr<ast::operation> n) {
			auto expr = processing_double_operator(processing_single_operator(processing_apply_function(n->value())));
			unify(scope_, n->return_type(), expr->return_type());
			return process_expression(*this, scope_,expr);
		}
		llvm::Value* visit(std::shared_ptr<ast::apply_function> n) {
			auto target = n->target();
			if (target->type() == ast::leaf_type::call_name) {
				return scope_->getSymbolInfo(std::static_pointer_cast<ast::call_name>(target)->value())->receive(scope_,n);
			}
			auto x = process_expression(*this,scope_, target);
			auto&& args = n->args();
			std::vector<llvm::Value*> llvm_args;
			for (int i = 0; i < args.size(); ++i) {
				llvm_args.push_back(process_expression(scope_, args[i]));
			}
			Builder.SetInsertPoint(scope_->getLLVMBasicBlock(scope_));
			return Builder.CreateCall(x, llvm_args, "lambda_calltmp");
		}
		auto visit(std::shared_ptr<ast::named_function> n) {

			auto s=scope_->getChildScope(n->name());
			if (s->type() != scope_type::function) {
				throw "invalid state";
			}
			auto fscope = static_cast<FunctionScope*>(s);
			auto tfscope=fscope->getTypedFunctionScope(scope_,n->return_type_func());
			auto v = process_expression(fscope, n->body()->body());

			Builder.SetInsertPoint(fscope->getLLVMBasicBlock(scope_));
			Builder.CreateRet(v);
			auto F = tfscope->getLLVMFunction();
			llvm::verifyFunction(*F);
			
			return F;
		}
		auto visit(std::shared_ptr<ast::call_name> n) {
			return scope_->getSymbolInfo(n->value())->receive(scope_, n);
		}
		auto visit(std::shared_ptr<ast::expression_block> n) {
			auto&& name=scope_->generateUniqueName();
			llvm::BasicBlock* BB = scope_->getLLVMBasicBlock(scope_);
			auto bs=BlockScope::create(scope_,name,BB);
			scope_->addChildScope(name, bs);
			llvm::Value* v;
			for (auto&& expr : n->value()) {
				v = process_expression(bs,expr);
			}

			return v;
		}
		auto visit(std::shared_ptr<ast::declaration_variable> v) {
			auto&& name = v->name();
			auto&& expr=process_expression(scope_, v->body());
			struct Val :SymbolInfo {
				llvm::Value* value;
				Val(llvm::Value* value) :value(value) {}
				llvm::Value* receive(Scope* s, std::shared_ptr<ast::call_name> cn)override {
					return value;
				}
				llvm::Value* receive(Scope* s, std::shared_ptr<ast::apply_function> af)override {
					throw "not implementd";//TODO
				}
			};
			expr->setName(to_string(name));
			scope_->addSymbolInfo(name,std::make_shared<Val>(expr));
			return expr;
		}
		auto visit(std::shared_ptr<ast::expression> n) {
			throw "not implemented";
			return nullptr;
		}


	};
	class module_visitor {
		Scope* scope_;
	public:
		module_visitor(Scope* s) :scope_(s) {}

		std::nullptr_t visit(std::shared_ptr<ast::declaration_module> n) {
			for (auto&& member : n->members()) {
				process_module(scope_->getChildScope(n->name()) , member);

			}
			return nullptr;

		}
		std::nullptr_t visit(std::shared_ptr<ast::declaration_variable> n) {
			return nullptr;

		}
		std::nullptr_t visit(std::shared_ptr<ast::declaration_export>n) {
			return nullptr;

		}
		std::nullptr_t visit(std::shared_ptr<ast::named_function>n) {

			auto expect_fscope = scope_->getChildScope(n->name());
			if (expect_fscope->type() != scope_type::function) {
				throw "N_EXPECT";
			}
			auto fscope = static_cast<FunctionScope*>(expect_fscope);
			auto tfscope = fscope->getTypedFunctionScope(scope_,n->return_type_func());
			if (!tfscope) {
				tfscope = TypedFunctionScope::create(fscope, n->return_type_func(), llvm::GlobalValue::LinkageTypes::ExternalLinkage);
				std::vector<string_type> vec;
				auto&& args = n->return_type_func()->args;
				for (auto itr = begin(args); itr != end(args); ++itr) {
					auto rty=resolve_type(tfscope,*itr);
					if (rty->type()!=ast::type_data_type::simple)
					{
						throw "NIMPL";
					}
					auto srty = std::static_pointer_cast<ast::simple_type_data>(rty);
					vec.push_back(srty->value());
				}
				fscope->addTypedFunctionScope(vec, tfscope);

				//throw "N_IMPL";
			}
			auto v = process_expression(tfscope, n->body()->body());

			Builder.SetInsertPoint(tfscope->getLLVMBasicBlock(tfscope));

			Builder.CreateRet(v);
			if (fscope->type() == scope_type::function) {
				//auto rfs=static_cast<FunctionScope*>(fscope);
				//rfs->addTypedFunctionScope();
				auto F = tfscope->getLLVMFunction();
				//llvm::verifyFunction(*F);
				//throw "NIMPL";
			}
			else {
				throw "not function";
			}

			return nullptr;
		}
	};

}
