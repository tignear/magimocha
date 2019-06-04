#include "scope.h"
#include "llvm-type.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
namespace tig::magimocha::codegen {
	Scope* ScopeHelper::getChildScope(const string_type& name){
		auto itr = children_.find(name);
		if (itr == end(children_))
		{
			return nullptr;
		}
		return itr->second;
	}

	void PreludeScope::init_functions() {
		//auto self = shared_from_this();
		struct Mul :SymbolInfo {
			PreludeScope* self;
			Mul(PreludeScope* self) :self(self) {
			}
			llvm::Value* receive(Scope* s, std::shared_ptr<typename ast::apply_function> f)override {
				auto&& arg0 = f->args().at(0);
				auto&& arg1 = f->args().at(1);
				unify(s, arg0->return_type(), arg1->return_type());
				auto arg0_ll = process_expression(s, arg0);
				auto arg1_ll = process_expression(s, arg1);
				Builder.SetInsertPoint(s->getLLVMBasicBlock(s));
				return Builder.CreateFMul(arg0_ll, arg1_ll, "multmp");
			}
			llvm::Value* receive(Scope* s, std::shared_ptr<ast::call_name> cn)override {
				//return self->childFunctions_.at(U"*")->getLLVMFunction(self);
				throw "NIMPL";

			}
		};
		struct Div :SymbolInfo {
			PreludeScope* self;
			Div(PreludeScope* self) :self(self) {
			}
			llvm::Value* receive(Scope* s, std::shared_ptr<typename ast::apply_function> f)override {
				auto&& arg0 = f->args().at(0);
				auto&& arg1 = f->args().at(1);
				unify(s, arg0->return_type(), arg1->return_type());
				auto arg0_ll = process_expression(s, arg0);
				auto arg1_ll = process_expression(s, arg1);
				Builder.SetInsertPoint(s->getLLVMBasicBlock(s));

				return Builder.CreateFDiv(arg0_ll, arg1_ll, "divtmp");
			}
			llvm::Value* receive(Scope* s, std::shared_ptr<ast::call_name> cn)override {
				//return self->childFunctions_.at(U"/")->getLLVMFunction(self);
				throw "NIMPL";

			}
		};
		struct Add :SymbolInfo {
			PreludeScope* self;
			Add(PreludeScope* self) :self(self) {
			}
			llvm::Value* receive(Scope* s, std::shared_ptr<typename ast::apply_function> f)override {
				auto&& arg0 = f->args().at(0);
				auto&& arg1 = f->args().at(1);
				unify(s, arg0->return_type(), arg1->return_type());
				auto arg0_ll = process_expression(s, arg0);
				auto arg1_ll = process_expression(s, arg1);
				Builder.SetInsertPoint(s->getLLVMBasicBlock(s));
				return Builder.CreateFAdd(arg0_ll, arg1_ll, "addtmp");
			}
			llvm::Value* receive(Scope* s, std::shared_ptr<ast::call_name> cn)override {
				//return self->childFunctions_.at(U"+")->getLLVMFunction(self);
				throw "NIMPL";

			}
		};
		struct Sub :SymbolInfo {
			PreludeScope* self;
			Sub(PreludeScope* self) :self(self) {
			}
			llvm::Value* receive(Scope* s, std::shared_ptr<typename ast::apply_function> f)override {
				auto&& arg0 = f->args().at(0);
				auto&& arg1 = f->args().at(1);
				unify(s, arg0->return_type(), arg1->return_type());
				auto arg0_ll = process_expression(s, arg0);
				auto arg1_ll = process_expression(s, arg1);
				Builder.SetInsertPoint(s->getLLVMBasicBlock(s));
				return Builder.CreateFSub(arg0_ll, arg1_ll, "subtmp");
			}
			llvm::Value* receive(Scope* s, std::shared_ptr<ast::call_name> cn)override {
				throw "NIMPL";
				//return self->childFunctions_.at(U"+")->getTypedFunctionScope(cn->return_type)->getLLVMFunction(self);
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
	void PreludeScope::setup_double_op(const std::u32string& name, FunctionScope*& fscope, TypedFunctionScope*& tfscope, llvm::Function*& F, llvm::Value*& arg0, llvm::Value*&arg1) {
		struct SymbolInfoImpl :public SymbolInfo {
			llvm::Value* arg;
			SymbolInfoImpl(llvm::Value* arg) :arg(arg) {}
			llvm::Value* receive(Scope* s, std::shared_ptr<ast::call_name> cn)override {
				return arg;
			}
			llvm::Value* receive(Scope* s, std::shared_ptr<ast::apply_function> af)override {
				throw "invalid operation";
			}
		};
		//std::vector<llvm::Type*> Doubles(2, llvm::Type::getDoubleTy(TheContext));
		//llvm::FunctionType *FT = llvm::FunctionType::get(llvm::Type::getDoubleTy(TheContext), Doubles, false);

		//F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, to_string(name), PreludeInternalModule_.get());

		fscope = FunctionScope::create(this, name);
		tfscope = TypedFunctionScope::create(
			fscope,
			std::make_shared<ast::function_type_data>(
				std::make_shared<ast::simple_type_data>(U"double"),
				std::vector<std::shared_ptr<ast::type_data>>{
					std::make_shared<ast::simple_type_data>(U"double"),
					std::make_shared<ast::simple_type_data>(U"double")
				}
			),
			llvm::GlobalValue::LinkageTypes::ExternalLinkage
		);
		F = tfscope->getLLVMFunction();
		auto f = F->arg_begin();
		arg0 = f;
		arg0->setName("a");
		tfscope->addSymbolInfo(U"a", std::make_shared<SymbolInfoImpl>(arg0));
		arg1 = f + 1;
		arg1->setName("b");
		tfscope->addSymbolInfo(U"b", std::make_shared<SymbolInfoImpl>(arg1));
		Builder.SetInsertPoint(tfscope->getLLVMBasicBlock(this));
	}
	llvm::Function* TypedFunctionScope::getLLVMFunction() {
		if (is_comp_F) {
			//comp->comp
			return F_;
		}
		auto llvm_ty = type2llvmType(this, function_type_data_);
		if (llvm_ty) {
			if (F_) {
				//ucomp->comp
				auto temp = llvm::Function::Create(llvm_ty, linkage_, to_string(upper_->mangling(U"")), upper_->getLLVMModule());
				F_->replaceAllUsesWith(temp);
				F_ = temp;
				is_comp_F = true;
				BB_->insertInto(F_);
				return F_;
			}
			else {
				//null->comp
				F_=llvm::Function::Create(llvm_ty, linkage_, to_string(upper_->mangling(U"")), upper_->getLLVMModule());
				is_comp_F = true;
				BB_->insertInto(F_);
				return F_;
			}
		}
		else {
			if (F_) {
				//ucomp->ucomp
				return F_;
			}
			else {
				//null->ucomp
				F_ = llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(TheContext),false), linkage_, to_string(upper_->mangling(U"")));
				return F_;
			}
		}
	}

	llvm::Argument* TypedFunctionScope::getLLVMArgument(unsigned no) {
		if (is_comp_F) {
			auto itr=F_->arg_begin();
			itr += no;
			return itr;
		}
		{
			auto itr = temp_args_.find(no);
			if (itr != end(temp_args_)) {
				return itr->second;
			}
		}
		auto elem = new llvm::Argument(type2llvmType(this, function_type_data_->args.at(no)), "temp" + std::to_string(no), nullptr, no);
		temp_args_[no] = elem;
		return elem;
	}
}