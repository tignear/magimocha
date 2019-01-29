#include "gtest/gtest.h"
#include "magimocha.h"
template<typename Iterator>
struct src {
	Iterator it_;
	Iterator end_;
	src() = default;
	src(Iterator it,Iterator end) noexcept : it_(it),end_(end) {}
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
using u32src = src<std::u32string::const_iterator>;
using x = tig::magimocha::rawast::p<u32src>;
using ast = x::ast;
TEST(MagiMocha, anyc) {
	const std::u32string u32 = U"xyz";

	auto v=x::anyc()(src{cbegin(u32),cend(u32)});
	EXPECT_EQ(v.get(), U'x');
}
TEST(MagiMocha, any) {
	const std::u32string u32 = U"xyz";

	auto v = x::any()(src{ cbegin(u32),cend(u32) });
	EXPECT_EQ(v.get(), std::u32string{U'x'});
}
TEST(MagiMocha, cr_success) {
	std::u32string u32 = {0x0D,0x0A};
	auto p = x::CR();
	auto v=p(src{ cbegin(u32),cend(u32) });
	EXPECT_EQ(v.get(),  0x0D );

}
TEST(MagiMocha,cr_fail) {
	std::u32string u32 = {0x0A };
	auto p = x::CR();
	EXPECT_THROW(p(src{ cbegin(u32),cend(u32) }), tig::magimocha::unexpected_token_exception);
}
TEST(MagiMocha, lf_success) {
	std::u32string u32 = { 0x0A };
	auto p = x::LF();
	auto v = p(src{ cbegin(u32),cend(u32) });
	EXPECT_EQ(v.get(), 0x0A );

}
TEST(MagiMocha, lf_fail) {
	std::u32string u32 = { 0x0D };
	auto p = x::LF();
	EXPECT_THROW(p(src{ cbegin(u32),cend(u32) }), tig::magimocha::unexpected_token_exception);
}

TEST(MagiMocha, crlf_success) {
	std::u32string u32 = { 0x0D,0x0A };
	auto p = x::CRLF();
	auto v = p(src{ cbegin(u32),cend(u32) });
	EXPECT_EQ(v.get(), u32);

}
TEST(MagiMocha, crlf_fail) {
	std::u32string u32 = { 0x0D ,0x0F };
	auto p = x::CRLF();
	EXPECT_THROW(p(src{ cbegin(u32),cend(u32) }), tig::magimocha::unexpected_token_exception);
}
TEST(MagiMocha, line_break_success_crlf) {
	using namespace  std::string_literals;
	std::u32string r = { 0x0D,0x0A };
	std::u32string t = r;

	auto p = x::line_break();
	auto v = p(src{ cbegin(t),cend(t) });
	EXPECT_EQ(v.get(), r);
}
TEST(MagiMocha, line_break_success_cr) {
	std::u32string t = { 0x0D,0x98 };

	std::u32string r = { 0x0D};
	auto p = x::line_break();
	auto v = p(src{ cbegin(t),cend(t) });
	EXPECT_EQ(v.get(), r);
}
TEST(MagiMocha, line_break_success_lf) {
	std::u32string t = { 0x0A,0x98 };

	std::u32string r = { 0x0A };
	auto p = x::line_break();
	auto v = p(src{ cbegin(t),cend(t) });
	EXPECT_EQ(v.get(), r);
}
TEST(MagiMocha, line_break_fail) {
	std::u32string u32 = { 0x0F ,0x0F };
	auto p = x::line_break();
	EXPECT_THROW(p(src{ cbegin(u32),cend(u32) }), tig::magimocha::unexpected_token_exception);
}
TEST(MagiMocha, single_line_comment_with_eof) {
	using namespace std::string_literals;
	std::u32string s = U"//xyz"s;
	auto p = x::single_line_comment();
	EXPECT_EQ(p(src{ cbegin(s),cend(s) }).get(), s);
}

TEST(MagiMocha, single_line_comment_with_cr) {
	using namespace std::string_literals;
	std::u32string s = U"//xyz\rxxx"s;
	
	auto p = x::single_line_comment();
	EXPECT_EQ(p(src{ cbegin(s),cend(s) }).get(), U"//xyz\r");
}
TEST(MagiMocha, single_line_comment_with_crlf) {
	using namespace std::string_literals;
	std::u32string s = U"//xyz\r\nxxx"s;

	auto p = x::single_line_comment();
	EXPECT_EQ(p(src{ cbegin(s),cend(s) }).get(), U"//xyz\r\n");
}
TEST(MagiMocha, single_line_comment_with_lf) {
	using namespace std::string_literals;
	std::u32string s = U"//xyz\nxxx"s;

	auto p = x::single_line_comment();
	EXPECT_EQ(p(src{ cbegin(s),cend(s) }).get(), U"//xyz\n");
}

TEST(MagiMocha, multi_line_comment) {
	using namespace std::string_literals;
	std::u32string s = U"/*xyz*/";

	auto p = x::multi_line_comment();
	EXPECT_EQ(p(src{ cbegin(s),cend(s) }).get(),s);
}
TEST(MagiMocha, whitespace) {
	std::u32string u32 =U"   \t";
	auto p = x::whitespace();
	auto v = p(src{ cbegin(u32),cend(u32) });
	EXPECT_EQ(v.get(), u32);

}
/*
literals
*/
/*
integer literals 
*/
TEST(MagiMocha, binary_literal) {
	using namespace std::string_literals;
	std::u32string s = U"0b110_10";

	auto p = x::binary_literal();
	EXPECT_EQ(p(src{ cbegin(s),cend(s) }).get()->value(), 0b11010);
}
TEST(MagiMocha, octal_literal) {
	using namespace std::string_literals;
	std::u32string s = U"0o75_45";

	auto p = x::octal_literal();
	EXPECT_EQ(p(src{ cbegin(s),cend(s) }).get()->value(), 07545);
}
TEST(MagiMocha, decimal_literal) {
	using namespace std::string_literals;
	std::u32string s = U"75_45";

	auto p = x::decimal_literal();
	EXPECT_EQ(p(src{ cbegin(s),cend(s) }).get()->value(), 7545);
}
TEST(MagiMocha, hexadecimal_literal) {
	using namespace std::string_literals;
	std::u32string s = U"0x7_545";

	auto p = x::hexadecimal_literal();
	EXPECT_EQ(p(src{ cbegin(s),cend(s) }).get()->value(), 0x7545);
}

TEST(MagiMocha, integer_literal) {
	using namespace std::string_literals;
	std::u32string s2= U"0b110_10";

	std::u32string s8 = U"0o75_45";
	std::u32string s10 = U"75_45";

	std::u32string s16 = U"0x7_545";


	auto p = x::integer_literal();
	EXPECT_EQ(p(src{ cbegin(s2),cend(s2) }).get()->value(), 0b11010);
	EXPECT_EQ(p(src{ cbegin(s8),cend(s8) }).get()->value(), 07545);
	EXPECT_EQ(p(src{ cbegin(s10),cend(s10) }).get()->value(), 7545);
	EXPECT_EQ(p(src{ cbegin(s16),cend(s16) }).get()->value(), 0x7545);

}
/*
floating literals
*/
TEST(MagiMocha, floating_literal) {
	using namespace std::string_literals;
	std::u32string s1 = U"-0.635";

	std::u32string s2 = U"0x0.756P4";
	std::u32string s3 = U"-0.756E4";

	std::u32string s4 = U"0.756E-4";


	auto p = x::floating_point_literal();
	EXPECT_EQ(p(src{ cbegin(s1),cend(s1) }).get()->value(), -0.635);
	EXPECT_EQ(p(src{ cbegin(s2),cend(s2) }).get()->value(), 0x0.756P4);
	EXPECT_EQ(p(src{ cbegin(s3),cend(s3) }).get()->value(), -0.756E4);
	EXPECT_EQ(p(src{ cbegin(s4),cend(s4) }).get()->value(), 0.756E-4);

}
/*
numeric literal
*/

template<class R,class T>
auto dynamic(T&& t) {
	return dynamic_impl<R,T>(std::move(t));
}
TEST(MagiMocha, numeric_literal_unsigned) {
	using namespace std::string_literals;
	std::u32string s2 = U"0b110_10";
	std::u32string s8 = U"0o75_45";
	std::u32string s10 = U"75_45";
	std::u32string s16 = U"0x7_545";

	auto p = x::numeric_literal();
	EXPECT_EQ(std::dynamic_pointer_cast<ast::unsigned_number_literal>(p(src{ cbegin(s2),cend(s2) }).get())->value(), 0b11010);
	EXPECT_EQ(std::dynamic_pointer_cast<ast::unsigned_number_literal>(p(src{ cbegin(s8),cend(s8) }).get())->value(), 07545);
	EXPECT_EQ(std::dynamic_pointer_cast<ast::unsigned_number_literal>(p(src{ cbegin(s10),cend(s10) }).get())->value(), 7545);
	EXPECT_EQ(std::dynamic_pointer_cast<ast::unsigned_number_literal>(p(src{ cbegin(s16),cend(s16) }).get())->value(), 0x7545);

}
TEST(MagiMocha, numeric_literal_signed) {
	using namespace std::string_literals;

	std::u32string s2 = U"-0b110_10";
	std::u32string s8 = U"-0o75_45";
	std::u32string s10 = U"-75_45";
	std::u32string s16 = U"-0x7_545";

	auto p = x::numeric_literal();
	EXPECT_EQ(std::dynamic_pointer_cast<ast::signed_number_literal>(p(src{ cbegin(s2),cend(s2) }).get())->value(), -0b11010);
	EXPECT_EQ(std::dynamic_pointer_cast<ast::signed_number_literal>(p(src{ cbegin(s8),cend(s8) }).get())->value(), -07545);
	EXPECT_EQ(std::dynamic_pointer_cast<ast::signed_number_literal>(p(src{ cbegin(s10),cend(s10) }).get())->value(), -7545);
	EXPECT_EQ(std::dynamic_pointer_cast<ast::signed_number_literal>(p(src{ cbegin(s16),cend(s16) }).get())->value(), -0x7545);

}
TEST(MagiMocha, numeric_literal_floating) {

	std::u32string fs1 = U"-0.635";
	std::u32string fs2 = U"0x0.756P4";
	std::u32string fs3 = U"-0.756E4";
	std::u32string fs4 = U"0.756E-4";
	auto p = x::numeric_literal();

	EXPECT_EQ(std::dynamic_pointer_cast<ast::floating_literal>(p(src{ cbegin(fs1),cend(fs1) }).get())->value(), -0.635);
	EXPECT_EQ(std::dynamic_pointer_cast<ast::floating_literal>(p(src{ cbegin(fs2),cend(fs2) }).get())->value(), 0x0.756P4);
	EXPECT_EQ(std::dynamic_pointer_cast<ast::floating_literal>(p(src{ cbegin(fs3),cend(fs3) }).get())->value(), -0.756E4);
	EXPECT_EQ(std::dynamic_pointer_cast<ast::floating_literal>(p(src{ cbegin(fs4),cend(fs4) }).get())->value(), 0.756E-4);

}


TEST(MagiMocha, string_literal) {

	std::u32string s1 = U"\"-0.635\"";

	auto p = x::string_literal();

	EXPECT_EQ(p(src{ cbegin(s1),cend(s1) }).get()->value(), U"-0.635");


}
TEST(MagiMocha, identifier_head) {
	using namespace std::string_literals;
	std::u32string s = U"xyz";

	auto p = x::identifier_head();
	EXPECT_EQ(p(src{ cbegin(s),cend(s) }).get(), U'x');
}
TEST(MagiMocha, declation_name) {
	using namespace std::string_literals;
	std::u32string s = U"xyz//";

	auto p = x::declaration_name();
	EXPECT_EQ(p(src{ cbegin(s),cend(s) }).get()->name(), ast::declaration_name(U"xyz").name());
}

TEST(MagiMocha, declation_lambda_arg2) {
	using namespace std::string_literals;
	std::u32string s = U"&(x,y)";

	auto p = x::declaration_lambda();
	auto r = std::get<0>(p(src{ cbegin(s),cend(s) }).get());
	EXPECT_EQ(r.at(0)->name(),U"x" );
	EXPECT_EQ(r.at(1)->name(), U"y");
}
TEST(MagiMocha, declation_lambda_arg_0) {
	using namespace std::string_literals;
	std::u32string s = U"&()";

	auto p = x::declaration_lambda();
	EXPECT_TRUE(std::get<0>(p(src{ cbegin(s),cend(s) }).get()).empty());

}
TEST(MagiMocha, declation_lambda_arg_ignore) {
	using namespace std::string_literals;
	std::u32string s = U"&(_,_)";

	auto p = x::declaration_lambda();
	auto r = std::get<0>(p(src{ cbegin(s),cend(s) }).get());

	EXPECT_EQ(r.at(0)->name(),std::nullopt);
	EXPECT_EQ(r.at(1)->name(), std::nullopt);
}
TEST(MagiMocha, declaration_lambda_bad) {
	using namespace std::string_literals;
	std::u32string s = U"&(x,)";

	auto p = x::declaration_lambda();
	EXPECT_THROW(p(src{ cbegin(s),cend(s) }).get(),tig::magimocha::unexpected_token_exception);

}
TEST(MagiMocha, declaration_lambda_bad2) {
	using namespace std::string_literals;
	std::u32string s = U"&(0x,x)";

	auto p = x::declaration_lambda();
	EXPECT_THROW(p(src{ cbegin(s),cend(s) }).get(), tig::magimocha::unexpected_token_exception);

}
TEST(MagiMocha, expression) {
	using namespace std::string_literals;
	std::u32string s = U"&(0x,x)";

	auto p = x::expression();
	EXPECT_THROW(p(src{ cbegin(s),cend(s) }).get(), tig::magimocha::unexpected_token_exception);

}