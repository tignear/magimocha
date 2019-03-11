#pragma once
#include <optional>
#include <unordered_map>
#include <memory>
#include <string> 
#include "codegen.h"
#include "visitor.h"

namespace tig::magimocha::codegen {
	enum class scope_type {
		prelude,global,module_,function,block
	};
	enum class infix_type {
		left, right
	};
	class FunctionScope;
	class Scope;
	struct OperatorInfo {
		unsigned priority;
		infix_type infix;
	};

	struct SymbolInfo {
		virtual llvm::Value* receive(Scope* s,std::shared_ptr<ast::apply_function> af)=0;
		virtual llvm::Value* receive(Scope* s, std::shared_ptr<ast::call_name> cn) = 0;
		//virtual std::shared_ptr<ast::type_data> type_data()=0;
		virtual ~SymbolInfo() {}
	};
	class Scope {
	protected:
		struct constructing_tag {};
	public:
		Scope(constructing_tag t) {};
		virtual scope_type type()=0;
		virtual std::optional<OperatorInfo> getOperatorInfo(Scope*,std::shared_ptr<ast::call_name>) = 0;
		virtual SymbolInfo* getSymbolInfo(const string_type&)=0;
		virtual void addSymbolInfo(const string_type&, std::shared_ptr<SymbolInfo>&& info) = 0;
		virtual llvm::BasicBlock* getLLVMBasicBlock(Scope*) = 0;
		virtual llvm::Module* getLLVMModule() = 0;
		virtual Scope* getChildScope(const string_type& name)=0;
		virtual void addChildScope(const string_type& name,Scope* child)=0;
		virtual string_type mangling(const string_type&)=0;
		virtual string_type generateUniqueName()=0;
		virtual std::shared_ptr<ast::type_data> getTypeData(std::shared_ptr<ast::type_data>)=0;
		virtual ~Scope() {}
	};
	class ScopeHelper:public Scope {
	protected:
		bool checkLLVMModule(Scope* s) {
			return s->getLLVMModule() == getLLVMModule();
		}
		std::unordered_map<string_type, Scope*> children_;
		std::atomic_uint_fast64_t uniqNameSrc_;
	public:
		using Scope::Scope;
		Scope* getChildScope(const string_type& name)override {
			auto itr = children_.find(name);
			if (itr==end(children_)) 
			{
				return nullptr;
			}
			return itr->second;
		}
		void addChildScope(const string_type& name, Scope* child)override {
			children_[name] = child;
		}
		string_type generateUniqueName()override {
			std::u32string res;
			res.push_back(U'#');
			auto str=std::to_string(uniqNameSrc_++);
			for (auto c = cbegin(str); c != cend(str); ++c)
			{
				char t = *c;
				res.push_back(t);
			}
			
			return res;
		}

	};
	class ModuleScope final :public ScopeHelper {
		Scope* upper_;
		std::unordered_map<string_type, std::shared_ptr<SymbolInfo>> symbols_;
		string_type name_;
	public:
		
		ModuleScope(constructing_tag t, Scope* upper, const string_type& name) :ScopeHelper(t), upper_(upper), name_(name) {}
		ModuleScope(const ModuleScope&) = delete;
		scope_type type()override {
			return scope_type::module_;
		}
		static ModuleScope* create(Scope* upper,const string_type& name) {
			return scopes.create<ModuleScope>(Scope::constructing_tag{},upper,name);
		}
		std::optional<OperatorInfo> getOperatorInfo(Scope* s, std::shared_ptr<ast::call_name> cn)override {
			return upper_->getOperatorInfo(s, cn);
		}

		llvm::BasicBlock* getLLVMBasicBlock(Scope* s)override {
			return upper_->getLLVMBasicBlock(s);
		}
		llvm::Module* getLLVMModule()override {
			return upper_->getLLVMModule();
		}
		string_type mangling(const string_type& s)override {
			return upper_->mangling(name_ + U"@" + s);
		}
		SymbolInfo* getSymbolInfo(const string_type& name)override {
			auto itr = symbols_.find(name);
			if (itr != end(symbols_)) {
				return itr->second.get();
			}
			return upper_->getSymbolInfo(name);
		}
		void addSymbolInfo(const string_type& name, std::shared_ptr<SymbolInfo>&& info)override {
			if (!symbols_.emplace(name, std::move(info) ).second) {
				throw "name is defined!";
			}
		}
		std::shared_ptr<ast::type_data> getTypeData(std::shared_ptr<ast::type_data> s)override {
			return upper_->getTypeData(s);
		}
	};
	class FunctionScope final :public ScopeHelper/*, public std::enable_shared_from_this<FunctionScope> */{
		Scope* upper_;
		string_type name_;
		llvm::Function* F_;
		std::unordered_map<string_type, std::shared_ptr<SymbolInfo>> symbols_;


