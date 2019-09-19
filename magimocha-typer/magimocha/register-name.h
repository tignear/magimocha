#pragma once
#include "magimocha/ast.h"
#include <string>
#include <unordered_map>
namespace tig::magimocha::name {
struct variable_ref {
    virtual std::shared_ptr<ast::typed_data> get() = 0;
    virtual void set(std::shared_ptr<ast::typed_data> td) = 0;
    virtual operator bool() const = 0;
    virtual ~variable_ref() {}
};
struct variable_table {
    virtual std::unique_ptr<variable_ref>
    find_shallow(const ast::string_type &name) = 0;
    virtual std::shared_ptr<ast::typed_data>
    get_deep(const ast::string_type &name) = 0;
    virtual ~variable_table() {}
};
std::shared_ptr<name::variable_table>
create_variable_table(std::shared_ptr<name::variable_table> upper);

} // namespace tig::magimocha::name