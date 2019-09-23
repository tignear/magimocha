#include "operation.h"
namespace tig::magimocha::codegen2 {
namespace {
struct
    operation_to_function_applying_process_op_token_function_applying_visitor_2 {
    using R = std::shared_ptr<ast::expression>;
    template <typename T> R operator()(T x) { return x; }
    R operator()(ast::op_token_single ots) { return ots.name(); }
    R operator()(ast::op_token_double otd) { return otd.name(); }
};
struct
    operation_to_function_applying_process_op_token_function_applying_visitor {
    std::list<operator_token_type_processed_op_token_function_applying>
        &list_ref;
    operation_to_function_applying_process_op_token_function_applying_visitor(
        std::list<operator_token_type_processed_op_token_function_applying>
            &list_ref)
        : list_ref(list_ref) {}
    using R = std::monostate;
    static constexpr const R r = std::monostate{};
    R operator()(ast::op_token_function_applying otfa) {

        auto e = std::visit(
            operation_to_function_applying_process_op_token_function_applying_visitor_2{},
            list_ref.back());

        list_ref.pop_back();
        auto &&args = otfa.args();
        list_ref.push_back(std::make_shared<ast::apply_function>(e, args));
        return r;
    }
    template <typename T> R operator()(T x) {
        list_ref.push_back(x);
        return r;
    }
};
} // namespace
std::list<operator_token_type_processed_op_token_function_applying>
operation_to_function_applying_process_op_token_function_applying(
    const std::list<ast::operator_token_type> &src) {
    std::list<operator_token_type_processed_op_token_function_applying> ret;
    auto vis =
        operation_to_function_applying_process_op_token_function_applying_visitor{
            ret};
    auto end = src.cend();
    for(auto itr = src.cbegin(); itr != end; ++itr) {
        std::visit(vis, *itr);
    }
    return ret;
}
namespace {
struct operation_to_function_applying_process_op_token_single_visitor {
    std::optional<ast::op_token_single> buf = std::nullopt;
    std::list<operator_token_type_processed_op_token_single> &list_ref;
    operation_to_function_applying_process_op_token_single_visitor(
        std::list<operator_token_type_processed_op_token_single> &list_ref)
        : list_ref(list_ref) {}
    template <class T> void operator()(T x) {
        if(!buf) {
            list_ref.push_back(x);
            return;
        }
        list_ref.push_back(std::make_shared<ast::apply_function>(
            buf.value().name(),
            std::vector<std::shared_ptr<ast::expression>>{x}));
        buf = std::nullopt;
    }
    void operator()(ast::op_token_double x) {
        if(!buf) {
            list_ref.push_back(x);
            return;
        };
        list_ref.push_back(std::make_shared<ast::apply_function>(
            buf.value().name(),
            std::vector<std::shared_ptr<ast::expression>>{x.name()}));
        buf = std::nullopt;
    }
    void operator()(ast::op_token_single ots) {
        if(buf) {
            throw "illegal state";
        }
        buf = ots;
    }
};
} // namespace
std::list<operator_token_type_processed_op_token_single>
operation_to_function_applying_process_op_token_single(
    const std::list<operator_token_type_processed_op_token_function_applying>
        &src) {
    std::list<operator_token_type_processed_op_token_single> ret;
    auto v =
        operation_to_function_applying_process_op_token_single_visitor{ret};
    for(auto &&e : src) {
        std::visit(v, e);
    }
    return ret;
}

namespace {
struct operation_to_function_applying_process_op_token_double_visitor {
    using R = std::optional<operator_info>;
    std::shared_ptr<operator_info_table> op_infos;
    operation_to_function_applying_process_op_token_double_visitor(
        std::shared_ptr<operator_info_table> op_infos)
        : op_infos(op_infos) {}
    template <class T> R operator()(T x) { return std::nullopt; }

    R operator()(ast::op_token_double ots) {
        const auto &v = ots.name()->value();
        if(v.size()!=1){//TODO
            throw "NIMPL";
        }
        return op_infos->get_deep(v.back());
    }
};
struct operation_to_function_applying_process_op_token_double_visitor_2 {
    using R = std::shared_ptr<ast::expression>;
    operation_to_function_applying_process_op_token_double_visitor_2() {}
    template <class T> R operator()(T x) { return x; }

