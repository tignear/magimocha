#include "magimocha/parser.h"
#include "testutil.h"
#include "gtest/gtest.h"
#include <cstddef>
using u32src = src<std::u32string::const_iterator>;
using x = tig::magimocha::parser::p<u32src>;
namespace ast = tig::magimocha::ast;
namespace cppcp = tig::cppcp;
std::string ver_string(int a, int b, int c) {
    std::ostringstream ss;
    ss << a << '.' << b << '.' << c;
    return ss.str();
}

TEST(MagiMocha, anyc) {
    const std::u32string u32 = U"xyz";

    auto v = x::anyc()(src{cbegin(u32), cend(u32)});
    EXPECT_EQ(v.get(), U'x');
}
TEST(MagiMocha, any) {
    const std::u32string u32 = U"xyz";

    auto v = x::any()(src{cbegin(u32), cend(u32)});
    EXPECT_EQ(v.get(), std::u32string{U'x'});
}
TEST(MagiMocha, cr_success) {
    std::u32string u32 = {0x0D, 0x0A};
    auto p = x::CR();
    auto v = p(src{cbegin(u32), cend(u32)});
    EXPECT_EQ(v.get(), 0x0D);
}
TEST(MagiMocha, cr_fail) {
    std::u32string u32 = {0x0A};
    auto p = x::CR();
    EXPECT_THROW(p(src{cbegin(u32), cend(u32)}),
                 tig::magimocha::unexpected_token_exception);
}
TEST(MagiMocha, lf_success) {
    std::u32string u32 = {0x0A};
    auto p = x::LF();
    auto v = p(src{cbegin(u32), cend(u32)});
    EXPECT_EQ(v.get(), 0x0A);
}
TEST(MagiMocha, lf_fail) {
    std::u32string u32 = {0x0D};
    auto p = x::LF();
    EXPECT_THROW(p(src{cbegin(u32), cend(u32)}),
                 tig::magimocha::unexpected_token_exception);
}

TEST(MagiMocha, crlf_success) {
    std::u32string u32 = {0x0D, 0x0A};
    auto p = x::CRLF();
    auto v = p(src{cbegin(u32), cend(u32)});
    EXPECT_EQ(v.get(), u32);
}
TEST(MagiMocha, crlf_fail) {
    std::u32string u32 = {0x0D, 0x0F};
    auto p = x::CRLF();
    EXPECT_THROW(p(src{cbegin(u32), cend(u32)}),
                 tig::magimocha::unexpected_token_exception);
}
TEST(MagiMocha, line_break_success_crlf) {
    using namespace std::string_literals;
    std::u32string r = {0x0D, 0x0A};
    std::u32string t = r;

    auto p = x::line_break();
    auto v = p(src{cbegin(t), cend(t)});
    EXPECT_EQ(v.get(), r);
}
TEST(MagiMocha, line_break_success_cr) {
    std::u32string t = {0x0D, 0x98};

    std::u32string r = {0x0D};
    auto p = x::line_break();
    auto v = p(src{cbegin(t), cend(t)});
    EXPECT_EQ(v.get(), r);
}
TEST(MagiMocha, line_break_success_lf) {
    std::u32string t = {0x0A, 0x98};

    std::u32string r = {0x0A};
    auto p = x::line_break();
    auto v = p(src{cbegin(t), cend(t)});
    EXPECT_EQ(v.get(), r);
}
TEST(MagiMocha, line_break_fail) {
    std::u32string u32 = {0x0F, 0x0F};
    auto p = x::line_break();
    EXPECT_THROW(p(src{cbegin(u32), cend(u32)}),
                 tig::magimocha::unexpected_token_exception);
}
TEST(MagiMocha, single_line_comment_with_eof) {
    using namespace std::string_literals;
    std::u32string s = U"//xyz"s;
    auto p = x::single_line_comment();
    EXPECT_EQ(p(src{cbegin(s), cend(s)}).get(), s);
}

TEST(MagiMocha, single_line_comment_with_cr) {
    using namespace std::string_literals;
    std::u32string s = U"//xyz\rxxx"s;

    auto p = x::single_line_comment();
    EXPECT_EQ(p(src{cbegin(s), cend(s)}).get(), U"//xyz\r");
}
TEST(MagiMocha, single_line_comment_with_crlf) {
    using namespace std::string_literals;
    std::u32string s = U"//xyz\r\nxxx"s;

    auto p = x::single_line_comment();
    EXPECT_EQ(p(src{cbegin(s), cend(s)}).get(), U"//xyz\r\n");
}
TEST(MagiMocha, single_line_comment_with_lf) {
    using namespace std::string_literals;
    std::u32string s = U"//xyz\nxxx"s;

    auto p = x::single_line_comment();
    EXPECT_EQ(p(src{cbegin(s), cend(s)}).get(), U"//xyz\n");
}

