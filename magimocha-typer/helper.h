#pragma once
#include <functional>
namespace tig::helper {
template <class T> inline void hash_combine(size_t &seed, const T &v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
} // namespace tig::helper
namespace std {
template <typename T> struct hash<vector<T>> {
    inline size_t operator()(const vector<T> &v) const noexcept {
        size_t seed = v.size();

        for(auto &e : v) {
            tig::helper::hash_combine(seed, e);
        }

        return seed;
    }
};
} // namespace std