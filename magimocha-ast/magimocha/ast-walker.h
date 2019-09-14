#include "ast-visitor.h"

namespace tig::magimocha::ast
{
template <class R, class Wal>
struct walk_visitor_impl
{
    Wal w;
    walk_visitor_impl(Wal &wal) : w(wal)
    {
    }
    template <class T>
    R operator()(T e)
    {
        if constexpr (std::is_base_of_v<ast::make_scope, T>)
        {
            w.scope_in(e);
            auto &&r = w(e);
            w.scope_out(e);
            return r;
        }
        return w(e);
    }
};

template <class R, class Wal, class E>
R walk(Wal &wal, E e)
{
    auto vis = walk_visitor_impl(wal);
    return ast::visit<R>(vis, e);
}
} // namespace tig::magimocha::ast