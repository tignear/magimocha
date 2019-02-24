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
namespace tig::magimocha::codegen {
	using char_type = char32_t;
	using string_type = std::basic_string<char_type>;
	static llvm::LLVMContext TheContext;
	static llvm::IRBuilder<> Builder(TheContext);
	class Scope;
	class visitor;
	namespace ast = tig::magimocha::ast;
	llvm::Value* process(std::shared_ptr<Scope>, std::shared_ptr<ast::expression>);
	llvm::Value* process(visitor& v, std::shared_ptr<ast::expression> expr);
	enum class infix_type {
		left, right
	};
	struct OperatorInfo {
		unsigned priority;
		infix_type infix;
	};
	struct FunctionInfo {
		std::function<llvm::Value*(std::shared_ptr<Scope>,std::shared_ptr<typename ast::apply_function>)> receiver;
	};
	struct VariableInfo {
		std::function<llvm::Value*(std::shared_ptr<Scope>, std::shared_ptr<typename ast::call_name>)> receiver;
	};
	class Scope {
	protected:
		struct constructing_tag {};
	public:
		Scope(constructing_tag t) {};
		virtual std::optional<OperatorInfo> getOperatorInfo(std::shared_ptr<ast::call_name>)=0;
		virtual std::optional<FunctionInfo> getFunctionInfo(std::shared_ptr<ast::call_name>)=0;
		virtual std::optional<VariableInfo> getVariableInfo(std::shared_ptr<ast::call_name>) = 0;
		virtual llvm::BasicBlock* getLLVMBasicBlock() = 0;
		virtual llvm::Module* getLLVMModule()=0;
	};
	class FunctionScope final :public Scope, std::enable_shared_from_this<FunctionScope> {
		std::shared_ptr<Scope> upper_;
		llvm::Function* F_;
		std::unordered_map<string_type, VariableInfo> variables_;
		llvm::BasicBlock * BB;
	public:
		FunctionScope(Scope::constructing_tag t, std::shared_ptr<Scope> upper, llvm::Function* F, std::unordered_map<string_type, VariableInfo>&& variables) :
			Scope(t),
			upper_(upper), 
			BB(llvm::BasicBlock::Create(TheContext, "entry",F)),
			F_(F),
			variables_(variables){

		}
		static std::shared_ptr<FunctionScope> create(std::shared_ptr<Scope> upper,llvm::Function* F, std::unordered_map<string_type, VariableInfo>&& variables) {
			return std::make_shared<FunctionScope>(Scope::constructing_tag{}, upper,F, std::move(variables));
		}
		llvm::Module* getLLVMModule()override {
			return upper_->getLLVMModule();
		}
		std::optional<FunctionInfo> getFunctionInfo(std::shared_ptr<ast::call_name> cn)override {
			return upper_->getFunctionInfo(cn);
		}
		std::optional<OperatorInfo> getOperatorInfo(std::shared_ptr<ast::call_name> cn)override {
			return upper_->getOperatorInfo(cn);

		}
		std::optional<VariableInfo> getVariableInfo(std::shared_ptr<ast::call_name> cn)override {
			return variables_.at(cn->value());
		}
		llvm::BasicBlock* getLLVMBasicBlock()override {
			return BB;
		}
	};
	
	class visitor {
		std::shared_ptr<codegen::Scope> scope_;
	public:
		visitor(std::shared_ptr<codegen::Scope> scope) :scope_(scope) {

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
			return scope_->getOperatorInfo(token.name());

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
			return process(*this, processing_double_operator(processing_single_operator(processing_apply_function(n->value()))));
		}
		llvm::Value* visit(std::shared_ptr<ast::apply_function> n) {
			auto target = n->target();
			if (target->type() == ast::leaf_type::call_name) {
				return scope_->getFunctionInfo(std::static_pointer_cast<ast::call_name>(target))->receiver(scope_,n);
			}
			auto x = process(*this, target);
			Builder.SetInsertPoint(scope_->getLLVMBasicBlock());
			return Builder.CreateCall(x);
			//throw "not implemented";
		}
		auto visit(std::shared_ptr<ast::named_function> n) {
			auto& params = n->body()->params();

			std::vector<llvm::Type*> Doubles(params.size(), llvm::Type::getDoubleTy(TheContext));
			llvm::FunctionType *FT = llvm::FunctionType::get(llvm::Type::getDoubleTy(TheContext), Doubles, false);

			llvm::Function *F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, to_string(n->name()), scope_->getLLVMModule());
			std::unordered_map<string_type, VariableInfo> variables;
			auto farg = F->arg_begin();
			{
				for (auto param = begin(params); param != end(params);) {

					if ((*param)->name()) {
						variables[(*param)->name().value()] = VariableInfo{
							[farg](std::shared_ptr<Scope>,std::shared_ptr<ast::call_name>) {
								return farg;
							}
						};
					}
					++farg;
					++param;
				}
			}
			auto fscope = FunctionScope::create(scope_,F,std::move(variables));

			
			auto v = process(visitor{ fscope }, n->body()->body());

