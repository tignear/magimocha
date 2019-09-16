#pragma once
#include <vector>
#include <memory>
#include <optional>
#include <variant>
#include <list>
#include <stdexcept>
namespace tig::magimocha
{
namespace ast
{
enum class leaf_type
{
	//declaration_name,
	declaration_parameter,
	declaration_function,
	signed_number_literal,
	unsigned_number_literal,
	floating_literal,
	string_literal,
	apply_function,
	call_name,
	expression_block,
	operation,
	named_function,
	declaration_module,
	declaration_variable,
	declaration_export,
	declaration_infix
};
using char_type = char32_t;
using string_type = std::basic_string<char_type>;
//struct expression;

struct ast_base
{
	virtual ~ast_base() {}
	virtual leaf_type type() const = 0;
};
struct ast_leaf : public virtual ast_base
{
};
struct make_scope : public virtual ast_base
{
};
enum class type_data_type
{
	simple,
	function /*,polymorphic*/,
	var
};
struct type_data
{
	virtual type_data_type type() const = 0;
	virtual ~type_data() {}
};
class simple_type_data final : public type_data
{
	string_type value_;

public:
	explicit simple_type_data(string_type value) : value_(value) {}
	type_data_type type() const override
	{
		return type_data_type::simple;
	}
	const string_type &value() const
	{
		return value_;
	}
};
class function_type_data final : public type_data
{
public:
	std::shared_ptr<type_data> result_type;
	std::vector<std::shared_ptr<type_data>> args;
	function_type_data(std::shared_ptr<type_data> result_type, std::vector<std::shared_ptr<type_data>> args) : result_type(result_type), args(args)
	{
	}
	type_data_type type() const override
	{
		return type_data_type::function;
	}
};
struct var_type_data final : public type_data
{
	var_type_data() = default;
	type_data_type type() const override
	{
		return type_data_type::var;
	}
};
/*class ref_type_data final:public type_data {
			public:
				std::shared_ptr<type_data>  value;

				ref_type_data(std::shared_ptr<type_data> value) :value(value) {}
				ref_type_data(){}

				type_data_type type()override {
					return type_data_type::ref;
				}

			};*/
struct typed_data : public virtual ast_base
{
	virtual std::shared_ptr<type_data> return_type() const = 0;
	//virtual void set_return_type(std::shared_ptr<type_data>) = 0;
};
struct expression : public typed_data
{
};
struct module_member : public virtual ast_base
{
};

/*class declaration_name final :public typed_data {
				string_type name_;
				std::shared_ptr<type_data> type_data_;
			public:
				leaf_type type()override {
					return leaf_type::declaration_name;
				}
				declaration_name(const string_type& name, std::shared_ptr<type_data> type_data) :name_(name) {

				}

				const string_type& value()const {
					return name_;
				}
				std::shared_ptr<type_data> return_type()override {

				}
			};*/
class declaration_parameter final : public typed_data, public ast_leaf
{
	std::optional<string_type> name_;
	std::shared_ptr<type_data> type_data_;

public:
	leaf_type type() const override
	{
		return leaf_type::declaration_parameter;
	}
	declaration_parameter(const string_type &name, std::shared_ptr<type_data> type_data) : name_(name), type_data_(type_data)
	{
	}
	declaration_parameter(std::shared_ptr<type_data> type_data) : name_(std::nullopt), type_data_(type_data)
	{
	}
	declaration_parameter(std::optional<string_type> s, std::shared_ptr<type_data> type_data) : name_(s), type_data_(type_data)
	{
	}
	std::shared_ptr<type_data> return_type() const override
	{

		return type_data_;
	}
	std::optional<string_type> name() const
	{
		return name_;
	}
	bool is_ignore_parameter() const
	{
		return !name_;
	}
};

class declaration_function  : public expression, public module_member, public make_scope, public ast_leaf
{
	std::vector<std::shared_ptr<declaration_parameter>> params_;
	std::shared_ptr<expression> body_;
	std::shared_ptr<function_type_data> type_data_;

public:
	leaf_type type() const override
	{
		return leaf_type::declaration_function;
	}
	declaration_function(
		std::vector<std::shared_ptr<declaration_parameter>> params,
		std::shared_ptr<function_type_data> type_data,
		std::shared_ptr<expression> body) : params_(params), type_data_(type_data), body_(body)
	{
	}
	const std::vector<std::shared_ptr<declaration_parameter>> &params() const
	{
		return params_;
	}
	std::shared_ptr<expression> body() const
	{
		return body_;
	}
	std::shared_ptr<type_data> return_type() const override
	{
		return type_data_;
	}
	std::shared_ptr<function_type_data> return_type_func() const
	{
		return type_data_;
	}
};
class named_function final :public make_scope, public expression, public module_member ,public ast_leaf
{
	string_type name_;
	std::shared_ptr<declaration_function> body_;

public:
	named_function(
		string_type name,
		std::shared_ptr<declaration_function> body) : name_(name),
													  body_(body) {}
	leaf_type type() const override
	{
		return leaf_type::named_function;
	}
	auto body()const
	{
		return body_;
	}
	const auto &name()const
	{
		return name_;
	}
	std::shared_ptr<type_data> return_type() const override
	{
		return body_->return_type();
	}
	std::shared_ptr<function_type_data> return_type_func() const
	{
		return body_->return_type_func();
	}
};
struct literal_ : public expression
{
};
struct numeric_literal : public literal_
{
};
struct integer_literal : public numeric_literal
{
};

class signed_number_literal final : public integer_literal, public ast_leaf
{
	std::int64_t value_;

public:
	leaf_type type() const override
	{
		return leaf_type::signed_number_literal;
	}
	explicit signed_number_literal(const std::int64_t value) : value_(value)
	{
	}
	const std::int64_t value() const
	{
		return value_;
	}
	std::shared_ptr<type_data> return_type() const override
	{
		return std::make_shared<simple_type_data>(U"Int64");
	}
};
class unsigned_number_literal final : public integer_literal, public ast_leaf
{
	std::uint64_t value_;

public:
	leaf_type type() const override
	{
		return leaf_type::unsigned_number_literal;
	}
	explicit unsigned_number_literal(const std::uint64_t value) : value_(value)
	{
	}
	const std::uint64_t value() const
	{
		return value_;
	}
	const std::int64_t signed_p() const
	{
		if (static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) >= value_)
		{
			return value_;
		}
		throw std::invalid_argument("");
	}
	const std::int64_t signed_n() const
	{
		if (static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1 >= value_)
		{
			return -static_cast<int64_t>(value_);
		}
		throw std::invalid_argument("");
	}
	std::shared_ptr<type_data> return_type() const override
	{
		return std::make_shared<simple_type_data>(U"UInt64");
	}
};
class floating_literal final : public numeric_literal, public ast_leaf
{
	double value_;

public:
	leaf_type type() const override
	{
		return leaf_type::floating_literal;
	}
	explicit floating_literal(double value) : value_(value)
	{
	}
	double value() const
	{
		return value_;
	}
	std::shared_ptr<type_data> return_type() const override
	{
		return std::make_shared<simple_type_data>(U"Double");
	}
};
class string_literal final : public literal_, public ast_leaf
{
	std::u32string value_;
	std::shared_ptr<type_data> rt = std::make_shared<simple_type_data>(U"String"); //TODO
public:
	leaf_type type() const override
	{
		return leaf_type::string_literal;
	}
	explicit string_literal(const std::u32string &value) : value_(value)
	{
	}
	const std::u32string &value() const
	{
		return value_;
	}
	std::shared_ptr<type_data> return_type() const override
	{
		return rt;
	}
};
class apply_function final : public expression, public ast_leaf
{
	std::shared_ptr<expression> target_;
	std::vector<std::shared_ptr<expression>> args_;
	std::shared_ptr<type_data> return_type_ = std::make_shared<var_type_data>();

public:
	leaf_type type() const override
	{
		return leaf_type::apply_function;
	}
	apply_function(std::shared_ptr<expression> target, std::vector<std::shared_ptr<expression>> args) : target_(target), args_(args)
	{
	}
	std::shared_ptr<type_data> return_type() const override
	{
		return return_type_;
	}
	const auto &target() const
	{
		return target_;
	}
	const auto &args() const
	{
		return args_;
	}
};
class call_name final : public expression, public ast_leaf
{
	string_type value_;
	std::shared_ptr<type_data> type_data_ = std::make_shared<var_type_data>();

public:
	leaf_type type() const override
	{
		return leaf_type::call_name;
	}
	explicit call_name(const string_type &value) : value_(value)
	{
	}
	const string_type &value() const
	{
		return value_;
	}
	std::shared_ptr<type_data> return_type() const override
	{
		return type_data_;
	}
};
class expression_block final : public expression, public ast_leaf, public make_scope
{
	std::vector<std::shared_ptr<expression>> value_;
	std::shared_ptr<type_data> type_data_ = std::make_shared<var_type_data>();

public:
	leaf_type type() const override
	{
		return leaf_type::expression_block;
	}
	explicit expression_block(std::vector<std::shared_ptr<expression>> value) : value_(value)
	{
	}
	const auto &value() const
	{
		return value_;
	}
	std::shared_ptr<type_data> return_type() const override
	{
		return type_data_;
	}
};
class op_token_double
{

