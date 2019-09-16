#include "magimocha/codegen/codegen.h"
#include "magimocha/ast.h"
#include "magimocha/parser.h"
#include "magimocha/convert-ast.h"
#include "magimocha/operation.h"
#include "magimocha/typing.h"
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
namespace cg2=tig::magimocha::codegen2;
namespace cppcp = tig::cppcp;

int main() {
    std::u32string s = U"\
		module a {\n\
			def main():double={\
				//2.0*3.0\n\
				add(2.0,4.0)\n\
			}\
			def add( a:double , b:double ):double = {\n\
				val x = a*2.0\n\
                val y = x*4.0\n\
				x+y\n\
			}\n\
			//def add2(a,b):double=(def sub(x,y):double=x-y)(a,b)+add(a,b)\n\
		}";
    auto ast =
        cppcp::get0(cppcp::join(cppcp::skip(cppcp::option(p::whitespace())),
                                p::module_p()))(u32src(cbegin(s), cend(s))).get();
    auto ast2 = cg2::convert_scope(ast);
    auto info_table = std::make_shared<cg2::operator_info_table_impl>(
        std::shared_ptr<cg2::operator_info_table>());
    std::unordered_map<std::shared_ptr<ast::make_scope>,
                       std::shared_ptr<cg2::operator_info_table>>
        map;
    cg2::extract_operator_info( ast::to_ast_leaf(ast2),info_table, map);
    auto r=cg2::operation_to_function_applying_all_leaf(ast::to_ast_leaf(ast2),info_table,map);
	
    //cg2::codegen(std::static_pointer_cast<ast::declaration_module>(r),);
}