TEST(MagiMocha, multi_line_comment) {
    using namespace std::string_literals;
    std::u32string s = U"/*xyz*/";

    auto p = x::multi_line_comment();
    EXPECT_EQ(p(src{cbegin(s), cend(s)}).get(), s);
}
TEST(MagiMocha, whitespace) {
    std::u32string u32 = U"   \t";
    auto p = x::whitespace();
    auto v = p(src{cbegin(u32), cend(u32)});
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
    EXPECT_EQ(p(src{cbegin(s), cend(s)}).get()->value(), 0b11010);
}
TEST(MagiMocha, octal_literal) {
    using namespace std::string_literals;
    std::u32string s = U"0o75_45";

    auto p = x::octal_literal();
    EXPECT_EQ(p(src{cbegin(s), cend(s)}).get()->value(), 07545);
}
TEST(MagiMocha, decimal_literal) {
    using namespace std::string_literals;
    std::u32string s = U"75_45";

    auto p = x::decimal_literal();
    EXPECT_EQ(p(src{cbegin(s), cend(s)}).get()->value(), 7545);
}
TEST(MagiMocha, hexadecimal_literal) {
    using namespace std::string_literals;
    std::u32string s = U"0x7_545";

    auto p = x::hexadecimal_literal();
    EXPECT_EQ(p(src{cbegin(s), cend(s)}).get()->value(), 0x7545);
}

TEST(MagiMocha, integer_literal) {
    using namespace std::string_literals;
    std::u32string s2 = U"0b110_10";

    std::u32string s8 = U"0o75_45";
    std::u32string s10 = U"75_45";

    std::u32string s16 = U"0x7_545";

    auto p = x::integer_literal();
    EXPECT_EQ(p(src{cbegin(s2), cend(s2)}).get()->value(), 0b11010);
    EXPECT_EQ(p(src{cbegin(s8), cend(s8)}).get()->value(), 07545);
    EXPECT_EQ(p(src{cbegin(s10), cend(s10)}).get()->value(), 7545);
    EXPECT_EQ(p(src{cbegin(s16), cend(s16)}).get()->value(), 0x7545);
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
    EXPECT_EQ(p(src{cbegin(s1), cend(s1)}).get()->value(), -0.635);
    EXPECT_EQ(p(src{cbegin(s2), cend(s2)}).get()->value(), 0x0.756P4);
    EXPECT_EQ(p(src{cbegin(s3), cend(s3)}).get()->value(), -0.756E4);
    EXPECT_EQ(p(src{cbegin(s4), cend(s4)}).get()->value(), 0.756E-4);
}
/*
numeric literal
*/

/*template<class R,class T>
auto dynamic(T&& t) {
        return dynamic_impl<R,T>(std::move(t));
}*/
TEST(MagiMocha, numeric_literal_unsigned) {
    using namespace std::string_literals;
    std::u32string s2 = U"0b110_10";
    std::u32string s8 = U"0o75_45";
    std::u32string s10 = U"75_45";
    std::u32string s16 = U"0x7_545";

    auto p = x::numeric_literal();
    EXPECT_EQ(std::dynamic_pointer_cast<ast::unsigned_number_literal>(
                  p(src{cbegin(s2), cend(s2)}).get())
                  ->value(),
              0b11010);
    EXPECT_EQ(std::dynamic_pointer_cast<ast::unsigned_number_literal>(
                  p(src{cbegin(s8), cend(s8)}).get())
                  ->value(),
              07545);
    EXPECT_EQ(std::dynamic_pointer_cast<ast::unsigned_number_literal>(
                  p(src{cbegin(s10), cend(s10)}).get())
                  ->value(),
              7545);
    EXPECT_EQ(std::dynamic_pointer_cast<ast::unsigned_number_literal>(
                  p(src{cbegin(s16), cend(s16)}).get())
                  ->value(),
              0x7545);
}
TEST(MagiMocha, numeric_literal_signed) {
    using namespace std::string_literals;

    std::u32string s2 = U"-0b110_10";
    std::u32string s8 = U"-0o75_45";
    std::u32string s10 = U"-75_45";
    std::u32string s16 = U"-0x7_545";

    auto p = x::numeric_literal();
    EXPECT_EQ(std::dynamic_pointer_cast<ast::signed_number_literal>(
                  p(src{cbegin(s2), cend(s2)}).get())
                  ->value(),
              -0b11010);
    EXPECT_EQ(std::dynamic_pointer_cast<ast::signed_number_literal>(
                  p(src{cbegin(s8), cend(s8)}).get())
                  ->value(),
              -07545);
    EXPECT_EQ(std::dynamic_pointer_cast<ast::signed_number_literal>(
                  p(src{cbegin(s10), cend(s10)}).get())
                  ->value(),
              -7545);
    EXPECT_EQ(std::dynamic_pointer_cast<ast::signed_number_literal>(
                  p(src{cbegin(s16), cend(s16)}).get())
                  ->value(),
              -0x7545);
}
TEST(MagiMocha, numeric_literal_floating) {

    std::u32string fs1 = U"-0.635";
    std::u32string fs2 = U"0x0.756P4";
    std::u32string fs3 = U"-0.756E4";
    std::u32string fs4 = U"0.756E-4";
    auto p = x::numeric_literal();

    EXPECT_EQ(std::dynamic_pointer_cast<ast::floating_literal>(
                  p(src{cbegin(fs1), cend(fs1)}).get())
                  ->value(),
              -0.635);
    EXPECT_EQ(std::dynamic_pointer_cast<ast::floating_literal>(
                  p(src{cbegin(fs2), cend(fs2)}).get())
                  ->value(),
              0x0.756P4);
    EXPECT_EQ(std::dynamic_pointer_cast<ast::floating_literal>(
                  p(src{cbegin(fs3), cend(fs3)}).get())
                  ->value(),
              -0.756E4);
    EXPECT_EQ(std::dynamic_pointer_cast<ast::floating_literal>(
                  p(src{cbegin(fs4), cend(fs4)}).get())
                  ->value(),
              0.756E-4);
}