	std::shared_ptr<call_name> s_;

public:
	explicit op_token_double(std::shared_ptr<call_name> s) : s_(s)
	{
	}
	std::shared_ptr<call_name> name() const
	{
		return s_;
	}
};
class op_token_single
{
	std::shared_ptr<call_name> s_;

public:
	explicit op_token_single(std::shared_ptr<call_name> s) : s_(s)
	{
	}
	std::shared_ptr<call_name> name() const
	{
		return s_;
	}
};
class op_token_function_applying
{
	std::vector<std::shared_ptr<expression>> args_;

public:
	explicit op_token_function_applying(std::vector<std::shared_ptr<expression>> args) : args_(args)
	{
	}

	const auto &args() const
	{
		return args_;
	}
};
using operator_token_type = std::variant<
	op_token_single,
	op_token_double,
	op_token_function_applying,
	std::shared_ptr<declaration_function>,
	std::shared_ptr<literal_>,
	std::shared_ptr<expression>,
	std::shared_ptr<call_name>>;
class operation final : public expression, public ast_leaf
{
	std::list<operator_token_type> value_;
	std::shared_ptr<type_data> type_data_ = std::make_shared<ast::var_type_data>();

public:
	leaf_type type() const override
	{
		return leaf_type::operation;
	}
	explicit operation(std::list<operator_token_type> value) : value_(value)
	{
	}
	std::shared_ptr<type_data> return_type() const override
	{
		return type_data_;
	}

