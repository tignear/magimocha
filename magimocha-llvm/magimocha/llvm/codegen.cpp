#include "magimocha/parser.h"
#include "magimocha/ast-visitor.h"
#include "codegen.h"
#include "visitor.h"
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
		struct visitor_extract_named_fn {
			Scope* s;
			std::monostate visit(std::shared_ptr<ast::named_function> n) {
				auto& params = n->body()->params();

				std::vector<llvm::Type*> Doubles(params.size(), llvm::Type::getDoubleTy(TheContext));
				llvm::FunctionType *FT = llvm::FunctionType::get(llvm::Type::getDoubleTy(TheContext), Doubles, false);

				llvm::Function *F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, to_string(n->name()), s->getLLVMModule());
				std::unordered_map<string_type, VariableInfo> variables;
				auto farg = F->arg_begin();
				{
					for (auto param = begin(params); param != end(params);) {

						if ((*param)->name()) {
							farg->setName(to_string((*param)->name().value()));
							variables[(*param)->name().value()] = VariableInfo{
								[farg](Scope*,std::shared_ptr<ast::call_name>) {
									return farg;
								}
							};
						}
						++farg;
						++param;
					}
				}
				auto fscope = FunctionScope::create(s, F, std::move(variables));
				s->addFunctionInfo(
					s,
					n->name(),
					FunctionInfoThisScope{
						[F](auto,auto)->llvm::Value* {
							return F;
						},
						fscope
					}
				);
				return std::monostate{};
			}
			std::monostate visit(std::shared_ptr<ast::expression> n) {
				return std::monostate{};
			}
		};
		ast::visit<std::monostate>(visitor_extract_named_fn{s},expr);
		return ast::visit<llvm::Value*>(v, expr);
	}
	void process_module(Scope* s, std::shared_ptr<ast::module_member> mod) {
		auto v=module_visitor{ s };
		process_module(v, s, mod);
	}
	void process_module(module_visitor& v, Scope* s, std::shared_ptr<ast::module_member> mod) {
		ast::visit<std::nullptr_t>(v, mod);
	}
}

int main() {

	//std::u32string s = U"def aaa(a,c)=(def b()=0.0)()";
	std::u32string s = U"module a{def add(a,b)=a+b\ndef sub(a,b)=a-b}";
	auto gscope = codegen::GlobalScope::create(codegen::PreludeScope::create());
	auto ast = p::module_p()(u32src(cbegin(s), cend(s)));
	std::shared_ptr<ast::declaration_module> m = ast.get();
	std::cout << reinterpret_cast<uintptr_t>(m.get());
	//process(gscope,ast.get());
	//process(gscope,p::expression()(ast.itr()).get());
	//gscope->getLLVMModule()->print(llvm::errs(),nullptr);
	system("pause");
}