    R operator()(ast::op_token_double ots) { return ots.name(); }
};
} // namespace
std::shared_ptr<ast::expression>
operation_to_function_applying_process_op_token_double(
    std::list<operator_token_type_processed_op_token_single>::const_iterator
        begin,
    std::list<operator_token_type_processed_op_token_single>::const_iterator
        end,
    std::shared_ptr<operator_info_table> current,
    std::unordered_map<std::shared_ptr<ast::make_scope>,
                       std::shared_ptr<operator_info_table>> &map) {
    operation_to_function_applying_process_op_token_double_visitor vis{current};
    std::optional<operator_info> low_info;
    std::list<operator_token_type_processed_op_token_single>::const_iterator
        low_itr;
    auto itr = begin;
    ++itr;
    if(itr == end) {
        return std::visit(
            operation_to_function_applying_process_op_token_double_visitor_2{},
            *begin);
    }
    for(; itr != end; ++itr) {
        auto r = std::visit(vis, *itr);
        if(!r) {
            continue;
        }
        if(!low_info) {
            low_info = r;
            low_itr = itr;
            continue;
        }
        auto &liv = low_info.value();
        auto &rv = r.value();
        if(liv.priority < rv.priority) {
            continue;
        }
        if(liv.priority > rv.priority) {
            low_info = r;
            low_itr = itr;
            continue;
        }
        switch(liv.infix) {
        case ast::infix_type::left:
            switch(rv.infix) {
            case ast::infix_type::left:
                low_info = r;
                low_itr = itr;
                continue;
            case ast::infix_type::right:
                low_info = r;
                low_itr = itr;
                continue;
            }
        case ast::infix_type::right:
            switch(rv.infix) {
            case ast::infix_type::left:
                low_info = r;
                low_itr = itr;
                continue;
            case ast::infix_type::right:
                continue;
            }

        default:
            break;
        }
    }

    if(!low_info) {
        throw "syntax error";
    }
    return std::make_shared<ast::apply_function>(
        std::visit(
            operation_to_function_applying_process_op_token_double_visitor_2{},
            *low_itr),
        std::vector<std::shared_ptr<ast::expression>>{
            operation_to_function_applying_process_op_token_double(
                begin, low_itr, current, map),
            operation_to_function_applying_process_op_token_double(
                std::next(low_itr), end, current, map)});
}
std::shared_ptr<ast::expression> operation_to_function_applying(
    std::shared_ptr<ast::operation> op,
    std::shared_ptr<operator_info_table> current,
    std::unordered_map<std::shared_ptr<ast::make_scope>,
                       std::shared_ptr<operator_info_table>> &map) {
    auto &&tokens = op->value();
    auto buf = operation_to_function_applying_process_op_token_single(
        operation_to_function_applying_process_op_token_function_applying(
            tokens));
    return operation_to_function_applying_process_op_token_double(
        buf.begin(), buf.end(), current, map);
}
namespace {
struct operation_to_function_applying_visitor {
    std::shared_ptr<operator_info_table> current;
    std::unordered_map<std::shared_ptr<ast::make_scope>,
                       std::shared_ptr<operator_info_table>> &op_infos;
    // std::unordered_map<std::shared_ptr<ast::typed_data>,
    // std::shared_ptr<ast::typed_data>>& mapped;
    auto operator()(std::shared_ptr<ast::declaration_function> x) {
        return std::make_shared<ast::declaration_function>(
            x->params(), x->return_type_func(),
            operation_to_function_applying_all(x->body(), op_infos.at(x), op_infos));
    }
    auto operator()(std::shared_ptr<ast::signed_number_literal> x) {
        // do nothing
        return x;
    }
    auto operator()(std::shared_ptr<ast::unsigned_number_literal> x) {
        // do nothing
        return x;
    }
    auto operator()(std::shared_ptr<ast::floating_literal> x) {
        // do nothing
        return x;
    }