			Builder.SetInsertPoint(fscope->getLLVMBasicBlock());

			Builder.CreateRet(v);
			llvm::verifyFunction(*F);
			
			return F;
		}
		auto visit(std::shared_ptr<ast::call_name> n) {
			return scope_->getVariableInfo(n)->receiver(scope_, n);
			
		}
		auto visit(std::shared_ptr<ast::ast_base> n) {
			throw "not implemented";
			return nullptr;
		}


	};
	struct GlobalScope final:public Scope,std::enable_shared_from_this<GlobalScope> {
		GlobalScope(Scope::constructing_tag t) :Scope(t) {}
		static std::shared_ptr<GlobalScope> create() {
			return std::make_shared<GlobalScope>(Scope::constructing_tag{});
		}
		std::unique_ptr<llvm::Module> TheModule = llvm::make_unique<llvm::Module>("global", TheContext);
		std::optional<OperatorInfo> getOperatorInfo(std::shared_ptr<ast::call_name> call)override {
			const auto name = call->value();
			if (name == U"**")return OperatorInfo{ 2000 ,infix_type::left};
			if (name == U"*" || name == U"/") return OperatorInfo{ 1000,infix_type::left };
			if (name == U"+" || name == U"-")return OperatorInfo{ 500,infix_type::left };
			if (name == U"=") {
				return OperatorInfo{ 0,infix_type::right };
			}
			return std::nullopt;
		}
		std::optional<FunctionInfo> getFunctionInfo(std::shared_ptr<ast::call_name> cn)override {

			auto name = cn->value();
			/*if (name == U"=") {
				return FunctionInfo{
					[self = shared_from_this()](std::shared_ptr<typename ast::apply_function> f) {
						auto v = codegen::visitor{ self};
						return
					}
				};
			}*/
			if (name == U"*") {
				return FunctionInfo{ 
					[self = shared_from_this()](std::shared_ptr<Scope> s,std::shared_ptr<typename ast::apply_function> f) {
						auto v = codegen::visitor{ s};
						auto arg0 = process(v, f->args().at(0));
						auto arg1 = process(v, f->args().at(1));
						Builder.SetInsertPoint(s->getLLVMBasicBlock());
						return Builder.CreateFMul(arg0, arg1, "multmp");
					} 
				};
			}
			if (name == U"/") {
				return FunctionInfo{
					[self = shared_from_this()](std::shared_ptr<Scope> s,std::shared_ptr<typename ast::apply_function> f) {
						auto v = codegen::visitor{ s};
						auto arg0 = process(v, f->args().at(0));
						auto arg1 = process(v, f->args().at(1));
						Builder.SetInsertPoint(s->getLLVMBasicBlock());

						return Builder.CreateFDiv(arg0, arg1, "divtmp");
					}
				};
				
			}
			if (name == U"+") {
				return FunctionInfo{
					[self = shared_from_this()](std::shared_ptr<Scope> s,std::shared_ptr<typename ast::apply_function> f) {
						auto v = codegen::visitor{ s};
						auto arg0 = process(v, f->args().at(0));
						auto arg1 = process(v, f->args().at(1));
						Builder.SetInsertPoint(s->getLLVMBasicBlock());
						return Builder.CreateFAdd(arg0, arg1, "addtmp");
					}
				};
			}
			if (name == U"-") {
				return FunctionInfo{
					[self = shared_from_this()](std::shared_ptr<Scope> s,std::shared_ptr<typename ast::apply_function> f) {
						auto v = codegen::visitor{ s};
						auto arg0 = process(v, f->args().at(0));
						auto arg1 = process(v, f->args().at(1));
						Builder.SetInsertPoint(s->getLLVMBasicBlock());
						return Builder.CreateFSub(arg0,arg1, "subtmp");
					}
				};
			}
			if (name == U"**")throw "not implemented";
			return std::nullopt;
		}
		llvm::Module* getLLVMModule()override {
			return TheModule.get();
		}
		std::optional<VariableInfo> getVariableInfo(std::shared_ptr<ast::call_name>)override {
			return std::nullopt;
		}

		llvm::BasicBlock* getLLVMBasicBlock() override{
			return nullptr;
		}
	};

}
