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
TEST(MagiMochaTyping, named_function_expression_scope_id_int) {
	using namespace std::string_literals;

	std::u32string s = U"def id(x):int32 = x";
	auto p = x::named_function_expression_scope();
	auto info_table = std::make_shared<cg2::operator_info_table_impl>( std::shared_ptr<cg2::operator_info_table>(),std::unordered_map<ast::string_type, cg2::operator_info>() );
	auto pr = p(src{ cbegin(s),cend(s) }).get();
	auto r = cg2::operation_to_function_applying_all(pr, info_table);
	auto type_table = std::make_shared<typing::type_table_impl>();
	auto context = typing::context{
		typing::create_variable_table(std::shared_ptr< typing::variable_table>()),
		type_table,
		std::make_shared<typing::type_schema_table_impl>()
	};
	typing::infer(context, r);
	EXPECT_EQ(r->type(),ast::leaf_type::named_function);
	auto rr = std::static_pointer_cast<ast::named_function>(r);
	EXPECT_EQ(rr->return_type_func()->result_type->type(),ast::type_data_type::simple );
	EXPECT_EQ(std::static_pointer_cast<ast::simple_type_data>(rr ->return_type_func()->result_type)->value(),U"int32");
	EXPECT_EQ(rr->return_type_func()->args[0]->type(), ast::type_data_type::var);
	std::unordered_set<std::shared_ptr<ast::var_type_data>> free;
	auto args0 = typing::resolve_type(type_table,std::static_pointer_cast<ast::var_type_data>(rr->return_type_func()->args[0]),free);
	EXPECT_EQ(args0->type(), ast::type_data_type::simple);
	EXPECT_EQ(std::static_pointer_cast<ast::simple_type_data>(args0)->value(),
		U"int32"
	);
	EXPECT_TRUE(free.empty());
}
TEST(MagiMochaTyping, named_function_expression_scope_id) {
	using namespace std::string_literals;

	std::u32string s = U"def id(x) = x";
	auto p = x::named_function_expression_scope();
	auto info_table = std::make_shared<cg2::operator_info_table_impl>(std::shared_ptr<cg2::operator_info_table>(), std::unordered_map<ast::string_type, cg2::operator_info>());
	auto pr = p(src{ cbegin(s),cend(s) }).get();
	auto r = cg2::operation_to_function_applying_all(pr, info_table);
	auto type_table = std::make_shared<typing::type_table_impl>();
	auto schema_table=std::make_shared<typing::type_schema_table_impl>();
	auto context = typing::context{
		typing::create_variable_table(std::shared_ptr< typing::variable_table>()), type_table, schema_table
	};
	typing::infer(context, r);
	EXPECT_EQ(r->type(), ast::leaf_type::named_function);
	auto rr = std::static_pointer_cast<ast::named_function>(r);
	EXPECT_EQ(schema_table->get(rr->body()).value().type_data,rr->return_type());
	EXPECT_EQ(schema_table->get(rr->body()).value().type_vars.size(),1);
}
TEST(MagiMochaTyping, expressions) {
	std::u32string s = U"{def id(x) = x\nval x=32;x}";
	auto p = x::named_function_expression_scope();
}
TEST(MagiMochaTyping, named_function_expression_scope_id_and_apply) {
	std::u32string s = U"{def id(x) = x,\
							id(32),id(3.6)}";
	auto p = x::expression_block();
	auto info_table = std::make_shared<cg2::operator_info_table_impl>(std::shared_ptr<cg2::operator_info_table>(), std::unordered_map<ast::string_type, cg2::operator_info>());
	auto pr = p(src{ cbegin(s),cend(s) }).get();
	auto r = cg2::operation_to_function_applying_all(pr, info_table);
	auto type_table = std::make_shared<typing::type_table_impl>();
	auto schema_table = std::make_shared<typing::type_schema_table_impl>();
	auto context = typing::context{
		typing::create_variable_table(std::shared_ptr< typing::variable_table>()),
		type_table,
		schema_table
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
		U"uint64"
	);
	auto l3 = *(++itr);
	EXPECT_EQ(l3->type(), ast::leaf_type::apply_function);
	auto l3r = std::static_pointer_cast<ast::apply_function>(l3);
	auto l3rrt = typing::resolve_type(type_table, l3r->return_type());
	EXPECT_EQ(l3rrt->type(), ast::type_data_type::simple);
	EXPECT_EQ(std::static_pointer_cast<ast::simple_type_data>(l3rrt)->value(),
		U"double"
	);
	auto rrt = typing::resolve_type(type_table, r->return_type());
	EXPECT_EQ(rrt->type(), ast::type_data_type::simple);

	EXPECT_EQ(std::static_pointer_cast<ast::simple_type_data>(rrt)->value(),
		U"double"
	);
}