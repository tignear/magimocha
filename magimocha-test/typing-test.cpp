#include "gtest/gtest.h"
#include "magimocha/parser.h"

#include "magimocha/typing.h"
#include "magimocha/operation.h"
#include "testutil.h"
using u32src = src<std::u32string::const_iterator>;
using x = tig::magimocha::parser::p<u32src>;
namespace ast = tig::magimocha::ast;
namespace cg2 = tig::magimocha::codegen2;

namespace typing =tig::magimocha::typing;
TEST(MagiMochaTyping, named_function_expression_scope) {
	using namespace std::string_literals;

	std::u32string s = U"def id(x):int32 = x";
	auto p = x::named_function_expression_scope();
	auto info_table = std::make_shared<cg2::operator_info_table_impl>( std::shared_ptr<cg2::operator_info_table>(),std::unordered_map<ast::string_type, cg2::operator_info>() );
	auto pr = p(src{ cbegin(s),cend(s) }).get();
	auto r = cg2::operation_to_function_applying_all(pr, info_table);
	auto type_table = std::make_shared<typing::type_table_impl>();
	typing::infer(typing::create_variable_table(std::shared_ptr< typing::variable_table>()),type_table, std::make_shared<typing::schema_table>(), r);
	EXPECT_EQ(r->type(),ast::leaf_type::named_function);
	auto rr = std::static_pointer_cast<ast::named_function>(r);
	EXPECT_EQ(std::static_pointer_cast<ast::simple_type_data>(rr ->return_type_func()->result_type)->value(),U"int32");
	EXPECT_EQ(std::static_pointer_cast<ast::simple_type_data>(
		type_table->find(std::static_pointer_cast<ast::var_type_data>(rr->return_type_func()->args[0]))->get())->value(),
		U"int32"
	);

}