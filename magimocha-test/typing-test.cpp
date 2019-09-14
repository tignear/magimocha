#include "gtest/gtest.h"
#include "magimocha/parser.h"
#include "magimocha/typing.h"
#include "magimocha/operation.h"
#include "magimocha/convert-ast.h"
#include "testutil.h"
using u32src = src<std::u32string::const_iterator>;
using x = tig::magimocha::parser::p<u32src>;
namespace ast = tig::magimocha::ast;
namespace cg2 = tig::magimocha::codegen2;

namespace typing =tig::magimocha::typing;
TEST(MagiMochaTyping, named_function_expression_scope_id_int) {
	using namespace std::string_literals;

	std::u32string s = U"def Id(x):Int32 = x";
	auto p = x::named_function_expression_scope();
	auto info_table = std::make_shared<cg2::operator_info_table_impl>( std::shared_ptr<cg2::operator_info_table>(),std::unordered_map<ast::string_type, cg2::operator_info>() );
	auto pr = p(src{ cbegin(s),cend(s) }).get();
	auto r = cg2::operation_to_function_applying_all(pr, info_table);
	auto type_table = std::make_shared<typing::type_table_impl>();
	auto context = typing::context{
		typing::create_variable_table(std::shared_ptr< typing::variable_table>()),
		type_table,
		std::make_shared<typing::type_schema_table_impl>(),
		std::make_shared<typing::typed_data_2_variable_table_table_impl>()
	};
	typing::infer(context, r);
	EXPECT_EQ(r->type(),ast::leaf_type::named_function);
	auto rr = std::static_pointer_cast<ast::named_function>(r);
	EXPECT_EQ(rr->return_type_func()->result_type->type(),ast::type_data_type::simple );
	EXPECT_EQ(std::static_pointer_cast<ast::simple_type_data>(rr ->return_type_func()->result_type)->value(),U"Int32");
	EXPECT_EQ(rr->return_type_func()->args[0]->type(), ast::type_data_type::var);
	std::unordered_set<std::shared_ptr<ast::var_type_data>> free;
	auto args0 = typing::resolve_type(type_table,std::static_pointer_cast<ast::var_type_data>(rr->return_type_func()->args[0]),free);
	EXPECT_EQ(args0->type(), ast::type_data_type::simple);
	EXPECT_EQ(std::static_pointer_cast<ast::simple_type_data>(args0)->value(),
		U"Int32"
	);
	EXPECT_TRUE(free.empty());
}
TEST(MagiMochaTyping, named_function_expression_scope_id) {
	using namespace std::string_literals;

	std::u32string s = U"def Id(x) = x";
	auto p = x::named_function_expression_scope();
	auto info_table = std::make_shared<cg2::operator_info_table_impl>(std::shared_ptr<cg2::operator_info_table>(), std::unordered_map<ast::string_type, cg2::operator_info>());
	auto pr = p(src{ cbegin(s),cend(s) }).get();
	auto r = cg2::operation_to_function_applying_all(pr, info_table);
	auto type_table = std::make_shared<typing::type_table_impl>();
	auto schema_table=std::make_shared<typing::type_schema_table_impl>();
	auto context = typing::context{
		typing::create_variable_table(std::shared_ptr< typing::variable_table>()),
		type_table,
		schema_table,
		std::make_shared<typing::typed_data_2_variable_table_table_impl>()
	};
	typing::infer(context, r);
	EXPECT_EQ(r->type(), ast::leaf_type::named_function);
	auto rr = std::static_pointer_cast<ast::named_function>(r);
	EXPECT_EQ(schema_table->get(rr->body()).value().type_data,rr->return_type());
	EXPECT_EQ(schema_table->get(rr->body()).value().type_vars.size(),1);
}
TEST(MagiMochaTyping, expressions) {
	std::u32string s = U"{def Id(x) = x\nval x=32;x}";
	auto p = x::named_function_expression_scope();
}
TEST(MagiMochaTyping, named_function_expression_scope_id_and_apply) {
	std::u32string s = U"{def Id(x) = x;Id(32);Id(3.6)}";
	auto p = x::expression_block();
	auto info_table = std::make_shared<cg2::operator_info_table_impl>(std::shared_ptr<cg2::operator_info_table>(), std::unordered_map<ast::string_type, cg2::operator_info>());
	auto pr = p(src{ cbegin(s),cend(s) }).get();
	auto r = cg2::operation_to_function_applying_all(pr, info_table);
	auto type_table = std::make_shared<typing::type_table_impl>();
	auto schema_table = std::make_shared<typing::type_schema_table_impl>();
	auto context = typing::context{
		typing::create_variable_table(std::shared_ptr< typing::variable_table>()),
		type_table,
		schema_table,
		std::make_shared<typing::typed_data_2_variable_table_table_impl>()
	};
	typing::infer(context, r);

	EXPECT_EQ(r->type(), ast::leaf_type::expression_block);
	auto block = std::static_pointer_cast<ast::expression_block>(r);
	auto&& itr = begin(block->value());
	auto l1 = *itr;
	EXPECT_EQ(l1->type(), ast::leaf_type::named_function);
	auto l1r= std::static_pointer_cast<ast::named_function>(l1);
	EXPECT_EQ(schema_table->get(l1r->body()).value().type_data, l1r->return_type());
	EXPECT_EQ(schema_table->get(l1r->body()).value().type_vars.size(), 1);
	auto l2 = *(++itr);
	EXPECT_EQ(l2->type(), ast::leaf_type::apply_function);
	auto l2r = std::static_pointer_cast<ast::apply_function>(l2);
	auto l2rrt = typing::resolve_type(type_table, l2r->return_type());
	EXPECT_EQ(l2rrt->type(), ast::type_data_type::simple);
	EXPECT_EQ(std::static_pointer_cast<ast::simple_type_data>(l2rrt)->value(),
		U"UInt64"
	);
	auto l3 = *(++itr);
	EXPECT_EQ(l3->type(), ast::leaf_type::apply_function);
	auto l3r = std::static_pointer_cast<ast::apply_function>(l3);
	auto l3rrt = typing::resolve_type(type_table, l3r->return_type());
	EXPECT_EQ(l3rrt->type(), ast::type_data_type::simple);
	EXPECT_EQ(std::static_pointer_cast<ast::simple_type_data>(l3rrt)->value(),
		U"Double"
	);
	auto rrt = typing::resolve_type(type_table, r->return_type());
	EXPECT_EQ(rrt->type(), ast::type_data_type::simple);

	EXPECT_EQ(std::static_pointer_cast<ast::simple_type_data>(rrt)->value(),
		U"Double"
	);
}

