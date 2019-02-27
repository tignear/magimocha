#pragma once
#include <optional>
#include "codegen.h"
#include "visitor.h"
namespace tig::magimocha::codegen {

	enum class infix_type {
		left, right
	};
	class FunctionScope;
	class Scope;
	struct OperatorInfo {
		unsigned priority;
		infix_type infix;
	};
	struct FunctionInfo {
		std::function<llvm::Value*(Scope*, std::shared_ptr<typename ast::apply_function>)> receiver;
		
	};
	struct FunctionInfoThisScope :FunctionInfo {
		FunctionScope* scope;
	};
	struct VariableInfo {
		std::function<llvm::Value*(Scope*, std::shared_ptr<typename ast::call_name>)> receiver;
	};
	class Scope {
	protected:
		struct constructing_tag {};
	public:
		Scope(constructing_tag t) {};
		virtual std::optional<OperatorInfo> getOperatorInfo(Scope*,std::shared_ptr<ast::call_name>) = 0;
		virtual std::optional<FunctionInfo> getFunctionInfo(Scope*,std::shared_ptr<ast::call_name>) = 0;
		virtual std::optional<FunctionInfoThisScope> getFunctionInfoThisScope(Scope*,const string_type&) = 0;

		virtual std::optional<VariableInfo> getVariableInfo(Scope*,std::shared_ptr<ast::call_name>) = 0;
		virtual void addFunctionInfo(Scope*,const string_type&, FunctionInfoThisScope) = 0;
		virtual llvm::BasicBlock* getLLVMBasicBlock(Scope*) = 0;
		virtual llvm::Module* getLLVMModule() = 0;
	};
	class ScopeHelper:public Scope {
	protected:
		bool checkLLVMModule(Scope* s) {
			return s->getLLVMModule() == getLLVMModule();
		}
	public:
		using Scope::Scope;

	};

	class ModuleScope final :public ScopeHelper {
		Scope* upper_;
		std::unordered_map<string_type, FunctionInfoThisScope> functions_;
	public:
		
		ModuleScope(constructing_tag t, Scope* upper) :ScopeHelper(t),upper_(upper) {}
		static ModuleScope* create(Scope* upper) {
			return scopes.create<ModuleScope>(Scope::constructing_tag{},upper);
		}
		std::optional<OperatorInfo> getOperatorInfo(Scope* s, std::shared_ptr<ast::call_name> cn)override {
			return upper_->getOperatorInfo(s, cn);
		}
		std::optional<FunctionInfo> getFunctionInfo(Scope* s, std::shared_ptr<ast::call_name> cn)override {
			auto itr = functions_.find(cn->value());
			if (itr != end(functions_)) {
				return itr->second;
			}
			return upper_->getFunctionInfo(s,cn) ;
		}
		std::optional<FunctionInfoThisScope> getFunctionInfoThisScope(Scope*, const string_type& n)override {
			return functions_.at(n);
		}

		std::optional<VariableInfo> getVariableInfo(Scope* s, std::shared_ptr<ast::call_name> cn)override {
			return upper_->getVariableInfo(s,cn);
		}
		void addFunctionInfo(Scope* s, const string_type& n, FunctionInfoThisScope i)override {
			functions_[n] = i;
		}
		llvm::BasicBlock* getLLVMBasicBlock(Scope* s)override {
			return upper_->getLLVMBasicBlock(s);
		}
		llvm::Module* getLLVMModule()override {
			return upper_->getLLVMModule();
		}
	};
	class FunctionScope final :public ScopeHelper/*, public std::enable_shared_from_this<FunctionScope> */{
		Scope* upper_;
		llvm::Function* F_;
		std::unordered_map<string_type, VariableInfo> variables_;
		std::unordered_map<string_type, FunctionInfoThisScope> functions_;

		llvm::BasicBlock * BB;
	public:
		FunctionScope(Scope::constructing_tag t, Scope* upper, llvm::Function* F, std::unordered_map<string_type, VariableInfo>&& variables) :
			ScopeHelper(t),
			upper_(upper),
			BB(llvm::BasicBlock::Create(TheContext, "entry", F)),
			F_(F),
			variables_(variables) {

		}
		static FunctionScope* create(Scope* upper, llvm::Function* F, std::unordered_map<string_type, VariableInfo>&& variables) {
			return scopes.create< FunctionScope>(Scope::constructing_tag{}, upper, F, std::move(variables));
		}
		llvm::Module* getLLVMModule()override {
			return upper_->getLLVMModule();
		}
		std::optional<FunctionInfo> getFunctionInfo(Scope* s,std::shared_ptr<ast::call_name> cn)override {
			auto itr = functions_.find(cn->value());
			if (itr != end(functions_)) {
				return itr->second;
			}
			return upper_->getFunctionInfo(s,cn);
		}
		std::optional<FunctionInfoThisScope> getFunctionInfoThisScope(Scope* s,const string_type& name)override {
			auto itr = functions_.find(name);
			if (itr != end(functions_)) {
				return itr->second;
			}
			return std::nullopt;
		}
		void addFunctionInfo(Scope* s,const string_type& n, FunctionInfoThisScope info)override {
			functions_[n] = info;
		};

