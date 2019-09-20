#include "register-name.h"
#include "magimocha/ast-visitor.h"
namespace tig::magimocha::name {

template <class T> struct table_of_T_impl : name::table_of_T<T> {
    using map_t = std::unordered_map<ast::string_type, std::shared_ptr<T>>;
    struct ref_of_T_impl : name::ref_of_T<T> {
        const ast::string_type &k;
        typename map_t::const_iterator itr;
        map_t &map;
        ref_of_T_impl(const ast::string_type &k,
                      typename map_t::const_iterator itr, map_t &map)
            : k(k), itr(itr), map(map) {}
        std::shared_ptr<T> get() override { return itr->second; }
        void set(std::shared_ptr<T> td) override {
            map.insert_or_assign(k, td);
        }
        operator bool() const override { return itr != map.cend(); }
    };
    std::shared_ptr<name::table_of_T<T>> upper;
    map_t map;
    table_of_T_impl(std::shared_ptr<name::table_of_T<T>> upper)
        : upper(upper) {}
    std::unique_ptr<name::ref_of_T<T>>
    find_shallow(const ast::string_type &name) override {
        return std::make_unique<ref_of_T_impl>(name, map.find(name), map);
    }
    std::shared_ptr<T> get_deep(const ast::string_type &name) override {
        auto itr = map.find(name);
        if(itr != map.cend()) {
            return itr->second;
        }
        if(!upper) {
            return std::shared_ptr<T>();
        }
        return upper->get_deep(name);
    }
};
std::shared_ptr<name::variable_table>
create_variable_table(std::shared_ptr<name::variable_table> upper =
                          std::shared_ptr<name::variable_table>()) {
    return std::make_shared<table_of_T_impl<ast::typed_data>>(upper);
}
std::shared_ptr<name::module_table>
create_module_table(std::shared_ptr<name::module_table> upper) {
    return std::make_shared<table_of_T_impl<ast::declaration_module>>(upper);
}
struct register_name_visitor {
    using R = std::monostate;
    std::shared_ptr<variable_table> vt;
    std::unordered_map<std::shared_ptr<ast::make_scope>,
                       std::shared_ptr<variable_table>> &ms2vt;
    R r;
    R operator()(std::shared_ptr<ast::named_function> nf) {
        auto ref = vt->find_shallow(nf->name());
        if(*ref) {
            throw "defined name";
        }
        ref->set(nf);
        auto nvt = create_variable_table(vt);
        ms2vt[nf] = nvt;
        auto df = nf->body();
        ms2vt[df] = nvt;
        process_declaration_function(nvt, df);
        return r;
    }
    R operator()(std::shared_ptr<ast::declaration_function> df) {
        auto nvt = create_variable_table(vt);
        ms2vt[df] = nvt;
        process_declaration_function(nvt, df);
        return r;
    }
    void process_declaration_function(
        std::shared_ptr<variable_table> nvt,
        std::shared_ptr<ast::declaration_function> df) {
        for(auto param : df->params()) {
            auto no = param->name();
            if(!no) {
                continue;
            }
            auto ref = nvt->find_shallow(*no);
            if(*ref) {
                throw "defined name";
            }
            ref->set(param);
        }
        auto vis = register_name_visitor{nvt, ms2vt};
        ast::visit<R>(vis, df->body());
    }
    R operator()(std::shared_ptr<ast::expression_block> eb) {
        auto nvt = create_variable_table(vt);
        ms2vt[eb] = nvt;
        auto vis = register_name_visitor{nvt, ms2vt};
        for(auto expr : eb->value()) {
            ast::visit<R>(vis, expr);
        }
        return r;
    }
    R operator()(std::shared_ptr<ast::declaration_variable> dv) {
        auto ref = vt->find_shallow(dv->name());
        if(*ref) {
            throw "defined name";
        }
        ast::visit<R>(*this, dv->body());
        return r;
    }
    struct register_name_operation_visitor {
        using R = void;
        std::shared_ptr<variable_table> vt;
        std::unordered_map<std::shared_ptr<ast::make_scope>,
                           std::shared_ptr<variable_table>> &ms2vt;
        R operator()(ast::op_token_single) {}
        R operator()(ast::op_token_double) {}
        R operator()(ast::op_token_function_applying) {}
        R operator()(std::shared_ptr<ast::declaration_function>) {}
        R operator()(std::shared_ptr<ast::literal_>) {}
        R operator()(std::shared_ptr<ast::expression> expr) {
            auto vis = register_name_visitor{vt, ms2vt};
            ast::visit<register_name_visitor::R>(vis, expr);
        }
        R operator()(std::shared_ptr<ast::call_name>) {}
    };
    R operator()(std::shared_ptr<ast::operation> op) {
        auto v = register_name_operation_visitor{vt, ms2vt};
        for(auto e : op->value()) {
            std::visit(v, e);
        }
    }
    R operator()(std::shared_ptr<ast::literal_> lit) { return r; }
    R operator()(std::shared_ptr<ast::apply_function> af) { return r; }
    R operator()(std::shared_ptr<ast::call_name> cn) { return r; }
    R operator()(std::shared_ptr<ast::declaration_infix> di) { return r; }
};
struct register_name_visitor_module {
    using R = std::monostate;
    std::shared_ptr<module_table> mt;
    std::shared_ptr<variable_table> vt;
    std::unordered_map<std::shared_ptr<ast::declaration_module>,
                       std::shared_ptr<module_table>> &dm2mm;
    std::unordered_map<std::shared_ptr<ast::make_scope>,
                       std::shared_ptr<variable_table>> &ms2vt;
    R r;
    R operator()(std::shared_ptr<ast::declaration_module> dm) {
        auto ref = mt->find_shallow(dm->name());
        if(*ref) {
            throw "defined name";
        }
        ref->set(dm);
        auto nmt = create_module_table(mt);
        auto nvt = create_variable_table(vt);
        dm2mm[dm] = nmt;
        ms2vt[dm] = nvt;
        auto vis = register_name_visitor_module{nmt, nvt, dm2mm, ms2vt};
        for(auto m : dm->members()) {
            ast::visit<register_name_visitor_module::R>(vis, m);
        }
        return r;
    }
    R operator()(std::shared_ptr<ast::named_function> nf) {
        register_name_visitor{vt, ms2vt}(nf);
        return r;
    }
    R operator()(std::shared_ptr<ast::declaration_variable> dv) {
        auto ref = vt->find_shallow(dv->name());
        if(*ref) {
            throw "defined name";
        }
        ref->set(dv);
        auto vis = register_name_visitor{vt, ms2vt};
        ast::visit<R>(vis, dv->body());
        return r;
    }
    R operator()(std::shared_ptr<ast::declaration_export> de) {
        // TODO
        return r;
    }
    R operator()(std::shared_ptr<ast::declaration_infix> de) {
        // TODO
        return r;
    }
};
void register_name(std::shared_ptr<ast::declaration_module> target,
                   std::shared_ptr<module_table> target_parent_mt,
                   std::shared_ptr<variable_table> target_parent_vt,
                   std::unordered_map<std::shared_ptr<ast::declaration_module>,
                                      std::shared_ptr<module_table>> &dm2mm,
                   std::unordered_map<std::shared_ptr<ast::make_scope>,
                                      std::shared_ptr<variable_table>> &ms2vt) {
    auto vis = register_name_visitor_module{target_parent_mt, target_parent_vt,
                                            dm2mm, ms2vt};
    vis(target);
}
void register_name(std::shared_ptr<ast::expression> target,
                   std::shared_ptr<variable_table> target_parent,
                   std::unordered_map<std::shared_ptr<ast::make_scope>,
                                      std::shared_ptr<variable_table>> &ms2vt) {
    auto vis = register_name_visitor{target_parent, ms2vt};
    ast::visit<register_name_visitor::R>(vis, target);
}
} // namespace tig::magimocha::name