TEST(MagiMocha, string_literal) {

    std::u32string s1 = U"\"-0.635\"";

    auto p = x::string_literal();

    EXPECT_EQ(p(src{cbegin(s1), cend(s1)}).get()->value(), U"-0.635");
}
TEST(MagiMocha, identifier_head) {
    using namespace std::string_literals;
    std::u32string s = U"xyz";

    auto p = x::identifier_head();
    EXPECT_EQ(p(src{cbegin(s), cend(s)}).get(), U'x');
}
/*TEST(MagiMocha, declation_name) {
        using namespace std::string_literals;
        std::u32string s = U"xyz//";

        auto p = x::declaration_name();
        EXPECT_EQ(p(src{ cbegin(s),cend(s) }).get()->value(), U"xyz");
}*/

TEST(MagiMocha, declation_lambda_arg2) {
    using namespace std::string_literals;
    std::u32string str = U"\\(x,y)->1+1"s;

    auto p = x::declaration_lambda();
    auto br = p(src{cbegin(str), cend(str)});
    auto r = br.get()->params();
    // EXPECT_EQ(r.at(0)->name(),U"x" );
    // EXPECT_EQ(r.at(1)->name(), U"y");
}
TEST(MagiMocha, declation_lambda_arg_0) {
    using namespace std::string_literals;
    std::u32string s = U"\\()->1+1";

    auto p = x::declaration_lambda();
    EXPECT_TRUE(p(src{cbegin(s), cend(s)}).get()->params().empty());
}
TEST(MagiMocha, declation_lambda_arg_ignore) {
    using namespace std::string_literals;
    std::u32string s = U"\\(_,_)->1+1";

    auto p = x::declaration_lambda();
    auto r = p(src{cbegin(s), cend(s)}).get();
    auto pa = r->params();
    std::optional<std::u32string> n1 = pa.at(0)->name();
    EXPECT_EQ(n1, std::nullopt);
    std::optional<std::u32string> n2 = pa.at(0)->name();
    EXPECT_EQ(n2, std::nullopt);
}
TEST(MagiMocha, declaration_lambda_bad) {
    using namespace std::string_literals;
    std::u32string s = U"\\(x,)";

    auto p = x::declaration_lambda();
    EXPECT_THROW(p(src{cbegin(s), cend(s)}).get(),
                 tig::magimocha::unexpected_token_exception);
}
TEST(MagiMocha, declaration_lambda_bad2) {
    using namespace std::string_literals;
    std::u32string s = U"\\(0x,x)";

    auto p = x::declaration_lambda();
    EXPECT_THROW(p(src{cbegin(s), cend(s)}).get(),
                 tig::magimocha::unexpected_token_exception);
}
/*TEST(MagiMocha, operation) {
        using namespace std::string_literals;
        std::u32string s = U"1+1";

        auto p = x::operation();
        EXPECT_EQ(
                std::dynamic_pointer_cast<ast::call_name>(
                        std::dynamic_pointer_cast<ast::apply_function>(p(src{
cbegin(s),cend(s) }).get())->target()
                )->value(),U"+"
        );

}*/
TEST(MagiMocha, expression) {
    using namespace std::string_literals;
    std::u32string s = U"1+1*2**9/6";

    auto p = x::expression();
    auto r = p(src{cbegin(s), cend(s)}).get();
    EXPECT_TRUE(r->type() == ast::leaf_type::operation);
}

