#include "register-name.h"
namespace tig::magimocha::name{
std::shared_ptr<name::variable_table>
create_variable_table(std::shared_ptr<name::variable_table> upper) {
    using map_t =
        std::unordered_map<ast::string_type, std::shared_ptr<ast::typed_data>>;
    struct variable_table_impl : name::variable_table {

        struct variable_ref_impl : name::variable_ref {
            const ast::string_type &k;
            map_t::const_iterator itr;
            map_t &map;
            variable_ref_impl(const ast::string_type &k,
                              map_t::const_iterator itr, map_t &map)
                : k(k), itr(itr), map(map) {}
            std::shared_ptr<ast::typed_data> get() override {
                return itr->second;
            }
            void set(std::shared_ptr<ast::typed_data> td) override {
                map.insert_or_assign(k, td);
            }
            operator bool() const override { return itr != map.cend(); }
        };
        std::shared_ptr<variable_table> upper;
        map_t map;
        variable_table_impl(std::shared_ptr<variable_table> upper)
            : upper(upper) {}
        std::unique_ptr<name::variable_ref>
        find_shallow(const ast::string_type &name) override {
            return std::make_unique<variable_ref_impl>(name, map.find(name),
                                                       map);
        }
        std::shared_ptr<ast::typed_data>
        get_deep(const ast::string_type &name) override {
            auto itr = map.find(name);
            if(itr != map.cend()) {
                return itr->second;
            }
            if(!upper) {
                return std::shared_ptr<ast::typed_data>();
            }
            return upper->get_deep(name);
        }
    };
    return std::make_shared<variable_table_impl>(upper);
}
}