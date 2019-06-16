#include "magimocha/parser.h"
#include "magimocha/ast-visitor.h"
#include "codegen.h"
#include "pass.h"
#include "visitor.h"
#include "llvm-type.h"
#include "llvm/Analysis/InstructionSimplify.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include <iostream>

template<typename Iterator>
struct src {
	Iterator it_;
	Iterator end_;
	src() = default;
	src(Iterator it, Iterator end) noexcept : it_(it), end_(end) {}
	src(const src&) = default;
	src(src&&) = default;
	src& operator=(const src&) = default;
	src& operator=(src&&) = default;

	using iterator_category = typename std::iterator_traits<Iterator>::iterator_category;
	using value_type = typename std::iterator_traits<Iterator>::value_type;
	using difference_type = typename std::iterator_traits<Iterator>::difference_type;
	using pointer = typename std::iterator_traits<Iterator>::pointer;
	using reference = typename std::iterator_traits<Iterator>::reference;
	constexpr auto is_end() {
		return it_ == end_;
	}
	const value_type& operator*() noexcept { return *it_; }
	value_type operator*() const noexcept { return *it_; }
	src& operator++()
	{
		++it_;
		return *this;
	}
	src operator++(int)
	{
		Iterator _Tmp = *this;
		++*this;
		return (_Tmp);
	}
	src& operator+=(difference_type x)
	{
		it_ += x;
		return *this;
	}
};
using u32src = src<std::u32string::const_iterator>;