		llvm::BasicBlock * BB;
	public:
		scope_type type()override {
			return scope_type::function;
		}
		FunctionScope(Scope::constructing_tag t, Scope* upper,const string_type& name, llvm::Function* F, std::unordered_map<string_type, std::shared_ptr<SymbolInfo>>&& symbols) :
			ScopeHelper(t),
			upper_(upper),
			name_(name),
			BB(llvm::BasicBlock::Create(TheContext, "entry", F)),
			F_(F),
			symbols_(std::move(symbols)) {

		}
		static FunctionScope* create(Scope* upper,const string_type& name, llvm::Function* F, std::unordered_map<string_type, std::shared_ptr<SymbolInfo>>&& symbols) {
			return scopes.create< FunctionScope>(Scope::constructing_tag{}, upper,name, F, std::move(symbols));
		}
		llvm::Module* getLLVMModule()override {
			return upper_->getLLVMModule();
		}
		SymbolInfo* getSymbolInfo(const string_type& name)override {
			auto itr = symbols_.find(name);
			if (itr != end(symbols_)) {
				return itr->second.get();
			}
			return upper_->getSymbolInfo(name);
		}
		void addSymbolInfo(const string_type& name, std::shared_ptr<SymbolInfo>&& info)override {
			if (!symbols_.emplace( name, std::move(info) ).second) {
				throw "name is defined!";
			}
		}

		std::optional<OperatorInfo> getOperatorInfo(Scope* s,std::shared_ptr<ast::call_name> cn)override {
			return upper_->getOperatorInfo(s,cn);

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
		string_type mangling(const string_type& s)override {
			return upper_->mangling(name_+U"@" + s);
		}
		std::shared_ptr<ast::type_data> getTypeData(std::shared_ptr<ast::type_data> s)override {
			return upper_->getTypeData(s);
		}
	};
	class PreludeScope final :public Scope {
		std::unique_ptr<llvm::Module> PreludeInternalModule_= llvm::make_unique<llvm::Module>("prelude", TheContext);
		std::unordered_map<string_type, FunctionScope*> childFunctions_;
		void setup_double_op(const std::u32string& name,FunctionScope*& scope, llvm::Function*& F, llvm::Value*& arg0, llvm::Value*&arg1) {
			struct SymbolInfoImpl:public SymbolInfo {
				llvm::Value* arg;
				SymbolInfoImpl(llvm::Value* arg) :arg(arg) {}
				llvm::Value* receive(Scope* s,std::shared_ptr<ast::call_name> cn)override {
					return arg;
				}
				llvm::Value* receive(Scope* s, std::shared_ptr<ast::apply_function> af)override {
					throw "invalid operation";
				}
			};
			std::vector<llvm::Type*> Doubles(2, llvm::Type::getDoubleTy(TheContext));
			llvm::FunctionType *FT = llvm::FunctionType::get(llvm::Type::getDoubleTy(TheContext), Doubles, false);

			F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, to_string(name), PreludeInternalModule_.get());
			std::unordered_map<string_type, std::shared_ptr<SymbolInfo>> variables;
			auto f = F->arg_begin();
			arg0 = f;
			arg0->setName("a");
			variables[U"a"] = std::make_shared<SymbolInfoImpl>(arg0);
			arg1 = f + 1;
			arg1->setName("b");
			variables[U"b"] = std::make_shared<SymbolInfoImpl>(arg1);
			scope = FunctionScope::create(this,name, F, std::move(variables));



			Builder.SetInsertPoint(scope->getLLVMBasicBlock(this));
		}
		FunctionScope* init_mulF() {
			FunctionScope* scope;
			llvm::Function* F;
			llvm::Value* arg0;
			llvm::Value* arg1;
			setup_double_op(U"*", scope, F, arg0, arg1);


			Builder.CreateRet(Builder.CreateFMul(arg0, arg1, "multmp"));
			llvm::verifyFunction(*F);

			return scope;
		}
		FunctionScope* init_divF() {
			FunctionScope* scope;
			llvm::Function* F;
			llvm::Value* arg0;
			llvm::Value* arg1;
			setup_double_op(U"/", scope, F, arg0, arg1);


			Builder.CreateRet(Builder.CreateFDiv(arg0, arg1, "divtmp"));
			llvm::verifyFunction(*F);

			return scope;
		}