TEST(MagiMochaTyping, expression_block_convert_scope) {
	auto p = x::expression_block();
	std::u32string s = U"{\
		val x=0\n\
		val y=1\n\
		val z=2\
	}";
	auto info_table = std::make_shared<cg2::operator_info_table_impl>(std::shared_ptr<cg2::operator_info_table>());
	auto pr = p(src{ cbegin(s),cend(s) }).get();
	auto rraw=cg2::convert_scope(ast::to_ast_leaf(pr)());
	EXPECT_EQ(rraw->type(), ast::leaf_type::expression_block);
	auto r = std::static_pointer_cast<ast::expression_block>(rraw);
	auto rv = r->value();
	EXPECT_EQ(rv.size(),2);
	auto rv0raw = rv.at(0);
	EXPECT_EQ(rv0raw->type(), ast::leaf_type::declaration_variable);
	auto rv0= std::static_pointer_cast<ast::declaration_variable>(rv0raw);
	EXPECT_EQ(rv0->name(), U"x");
	auto rv1raw = rv.at(1);
	EXPECT_EQ(rv1raw->type(), ast::leaf_type::expression_block);
}
TEST(MagiMochaTyping, extract_infix_basic) {
	auto p = x::expression_block();
	std::u32string s = U"{infix Id left 145}";
	auto info_table=std::make_shared<cg2::operator_info_table_impl>(std::shared_ptr<cg2::operator_info_table>());
	auto pr = ast::to_ast_leaf(p(src{ cbegin(s),cend(s) }).get());
	std::unordered_map<std::shared_ptr<ast::ast_leaf>, std::shared_ptr<cg2::operator_info_table>> map;
	cg2::extract_operator_info(info_table,pr,map);
	auto ref = map.at(pr)->find_shallow(U"Id");
	EXPECT_TRUE(ref);
	auto info=ref->get();
	EXPECT_EQ(info.priority, 145);
	EXPECT_EQ(info.infix, ast::infix_type::left);
}