		std::optional<OperatorInfo> getOperatorInfo(Scope* s,std::shared_ptr<ast::call_name> cn)override {
			return upper_->getOperatorInfo(s,cn);

		}
		std::optional<VariableInfo> getVariableInfo(Scope* s,std::shared_ptr<ast::call_name> cn)override {
			return variables_.at(cn->value());
		}
		llvm::BasicBlock* getLLVMBasicBlock(Scope* s)override {
			if (!checkLLVMModule(s)) {
				throw "not allow access";
			}
			return BB;
		}
		llvm::Function* getLLVMFunction(Scope* s) {
			if (!checkLLVMModule(s)) {
				throw "not allow access";
			}
			return F_;
		}
	};
	class PreludeScope final :public Scope {
		void setup_double_op(const std::string& name,FunctionScope*& scope, llvm::Function*& F, llvm::Value*& arg0, llvm::Value*&arg1) {
			std::vector<llvm::Type*> Doubles(2, llvm::Type::getDoubleTy(TheContext));
			llvm::FunctionType *FT = llvm::FunctionType::get(llvm::Type::getDoubleTy(TheContext), Doubles, false);

			F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, name, getLLVMModule());
			std::unordered_map<string_type, VariableInfo> variables;
			auto f = F->arg_begin();
			arg0 = f;
			arg0->setName("a");
			variables[U"a"] = VariableInfo{
				[arg0](Scope*,std::shared_ptr<ast::call_name>) {
					return arg0;
				}
			};
			arg1 = f + 1;
			arg1->setName("b");
			variables[U"b"] = VariableInfo{
				[arg1](Scope*,std::shared_ptr<ast::call_name>) {
					return arg1;
				}
			};
			scope = FunctionScope::create(this, F, std::move(variables));



			Builder.SetInsertPoint(scope->getLLVMBasicBlock(this));
		}
		FunctionScope* init_mulF() {
			FunctionScope* scope;
			llvm::Function* F;
			llvm::Value* arg0;
			llvm::Value* arg1;
			setup_double_op("*", scope, F, arg0, arg1);


			Builder.CreateRet(Builder.CreateFMul(arg0, arg1, "multmp"));
			llvm::verifyFunction(*F);

			return scope;
		}
		FunctionScope* init_divF() {
			FunctionScope* scope;
			llvm::Function* F;
			llvm::Value* arg0;
			llvm::Value* arg1;
			setup_double_op("/", scope, F, arg0, arg1);


			Builder.CreateRet(Builder.CreateFDiv(arg0, arg1, "divtmp"));
			llvm::verifyFunction(*F);

			return scope;
		}

		FunctionScope* init_addF() {
			FunctionScope* scope;
			llvm::Function* F;
			llvm::Value* arg0;
			llvm::Value* arg1;
			setup_double_op("+", scope, F, arg0, arg1);


			Builder.CreateRet(Builder.CreateFAdd(arg0, arg1, "addtmp"));
			llvm::verifyFunction(*F);

			return scope;
		}
		FunctionScope* init_subF() {
			FunctionScope* scope;
			llvm::Function* F;
			llvm::Value* arg0;
			llvm::Value* arg1;
			setup_double_op("-", scope, F, arg0, arg1);


			Builder.CreateRet(Builder.CreateFSub(arg0, arg1, "subtmp"));
			llvm::verifyFunction(*F);

			return scope;
		}
		std::unordered_map<string_type, FunctionInfoThisScope> functions_;// = std::unordered_map<string_type, FunctionInfo>{
		void init_functions() {
			//auto self = shared_from_this();
			functions_ = std::unordered_map<string_type, FunctionInfoThisScope>{
				{
					U"*",
					FunctionInfoThisScope{
						 [](Scope* s, std::shared_ptr<typename ast::apply_function> f) {
							auto arg0 = process_expression(s, f->args().at(0));
							auto arg1 = process_expression(s, f->args().at(1));
							Builder.SetInsertPoint(s->getLLVMBasicBlock(s));
							return Builder.CreateFMul(arg0, arg1, "multmp");
						},
						init_mulF()
					}
				},
				{
					U"/",
					FunctionInfoThisScope{
						[](Scope* s,std::shared_ptr<typename ast::apply_function> f) {
							auto arg0 = process_expression(s, f->args().at(0));
							auto arg1 = process_expression(s, f->args().at(1));
							Builder.SetInsertPoint(s->getLLVMBasicBlock(s));

							return Builder.CreateFDiv(arg0, arg1, "divtmp");
						},
						init_divF()
					}
				},
				{
					U"+",
					FunctionInfoThisScope{
						[](Scope* s,std::shared_ptr<typename ast::apply_function> f) {
							auto arg0 = process_expression(s, f->args().at(0));
							auto arg1 = process_expression(s, f->args().at(1));
							Builder.SetInsertPoint(s->getLLVMBasicBlock(s));
							return Builder.CreateFAdd(arg0, arg1, "addtmp");
						},
						init_addF()
					}
				},
				{
					U"-",
					FunctionInfoThisScope{
						[](Scope* s,std::shared_ptr<typename ast::apply_function> f) {
							auto arg0 = process_expression(s, f->args().at(0));
							auto arg1 = process_expression(s, f->args().at(1));
							Builder.SetInsertPoint(s->getLLVMBasicBlock(s));
							return Builder.CreateFSub(arg0,arg1, "subtmp");
						},
						init_subF()
					}
				}
			};
		}