using p = tig::magimocha::parser::p<u32src>;
namespace ast = tig::magimocha::ast;
namespace codegen=tig::magimocha::codegen;
namespace tig::magimocha::codegen {
	//global variables
	llvm::LLVMContext TheContext;
	llvm::IRBuilder<llvm::ConstantFolder> Builder(TheContext);
	ScopeHolder scopes;
	//functions
	llvm::Value* process_expression(Scope* s, std::shared_ptr<ast::expression> expr) {
		expression_visitor v{ s };
		return process_expression(v,s, expr);
	}
	llvm::Value* process_expression(expression_visitor& v, Scope* s, std::shared_ptr<ast::expression> expr) {
		register_expression_symbols(s,expr);
		return ast::visit<llvm::Value*>(v, expr);
	}
	void process_module(Scope* s, std::shared_ptr<ast::module_member> mod) {
		auto v=module_visitor{ s };
		process_module(v, s, mod);
	}
	void register_expression_symbols(Scope* s, std::shared_ptr<ast::expression> expr) {
		class visitor_extract_named_fn {
			Scope* s;
		public:
			visitor_extract_named_fn(Scope* s) {
				if (s == nullptr) {
					throw "invalid";
				}
				this->s = s;
			}

			std::monostate visit(std::shared_ptr<ast::named_function> n) {
				auto&& ty_func = n->return_type_func();
				auto&& params = n->body()->params();
				//llvm::FunctionType*FT = type2llvmType(s,ty_func);
				//llvm::Function *F = llvm::Function::Create(FT, llvm::Function::PrivateLinkage, to_string(s->mangling(n->name())), s->getLLVMModule());
				struct Arg :SymbolInfo {
					std::shared_ptr<ast::declaration_parameter> param;
					TypedFunctionScope* tfscope;
					unsigned param_no;
					Arg(std::shared_ptr<ast::declaration_parameter> param, unsigned param_no):param(param),param_no(param_no){}
					llvm::Value* receive(Scope* s, std::shared_ptr<ast::call_name> cn)override {
						
						unify(s, param->return_type(), cn->return_type());
						return tfscope->getLLVMArgument(param_no);
					}
					llvm::Value* receive(Scope* s, std::shared_ptr<ast::apply_function> af)override {
						std::vector<llvm::Value*> llvm_args;
						for (auto&& arg : af->args()) {
							llvm_args.push_back(process_expression(s, arg));
						}
						Builder.SetInsertPoint(s->getLLVMBasicBlock(s));
						return Builder.CreateCall(tfscope->getLLVMArgument(param_no), llvm_args,"lambda_calltmp");
					}
				};
				auto fscope = FunctionScope::create(s, n->name());
				
				{
					unsigned cnt = 0;
					//auto farg = F->arg_begin();
					for (auto param = begin(params); param != end(params);) {

						if ((*param)->name()) {
							//farg->setName(to_string((*param)->name().value()));
							fscope->addSymbolInfo((*param)->name().value(), std::make_shared<Arg>(*param,cnt));
						}
						++cnt;
						++param;
					}
				}
				struct Fun :SymbolInfo {
					FunctionScope* fscope;
					Fun(FunctionScope* fscope) :fscope(fscope) {}
					llvm::Value* receive(Scope* s, std::shared_ptr<typename ast::apply_function> af)override {
						auto expect_ft = af->target()->return_type();
						if (expect_ft->type() != ast::type_data_type::function) {
							throw "error";
						}
						auto tfscope=fscope->getTypedFunctionScope(s,std::static_pointer_cast<ast::function_type_data>(expect_ft));
						if (!tfscope) {
							throw "NIMPL";
						}
						auto F=tfscope->getLLVMFunction();
						auto&& args = af->args();
						if (F->arg_size() != args.size())
							throw "not match args cnt";

						std::vector<llvm::Value*> llvm_args;
						for (int i = 0; i < args.size(); ++i) {
							llvm_args.push_back(process_expression(s, args[i]));
						}
						Builder.SetInsertPoint(s->getLLVMBasicBlock(s));
						return Builder.CreateCall(F, llvm_args,"calltmp");
					}
					llvm::Value* receive(Scope* s, std::shared_ptr<typename ast::call_name> cn)override {
						//return F;
						throw "NIMPL";
					}
				};
				s->addSymbolInfo(
					n->name(),
					std::make_shared<Fun>(fscope)
				);
				s->addChildScope(n->name(),fscope);
				return std::monostate{};
			}
			std::monostate  operator()(std::shared_ptr<ast::expression>) {
				return std::monostate{};
			}
		};
		ast::visit<std::monostate>(visitor_extract_named_fn{ s }, expr);

	}
	void register_module_symbols(Scope* s, std::shared_ptr<ast::module_member> mod) {
		class visitor_extract_named_fn {
			Scope* s;
		public:
			visitor_extract_named_fn(Scope* s) {
				if (s == nullptr) {
					throw "invalid";
				}
				this->s = s;
			}
			std::monostate  operator()(std::shared_ptr<ast::declaration_module> n) {
				Scope* ms;
				ms = s->getChildScope(n->name());
				if (ms == nullptr) {
					ms = ModuleScope::create(s,n->name());
					s->addChildScope(n->name(), ms);
				}
				auto v = visitor_extract_named_fn{ ms };
				for (auto member : n->members()) {
					ast::visit<std::monostate>(v, member);

				}

				return std::monostate{};

			}
			std::monostate  operator()(std::shared_ptr<ast::named_function> n) {
				auto&& ty_func = n->return_type_func();
				auto&& params = n->body()->params();
				//llvm::FunctionType*FT = type2llvmType(s,ty_func);
				//llvm::Function *F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, to_string(s->mangling(n->name())), s->getLLVMModule());
				struct Arg :SymbolInfo {
					std::shared_ptr<ast::declaration_parameter> param;
					unsigned param_no;
					llvm::Value* farg;


					Arg(std::shared_ptr<ast::declaration_parameter> param, unsigned param_no) :param(param), param_no(param_no) {}
					llvm::Value* receive(Scope* s, std::shared_ptr<ast::call_name> cn)override {

						unify(s, param->return_type(), cn->return_type());
						if (!farg) {
							farg = new llvm::Argument(type2llvmType(s, param->return_type()), to_string(param->name().value()));
						}
						return farg;
					}
					llvm::Value* receive(Scope* s, std::shared_ptr<ast::apply_function> af)override {
						std::vector<llvm::Value*> llvm_args;
						for (auto&& arg : af->args()) {
							llvm_args.push_back(process_expression(s, arg));
						}
						Builder.SetInsertPoint(s->getLLVMBasicBlock(s));
						return Builder.CreateCall(farg, llvm_args, "lambda_calltmp");
					}
				};
				auto fscope = FunctionScope::create(s, n->name());

				{
					unsigned cnt = 0;
					//auto farg = F->arg_begin();
					for (auto param = begin(params); param != end(params);) {

						if ((*param)->name()) {
							//farg->setName(to_string((*param)->name().value()));
							fscope->addSymbolInfo((*param)->name().value(), std::make_shared<Arg>(*param, cnt));
						}
						++cnt;
						++param;
					}
				}
				struct Fun :SymbolInfo {
					FunctionScope* fscope;
					std::shared_ptr<ast::expression> body;
					Fun(FunctionScope* fscope, std::shared_ptr<ast::expression> body) :fscope(fscope),body(body) {}
					llvm::Value* receive(Scope* s, std::shared_ptr<typename ast::apply_function> af)override {
						auto&& args = af->args();
						std::vector<std::shared_ptr<ast::type_data>> arg_type;
						std::vector<llvm::Value*> llvm_args;
						for (auto itr = begin(args); itr != end(args); ++itr) {
							llvm_args.push_back(process_expression(s, *itr));
							arg_type.push_back(resolve_type(s,(*itr)->return_type()));
						}

						/*if (target->type()!=ast::leaf_type::call_name) {
							throw "NIMPL";
						}
						auto target_cn = std::static_pointer_cast<ast::call_name>(target);*/
						auto ftd = std::make_shared<ast::function_type_data>(std::make_shared<ast::var_type_data>(), arg_type);
						auto tfscope = fscope->getTypedFunctionScope(
							s,
							ftd
						);
						if (!tfscope) {
							tfscope=TypedFunctionScope::create(fscope,ftd,llvm::GlobalValue::LinkageTypes::ExternalLinkage);
							std::vector<string_type> vec;
							for (auto itr = cbegin(arg_type); itr != cend(arg_type);++itr) {
								if ((*itr)->type()!=ast::type_data_type::simple) {
									throw "NIMPL";
								}
								vec.push_back(std::static_pointer_cast<ast::simple_type_data>(*itr)->value());
							}
							fscope->addTypedFunctionScope(vec,tfscope);
						}

						process_expression(tfscope, body);
						unify(s, af->return_type(), resolve_type(tfscope, body->return_type()));
						unify(tfscope, af->return_type(), resolve_type(s, body->return_type()));

						auto F = tfscope->getLLVMFunction();
						/*if (F->arg_size() != args.size()) {
							throw "not match args cnt";
						}*/
						Builder.SetInsertPoint(s->getLLVMBasicBlock(s));
						return Builder.CreateCall(F, llvm_args, "calltmp");
					}
					llvm::Value* receive(Scope* s, std::shared_ptr<typename ast::call_name> cn)override {
						//return F;
						throw "NIMPL";
					}
				};
				s->addSymbolInfo(
					n->name(),
					std::make_shared<Fun>(fscope,n->body()->body())
				);
				s->addChildScope(n->name(), fscope);
				return std::monostate{};
			}
			std::monostate  operator()(std::shared_ptr<ast::module_member>) {
				return std::monostate{};
			}
		};
		auto v = visitor_extract_named_fn{ s };
		ast::visit<std::monostate>(v, mod);
	}
	void process_module(module_visitor& v, Scope* s, std::shared_ptr<ast::module_member> mod) {

		ast::visit<std::nullptr_t>(v, mod);
	}
}
namespace cppcp= tig::cppcp;
int main() {

	std::u32string s = U"\
		module a {\n\
			def main():double={\
				//2.0*3.0\n\
				add(2.0,4.0)\n\
			}\
			def add( a:double , b:double ):double = {\n\
				val x = a*2.0\n\
				x+x\n\
			}\n\
			//def add2(a,b):double=(def sub(x,y):double=x-y)(a,b)+add(a,b)\n\
		}";
	auto gscopep = codegen::GlobalScope::create(codegen::PreludeScope::create());
	auto gscope=std::static_pointer_cast<codegen::GlobalScope>(codegen::scopes.get(gscopep));
	std::cout <<"taget triple:"<< gscope->getLLVMModule()->getTargetTriple()<<std::endl;
	auto ast =cppcp::get0(
		cppcp::join(
			cppcp::skip(cppcp::option(p::whitespace())),
			p::module_p()
		)
	)(u32src(cbegin(s), cend(s)));
	//std::shared_ptr<ast::declaration_module> m = ast.get();
	//std::cout << reinterpret_cast<uintptr_t>(m.get());
	register_module_symbols(gscopep, ast.get());
	process_module(gscopep,ast.get());
	codegen::scopes.clear();
	//process(gscope,p::expression()(ast.itr()).get());
	gscope->getLLVMModule()->print(llvm::errs(),nullptr);

	{
		// Initialize the target registry etc.
		//llvm::InitializeNativeTargetInfo();
		llvm::InitializeNativeTarget();
		//llvm::InitializeNativeTargetMC();
		llvm::InitializeNativeTargetAsmParser();
		llvm::InitializeNativeTargetAsmPrinter();

		auto TargetTriple = llvm::sys::getDefaultTargetTriple();
		gscope->getLLVMModule()->setTargetTriple(TargetTriple);

		std::string Error;
		auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);

		// Print an error and exit if we couldn't find the requested target.
		// This generally occurs if we've forgotten to initialise the
		// TargetRegistry or we have a bogus target triple.
		if (!Target) {
			llvm::errs() << Error;
			system("pause");
			return 1;
		}

		auto CPU = "generic";
		auto Features = "";

		llvm::TargetOptions opt;
		auto RM = llvm::Optional<llvm::Reloc::Model>();
		auto TheTargetMachine =
			Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);

		gscope->getLLVMModule()->setDataLayout(TheTargetMachine->createDataLayout());

		auto Filename = "C:\\Users\\tignear\\Desktop\\output.obj";
		std::error_code EC;
		llvm::raw_fd_ostream dest(Filename, EC, llvm::sys::fs::F_None);

		if (EC) {
			llvm::errs() << "Could not open file: " << EC.message();
			system("pause");

			return 1;
		}

		//tig::magimocha::codegen::writeObjectFileWithLegacyPass(gscope->getLLVMModule(), TheTargetMachine,dest);
	}
	system("pause");
}