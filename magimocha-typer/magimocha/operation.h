#pragma once
#include <unordered_map>
#include "enviroment.h"
#include "magimocha/ast.h"
#include "magimocha/ast-visitor.h"
namespace tig::magimocha::codegen2 {
	enum class infix_type {
		left, right
	};
	struct operator_info {
		signed priority;
		infix_type infix;
	};
	struct operator_info_ref {
		virtual operator_info get()=0;
		virtual void set(operator_info) = 0;
		virtual operator bool()=0;
		~operator_info_ref() {}
	};
	struct operator_info_table {
		virtual std::unique_ptr<operator_info_ref> find_shallow(const ast::string_type&)=0;
		virtual std::optional<operator_info>  get_deep(const ast::string_type&) = 0;
		virtual ~operator_info_table() {}
	};

	class operator_info_table_impl:public operator_info_table {
		std::shared_ptr<operator_info_table> parent;
		std::unordered_map<ast::string_type, operator_info> infos;
	public:
		operator_info_table_impl(std::shared_ptr<operator_info_table> parent, std::unordered_map<ast::string_type, operator_info> infos):parent(parent),infos(infos){

		}
		std::unique_ptr<operator_info_ref> find_shallow(const ast::string_type&)override;
		std::optional<operator_info> get_deep(const ast::string_type&)override;
	};
	using operator_token_type_processed_op_token_function_applying = std::variant <
		ast::op_token_single,
		ast::op_token_double,
		std::shared_ptr<ast::declaration_function>,
		std::shared_ptr<ast::literal_>,
		std::shared_ptr<ast::expression>,
		std::shared_ptr <ast::call_name >
	>;
	std::list<operator_token_type_processed_op_token_function_applying> operation_to_function_applying_process_op_token_function_applying(const std::list<ast::operator_token_type>& src);
	using operator_token_type_processed_op_token_single = std::variant <
		ast::op_token_double,
		std::shared_ptr<ast::declaration_function>,
		std::shared_ptr<ast::literal_>,
		std::shared_ptr<ast::expression>,
		std::shared_ptr <ast::call_name >
	>;
	std::list<operator_token_type_processed_op_token_single> operation_to_function_applying_process_op_token_single(const std::list<operator_token_type_processed_op_token_function_applying>& src);
	std::shared_ptr<ast::expression> operation_to_function_applying_process_op_token_double(std::list<operator_token_type_processed_op_token_single>::const_iterator begin, std::list<operator_token_type_processed_op_token_single>::const_iterator end, std::shared_ptr<operator_info_table> op_infos);
	std::shared_ptr<ast::expression> operation_to_function_applying(std::shared_ptr<ast::operation> op, std::shared_ptr<operator_info_table> op_infos);
	std::shared_ptr<ast::expression> operation_to_function_applying_all(std::shared_ptr<ast::expression>, std::shared_ptr<operator_info_table> op_info);
}