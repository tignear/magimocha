#pragma once
#include "llvm/IR/Value.h"
#include "magimocha/ast.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "../unicode.h"
#include "codegen.h"

#include "scope.h"
namespace tig::magimocha::codegen {

	class Scope;
	class expression_visitor;
	class module_visitor;
	class FunctionScope;
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
		auto processing_apply_function(const std::list<typename ast::operator_token_type>& tokens) {

			std::list<ast::operator_token_type> r;
			for (auto itr = tokens.cbegin(); itr != tokens.cend(); ++itr) {
				if (std::holds_alternative<ast::op_token_function_applying>(*itr)) {
					r.pop_back();
					r.push_back(
						std::make_shared<ast::apply_function>(
							std::visit(
								[](auto&& e)->std::shared_ptr<typename ast::expression> {
									using T = std::decay_t<decltype(e)>;
									if constexpr (is_shared_ptr_v<T>) {
										return std::static_pointer_cast<typename ast::expression>(e);
									}
									else {
										throw std::invalid_argument("");
									}
								},
								*std::prev(itr)
							),
							std::get<ast::op_token_function_applying>(*itr).args()
						)
					);
					continue;
				}
				r.push_back(*itr);
			}
			return r;
		}
		auto processing_single_operator(std::list<typename ast::operator_token_type>&& e) {
			auto fitr = e.begin();
			auto sitr = std::next(fitr);
			if (std::holds_alternative<ast::op_token_single>(*fitr)) {
				auto n = std::make_shared<ast::apply_function>(
					std::static_pointer_cast<typename ast::expression>(std::get<ast::op_token_single>(*fitr).name()),
					std::vector{
						std::visit(
							[](auto&& e)->std::shared_ptr<typename ast::expression> {
								using T = std::decay_t<decltype(e)>;
								if constexpr (is_shared_ptr_v<T>) {
									return std::static_pointer_cast<typename ast::expression>(e);
								}
								else {
									throw std::invalid_argument("");
								}
							},
							*sitr
						)
					}
				);
				e.erase(fitr, sitr);
				e.push_front(n);
			}
			return e;
		}
		auto processing_double_operator(std::list<typename ast::operator_token_type>&& tokens) {
			while (tokens.size() != 1) {
				auto itr = tokens.begin();
				std::optional<std::list<ast::operator_token_type>::iterator> high_left_itr = std::nullopt;
				std::optional<OperatorInfo> high_left_info;
				std::optional<ast::op_token_double> high_left_elem = std::nullopt;
				for (; itr != tokens.end(); ++itr) {
					auto elem = std::visit(
						[](auto&& e)->std::optional<ast::op_token_double> {
							using T = std::decay_t<decltype(e)>;
							if constexpr (is_shared_ptr_v<T>) {
								return std::nullopt;
							}
							else if constexpr (std::is_same_v<T, typename ast::op_token_double>) {
								return e;
							}
							else {
								throw std::invalid_argument("");
							}
						},
						*itr
					);
					if (!elem) {
						continue;
					}
					if (!high_left_itr && !high_left_elem&& !high_left_info) {
						high_left_itr = itr;
						high_left_elem = elem;
						high_left_info = get_opinfo(elem.value());
						continue;
					}
					auto old_p = high_left_info.value().priority;
					auto ninfo = get_opinfo(elem.value());
					auto new_p =ninfo->priority;
					if (new_p > old_p) {
						high_left_itr = itr;
						high_left_elem = elem;
						high_left_info = ninfo;
						continue;
					}
					if (new_p == old_p) {
						if (high_left_info->infix == infix_type::left) {
							continue;
						}
						//old elem infix is right
						high_left_itr = itr;
						high_left_elem = elem;
						high_left_info = ninfo;
					}
				}
				if (!high_left_itr || !high_left_elem|| !high_left_info) {
					throw std::invalid_argument("");
				}
				auto pitr = std::prev(high_left_itr.value());
				auto nitr = std::next(high_left_itr.value());
				auto nelem = std::make_shared<ast::apply_function>(
					high_left_elem.value().name(),
					std::vector<std::shared_ptr<ast::expression>>{
					std::visit(
						[](auto&& e)->std::shared_ptr<typename ast::expression> {
						using T = std::decay_t<decltype(e)>;
						if constexpr (is_shared_ptr_v<T>) {
							return std::static_pointer_cast<typename ast::expression>(e);
						}
						else {
							throw std::invalid_argument("");
						}
					},
						*pitr
						),
						std::visit(
							[](auto&& e)->std::shared_ptr<typename ast::expression> {
						using T = std::decay_t<decltype(e)>;
						if constexpr (is_shared_ptr_v<T>) {
							return std::static_pointer_cast<typename ast::expression>(e);
						}
						else {
							throw std::invalid_argument("");
						}
					},
							*nitr
						)
				}
				);
				tokens.insert(tokens.erase(pitr, ++nitr), nelem);
			}
			return std::visit(
				[](auto&& e)->std::shared_ptr<typename ast::expression> {
				using T = std::decay_t<decltype(e)>;
				if constexpr (is_shared_ptr_v<T>) {
					return std::static_pointer_cast<typename ast::expression>(e);
				}
				else {
					throw std::invalid_argument("");
				}
			},
				tokens.front()
				);
		}
		auto visit(std::shared_ptr<ast::operation> n) {
			return process_expression(*this, scope_, processing_double_operator(processing_single_operator(processing_apply_function(n->value()))));
		}
		llvm::Value* visit(std::shared_ptr<ast::apply_function> n) {
			auto target = n->target();
			if (target->type() == ast::leaf_type::call_name) {
				return scope_->getFunctionInfo(scope_,std::static_pointer_cast<ast::call_name>(target))->receiver(scope_,n);
			}
			auto x = process_expression(*this,scope_, target);
			Builder.SetInsertPoint(scope_->getLLVMBasicBlock(scope_));
			return Builder.CreateCall(x);
			//throw "not implemented";
		}
		auto visit(std::shared_ptr<ast::named_function> n) {

			auto info=scope_->getFunctionInfoThisScope(scope_,n->name());
			auto fscope = info->scope;
			auto v = process_expression(fscope, n->body()->body());

			Builder.SetInsertPoint(fscope->getLLVMBasicBlock(scope_));

			Builder.CreateRet(v);
			auto F = fscope->getLLVMFunction(scope_);
			llvm::verifyFunction(*F);
			
			return F;
		}
		auto visit(std::shared_ptr<ast::call_name> n) {
			return scope_->getVariableInfo(scope_,n)->receiver(scope_, n);
			
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
				process_module(ModuleScope::create(scope_), member);

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

			auto info = scope_->getFunctionInfoThisScope(scope_,n->name());
			auto fscope = info->scope;
			auto v = process_expression(fscope, n->body()->body());

			Builder.SetInsertPoint(fscope->getLLVMBasicBlock(scope_));

			Builder.CreateRet(v);
			auto F = fscope->getLLVMFunction(scope_);
			llvm::verifyFunction(*F);
			return nullptr;
		}
	};

}