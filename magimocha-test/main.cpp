#include "magimocha.h"
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
	constexpr auto isEnd() {
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
int main(int argc, char** args) {
	using u32src = src<std::u32string::const_iterator>;
	using x = tig::magimocha::rawast::p<u32src>;
	using ast = x::ast;
	using namespace std::string_literals;
	std::u32string s = U"1+1";

	auto p = x::operation();
	std::dynamic_pointer_cast<ast::call_name>(
		std::dynamic_pointer_cast<ast::apply_function>(p(src{ cbegin(s),cend(s) }).get())->target()
		)->value();
	
	return 0;
}