    auto operator()(std::shared_ptr<ast::string_literal> x) {
        // do nothing
        return x;
    }
    auto operator()(std::shared_ptr<ast::apply_function> x) {
        std::vector<std::shared_ptr<ast::expression>> exprs;
        for(auto &&arg : x->args()) {
            exprs.push_back(
                operation_to_function_applying_all(arg, current, op_infos));
        }
        return std::make_shared<ast::apply_function>(
            operation_to_function_applying_all(x->target(), current, op_infos),
            exprs);
    }
    auto operator()(std::shared_ptr<ast::call_name> x) {
        // do nothing
        return x;
    }
    auto operator()(std::shared_ptr<ast::expression_block> x) {
        std::vector<std::shared_ptr<ast::expression>> exprs;
        for(auto &&expr : x->value()) {
            exprs.push_back(
                operation_to_function_applying_all(expr, op_infos.at(x), op_infos));
        }
        return std::make_shared<ast::expression_block>(std::move(exprs));
    }
    auto operator()(std::shared_ptr<ast::operation> x) {
        auto &&m = operation_to_function_applying(x, current, op_infos);
        return operation_to_function_applying_all(m, current, op_infos);
    }
    auto operator()(std::shared_ptr<ast::named_function> x) {

        return std::make_shared<ast::named_function>(
            x->name(), this->operator()(x->body()));
    }
    auto operator()(std::shared_ptr<ast::declaration_variable> x) {
        return std::make_shared<ast::declaration_variable>(
            x->name(), x->return_type(),
            operation_to_function_applying_all(x->body(), current, op_infos));
    }
    auto operator()(std::shared_ptr<ast::declaration_infix> x) { return x; }
    auto operator()(std::shared_ptr<ast::declaration_parameter> x) { return x; }

    auto operator()(std::shared_ptr<ast::declaration_module> x) {
        std::vector<std::shared_ptr<ast::module_member>> nmem;
        for(auto m : x->members()) {
            nmem.push_back(operation_to_function_applying_all_modlue_member(
                m, op_infos.at(x), op_infos));
        }
        return std::make_shared<ast::declaration_module>(x->name(), nmem);
    }
    auto operator()(std::shared_ptr<ast::declaration_export> x) { return x; }
};
struct operation_to_function_applying_laef_visitor
    : private operation_to_function_applying_visitor {
    operation_to_function_applying_laef_visitor(
        std::shared_ptr<operator_info_table> current,
        std::unordered_map<std::shared_ptr<ast::make_scope>,
                           std::shared_ptr<operator_info_table>> &op_infos)
        : operation_to_function_applying_visitor{current, op_infos} {}
    auto operator()(std::shared_ptr<ast::operation> x) {
        return ast::to_ast_leaf(
            operation_to_function_applying_visitor::operator()(x));
    }
    template <class T> auto operator()(std::shared_ptr<T> x) {
        return operation_to_function_applying_visitor::operator()(x);
    }
};
} // namespace
std::shared_ptr<ast::ast_leaf> operation_to_function_applying_all_leaf(
    std::shared_ptr<ast::ast_leaf> target,
    std::shared_ptr<operator_info_table> current,
    std::unordered_map<std::shared_ptr<ast::make_scope>,
                       std::shared_ptr<operator_info_table>> &map) {
    auto vis = operation_to_function_applying_laef_visitor{current, map};
    return ast::visit<std::shared_ptr<ast::ast_leaf>>(vis, target);
}
std::shared_ptr<ast::module_member>
operation_to_function_applying_all_modlue_member(
    std::shared_ptr<ast::module_member> target,
    std::shared_ptr<operator_info_table> current,
    std::unordered_map<std::shared_ptr<ast::make_scope>,
                       std::shared_ptr<operator_info_table>> &map) {
    auto vis = operation_to_function_applying_visitor{current, map};
    return ast::visit<std::shared_ptr<ast::module_member>>(vis, target);
}
std::shared_ptr<ast::expression> operation_to_function_applying_all(
    std::shared_ptr<ast::expression> target,
    std::shared_ptr<operator_info_table> current,
    std::unordered_map<std::shared_ptr<ast::make_scope>,
                       std::shared_ptr<operator_info_table>> &map) {
    auto vis = operation_to_function_applying_visitor{current, map};
    return ast::visit<std::shared_ptr<ast::expression>>(vis, target);
}
struct operator_info_ref_impl : operator_info_ref {

