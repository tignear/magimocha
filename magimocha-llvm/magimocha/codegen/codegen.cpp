#include "magimocha/parser.h"
#include "magimocha/ast-visitor.h"
#include "codegen.h"
#include "pass.h"
#include "llvm-type.h"
#include "visitor.h"
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
				auto& params = n->body()->params();

				std::vector<llvm::Type*> Doubles(params.size(), llvm::Type::getDoubleTy(TheContext));
				llvm::FunctionType *FT = llvm::FunctionType::get(llvm::Type::getDoubleTy(TheContext), Doubles, false);

				llvm::Function *F = llvm::Function::Create(FT, llvm::Function::PrivateLinkage, to_string(s->mangling(n->name())), s->getLLVMModule());
				std::unordered_map<string_type,std::shared_ptr<SymbolInfo>> variables;
				struct Arg :SymbolInfo {
					llvm::Argument* farg;
					Arg(llvm::Argument* farg) :farg(farg) {}
					llvm::Value* receive(Scope*, std::shared_ptr<ast::call_name>)override {
						return farg;
					}
					llvm::Value* receive(Scope* s, std::shared_ptr<ast::apply_function> af)override {
						std::vector<llvm::Value*> llvm_args;
						for (auto&& arg : af->args()) {
							llvm_args.push_back(process_expression(s, arg));
						}
						Builder.SetInsertPoint(s->getLLVMBasicBlock(s));
						return Builder.CreateCall(farg, llvm_args,"lambda_calltmp");
					}
				};
				{
					auto farg = F->arg_begin();
					for (auto param = begin(params); param != end(params);) {

						if ((*param)->name()) {
							farg->setName(to_string((*param)->name().value()));
							variables[(*param)->name().value()] = std::make_shared<Arg>(farg);
						}
						++farg;
						++param;
					}
				}
				auto fscope = FunctionScope::create(s,n->name(), F, std::move(variables));
				struct Fun :SymbolInfo {
					llvm::Function* F;
					Fun(llvm::Function* F) :F(F) {}
					llvm::Value* receive(Scope* s, std::shared_ptr<typename ast::apply_function> af)override {
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
						return F;
					}
				};
				s->addSymbolInfo(
					n->name(),
					std::make_shared<Fun>(F)
				);
				s->addChildScope(n->name(),fscope);
				return std::monostate{};
			}
			std::monostate visit(std::shared_ptr<ast::expression>) {
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
			std::monostate visit(std::shared_ptr<ast::declaration_module> n) {
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
			std::monostate visit(std::shared_ptr<ast::named_function> n) {
				auto& params = n->body()->params();

				std::vector<llvm::Type*> Doubles(params.size(), llvm::Type::getDoubleTy(TheContext));
				llvm::FunctionType *FT = llvm::FunctionType::get(llvm::Type::getDoubleTy(TheContext), Doubles, false);

				llvm::Function *F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, to_string(s->mangling(n->name())), s->getLLVMModule());
				std::unordered_map<string_type, std::shared_ptr<SymbolInfo>> variables;
				struct Arg :SymbolInfo {
					llvm::Argument* farg;
					Arg(llvm::Argument* farg) :farg(farg) {}
					llvm::Value* receive(Scope*, std::shared_ptr<ast::call_name>)override {
						return farg;
					}
					llvm::Value* receive(Scope* s, std::shared_ptr<ast::apply_function> af)override {
						std::vector<llvm::Value*> llvm_args;
						for (auto&& arg : af->args()) {
							llvm_args.push_back(process_expression(s, arg));
						}
						Builder.SetInsertPoint(s->getLLVMBasicBlock(s));
						return Builder.CreateCall(farg, llvm_args,"lambda_calltmp");
					}
				};
				{
					auto farg = F->arg_begin();
					for (auto param = begin(params); param != end(params);) {

						if ((*param)->name()) {
							farg->setName(to_string((*param)->name().value()));
							variables[(*param)->name().value()] = std::make_shared<Arg>(farg);
						}
						++farg;
						++param;
					}
				}
				auto fscope = FunctionScope::create(s, n->name(), F, std::move(variables));
				struct Fun :SymbolInfo {
					llvm::Function* F;
					Fun(llvm::Function* f) :F(f) {}
					llvm::Value* receive(Scope* s, std::shared_ptr<typename ast::apply_function> af)override {
						auto&& args = af->args();
						if (F->arg_size() != args.size())
							throw "not match args cnt";

						std::vector<llvm::Value*> llvm_args;
						for (int i = 0; i < args.size(); ++i) {
							llvm_args.push_back(process_expression(s, args[i]));
						}
						Builder.SetInsertPoint(s->getLLVMBasicBlock(s));
						return Builder.CreateCall(F, llvm_args,"calltmp" );
					}
					llvm::Value* receive(Scope* s, std::shared_ptr<typename ast::call_name> cn) override {
						return F;
					}
				};
				s->addSymbolInfo(
					n->name(),
					std::make_shared<Fun>(F)
				);
				s->addChildScope(n->name(), fscope);
				return std::monostate{};
			}
			std::monostate visit(std::shared_ptr<ast::module_member>) {
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
				add(2.0,4.0)\
			}\
			def add( a , b ):double = {\n\
				val x = a*2.0\n\
				x+x\n\
			}\n\
			def add2(a,b):double=(def sub(x,y):double=x-y)(a,b)+add(a,b)\n\
		}";
	auto gscope = codegen::GlobalScope::create(codegen::PreludeScope::create());
	std::cout <<"taget triple:"<< gscope->getLLVMModule()->getTargetTriple()<<std::endl;
	auto ast =cppcp::get0(
		cppcp::join(
			cppcp::skip(cppcp::option(p::whitespace())),
			p::module_p()
		)
	)(u32src(cbegin(s), cend(s)));
	//std::shared_ptr<ast::declaration_module> m = ast.get();
	//std::cout << reinterpret_cast<uintptr_t>(m.get());
	register_module_symbols(gscope, ast.get());
	process_module(gscope,ast.get());
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

		tig::magimocha::codegen::writeObjectFileWithLegacyPass(gscope->getLLVMModule(), TheTargetMachine,dest);
	}
	system("pause");
}