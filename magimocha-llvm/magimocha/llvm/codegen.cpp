#include "magimocha/parser.h"
#include "magimocha/ast-visitor.h"
#include "codegen.h"
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
	llvm::Value* process(std::shared_ptr<Scope> s, std::shared_ptr<ast::expression> expr) {
		visitor v{ s };
		return ast::visit<llvm::Value*>(v, expr);
	}
	llvm::Value* process(visitor& v,std::shared_ptr<ast::expression> expr) {
		
		return ast::visit<llvm::Value*>(v, expr);
	}
}

int main() {

	//std::u32string s = U"def aaa(a,c)=(def b()=0.0)()";
	std::u32string s = U"def add(a,b)=a+b";
	auto gscope = codegen::GlobalScope::create();
	auto ast = p::expression()(u32src(cbegin(s), cend(s))).get();
	process(gscope,ast);
	
	gscope->getLLVMModule()->print(llvm::errs(),nullptr);
	system("pause");
}