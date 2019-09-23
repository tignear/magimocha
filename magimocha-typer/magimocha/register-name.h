#pragma once
#include "magimocha/ast.h"
#include <string>
#include <unordered_map>
namespace tig::magimocha::name {
template <class T> struct ref_of_T {
    virtual std::shared_ptr<T> get() = 0;
    virtual void set(std::shared_ptr<T> td) = 0;
    virtual operator bool() const = 0;
    virtual ~ref_of_T() {}
};

using variable_ref = ref_of_T<ast::typed_data>;
using module_member_ref = ref_of_T<ast::module_member>;

template <class T> struct table_of_T {
    virtual std::unique_ptr<ref_of_T<T>>
    find_shallow(const ast::string_type &name) = 0;
    virtual std::shared_ptr<T> get_deep(const ast::string_type &name) = 0;
    virtual ~table_of_T() {}
};
using variable_table = table_of_T<ast::typed_data>;
using module_table = table_of_T<ast::declaration_module>;
std::shared_ptr<name::variable_table>
create_variable_table(std::shared_ptr<name::variable_table> upper);
std::shared_ptr<ast::typed_data>
get_with_path(const std::vector<ast::string_type> path,
              std::shared_ptr<module_table> mt,
              std::shared_ptr<variable_table> vt,
              const std::unordered_map<std::shared_ptr<ast::declaration_module>,
                                 std::shared_ptr<module_table>> &dm2mt,
              const std::unordered_map<std::shared_ptr<ast::make_scope>,
                                 std::shared_ptr<variable_table>> &ms2vt);
std::shared_ptr<name::module_table>
create_module_table(std::shared_ptr<name::module_table> upper);
void register_name(std::shared_ptr<ast::declaration_module> target,
                   std::shared_ptr<module_table> target_parent_mt,
                   std::shared_ptr<variable_table> target_parent_vt,
                   std::unordered_map<std::shared_ptr<ast::declaration_module>,
                                      std::shared_ptr<module_table>> &dm2mt,
                   std::unordered_map<std::shared_ptr<ast::make_scope>,
                                      std::shared_ptr<variable_table>> &ms2vt);
void register_name(std::shared_ptr<ast::expression> target,
                   std::shared_ptr<variable_table> target_parent,
                   std::unordered_map<std::shared_ptr<ast::make_scope>,
                                      std::shared_ptr<variable_table>> &ms2vt);
} // namespace tig::magimocha::name