    std::unordered_map<ast::string_type, operator_info> &map;
    ast::string_type key;
    std::unordered_map<ast::string_type, operator_info>::const_iterator itr;
    operator_info_ref_impl(
        std::unordered_map<ast::string_type, operator_info> &map,
        ast::string_type key,
        std::unordered_map<ast::string_type, operator_info>::const_iterator itr)
        : map(map), key(key), itr(itr) {}
    operator_info get() override { return itr->second; }
    void set(operator_info v) override { map.insert_or_assign(key, v); }
    operator bool() override { return itr != map.end(); }
};
std::unique_ptr<operator_info_ref>
operator_info_table_impl::find_shallow(const ast::string_type &k) {
    return std::make_unique<operator_info_ref_impl>(infos, k, infos.find(k));
}
std::optional<operator_info>
operator_info_table_impl::get_deep(const ast::string_type &k) {
    auto itr = infos.find(k);
    if(itr != infos.end()) {
        return itr->second;
    }
    if(!parent) {
        return std::nullopt;
    }
    return parent->get_deep(k);
}
struct extract_operator_info_visitor {
    std::shared_ptr<operator_info_table> p;
    std::unordered_map<std::shared_ptr<ast::make_scope>,
                       std::shared_ptr<operator_info_table>> &map;
    extract_operator_info_visitor(
        std::shared_ptr<operator_info_table> p,
        std::unordered_map<std::shared_ptr<ast::make_scope>,
                           std::shared_ptr<operator_info_table>> &map)
        : p(p), map(map) {}
    void operator()(ast::op_token_function_applying es) {
        for(auto &&e : es.args()) {
            extract_operator_info(ast::to_ast_leaf(e), p, map);
        }
    }
    template <class T> void operator()(T) {
        // do nothing
    }
    template <class T> void operator()(std::shared_ptr<T> e) {
        extract_operator_info(ast::to_ast_leaf(e), p, map);
    }
};
void extract_operator_info(
    std::shared_ptr<ast::ast_leaf> target,
    std::shared_ptr<operator_info_table> current,
    std::unordered_map<std::shared_ptr<ast::make_scope>,
                       std::shared_ptr<operator_info_table>> &map) {
    struct visitor {
        using R = std::monostate;
        std::shared_ptr<operator_info_table> p;
        std::monostate r;
        std::unordered_map<std::shared_ptr<ast::make_scope>,
                           std::shared_ptr<operator_info_table>> &map;
        visitor(std::shared_ptr<operator_info_table> p,
                std::unordered_map<std::shared_ptr<ast::make_scope>,
                                   std::shared_ptr<operator_info_table>> &map)
            : p(p), map(map) {}
        auto operator()(std::shared_ptr<ast::declaration_function> x) {
            auto np = std::make_shared<operator_info_table_impl>(p);
            map.insert(std::make_pair(x, np));
            extract_operator_info(ast::to_ast_leaf(x->body()), np, map);
            return r;
        }
        auto operator()(std::shared_ptr<ast::signed_number_literal> x) {
            // do nothing
            return r;
        }
        auto operator()(std::shared_ptr<ast::unsigned_number_literal> x) {
            // do nothing
            return r;
        }
        auto operator()(std::shared_ptr<ast::floating_literal> x) {
            // do nothing
            return r;
        }

        auto operator()(std::shared_ptr<ast::string_literal> x) {
            // do nothing
            return r;
        }
        auto operator()(std::shared_ptr<ast::apply_function> x) {
            extract_operator_info(ast::to_ast_leaf(x->target()), p, map);
            for(auto &&e : x->args()) {
                extract_operator_info(ast::to_ast_leaf(e), p, map);
            }
            return r;
        }
        auto operator()(std::shared_ptr<ast::call_name> x) {
            // do nothing
            return r;
        }
        auto operator()(std::shared_ptr<ast::expression_block> x) {
            auto np = std::make_shared<operator_info_table_impl>(p);
            map.insert(std::make_pair(x, np));
            for(auto e : x->value()) {
                extract_operator_info(ast::to_ast_leaf(e), np, map);
            }
            return r;
        }
        auto operator()(std::shared_ptr<ast::operation> x) {

            for(auto &&var : x->value()) {
                std::visit(extract_operator_info_visitor(p, map), var);
            }
            return r;
        }
        auto operator()(std::shared_ptr<ast::named_function> x) {
            extract_operator_info(ast::to_ast_leaf(x->body()), p, map);

            return r;
        }
        auto operator()(std::shared_ptr<ast::declaration_variable> x) {
            extract_operator_info(ast::to_ast_leaf(x->body()), p, map);
            return r;
        }
        auto operator()(std::shared_ptr<ast::declaration_infix> x) {
            auto &&ref = p->find_shallow(x->name());

            ref->set(operator_info{x->priority(), x->infix_type()});
            return r;
        }
        auto operator()(std::shared_ptr<ast::declaration_export> x) {

            return r;
        }
        auto operator()(std::shared_ptr<ast::declaration_module> x) {
            auto np = std::make_shared<operator_info_table_impl>(p);
            map.insert(std::make_pair(x, np));
            for(auto &&e : x->members()) {
                extract_operator_info(ast::to_ast_leaf(e), np, map);
            }

            return r;
        }
        auto operator()(std::shared_ptr<ast::declaration_parameter> x) {

            return r;
        }
    };
    auto v = visitor(current, map);
    ast::visit<visitor::R, visitor>(v, target);
}
} // namespace tig::magimocha::codegen2