		FunctionScope* init_addF() {
			FunctionScope* scope;
			llvm::Function* F;
			llvm::Value* arg0;
			llvm::Value* arg1;
			setup_double_op(U"/", scope, F, arg0, arg1);


			Builder.CreateRet(Builder.CreateFAdd(arg0, arg1, "addtmp"));
			llvm::verifyFunction(*F);

			return scope;
		}
		FunctionScope* init_subF() {
			FunctionScope* scope;
			llvm::Function* F;
			llvm::Value* arg0;
			llvm::Value* arg1;
			setup_double_op(U"/", scope, F, arg0, arg1);


			Builder.CreateRet(Builder.CreateFSub(arg0, arg1, "subtmp"));
			llvm::verifyFunction(*F);

			return scope;
		}
		std::unordered_map<string_type, std::shared_ptr<SymbolInfo>> symbols_;// = std::unordered_map<string_type, FunctionInfo>{
		void init_functions() {
			//auto self = shared_from_this();
			struct Mul :SymbolInfo {
				PreludeScope* self;
				Mul(PreludeScope* self) :self(self) {
				}
				llvm::Value* receive(Scope* s, std::shared_ptr<typename ast::apply_function> f)override {

					auto arg0 = process_expression(s, f->args().at(0));
					auto arg1 = process_expression(s, f->args().at(1));
					Builder.SetInsertPoint(s->getLLVMBasicBlock(s));
					return Builder.CreateFMul(arg0, arg1, "multmp");
				}
				llvm::Value* receive(Scope* s, std::shared_ptr<ast::call_name> cn)override {
					return self->childFunctions_.at(U"*")->getLLVMFunction(self);
				}
			};
			struct Div :SymbolInfo {
				PreludeScope* self;
				Div(PreludeScope* self) :self(self) {
				}
				llvm::Value* receive(Scope* s, std::shared_ptr<typename ast::apply_function> f)override {

					auto arg0 = process_expression(s, f->args().at(0));
					auto arg1 = process_expression(s, f->args().at(1));
					Builder.SetInsertPoint(s->getLLVMBasicBlock(s));

					return Builder.CreateFDiv(arg0, arg1,  "divtmp");
				}
				llvm::Value* receive(Scope* s, std::shared_ptr<ast::call_name> cn)override {
					return self->childFunctions_.at(U"/")->getLLVMFunction(self);
				}
			};
			struct Add :SymbolInfo {
				PreludeScope* self;
				Add(PreludeScope* self) :self(self) {
				}
				llvm::Value* receive(Scope* s, std::shared_ptr<typename ast::apply_function> f)override {

					auto arg0 = process_expression(s, f->args().at(0));
					auto arg1 = process_expression(s, f->args().at(1));
					Builder.SetInsertPoint(s->getLLVMBasicBlock(s));
					return Builder.CreateFAdd(arg0, arg1,"addtmp");
				}
				llvm::Value* receive(Scope* s, std::shared_ptr<ast::call_name> cn)override {
					return self->childFunctions_.at(U"+")->getLLVMFunction(self);
				}
			};
			struct Sub :SymbolInfo {
				PreludeScope* self;
				Sub(PreludeScope* self) :self(self){
				}
				llvm::Value* receive(Scope* s, std::shared_ptr<typename ast::apply_function> f)override {
					auto arg0 = process_expression(s, f->args().at(0));
					auto arg1 = process_expression(s, f->args().at(1));
					Builder.SetInsertPoint(s->getLLVMBasicBlock(s));
					return Builder.CreateFSub(arg0, arg1,  "subtmp" );
				}
				llvm::Value* receive(Scope* s, std::shared_ptr<ast::call_name> cn)override {
					return self->childFunctions_.at(U"+")->getLLVMFunction(self);
				}
			};
			childFunctions_ = {
				{
					U"*",
					init_mulF()
				},
				{
					U"/",
					init_divF()
				},
				{
					U"+",
					init_addF()
				},
				{
					U"-",
					init_subF()
				}
			};
			symbols_ = std::unordered_map<string_type, std::shared_ptr<SymbolInfo>>{
				{
					U"*",
					std::make_shared<Mul>(this)
				},
				{
					U"/",
					std::make_shared<Div>(this)
				},
				{
					U"+",
					std::make_shared<Add>(this)

				},
				{
					U"-",
					std::make_shared<Sub>(this)

				}
			};
		}

	public:
		scope_type type()override {
			return scope_type::prelude;
		}
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
		llvm::Module* getLLVMModule()override {
			return nullptr;
		}
		llvm::BasicBlock* getLLVMBasicBlock(Scope* s) override {
			throw "invalid operation";
		}
		Scope* getChildScope(const string_type& name) {
			return childFunctions_[name];
		}
		void addChildScope(const string_type& name, Scope* child) {
			throw "invalid operation";

		}
		string_type mangling(const string_type&)override {
			throw "invalid operation";
		}