TEST(MagiMocha, operator_tokenizer) {
    using namespace std::string_literals;
    std::u32string s = U"1+1*2**9/6";

    auto p = x::operator_tokenizer();
    auto r = p(src{cbegin(s), cend(s)}).get();
    EXPECT_TRUE(r.size() == 9);
}

TEST(MagiMocha, named_function_expression_scope) {
    using namespace std::string_literals;

    std::u32string s = U"def id(x) = x";
    auto p = x::named_function_expression_scope();
    auto r = p(src{cbegin(s), cend(s)}).get();
}

TEST(MagiMocha, expression_block) {
    using namespace std::string_literals;

    std::u32string s = U"{def id(x) = x;val r=32;r}";
    auto p = x::expression_block();
    auto r = p(src{cbegin(s), cend(s)}).get();
    EXPECT_TRUE(r);
    EXPECT_EQ(r->type(), ast::leaf_type::expression_block);
    auto rr = std::static_pointer_cast<ast::expression_block>(r);
    const auto &vec = rr->value();
    EXPECT_EQ(vec.size(), 3);
    EXPECT_EQ(vec[0]->type(), ast::leaf_type::named_function);
    EXPECT_EQ(vec[1]->type(), ast::leaf_type::declaration_variable);
    EXPECT_EQ(vec[2]->type(), ast::leaf_type::operation);
    auto v2v = std::static_pointer_cast<ast::operation>(vec[2]);

    EXPECT_EQ(v2v->value().size(), 1);
    EXPECT_TRUE(std::holds_alternative<std::shared_ptr<ast::call_name>>(
        v2v->value().front()));
}
TEST(MagiMocha, infix) {
    using namespace std::string_literals;
    std::u32string s = U"infix Id right 250";
    auto p = x::declaration_infix();
    auto r = p(src{cbegin(s), cend(s)}).get();
    EXPECT_TRUE(r);
    EXPECT_EQ(r->type(), ast::leaf_type::declaration_infix);
    EXPECT_EQ(r->name()->value(), std::vector<ast::string_type>{U"Id"});
    EXPECT_EQ(r->infix_type(), ast::infix_type::right);
    EXPECT_EQ(r->priority(), 250);
}

TEST(MagiMocha, infix_in_expression_block) {
    using namespace std::string_literals;
    std::u32string s = U"{infix Id right 250}";
    auto p = x::expression_block();
    auto rraw = p(src{cbegin(s), cend(s)}).get();
    EXPECT_EQ(rraw->type(), ast::leaf_type::expression_block);
    auto r = std::static_pointer_cast<ast::expression_block>(rraw);
    auto r2raw = r->value().at(0);
    EXPECT_TRUE(r2raw);
    EXPECT_EQ(r2raw->type(), ast::leaf_type::declaration_infix);
    auto r2 = std::static_pointer_cast<ast::declaration_infix>(r2raw);
    EXPECT_EQ(r2->name()->value(), std::vector<ast::string_type>{U"Id"});
    EXPECT_EQ(r2->infix_type(), ast::infix_type::right);
    EXPECT_EQ(r2->priority(), 250);
}
TEST(MagiMocha, module_basic) {
    using namespace std::string_literals;
    std::u32string s = U"module xxx{def Id(x) = x\n infix  Id right 450}";
    auto p = x::module_p();
    auto r = p(src{cbegin(s), cend(s)}).get();
}
TEST(MagiMocha, call_name_module_name) {
    using namespace std::string_literals;
    std::u32string s = U"module xxx{def Id(x) = xxx.yyy.x(x)}";
    auto p = x::module_p();
    auto r = p(src{cbegin(s), cend(s)}).get();
}
TEST(MagiMocha, maaain) {
    std::u32string s = U"\
		module a {\n\
			infix + right 250\n\
			infix * right 500\n\
            def +(_:Double,_:Double):Double=0.0\n\
            def *(_:Double,_:Double):Double=0.0\n\
            def add( a:Double , b:Double ):Double = {\n\
				val x = a*2.0\n\
                val y = x*4.0\n\
				x+y\n\
			}\n\
			def main():Double={\
				//2.0*3.0\n\
				add(2.0,4.0)\n\
			}\
			//def add2(a,b):Double=(def sub(x,y):Double=x-y)(a,b)+add(a,b)\n\
		}";
    auto ast =
        cppcp::get0(cppcp::join(cppcp::skip(cppcp::option(x::whitespace())),
                                x::module_p()))(src(cbegin(s), cend(s)))
            .get();
}
/*TEST(MagiMocha, nyan) {
        std::tuple<
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string,
                std::string ,
                std::string > s={
                std::string{{5,7,26,89}},
                                std::string{{5,7,26,89}},
                std::string{{5,7,26,89}},
                                std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},
std::string{{5,7,26,89}},

std::string{{5,7,26,89}}

        };
        std::cout<<std::get<5>(s);
}*/