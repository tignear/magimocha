#include "operation.h"
namespace tig::magimocha::codegen2 {
	namespace {
		struct operation_to_function_applying_process_op_token_function_applying_visitor_2 {
			using R=std::shared_ptr<ast::expression>;
			template <typename T>
			R operator()(T x) {
				return x;
			}
			R  operator()(ast::op_token_single ots) {
				return ots.name();
			}
			R  operator()(ast::op_token_double otd) {
				return otd.name();
			}
		};
		struct operation_to_function_applying_process_op_token_function_applying_visitor {
			std::list<operator_token_type_processed_op_token_function_applying>& list_ref;
			operation_to_function_applying_process_op_token_function_applying_visitor(std::list<operator_token_type_processed_op_token_function_applying>& list_ref) :list_ref(list_ref) {}
			using R=std::monostate;
			static constexpr const R r = std::monostate{};
			R operator()(ast::op_token_function_applying otfa) {

				auto e = std::visit(operation_to_function_applying_process_op_token_function_applying_visitor_2{}, list_ref.back());

				list_ref.pop_back();
				auto&& args = otfa.args();
				list_ref.push_back(std::make_shared<ast::apply_function>(e, args));
				return r;
			}
			template <typename T>
			R operator()(T x) {
				list_ref.push_back(x);
				return r;
			}
		};
	}
	std::list<operator_token_type_processed_op_token_function_applying> operation_to_function_applying_process_op_token_function_applying(const std::list<ast::operator_token_type>& src ) {
		std::list<operator_token_type_processed_op_token_function_applying> ret;
		auto vis = operation_to_function_applying_process_op_token_function_applying_visitor{ret};
		auto end=src.cend();
		for (auto itr = src.cbegin(); itr != end;++itr) {
			std::visit(vis,*itr);
		}
		return ret;
	}
	namespace {
		struct operation_to_function_applying_process_op_token_single_visitor {
			std::optional<ast::op_token_single> buf = std::nullopt;
			std::list<operator_token_type_processed_op_token_single>& list_ref;
			operation_to_function_applying_process_op_token_single_visitor(std::list<operator_token_type_processed_op_token_single>& list_ref):list_ref(list_ref){}
			template<class T>
			void operator()(T x) {
				if (!buf) {
					list_ref.push_back(x);
					return;
				}
				list_ref.push_back(std::make_shared<ast::apply_function>(buf.value().name(), std::vector<std::shared_ptr<ast::expression>>{x}));
				buf = std::nullopt;
			}
			void operator()(ast::op_token_double x) {
				if (!buf) {
					return;
				}
				;
				list_ref.push_back(std::make_shared<ast::apply_function>(buf.value().name(), std::vector<std::shared_ptr<ast::expression>>{x.name() }));
				buf = std::nullopt;

			}
			void operator()(ast::op_token_single ots) {
				if (buf) {
					throw "illegal state";
				}
				buf = ots;
			}
		};
	}
	std::list<operator_token_type_processed_op_token_single> operation_to_function_applying_process_op_token_single(const std::list<operator_token_type_processed_op_token_function_applying>& src){
		std::list<operator_token_type_processed_op_token_single> ret;
		auto v=operation_to_function_applying_process_op_token_single_visitor{ ret };
		for (auto&& e : src) {
			std::visit(v,e);
		}
		return ret;
	}

	namespace {
		struct operation_to_function_applying_process_op_token_double_visitor {
			using R=std::optional<operator_info>;
			std::shared_ptr<operator_info_table> op_infos;
			operation_to_function_applying_process_op_token_double_visitor(std::shared_ptr<operator_info_table> op_infos):op_infos(op_infos) {

			}
			template<class T>
			R operator()(T x) {
				return std::nullopt;
			}

			R operator()(ast::op_token_double ots) {
				return op_infos->get_deep(ots.name()->value());
			}
		};
		struct operation_to_function_applying_process_op_token_double_visitor_2 {
			using R=std::shared_ptr<ast::expression>;
			operation_to_function_applying_process_op_token_double_visitor_2() {

			}
			template<class T>
			R operator()(T x) {
				return x;
			}

