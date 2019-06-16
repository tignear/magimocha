#pragma once
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include "enviroment.h"
#include "magimocha/ast.h"
#include "magimocha/ast-visitor.h"
namespace tig::magimocha::typing {
	struct typing_exception : std::exception {

	};

	struct variable_ref {
		virtual std::shared_ptr<ast::typed_data>  get() = 0;
		virtual void set(std::shared_ptr<ast::typed_data> td) = 0;
		virtual operator bool()const = 0;
		virtual ~variable_ref() {}
	};
	struct variable_table {
		virtual std::unique_ptr<variable_ref> find_shallow(const ast::string_type& name) = 0;
		virtual std::shared_ptr<ast::typed_data> get_deep(const ast::string_type& name) = 0;
		virtual ~variable_table(){}
	};
	std::shared_ptr<variable_table> create_variable_table(std::shared_ptr<variable_table> upper) {
		using map_t=std::unordered_map<ast::string_type, std::shared_ptr<ast::typed_data>>;
		struct variable_table_impl : variable_table {

			struct variable_ref_impl:variable_ref {
				const ast::string_type& k;
				map_t::const_iterator itr;
				map_t& map;
				variable_ref_impl(const ast::string_type& k, map_t::const_iterator itr, map_t& map) :k(k),itr(itr),map(map)
				{}
				std::shared_ptr<ast::typed_data>  get()override {
					return itr->second;
				}
				void set(std::shared_ptr<ast::typed_data> td)override {
					map.insert_or_assign(k,td);
				}
				operator bool()const override {
					return itr != map.cend();
				}
			};
			std::shared_ptr<variable_table> upper;
			map_t map;
			variable_table_impl(std::shared_ptr<variable_table> upper):upper(upper) {}
			std::unique_ptr<variable_ref> find_shallow(const ast::string_type& name)override {
				return std::make_unique<variable_ref_impl>(name, map.find(name), map);
			}
			std::shared_ptr<ast::typed_data> get_deep(const ast::string_type& name)override {
				auto itr = map.find(name);
				if (itr != map.cend()) {
					return itr->second;
				}
				if (!upper) {
					return std::shared_ptr<ast::typed_data>();
				}
				return upper->get_deep(name);
			}
		};
		return std::make_shared<variable_table_impl>(upper);
	}
	struct type_value_ref {
		virtual std::shared_ptr<ast::type_data>  get()=0;
		virtual void set(std::shared_ptr<ast::type_data> t) = 0;
		virtual operator bool()const = 0;
		virtual ~type_value_ref(){}
	};
	struct type_table {
		virtual std::unique_ptr<type_value_ref> find(std::shared_ptr<ast::var_type_data> t)=0;
		virtual ~type_table(){}
	};
	class type_table_impl:public type_table {
		using tmap=std::unordered_map<std::shared_ptr<ast::var_type_data>, std::shared_ptr<ast::type_data>>;
		tmap map;
		struct type_value_ref_impl:public type_value_ref {
			tmap& map;
			tmap::const_iterator itr;
			std::shared_ptr<ast::var_type_data> t;
			explicit type_value_ref_impl(tmap& map, tmap::const_iterator itr, std::shared_ptr<ast::var_type_data> t)
				:map(map),itr(itr),t(t) {

			}
			std::shared_ptr<ast::type_data>  get()override{
				return itr->second;
			}
			void set(std::shared_ptr<ast::type_data> d)override {
				map.insert_or_assign(t, d);
			}
			operator bool()const {
				return itr!=map.cend();
			}
		};
	public:
		std::unique_ptr<type_value_ref> find(std::shared_ptr<ast::var_type_data> t)override {
			return std::make_unique<type_value_ref_impl>(map, map.find(t), t);
			
		}
	};
	struct schema_table {

	};
	//void unify(std::shared_ptr<type_table> table, std::shared_ptr<ast::type_data> t1, std::shared_ptr<ast::type_data> t2);
	std::shared_ptr<ast::type_data> resolve_type(std::shared_ptr<type_table> table, std::shared_ptr<ast::type_data> t,std::unordered_set<std::shared_ptr<ast::type_data>>& free);
	std::shared_ptr<ast::type_data> infer(std::shared_ptr<type_table> table,std::shared_ptr<ast::typed_data> expr);
	void unify(std::shared_ptr<type_table> table, std::shared_ptr<ast::type_data> t1, std::shared_ptr<ast::type_data> t2) {
		if (t1 == t2) {
			return;
		}
		if (t1->type() == ast::type_data_type::simple &&
			t2->type() == ast::type_data_type::simple &&
			std::static_pointer_cast<ast::simple_type_data>(t1)->value() == std::static_pointer_cast<ast::simple_type_data>(t2)->value()) {
			return;
		}
		if (t1->type() == ast::type_data_type::var) {
			auto t1v = table->find(std::static_pointer_cast<ast::var_type_data>(t1));
			if (!*t1v) {
				t1v->set(t2);
			}
			else {
				unify(table,t1v->get(), t2);
			}
			return;
		}
		if (t2->type() == ast::type_data_type::var) {
			auto t2v = table->find(std::static_pointer_cast<ast::var_type_data>(t2));
			if (!*t2v) {
				t2v->set(t1);
			}
			else {
				unify(table, t2v->get(), t1);
			}
			return;
		}
		throw "unification error";
	}
	std::shared_ptr<ast::type_data> resolve_type(std::shared_ptr<type_table> table, std::shared_ptr<ast::type_data> t,std::unordered_set<std::shared_ptr<ast::type_data>>& free) {
		switch(t->type())
		{
			case ast::type_data_type::simple:
				return t;
			case ast::type_data_type::function:
				{
					auto f=std::static_pointer_cast<ast::function_type_data>(t);
					std::vector<std::shared_ptr<ast::type_data>> args;
					for(auto e:f->args){
						args.push_back(resolve_type(table,e,free));
					}
					return std::make_shared<ast::function_type_data>(resolve_type(table,f->result_type,free),args);
				}
			case ast::type_data_type::var:
				{
					auto v=table->find(std::static_pointer_cast<ast::var_type_data>(t));
					if (*v) {
						return resolve_type(table,v->get(),free);
					}
					free.insert(t);
					return t;
				}
			default:
				throw "resolve_type error";
		}

	}
	std::shared_ptr<ast::type_data> infer(
		std::shared_ptr<variable_table> vars,
		std::shared_ptr<type_table> types,
		std::shared_ptr<schema_table> schemas,
		std::shared_ptr<ast::typed_data> td
	){
		using R=std::shared_ptr<ast::type_data>;

		struct visitor {
			std::shared_ptr<variable_table> vars;
			std::shared_ptr<type_table> types;
			std::shared_ptr<schema_table> schemas;
			R  operator()(std::shared_ptr<ast::literal_> l) {
				return l->return_type();
			}
			R  operator()(std::shared_ptr<ast::declaration_parameter> dp) {
				auto&& name = dp->name();
				if (!name) {
					return dp->return_type();
				}
				auto ref=vars->find_shallow(name.value());
				if (*ref) {
					throw "error:defined name";
				}
				ref->set(dp);
				return dp->return_type();
			}
			R  operator()(std::shared_ptr<ast::call_name> cn) {
				auto ptr=vars->get_deep(cn->value());
				if (!ptr) {
					throw "error:undefined name";
				}
				unify(types,cn->return_type(),ptr->return_type());
				return cn->return_type();
			}
			R  operator()(std::shared_ptr<ast::declaration_variable> dv) {
				auto&& name = dv->name();
				auto ref = vars->find_shallow(name);
				if (*ref) {
					throw "error:defined name";
				}
				ref->set(dv);
				unify(types,infer(vars, types,schemas, dv->body()),dv->return_type());

				return dv->return_type();
			}
			R  operator()(std::shared_ptr<ast::declaration_function> df) {
				auto new_vars=create_variable_table(vars);
				auto& params = df->params();
				for (auto&& e : params) {
					auto&& name = e->name();
					if (!name) {
						continue;
					}
					new_vars->find_shallow(name.value())->set(e);
				}

				unify(types, infer(new_vars, types,schemas, df->body()), df->return_type_func()->result_type);
				//TODO make and add schema
				return df->return_type();

			}
			R  operator()(std::shared_ptr<ast::apply_function> af) {
				auto target_type=infer(vars,types,schemas,af->target());
				//TODO call schema
				std::vector<std::shared_ptr<ast::type_data>> args_type;
				for (auto&& arg : af->args()) {
					args_type.push_back(infer(vars, types, schemas, arg));
				}
				auto ft=std::make_shared<ast::function_type_data>(af->return_type(), args_type);
				unify(types,target_type, ft);
				return af->return_type();
			}
			R  operator()(std::shared_ptr<ast::operation> ) {
				throw "ILLEGAL STATE";
			}
			R  operator()(std::shared_ptr<ast::expression_block>) {
				throw "not impl";
			}
			R  operator()(std::shared_ptr<ast::named_function> nf) {
				auto&& name = nf->name();
				auto ref=vars->find_shallow(name);
				if (*ref) {
					throw "error:defined name";
				}
				ref->set(nf);
				unify(types,nf->return_type(),infer(vars,types,schemas,nf->body()));
				return nf->return_type();
			}
		};
		return ast::visit<R>(visitor{vars,types,schemas}, td);
	}
}