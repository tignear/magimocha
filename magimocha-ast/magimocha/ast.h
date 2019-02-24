#pragma once
#include <vector>
#include <memory>
#include <optional>
#include <variant>
#include <list>
namespace tig::magimocha {
	namespace ast {
		enum class leaf_type {
			declaration_name,
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
			named_function
		};
		using char_type = char32_t;
		using string_type = std::basic_string<char_type>;

		struct ast_base {
			virtual ~ast_base() {}
			virtual leaf_type type() = 0;
		};
		struct expression :public ast_base {

		};
		class declaration_name final :public ast_base {
			string_type name_;

		public:
			leaf_type type()override {
				return leaf_type::declaration_name;
			}
			 declaration_name(const string_type& name) :name_(name) {

			}

			 const string_type& name()const {
				return name_;
			}

		};
		class declaration_parameter final :public ast_base {
			std::optional<string_type> name_;
		public:
			leaf_type type()override {
				return leaf_type::declaration_parameter;
			}
			 declaration_parameter(const string_type& name) :name_(name) {

			}
			 declaration_parameter() : name_(std::nullopt) {

			}
			 declaration_parameter(std::optional<string_type> s) : name_(s) {

			}

			 const std::optional<string_type> name()const {
				return name_;
			}
			 bool is_ignore_parameter() {
				return !name_;
			}
		};
		
		class declaration_function final :public expression {
			std::vector<std::shared_ptr<declaration_parameter>> params_;
			std::shared_ptr<expression> body_;
		public:
			leaf_type type()override {
				return leaf_type::declaration_function;
			}
			 declaration_function(
				std::vector<std::shared_ptr<declaration_parameter>> params,
				std::shared_ptr<expression> body
			) :params_(params), body_(body) {

			}
			const std::vector<std::shared_ptr<declaration_parameter>>& params() {
				return params_;
			}
			const std::shared_ptr<expression> body() {
				return body_;
			}
		};
		class named_function final:public expression {
			string_type name_;
			std::shared_ptr<declaration_function> body_;
		public:
			named_function(string_type name,std::shared_ptr<declaration_function> body):name_(name),body_(body){}
			leaf_type type()override {
				return leaf_type::named_function;
			}
			auto body() {
				return body_;
			}
			auto& name() {
				return name_;
			}
		};
		struct literal_ :public expression {

		};
		struct numeric_literal :public literal_ {
		};
		struct integer_literal :public numeric_literal {

		};

		class signed_number_literal final :public integer_literal {
			std::int64_t value_;
		public:
			leaf_type type()override {
				return leaf_type::signed_number_literal;
			}
			 signed_number_literal(const  std::int64_t value) :value_(value) {

			}
			 const std::int64_t value()const {
				return value_;
			}
		};
		class unsigned_number_literal final :public integer_literal {
			std::uint64_t value_;
		public:
			leaf_type type()override {
				return leaf_type::unsigned_number_literal;
			}
			 unsigned_number_literal(const  std::uint64_t value) :value_(value) {

			}
			 const std::uint64_t value()const {
				return value_;
			}
			 const std::int64_t signed_p()const {
				if (static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) >= value_) {
					return value_;
				}
				throw std::invalid_argument("");

			}
			 const std::int64_t signed_n()const {
				if (static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1 >= value_) {
					return -static_cast<int64_t>(value_);
				}
				throw std::invalid_argument("");
			}
		};
		class floating_literal final :public numeric_literal {
			double value_;
		public:
			leaf_type type()override {
				return leaf_type::floating_literal;
			}
			 floating_literal(double value) :value_(value) {

			}
			 const double value()const {
				return value_;
			}

		};
		class string_literal final :public literal_ {
			std::u32string value_;
		public:
			leaf_type type()override {
				return leaf_type::string_literal;
			}
			 string_literal(const std::u32string& value) :value_(value) {

			}
			 const std::u32string& value()const {
				return value_;
			}
		};
		class apply_function final :public expression {
			std::shared_ptr<expression> target_;
			std::vector< std::shared_ptr<expression>> args_;
		public:
			leaf_type type()override {
				return leaf_type::apply_function;
			}
			 apply_function(std::shared_ptr<expression> target, std::vector<std::shared_ptr<expression>> args) :target_(target), args_(args) {

			}
			 const auto& target()const {
				return target_;
			}
			 auto& target() {
				return target_;
			}
			 auto& args() {
				return args_;
			}
			 const auto& args()const {
				return args_;
			}
		};
		class call_name final :public expression {
			std::u32string value_;
		public:
			leaf_type type()override {
				return leaf_type::call_name;
			}
			 call_name(const std::u32string& value) :value_(value) {

			}
			 const std::u32string& value()const {
				return value_;
			}
		};
		class expression_block :public expression {
			std::vector<std::shared_ptr<expression>> expressions_;
		public:
			leaf_type type()override {
				return leaf_type::expression_block;
			}
			 expression_block(std::vector<std::shared_ptr<expression>>&& expressions) :expressions_(expressions) {

			}
			 const auto& value()const {
				return expressions_;
			}
		};
		class op_token_double {

			std::shared_ptr<typename call_name> s_;
		public:
			op_token_double(std::shared_ptr<typename call_name> s) :s_(s) {

			}
			 std::shared_ptr<typename call_name> name() {
				return s_;
			}
			 const std::shared_ptr<typename call_name> name()const {
				return s_;
			}

		};
		class op_token_single {
			std::shared_ptr<typename call_name> s_;
		public:
			op_token_single(std::shared_ptr<typename call_name> s) :s_(s) {

			}
			 std::shared_ptr<typename call_name> name() {
				return s_;
			}
		};
		class op_token_function_applying {
			std::vector<std::shared_ptr<typename expression>> vec_;
		public:
			 op_token_function_applying(std::vector<std::shared_ptr<typename expression>>  vec) :vec_(vec) {

			}
			 auto& args() {
				return vec_;
			}
			 const auto& args()const {
				return vec_;
			}
		};
		using operator_token_type = std::variant <
			op_token_single,
			op_token_double,
			op_token_function_applying,
			std::shared_ptr<typename declaration_function>,
			std::shared_ptr<typename literal_>,
			std::shared_ptr<typename expression>,
			std::shared_ptr <typename call_name >
		>;
		class operation :public expression {
			std::list<operator_token_type> op_;
		public:
			leaf_type type()override {
				return leaf_type::operation;
			}
			 operation(std::list<operator_token_type>&& op) :op_(op) {

			}
			 auto& value() {
				return op_;
			}
			 const auto& value()const {
				return op_;
			}
		};

	};
}