	const auto &value() const
	{
		return value_;
	}
};
class declaration_module final : public module_member, public ast_leaf, public make_scope
{
	std::vector<std::shared_ptr<module_member>> members_;
	string_type name_;

public:
	declaration_module(const string_type &name, std::vector<std::shared_ptr<module_member>> members) : name_(name), members_(members)
	{
	}
	leaf_type type() const override
	{
		return leaf_type::declaration_module;
	}
	const std::vector<std::shared_ptr<module_member>> &members() const
	{
		return members_;
	}
	const string_type &name() const
	{
		return name_;
	}
};
class declaration_variable final : public expression, public module_member, public ast_leaf, public make_scope
{
	string_type name_;
	std::shared_ptr<type_data> type_data_;
	std::shared_ptr<expression> body_;

public:
	declaration_variable(
		const string_type &name,
		std::shared_ptr<type_data> type_data,
		std::shared_ptr<expression> body) : name_(name), body_(body)
	{
	}
	std::shared_ptr<type_data> return_type() const override
	{
		return type_data_;
	}
	leaf_type type() const override
	{
		return leaf_type::declaration_variable;
	}
	const string_type &name() const
	{
		return name_;
	}
	std::shared_ptr<expression> body() const
	{
		return body_;
	}
};
class declaration_export final : public module_member, public ast_leaf
{
	std::vector<string_type> targets_;
	std::vector<string_type> to_;
	declaration_export(std::vector<string_type> &&targets, std::vector<string_type> to) : targets_(targets), to_(to)
	{
	}
	const std::vector<string_type> &targets() const
	{
		return targets_;
	}
	const std::vector<string_type> &to() const
	{
		return to_;
	}
	leaf_type type() const override
	{
		return leaf_type::declaration_export;
	}
};
enum class infix_type
{
	left,
	right
};
class declaration_infix final : public module_member, public expression, public ast_leaf
{
	string_type name_;
	infix_type infix_type_;
	int priority_;

public:
	declaration_infix(const string_type &name, infix_type infix_type, int priority) : name_(name),
																					  infix_type_(infix_type),
																					  priority_(priority)
	{
	}
	infix_type infix_type() const
	{
		return infix_type_;
	}
	int priority() const
	{
		return priority_;
	}
	const string_type &name() const
	{
		return name_;
	}
	std::shared_ptr<type_data> return_type() const override
	{
		return std::make_shared<simple_type_data>(U"Nothing");
	}
	leaf_type type() const override
	{
		return leaf_type::declaration_infix;
	}
};
}; // namespace ast

} // namespace tig::magimocha