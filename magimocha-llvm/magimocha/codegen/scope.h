#pragma once
#include <optional>
#include <unordered_map>
#include <memory>
#include <string>
#include "codegen.h"
#include "../unicode.h"
#include "llvm-type.h"
#include "llvm/IR/Verifier.h"
namespace std {
	template <class T>
	inline void hash_combine(std::size_t & seed, const T & v)
	{
		std::hash<T> hasher;
		seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	template<typename T> struct hash<vector<T>>
	{
		inline size_t operator()(const vector<T>& v) const noexcept
		{
			size_t seed = v.size();

			for (auto &e : v) {
				hash_combine(seed, e);
			}

			return seed;
		}
	};
}
namespace tig::magimocha::codegen {
	enum class scope_type {
		prelude,global,module_,function,block,typed_function
	};
	enum class infix_type {
		left, right
	};
	class FunctionScope;
	class Scope;
	class TypedFunctionScope;
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
		virtual std::shared_ptr<ast::type_data> getTypeData(std::shared_ptr<ast::var_type_data>)=0;
		virtual void addTypeData(std::shared_ptr<ast::var_type_data>, std::shared_ptr<ast::type_data>) = 0;
		virtual ~Scope() {}
	};
	class ScopeHelper:public Scope {
	protected:

		bool checkLLVMModule(Scope* s) {
			return s->getLLVMModule() == getLLVMModule();
		}
		std::unordered_map<string_type, Scope*> children_;
		std::atomic_uint_fast64_t uniqNameSrc_;
		std::unordered_map<string_type, std::shared_ptr<SymbolInfo>> symbols_;
		Scope* upper_;
	public:
		ScopeHelper(
			constructing_tag t,
			Scope* upper, 
			std::unordered_map<string_type, std::shared_ptr<SymbolInfo>> symbols= std::unordered_map<string_type, std::shared_ptr<SymbolInfo>>()
		):
			Scope(t),
			upper_(upper),
			symbols_(symbols)
		{

		}
		Scope* getChildScope(const string_type& name)override;
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

		std::optional<OperatorInfo> getOperatorInfo(Scope* s, std::shared_ptr<ast::call_name> cn)override {
			return upper_->getOperatorInfo(s, cn);
		}
		llvm::BasicBlock* getLLVMBasicBlock(Scope* s)override {
			return upper_->getLLVMBasicBlock(s);
		}
		llvm::Module* getLLVMModule()override {
			return upper_->getLLVMModule();
		}
		std::shared_ptr<ast::type_data> getTypeData(std::shared_ptr<ast::var_type_data> s)override {
			return upper_->getTypeData(s);
		}
		void addTypeData(std::shared_ptr<ast::var_type_data> k, std::shared_ptr<ast::type_data> v)override {
			upper_->addTypeData(k,v);
		}
	};
	class ModuleScope final :public ScopeHelper {
		string_type name_;
	public:
		
		ModuleScope(constructing_tag t, Scope* upper, const string_type& name) :ScopeHelper(t,upper), name_(name) {}
		ModuleScope(const ModuleScope&) = delete;
		scope_type type()override {
			return scope_type::module_;
		}
		static ModuleScope* create(Scope* upper,const string_type& name) {
			return scopes.create<ModuleScope>(Scope::constructing_tag{},upper,name);
		}