		string_type generateUniqueName()override {
			throw "invalid operation";
		}
		SymbolInfo* getSymbolInfo(const string_type& name)override {
			auto itr = symbols_.find(name);
			if (itr != end(symbols_)) {
				return itr->second.get();
			}
			return nullptr;
		}
		void addSymbolInfo(const string_type& name, std::shared_ptr<SymbolInfo>&& info)override {
			if (!symbols_.emplace( name, std::move(info)).second) {
				throw "name is defined!";
			}
		}
		std::shared_ptr<ast::type_data> getTypeData(std::shared_ptr<ast::type_data> s)override {
			throw "invalid operation";
		}
	};
	class GlobalScope final:public ScopeHelper {
		std::unique_ptr<llvm::Module> TheModule = llvm::make_unique<llvm::Module>("global", TheContext);

		Scope* upper_;
		std::unordered_map<string_type,std::shared_ptr<SymbolInfo>> symbols_;
	public:
		scope_type type()override {
			return scope_type::global;
		}
		GlobalScope(constructing_tag t, Scope* upper) :ScopeHelper(t), upper_(upper) {};
		static GlobalScope* create(Scope* upper) {
			return scopes.create<GlobalScope>(constructing_tag{}, upper);
		}
		std::optional<OperatorInfo> getOperatorInfo(Scope* s, std::shared_ptr<ast::call_name> cn)override {
			return upper_->getOperatorInfo(s,cn);
		}
		SymbolInfo* getSymbolInfo(const string_type& name)override {
			auto itr = symbols_.find(name);
			if (itr != end(symbols_)) {
				return itr->second.get();
			}
			return upper_->getSymbolInfo(name);
		}
		void addSymbolInfo(const string_type& name, std::shared_ptr<SymbolInfo>&& info)override {
			if (!symbols_.emplace( name, std::move(info) ).second) {
				throw "name is defined!";
			}
		}
		llvm::BasicBlock* getLLVMBasicBlock(Scope* s)override {
			return upper_->getLLVMBasicBlock(s);
		}
		llvm::Module* getLLVMModule() override {
			return TheModule.get();
		}
		string_type mangling(const string_type& s)override {
			return s;
		}

		std::shared_ptr<ast::type_data> getTypeData(std::shared_ptr<ast::type_data> s)override {
			return s;
		}
	};
	class BlockScope final :public ScopeHelper {
		Scope* upper_;
		string_type name_;
		std::unordered_map<string_type, std::shared_ptr<SymbolInfo>> symbols_;
		llvm::BasicBlock* BB_;
		std::unordered_map<string_type, Scope*> children_;
	public:
		scope_type type()override {
			return scope_type::block;
		}
		BlockScope(constructing_tag t, Scope* upper,const string_type& name,llvm::BasicBlock* BB) :
			ScopeHelper(t), upper_(upper),name_(name), BB_(BB) {}
		static BlockScope* create(Scope* upper, const string_type& name, llvm::BasicBlock* BB) {
			return scopes.create<BlockScope>(constructing_tag{}, upper, name,BB);
		}
		std::optional<OperatorInfo> getOperatorInfo(Scope* s, std::shared_ptr<ast::call_name> cn)override {
			return upper_->getOperatorInfo(s, cn);
		}
		SymbolInfo* getSymbolInfo(const string_type& name)override {
			auto itr = symbols_.find(name);
			if (itr != end(symbols_)) {
				return itr->second.get();
			}
			return upper_->getSymbolInfo(name);
		}
		void addSymbolInfo(const string_type& name, std::shared_ptr<SymbolInfo>&& info)override {
			if (!symbols_.emplace(name, std::move(info)).second) {
				throw "name is defined!";
			}
		}
		llvm::BasicBlock* getLLVMBasicBlock(Scope* s)override {
			return BB_;
		}
		llvm::Module* getLLVMModule()override {
			return upper_->getLLVMModule();
		}
		Scope* getChildScope(const string_type& name) {
			auto itr = children_.find(name);
			if (itr != end(children_)) {
				return itr->second;
			}
			return nullptr;
		}
		void addChildScope(const string_type& name, Scope* child)override {
			children_[name] = child;
		}
		string_type mangling(const string_type& s)override {
			return upper_->mangling(name_ + U'@' + s);
		}
		std::shared_ptr<ast::type_data> getTypeData(std::shared_ptr<ast::type_data> s)override {
			return upper_->getTypeData(s);
		}
		
	};
	class FunctionCallingScope :public ScopeHelper {
		Scope* upper_;

	};
}