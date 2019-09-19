#include "magimocha/ast.h"
#include "magimocha/codegen/codegen.h"
#include "magimocha/convert-ast.h"
#include "magimocha/operation.h"
#include "magimocha/parser.h"
#include "magimocha/typing.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
template <typename Iterator> struct src {
    Iterator it_;
    Iterator end_;
    src() = default;
    src(Iterator it, Iterator end) noexcept : it_(it), end_(end) {}
    src(const src &) = default;
    src(src &&) = default;
    src &operator=(const src &) = default;
    src &operator=(src &&) = default;

    using iterator_category =
        typename std::iterator_traits<Iterator>::iterator_category;
    using value_type = typename std::iterator_traits<Iterator>::value_type;
    using difference_type =
        typename std::iterator_traits<Iterator>::difference_type;
    using pointer = typename std::iterator_traits<Iterator>::pointer;
    using reference = typename std::iterator_traits<Iterator>::reference;
    constexpr auto is_end() { return it_ == end_; }
    const value_type &operator*() noexcept { return *it_; }
    value_type operator*() const noexcept { return *it_; }
    src &operator++() {
        ++it_;
        return *this;
    }
    src operator++(int) {
        Iterator _Tmp = *this;
        ++*this;
        return (_Tmp);
    }
    src &operator+=(difference_type x) {
        it_ += x;
        return *this;
    }
};
using u32src = src<std::u32string::const_iterator>;

using p = tig::magimocha::parser::p<u32src>;
namespace cg2 = tig::magimocha::codegen2;
using namespace tig::magimocha;
namespace cppcp = tig::cppcp;
int main() {
    std::u32string s = U"\
		module a {\n\
			infix + right 250\n\
			infix * right 500\n\
            def +(_:Double,_:Double):Double=0.0\n\
            def *(_:Double,_:Double):Double=0.0\n\
			def main():Double={\
				//2.0*3.0\n\
				add(2.0,4.0)\n\
			}\
            def add( a:Double , b:Double ):Double = {\n\
				val x = a*2.0\n\
                val y = x*4.0\n\
				x+y\n\
			}\n\
			//def add2(a,b):Double=(def sub(x,y):Double=x-y)(a,b)+add(a,b)\n\
		}";
    auto ast =
        cppcp::get0(cppcp::join(cppcp::skip(cppcp::option(p::whitespace())),
                                p::module_p()))(u32src(cbegin(s), cend(s)))
            .get();
    auto ast2 = cg2::convert_scope(ast);
    auto info_table = std::make_shared<cg2::operator_info_table_impl>(
        std::shared_ptr<cg2::operator_info_table>());
    std::unordered_map<std::shared_ptr<ast::make_scope>,
                       std::shared_ptr<cg2::operator_info_table>>
        map;
    cg2::extract_operator_info(ast::to_ast_leaf(ast2), info_table, map);
    auto rraw = cg2::operation_to_function_applying_all_leaf(
        ast::to_ast_leaf(ast2), info_table, map);
    auto r = std::static_pointer_cast<ast::declaration_module>(rraw);
    auto root =
        name::create_variable_table(std::shared_ptr<name::variable_table>());
    auto types = std::make_shared<typing::type_table_impl>();
    auto schemas = std::make_shared<typing::type_schema_table_impl>();
    auto ms2vt =
        std::make_shared<typing::make_scope_2_variable_table_table_impl>();
    typing::infer_all(typing::context{root, types, schemas, ms2vt}, r);
    auto llvmv = std::make_shared<cg2::llvm_values_impl>();
    llvm::LLVMContext con;
    auto mod = std::make_unique<llvm::Module>("root", con);
    cg2::codegen(std::static_pointer_cast<ast::declaration_module>(r), types,
                 schemas, ms2vt, llvmv, con, mod.get());
    mod->print(llvm::outs(), nullptr);
}