		string_type mangling(const string_type& s)override {
			return upper_->mangling(name_ + U"@" + s);
		}


	};
	class FunctionScope final :public ScopeHelper{
		string_type name_;
		//llvm::Function* F_;
		std::unordered_map<std::vector<string_type>, TypedFunctionScope*> typed_function_scopes_;
	public:
		scope_type type()override {
			return scope_type::function;
		}
		FunctionScope(
			Scope::constructing_tag t, 
			Scope* upper,
			const string_type& name) :
			ScopeHelper(t,upper),
			name_(name){

		}
		static FunctionScope* create(Scope* upper,const string_type& name) {
			return scopes.create< FunctionScope>(Scope::constructing_tag{}, upper,name);
		}
		llvm::BasicBlock* getLLVMBasicBlock(Scope* s)override {
			throw "not impl";
		}
		TypedFunctionScope* getTypedFunctionScope(Scope* s,std::shared_ptr<ast::function_type_data> data) {
			std::vector<string_type> k;
			for (auto&& arg:data->args) {
				if (arg->type() == ast::type_data_type::simple) {
					k.push_back(std::static_pointer_cast<ast::simple_type_data>(arg)->value());
				}
				else {
					throw "NIMPL";
				}
			}
			auto&& itr=typed_function_scopes_.find(k);
			if (itr == std::end(typed_function_scopes_)) {
				return nullptr;
			}
			return itr->second;
		}
		void addTypedFunctionScope(std::vector<string_type> arg_data, TypedFunctionScope* s) {
			typed_function_scopes_[arg_data] = s;
		}
		string_type mangling(const string_type& s)override {
			return upper_->mangling(name_ + U"@" + s);
		}
	};
	class PreludeScope final :public Scope {
		std::unique_ptr<llvm::Module> PreludeInternalModule_= llvm::make_unique<llvm::Module>("prelude", TheContext);
		std::unordered_map<string_type, FunctionScope*> childFunctions_;
		void setup_double_op(
			const std::u32string& name,
			FunctionScope*& fscope, 
			TypedFunctionScope*& tfscope, 
			llvm::Function*& F, 
			llvm::Value*& arg0,
			llvm::Value*&arg1
		);
		FunctionScope* init_mulF() {
			FunctionScope* fscope;
			TypedFunctionScope* tfscope;
			llvm::Function* F;
			llvm::Value* arg0;
			llvm::Value* arg1;
			setup_double_op(U"*", fscope, tfscope,F, arg0, arg1);


			Builder.CreateRet(Builder.CreateFMul(arg0, arg1, "multmp"));
			llvm::verifyFunction(*F);

			return fscope;
		}
		FunctionScope* init_divF() {
			FunctionScope* fscope;
			TypedFunctionScope* tfscope;
			llvm::Function* F;
			llvm::Value* arg0;
			llvm::Value* arg1;
			setup_double_op(U"/", fscope, tfscope, F, arg0, arg1);


			Builder.CreateRet(Builder.CreateFDiv(arg0, arg1, "divtmp"));
			llvm::verifyFunction(*F);

			return fscope;
		}

		FunctionScope* init_addF() {
			FunctionScope* fscope;
			TypedFunctionScope* tfscope;
			llvm::Function* F;
			llvm::Value* arg0;
			llvm::Value* arg1;
			setup_double_op(U"+", fscope, tfscope, F, arg0, arg1);



			Builder.CreateRet(Builder.CreateFAdd(arg0, arg1, "addtmp"));
			llvm::verifyFunction(*F);

			return fscope;
		}
		FunctionScope* init_subF() {
			FunctionScope* fscope;
			TypedFunctionScope* tfscope;
			llvm::Function* F;
			llvm::Value* arg0;
			llvm::Value* arg1;
			setup_double_op(U"-", fscope, tfscope, F, arg0, arg1);



			Builder.CreateRet(Builder.CreateFSub(arg0, arg1, "subtmp"));
			llvm::verifyFunction(*F);

			return fscope;
		}
		std::unordered_map<string_type, std::shared_ptr<SymbolInfo>> symbols_;// = std::unordered_map<string_type, FunctionInfo>{
		void init_functions();

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
			return PreludeInternalModule_.get();
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
		string_type mangling(const string_type& s)override {
			return s;
			//throw "invalid operation";
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
		std::shared_ptr<ast::type_data> getTypeData(std::shared_ptr<ast::var_type_data> s)override {
			throw "invalid operation";
		}
		void addTypeData(std::shared_ptr<ast::var_type_data>, std::shared_ptr<ast::type_data>)override {
			throw "invalid operation";
		}
		
	};
	class GlobalScope final:public ScopeHelper {
		std::unique_ptr<llvm::Module> TheModule = llvm::make_unique<llvm::Module>("global", TheContext);