			R operator()(ast::op_token_double ots) {
				return ots.name();
			}
		};
	}
	std::shared_ptr<ast::expression> operation_to_function_applying_process_op_token_double(std::list<operator_token_type_processed_op_token_single>::const_iterator begin, std::list<operator_token_type_processed_op_token_single>::const_iterator end, std::shared_ptr<operator_info_table> op_infos) {
		operation_to_function_applying_process_op_token_double_visitor vis{ op_infos };
		std::optional<operator_info> low_info;
		std::list<operator_token_type_processed_op_token_single>::const_iterator low_itr;
		auto itr = begin;
		++itr;
		if (itr == end) {
			return std::visit(operation_to_function_applying_process_op_token_double_visitor_2{}, *begin);
		}
		for (; itr != end; ++itr) {
			auto r=std::visit(vis,*itr);
			if (!r) {
				continue;
			}
			if (!low_info) {
				low_info = r;
				low_itr = itr;
				continue;
			}
			auto& liv=low_info.value();
			auto& rv = r.value();
			if (liv.priority < rv.priority) {
				continue;
			}
			if (liv.priority > rv.priority) {
				low_info = r;
				low_itr = itr;
				continue;
			}
			switch (liv.infix)
			{
			case infix_type::left:
				switch (rv.infix)
				{
				case infix_type::left:
					low_info = r;
					low_itr = itr;
					continue;
				case infix_type::right:
					low_info = r;
					low_itr = itr;
					continue;
				}
			case infix_type::right:
				switch (rv.infix)
				{
				case infix_type::left:
					low_info = r;
					low_itr = itr;
					continue;
				case infix_type::right:
					continue;
				}

			default:
				break;
			}
		}

		if (!low_info) {
			throw "syntax error";
		}
		return std::make_shared<ast::apply_function>(
			std::visit(operation_to_function_applying_process_op_token_double_visitor_2{}, *low_itr),
			std::vector<std::shared_ptr<ast::expression>>{
				operation_to_function_applying_process_op_token_double(begin,low_itr,op_infos),
				operation_to_function_applying_process_op_token_double(std::next(low_itr),end,op_infos)
			}
		);
	}
	std::shared_ptr<ast::expression> operation_to_function_applying(std::shared_ptr<ast::operation> op, std::shared_ptr<operator_info_table> op_infos) {
		auto&& tokens = op->value();
		auto buf = operation_to_function_applying_process_op_token_single(operation_to_function_applying_process_op_token_function_applying(tokens));
		return operation_to_function_applying_process_op_token_double( buf.begin(), buf.end(), op_infos);
	}
	namespace {
		struct operation_to_function_applying_visitor {
			using R=std::shared_ptr<ast::expression>;

			std::shared_ptr<operator_info_table> op_infos;
			//std::unordered_map<std::shared_ptr<ast::typed_data>, std::shared_ptr<ast::typed_data>>& mapped;
			auto operator()(std::shared_ptr < ast::declaration_function> x) {
				return std::make_shared<ast::declaration_function>(x->params(), x->return_type_func(),operation_to_function_applying_all(x->body(), op_infos));
			}
			auto operator()(std::shared_ptr < ast::signed_number_literal> x) {
				//do nothing
				return x;
			}
			auto operator()(std::shared_ptr < ast::unsigned_number_literal> x) {
				//do nothing
				return x;
			}
			auto operator()(std::shared_ptr < ast::floating_literal> x) {
				//do nothing
				return x;
			}

			auto operator()(std::shared_ptr<ast::string_literal> x) {
				//do nothing
				return x;
			}
			auto operator()(std::shared_ptr<ast::apply_function> x) {
				std::vector<std::shared_ptr<ast::expression>> exprs;
				for (auto&& arg : x->args()) {
					exprs.push_back(operation_to_function_applying_all(arg, op_infos));
				}
				return std::make_shared<ast::apply_function>(operation_to_function_applying_all(x->target(), op_infos), exprs);
			}
			auto operator()(std::shared_ptr<ast::call_name> x) {
				//do nothing
				return x;
			}
			auto operator()(std::shared_ptr<ast::expression_block> x) {
				std::vector<std::shared_ptr<ast::expression>> exprs;
				for (auto&& expr : x->value()) {
					exprs.push_back( operation_to_function_applying_all(expr, op_infos));
				}
				return std::make_shared<ast::expression_block>(std::move(exprs));
			}
			auto operator()(std::shared_ptr<ast::operation> x) {
				auto&& m = operation_to_function_applying(x, op_infos);
				
				return operation_to_function_applying_all(m, op_infos);
			}
			auto operator()(std::shared_ptr<ast::named_function> x) {
				
				return std::make_shared<ast::named_function>(x->name(), this->operator()(x->body()));
			}
			auto operator()(std::shared_ptr<ast::declaration_variable> x) {
				return std::make_shared<ast::declaration_variable>(
					x->name(),
					x->return_type(),
					operation_to_function_applying_all(x->body(), op_infos));
			}

		};
	}
	std::shared_ptr<ast::expression> operation_to_function_applying_all(std::shared_ptr<ast::expression> td, std::shared_ptr<operator_info_table> op_infos) {
		return ast::visit<operation_to_function_applying_visitor::R>(operation_to_function_applying_visitor{op_infos },td);
	}
	struct operator_info_ref_impl :operator_info_ref {

		std::unordered_map<ast::string_type, operator_info>& map;
		const ast::string_type& key;
		std::unordered_map<ast::string_type, operator_info>::const_iterator itr;
		operator_info_ref_impl(std::unordered_map<ast::string_type, operator_info>& map, const ast::string_type& key, std::unordered_map<ast::string_type, operator_info>::const_iterator itr):map(map),key(key),itr(itr) {

		}
		operator_info get()override {
			return itr->second;
		}
		void set(operator_info v)override {
			map.insert_or_assign(key, v);
		}
		operator bool()override {
			return itr != map.end();
		}
	};
	std::unique_ptr<operator_info_ref> operator_info_table_impl::find_shallow(const ast::string_type& k) {
		return std::make_unique<operator_info_ref_impl>(infos,k,infos.find(k));
	}
	std::optional<operator_info> operator_info_table_impl::get_deep(const ast::string_type& k) {
		auto itr=infos.find(k);
		if (itr != infos.end()) {
			return itr->second;
		}
		if (!parent) {
			return std::nullopt;
		}
		return parent->get_deep(k);
	}
}