#include "typing.h"
#include "register-name.h"

namespace tig::magimocha::typing {
std::shared_ptr<ast::type_data>
resolve_type(std::shared_ptr<type_table> table,
             std::shared_ptr<ast::type_data> t,
             std::unordered_set<std::shared_ptr<ast::var_type_data>> &free) {
    switch(t->type()) {
    case ast::type_data_type::simple:
        return t;
    case ast::type_data_type::function: {
        auto f = std::static_pointer_cast<ast::function_type_data>(t);
        std::vector<std::shared_ptr<ast::type_data>> args;
        for(auto e : f->args) {
            args.push_back(resolve_type(table, e, free));
        }
        return std::make_shared<ast::function_type_data>(
            resolve_type(table, f->result_type, free), args);
    }
    case ast::type_data_type::var: {
        auto v = table->find(std::static_pointer_cast<ast::var_type_data>(t));
        if(*v) {
            return resolve_type(table, v->get(), free);
        }
        free.insert(std::static_pointer_cast<ast::var_type_data>(t));
        return t;
    }
    default:
        throw "resolve_type error";
    }
}
struct resolve_name_visitor {
    std::shared_ptr<name::module_table> mt;
    std::shared_ptr<name::variable_table> vt;
    const std::unordered_map<std::shared_ptr<ast::declaration_module>,
                             std::shared_ptr<name::module_table>> &dm2mt;
    const std::unordered_map<std::shared_ptr<ast::make_scope>,
                             std::shared_ptr<name::variable_table>> &ms2vt;
    auto operator()(std::shared_ptr<ast::call_name> cn) {
        return name::get_with_path(cn->value(), mt, vt, dm2mt, ms2vt);
    }
    template <class T> T operator()(T v) { return v; }
};
std::shared_ptr<ast::typed_data> resolve_name(
    std::shared_ptr<name::module_table> mt,
    std::shared_ptr<name::variable_table> vt,
    const std::unordered_map<std::shared_ptr<ast::declaration_module>,
                             std::shared_ptr<name::module_table>> &dm2mt,
    const std::unordered_map<std::shared_ptr<ast::make_scope>,
                             std::shared_ptr<name::variable_table>> &ms2vt,
    std::shared_ptr<ast::typed_data> t) {
    auto vis = resolve_name_visitor{mt, vt, dm2mt, ms2vt};
    return ast::visit<std::shared_ptr<ast::typed_data>>(vis, t);
}
void unify(std::shared_ptr<type_table> table,
           std::shared_ptr<ast::type_data> t1,
           std::shared_ptr<ast::type_data> t2) {
    if(t1 == t2) {
        return;
    }
    if(t1->type() == ast::type_data_type::simple &&
       t2->type() == ast::type_data_type::simple &&
       std::static_pointer_cast<ast::simple_type_data>(t1)->value() ==
           std::static_pointer_cast<ast::simple_type_data>(t2)->value()) {
        return;
    }
    if(t1->type() == ast::type_data_type::function &&
       t2->type() == ast::type_data_type::function) {
        auto ft1 = std::static_pointer_cast<ast::function_type_data>(t1);
        auto ft2 = std::static_pointer_cast<ast::function_type_data>(t2);
        auto args1itr = ft1->args.cbegin();
        auto args2itr = ft2->args.cbegin();
        auto args1end = ft1->args.cend();
        auto args2end = ft2->args.cend();
        while(true) {
            if(args1itr == args1end) {
                if(args2itr == args2end) {
                    break;
                }
                throw "unification error";
            }
            unify(table, *args1itr, *args2itr);

            ++args1itr;
            ++args2itr;
        }
        unify(table, ft1->result_type, ft2->result_type);
        return;
    }

    if(t1->type() == ast::type_data_type::var) {
        auto t1v =
            table->find(std::static_pointer_cast<ast::var_type_data>(t1));
        if(!*t1v) {
            t1v->set(t2);
        } else {
            unify(table, t1v->get(), t2);
        }
        return;
    }
    if(t2->type() == ast::type_data_type::var) {
        auto t2v =
            table->find(std::static_pointer_cast<ast::var_type_data>(t2));
        if(!*t2v) {
            t2v->set(t1);
        } else {
            unify(table, t2v->get(), t1);
        }
        return;
    }
    throw "unification error";
}
std::shared_ptr<ast::type_data> infer(context con,
                                      std::shared_ptr<ast::expression> td) {
    using R = std::shared_ptr<ast::type_data>;

    struct visitor {
        context con;

        R operator()(std::shared_ptr<ast::literal_> l) {
            return l->return_type();
        }
        R operator()(std::shared_ptr<ast::call_name> cn) {
            auto ptr =
                name::get_with_path(cn->value(), con.mt, con.vars,
                                    con.declaration_module_2_module_table_table,
                                    con.make_scope_2_variable_table_table);
            if(!ptr) {
                throw "error:undefined name";
            }
            auto schema_opt = con.schemas->get(ptr);
            if(!schema_opt) {
                throw "ILLEGAL STATE";
            }
            unify(con.types, cn->return_type(), schema_opt->type_data);

            // return cn->return_type();
            return create_type_schema(con.types, schema_opt->type_data)
                .type_data;
            // return create_type_schema_from(schema_opt.value()).type_data;
        }
        R operator()(std::shared_ptr<ast::declaration_variable> dv) {
            auto &&name = dv->name();
            auto ref = con.vars->find_shallow(name);
            if(*ref) {
                throw "error:defined name";
            }
            ref->set(dv);
            unify(con.types, infer(con, dv->body()), dv->return_type());
            auto schema = create_type_schema(con.types, dv->return_type());
            con.schemas->set(dv, schema);
            return create_type_schema_from(con.types, schema).type_data;
        }
        R operator()(std::shared_ptr<ast::declaration_function> df) {
            // auto new_vars = name::create_variable_table(con.vars);
            // con.make_scope_2_variable_table_table->set(df, new_vars);
            auto &params = df->params();
            // std::vecttor<declaration_parameter> newparams;
            for(auto &&dp : params) {

                auto &&name = dp->name();
                auto schema = create_type_schema(con.types, dp->return_type());
                con.schemas->set(dp, schema);
                if(!name) {
                    continue;
                }
                /*auto ref = new_vars->find_shallow(name.value());
                if(*ref) {
                    throw "error:defined name";
                }
                ref->set(dp);*/
            }

            unify(con.types,
                  infer(context{con.mt,
                                con.make_scope_2_variable_table_table.at(df),
                                con.types, con.schemas,
                                con.declaration_module_2_module_table_table,
                                con.make_scope_2_variable_table_table},
                        df->body()),
                  df->return_type_func()->result_type);

            auto schema = create_type_schema(con.types, df->return_type());
            con.schemas->set(df, schema);

            return create_type_schema_from(con.types, schema).type_data;
        }
        R operator()(std::shared_ptr<ast::apply_function> af) {
            auto resolved = resolve_name(con.mt,con.vars,con.declaration_module_2_module_table_table,con.make_scope_2_variable_table_table, af->target());
            if(!resolved) {
                throw "undefined name";
            }
            auto schema_opt = con.schemas->get(resolved);
            if(!schema_opt) {
                throw "ILLEGAL STATE";
            }
            auto target_schema =
                create_type_schema_from(con.types, schema_opt.value());
            auto target_type = target_schema.type_data;
            std::vector<std::shared_ptr<ast::type_data>> args_type;
            for(auto &&arg : af->args()) {
                args_type.push_back(infer(con, arg));
            }
            auto ft = std::make_shared<ast::function_type_data>(
                af->return_type(), args_type);
            unify(con.types, target_type, ft);

            return af->return_type();
        }
        R operator()(std::shared_ptr<ast::operation>) { throw "ILLEGAL STATE"; }
        R operator()(std::shared_ptr<ast::expression_block> exprs) {
            auto &&list = exprs->value();
            std::shared_ptr<ast::type_data> r;
            auto ncon = context{con.mt,
                                con.make_scope_2_variable_table_table.at(exprs),
                                con.types,
                                con.schemas,
                                con.declaration_module_2_module_table_table,
                                con.make_scope_2_variable_table_table};
            for(auto &&e : list) {
                r = infer(ncon, e);
            }
            unify(con.types, exprs->return_type(), r);
            return exprs->return_type();
        }
        R operator()(std::shared_ptr<ast::named_function> nf) {
            /*auto &&name = nf->name();
            auto ref = con.vars->find_shallow(name);
            if(*ref) {
                throw "error:defined name";
            }
            ref->set(nf);*/
            auto ncon = context{con.mt,
                                con.make_scope_2_variable_table_table.at(nf),
                                con.types,
                                con.schemas,
                                con.declaration_module_2_module_table_table,
                                con.make_scope_2_variable_table_table};
            unify(con.types, nf->return_type(), infer(ncon, nf->body()));
            auto schema = create_type_schema(con.types, nf->return_type());
            con.schemas->set(nf, schema);

            return create_type_schema_from(con.types, schema).type_data;
        }
        R operator()(std::shared_ptr<ast::declaration_infix> di) {
            return di->return_type();
        }
    };
    auto vis = visitor{con};
    return ast::visit<R>(vis, td);
}
std::shared_ptr<ast::type_data>
replace_types(std::shared_ptr<type_table> types,
              std::shared_ptr<ast::type_data> type_data,
              const std::unordered_map<std::shared_ptr<ast::type_data>,
                                       std::shared_ptr<ast::type_data>> &map) {
    auto itr = map.find(type_data);
    if(itr != map.end()) {
        return replace_types(types, itr->second, map);
    }
    switch(type_data->type()) {
    case ast::type_data_type::var: {
        auto vt = std::static_pointer_cast<ast::var_type_data>(type_data);
        auto ref = types->find(vt);
        if(!*ref) {
            return vt;
        }
        return replace_types(types, ref->get(), map);
    }
    case ast::type_data_type::simple: {
        return type_data;
    }
    case ast::type_data_type::function: {
        auto ft = std::static_pointer_cast<ast::function_type_data>(type_data);

        std::vector<std::shared_ptr<ast::type_data>> args;
        for(auto &&arg : ft->args) {
            args.push_back(replace_types(types, arg, map));
        }
        return std::make_shared<ast::function_type_data>(
            replace_types(types, ft->result_type, map), args);
    }
    }
    throw "illegal code exception";
}

void infer_all(context con, std::shared_ptr<ast::module_member> mm) {
    struct visitor {
        context con;
        using R = std::monostate;
        R r;
        R operator()(std::shared_ptr<ast::named_function> df) {
            infer(con, df);
            return r;
        }
        R operator()(std::shared_ptr<ast::declaration_variable> dv) {
            infer(con, dv);
            return r;
        }
        R operator()(std::shared_ptr<ast::declaration_module> dm) {
            auto nvar = name::create_variable_table(con.vars);
            // con.make_scope_2_variable_table_table->set(dm, nvar);
            auto ncon = context{con};
            ncon.vars = nvar;
            for(auto m : dm->members()) {
                infer_all(ncon, m);
            }
            return r;
        }
        R operator()(std::shared_ptr<ast::declaration_infix>) { return r; }
        R operator()(std::shared_ptr<ast::declaration_export>) { return r; }
    };
    ast::visit<visitor::R>(visitor{con}, mm);
}
} // namespace tig::magimocha::typing