		std::unordered_map<string_type,std::shared_ptr<SymbolInfo>> symbols_;
	public:
		scope_type type()override {
			return scope_type::global;
		}
		GlobalScope(constructing_tag t, Scope* upper) :ScopeHelper(t,upper){};
		static GlobalScope* create(Scope* upper) {
			return scopes.create<GlobalScope>(constructing_tag{}, upper);
		}

		llvm::Module* getLLVMModule() override {
			return TheModule.get();
		}
		string_type mangling(const string_type& s)override {
			return s;
		}

		std::shared_ptr<ast::type_data> getTypeData(std::shared_ptr<ast::var_type_data> s)override {
			return s;
		}
	};
	class BlockScope final :public ScopeHelper {
		string_type name_;
		std::unordered_map<string_type, std::shared_ptr<SymbolInfo>> symbols_;
		llvm::BasicBlock* BB_;
	public:
		scope_type type()override {
			return scope_type::block;
		}
		BlockScope(constructing_tag t, Scope* upper,const string_type& name,llvm::BasicBlock* BB) :
			ScopeHelper(t, upper),name_(name), BB_(BB) {}
		static BlockScope* create(Scope* upper, const string_type& name, llvm::BasicBlock* BB) {
			return scopes.create<BlockScope>(constructing_tag{}, upper, name,BB);
		}
		string_type mangling(const string_type& s)override {
			return upper_->mangling(name_ + U'@' + s);
		}

		
	};
	class TypedFunctionScope final:public ScopeHelper {
		bool is_comp_F=false;
		llvm::Function* F_=nullptr;
		std::shared_ptr<ast::function_type_data> function_type_data_;
		std::unordered_map<std::shared_ptr<ast::var_type_data>, std::shared_ptr<ast::type_data>> type_vars_;
		llvm::BasicBlock* BB_;
		llvm::GlobalValue::LinkageTypes linkage_;
		std::unordered_map<unsigned, llvm::Argument*> temp_args_;
	public:
		TypedFunctionScope(
			constructing_tag t,
			Scope* upper,
			std::shared_ptr<ast::function_type_data> ftd,
			llvm::GlobalValue::LinkageTypes linkage,
			llvm::BasicBlock* BB
		):
			ScopeHelper(t,upper),
			function_type_data_(ftd),
			linkage_(linkage),
			BB_(BB)
		{

		}
		~TypedFunctionScope()noexcept {
			getLLVMFunction();
			if (!is_comp_F) {
				std::cout << "www";
			}
		}
		static TypedFunctionScope* create(Scope* upper,std::shared_ptr<ast::function_type_data> ftd, llvm::GlobalValue::LinkageTypes linkage) {
			return scopes.create<TypedFunctionScope>(constructing_tag{}, upper,ftd, linkage,llvm::BasicBlock::Create(TheContext,"entry"));
		}

		scope_type type()override {
			return scope_type::typed_function;
		}

		void addTypeData(std::shared_ptr<ast::var_type_data> vtd, std::shared_ptr<ast::type_data> td)override{
			type_vars_[vtd] = td;
		}
		std::shared_ptr<ast::type_data> getTypeData(std::shared_ptr<ast::var_type_data> k) override {
			 return type_vars_[k];
		}
		llvm::BasicBlock* getLLVMBasicBlock(Scope* s)override {
			return BB_;
		}
		string_type mangling(const string_type& s)override {
			return upper_->mangling(s);//TODO
		}
		llvm::Argument* getLLVMArgument(unsigned no);
		llvm::Function* getLLVMFunction();
	};
}