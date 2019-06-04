#include "visitor.h"
namespace tig::magimocha::codegen {
	std::list<ast::operator_token_type> expression_visitor::processing_apply_function(const std::list<typename ast::operator_token_type>& tokens) {
		std::list<ast::operator_token_type> r;
		for (auto itr = tokens.cbegin(); itr != tokens.cend(); ++itr) {
			if (std::holds_alternative<ast::op_token_function_applying>(*itr)) {
				r.pop_back();
				r.push_back(
					std::make_shared<ast::apply_function>(
						std::visit(
							[](auto&& e)->std::shared_ptr<typename ast::expression> {
					using T = std::decay_t<decltype(e)>;
					if constexpr (is_shared_ptr_v<T>) {
						return std::static_pointer_cast<typename ast::expression>(e);
					}
					else {
						throw std::invalid_argument("");
					}
				},
							*std::prev(itr)
					),
						std::get<ast::op_token_function_applying>(*itr).args()
					)
				);
				continue;
			}
			r.push_back(*itr);
		}
		return r;
	}
	std::list<ast::operator_token_type> expression_visitor::processing_single_operator(std::list<typename ast::operator_token_type>&& e) {
		auto fitr = e.begin();
		auto sitr = std::next(fitr);
		if (std::holds_alternative<ast::op_token_single>(*fitr)) {
			auto n = std::make_shared<ast::apply_function>(
				std::static_pointer_cast<typename ast::expression>(std::get<ast::op_token_single>(*fitr).name()),
				std::vector{
					std::visit(
						[](auto&& e)->std::shared_ptr<typename ast::expression> {
							using T = std::decay_t<decltype(e)>;
							if constexpr (is_shared_ptr_v<T>) {
								return std::static_pointer_cast<typename ast::expression>(e);
							}
							else {
								throw std::invalid_argument("");
							}
						},
						*sitr
					)
				}
			);
			e.erase(fitr, sitr);
			e.push_front(n);
		}
		return e;
	}
	std::shared_ptr<ast::expression> expression_visitor::processing_double_operator(std::list<typename ast::operator_token_type>&& tokens) {
		while (tokens.size() != 1) {
			auto itr = tokens.begin();
			std::optional<std::list<ast::operator_token_type>::iterator> high_left_itr = std::nullopt;
			std::optional<OperatorInfo> high_left_info;
			std::optional<ast::op_token_double> high_left_elem = std::nullopt;
			for (; itr != tokens.end(); ++itr) {
				auto elem = std::visit(
					[](auto&& e)->std::optional<ast::op_token_double> {
					using T = std::decay_t<decltype(e)>;
					if constexpr (is_shared_ptr_v<T>) {
						return std::nullopt;
					}
					else if constexpr (std::is_same_v<T, typename ast::op_token_double>) {
						return e;
					}
					else {
						throw std::invalid_argument("");
					}
				},
					*itr
					);
				if (!elem) {
					continue;
				}
				if (!high_left_itr && !high_left_elem && !high_left_info) {
					high_left_itr = itr;
					high_left_elem = elem;
					high_left_info = get_opinfo(elem.value());
					continue;
				}
				auto old_p = high_left_info.value().priority;
				auto ninfo = get_opinfo(elem.value());
				auto new_p = ninfo->priority;
				if (new_p > old_p) {
					high_left_itr = itr;
					high_left_elem = elem;
					high_left_info = ninfo;
					continue;
				}
				if (new_p == old_p) {
					if (high_left_info->infix == infix_type::left) {
						continue;
					}
					//old elem infix is right
					high_left_itr = itr;
					high_left_elem = elem;
					high_left_info = ninfo;
				}
			}
			if (!high_left_itr || !high_left_elem || !high_left_info) {
				throw std::invalid_argument("");
			}
			auto pitr = std::prev(high_left_itr.value());
			auto nitr = std::next(high_left_itr.value());
			auto nelem = std::make_shared<ast::apply_function>(
				high_left_elem.value().name(),
				std::vector<std::shared_ptr<ast::expression>>{
				std::visit(
					[](auto&& e)->std::shared_ptr<typename ast::expression> {
						using T = std::decay_t<decltype(e)>;
						if constexpr (is_shared_ptr_v<T>) {
							return std::static_pointer_cast<typename ast::expression>(e);
						}
						else {
							throw std::invalid_argument("");
						}
					},
					*pitr
					),
					std::visit(
						[](auto&& e)->std::shared_ptr<typename ast::expression> {
							using T = std::decay_t<decltype(e)>;
							if constexpr (is_shared_ptr_v<T>) {
								return std::static_pointer_cast<typename ast::expression>(e);
							}
							else {
								throw std::invalid_argument("");
							}
						},
						*nitr
					)
			}
			);
			tokens.insert(tokens.erase(pitr, ++nitr), nelem);
		}
		return std::visit(
			[](auto&& e)->std::shared_ptr<typename ast::expression> {
			using T = std::decay_t<decltype(e)>;
			if constexpr (is_shared_ptr_v<T>) {
				return std::static_pointer_cast<typename ast::expression>(e);
			}
			else {
				throw std::invalid_argument("");
			}
		},
			tokens.front()
			);
	}
}