	public:
		PreludeScope(Scope::constructing_tag t) :Scope(t) {

		}
		static PreludeScope* create() {
			auto r=scopes.create<PreludeScope>(Scope::constructing_tag{});
			r->init_functions();
			return r;
		}

		std::optional<OperatorInfo> getOperatorInfo(Scope* s,std::shared_ptr<ast::call_name> call)override {
			const auto name = call->value();
			if (name == U"**")return OperatorInfo{ 2000 ,infix_type::left };
			if (name == U"*" || name == U"/") return OperatorInfo{ 1000,infix_type::left };
			if (name == U"+" || name == U"-")return OperatorInfo{ 500,infix_type::left };
			if (name == U"=") {
				return OperatorInfo{ 0,infix_type::right };
			}
			return std::nullopt;
		}
		std::optional<FunctionInfoThisScope> getFunctionInfoThisScope(Scope* s, const string_type& name)override {
			/*if (name == U"=") {
				return FunctionInfo{
					[self = shared_from_this()](std::shared_ptr<typename ast::apply_function> f) {
						auto v = codegen::visitor{ self};
						return
					}
				};
			}*/
			auto itr = functions_.find(name);
			if (itr != end(functions_)) {
				return itr->second;
			}
			if (name == U"**")throw "not implemented";
			return std::nullopt;
		}
		std::optional<FunctionInfo> getFunctionInfo(Scope* s, std::shared_ptr<ast::call_name> cn)override {
			return getFunctionInfoThisScope(s,cn->value());

		}
		llvm::Module* getLLVMModule()override {
			return nullptr;
		}
		std::optional<VariableInfo> getVariableInfo(Scope* s,std::shared_ptr<ast::call_name>)override {
			return std::nullopt;
		}
		virtual void addFunctionInfo(Scope* s, const string_type& n, FunctionInfoThisScope info)override {
			throw "invalid operation";
			//functions_[n] = info;
		};
		llvm::BasicBlock* getLLVMBasicBlock(Scope* s) override {
			return nullptr;
		}
	};
	class GlobalScope :public Scope, public std::enable_shared_from_this<GlobalScope> {
		std::unique_ptr<llvm::Module> TheModule = llvm::make_unique<llvm::Module>("global", TheContext);

		Scope* upper_;
		std::unordered_map<string_type, FunctionInfoThisScope> functions_;
	public:
		GlobalScope(constructing_tag t, Scope* upper) :Scope(t), upper_(upper) {};
		static GlobalScope* create(Scope* upper) {
			return scopes.create<GlobalScope>(constructing_tag{}, upper);
		}
		std::optional<OperatorInfo> getOperatorInfo(Scope* s, std::shared_ptr<ast::call_name> cn)override {
			return upper_->getOperatorInfo(s,cn);
		}
		std::optional<FunctionInfo> getFunctionInfo(Scope* s, std::shared_ptr<ast::call_name>cn) override {
			auto i = getFunctionInfoThisScope(s,cn->value());
			if (i) {
				return i;
			}
			return upper_->getFunctionInfo(s,cn);
		}
		std::optional<FunctionInfoThisScope> getFunctionInfoThisScope(Scope* s, const string_type& n)override {
			auto itr = functions_.find(n);
			if (itr != end(functions_)) {
				return itr->second;
			}
			return std::nullopt;
		}

		std::optional<VariableInfo> getVariableInfo(Scope* s,std::shared_ptr<ast::call_name> cn)override {
			return upper_->getVariableInfo(s,cn);
		}
		void addFunctionInfo(Scope* s,const string_type& n, FunctionInfoThisScope info) override {
			functions_[n] = info;
		}
		llvm::BasicBlock* getLLVMBasicBlock(Scope* s)override {
			return upper_->getLLVMBasicBlock(s);
		}
		llvm::Module* getLLVMModule() override {
			return TheModule.get();
		}

	};
}