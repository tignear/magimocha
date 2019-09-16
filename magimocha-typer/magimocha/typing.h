#pragma once
#include <memory>
#include <unordered_map>
#include <unordered_set>
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
	std::shared_ptr<variable_table> create_variable_table(std::shared_ptr<variable_table> upper);
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

	struct type_schema {
		std::unordered_set<std::shared_ptr<ast::var_type_data>> type_vars;
		std::shared_ptr<ast::type_data> type_data;
	};
	struct type_schema_table {
		virtual std::optional<type_schema> get(std::shared_ptr<ast::typed_data> k) = 0;
		virtual void set(std::shared_ptr<ast::typed_data> k, type_schema v) = 0;

	};
	struct type_schema_table_impl:type_schema_table {
		std::unordered_map<std::shared_ptr<ast::typed_data>, type_schema> map;
		std::optional<type_schema> get(std::shared_ptr<ast::typed_data> k)override {
			auto itr = map.find(k);
			if (itr == map.end()) {
				return std::nullopt;
			}
			return itr->second;
		}
		void set(std::shared_ptr<ast::typed_data> k, type_schema v)override {
			map.insert_or_assign(k,v);
		}
	};
	struct make_scope_2_variable_table_table
	{
		virtual void set(std::shared_ptr<ast::make_scope> k, std::shared_ptr<variable_table> v)=0;
		virtual  std::shared_ptr<variable_table> get(std::shared_ptr<ast::make_scope> k) = 0;
	};
	struct make_scope_2_variable_table_table_impl:make_scope_2_variable_table_table {
		std::unordered_map<std::shared_ptr<ast::make_scope>, std::shared_ptr<variable_table>> map;
		void set(std::shared_ptr<ast::make_scope> k, std::shared_ptr<variable_table> v)override {
			map.insert_or_assign(k,v);
		}
		std::shared_ptr<variable_table> get(std::shared_ptr<ast::make_scope> k)override {
			auto itr=map.find(k);
			if (itr == cend(map)) {
				return std::shared_ptr<variable_table>();
			}
			return itr->second;
		}

	};
	struct context {
		std::shared_ptr<variable_table> vars;
		std::shared_ptr<type_table> types;
		std::shared_ptr<type_schema_table> schemas;
		std::shared_ptr<make_scope_2_variable_table_table> typed_data_2_variable_table_table;
	};
	std::shared_ptr<ast::type_data> replace_types(
		std::shared_ptr<type_table> types,
		std::shared_ptr<ast::type_data> type_data,
		const std::unordered_map<std::shared_ptr<ast::type_data>,std::shared_ptr<ast::type_data>>& map
	);
	inline static type_schema create_type_schema_from(std::shared_ptr<type_table> types,type_schema ts) {
		std::unordered_map<std::shared_ptr<ast::type_data>, std::shared_ptr<ast::type_data>> map;
		std::unordered_set<std::shared_ptr<ast::var_type_data>> set;
		for (auto&& var:ts.type_vars) {
			auto nvar = std::make_shared<ast::var_type_data>();
			map.insert(std::make_pair(var,nvar));
			set.insert(nvar);
		}
		return type_schema{ set,replace_types(types,ts.type_data,map) };
	}

	//void unify(std::shared_ptr<type_table> table, std::shared_ptr<ast::type_data> t1, std::shared_ptr<ast::type_data> t2);
	std::shared_ptr<ast::type_data> resolve_type(std::shared_ptr<type_table> table, std::shared_ptr<ast::type_data> t,std::unordered_set<std::shared_ptr<ast::var_type_data>>& free);
	static inline std::shared_ptr<ast::type_data> resolve_type(std::shared_ptr<type_table> table, std::shared_ptr<ast::type_data> t) {
		std::unordered_set<std::shared_ptr<ast::var_type_data>> ig;
		return resolve_type(table, t, ig);
	}
	std::shared_ptr<ast::typed_data> resolve_name(std::shared_ptr<variable_table> vars, std::shared_ptr<ast::typed_data> t);
	static inline type_schema create_type_schema(std::shared_ptr<type_table> types, std::shared_ptr<ast::type_data> t) {
		std::unordered_set<std::shared_ptr<ast::var_type_data>> free_vars;
		resolve_type(types, t, free_vars);
		return type_schema{ free_vars,t };
	}
	//std::shared_ptr<ast::type_data> infer(std::shared_ptr<type_table> table,std::shared_ptr<ast::typed_data> expr);
	void unify(std::shared_ptr<type_table> table, std::shared_ptr<ast::type_data> t1, std::shared_ptr<ast::type_data> t2);

	std::shared_ptr<ast::type_data> infer(
		context con,
		std::shared_ptr<ast::expression> td
	);
}