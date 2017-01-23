#pragma once

#include <exception>
#include <functional>
#include <memory>
#include <sstream>
#include <unordered_map>

namespace gg {
template<typename K>
concept bool hashable = requires(K k) {
    { std::hash<K>{}(k) };
};

template<typename K>
concept bool equalitycomparable = requires(K k, K l) {
    { k == l } -> bool;
};

template<typename K>
concept bool keytype = equalitycomparable<K> && hashable<K>;

struct bad_name_add : public std::exception {
private:
    std::string msg;

public:
    bad_name_add(const std::string& name) {
        std::stringstream ss;
        ss << "attempted to add a name that was already in scope: " << name;
        msg = ss.str();
    }

    virtual const char* what() const noexcept {
        return msg.c_str();
    }
};

struct bad_name_lookup : public std::exception {
private:
    std::string msg;

public:
    bad_name_lookup(const std::string& name) {
        std::stringstream ss;
        ss << "attempted to lookup a name that was not in scope: " << name;
        msg = ss.str();
    }

    virtual const char* what() const noexcept {
        return msg.c_str();
    }
};

template<keytype K, typename V>
struct scope : public std::enable_shared_from_this<scope<K, V>> {
private:
    using map_type = std::unordered_map<K, V>;
    map_type globals;
    std::vector<map_type> locals;

public:
    using key_type = K;
    using mapped_type = V;
    using value_type = std::pair<K, V>;
    using size_type = std::size_t;
    using hasher = std::hash<K>;
    using allocator_type = std::allocator<std::pair<const K, V>>;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = typename std::allocator_traits<allocator_type>::pointer;
    using const_pointer = typename std::allocator_traits<allocator_type>::const_pointer;

    scope(const map_type& globals) : globals(globals) {}
    scope(map_type&& globals) : globals(std::move(globals)) {}

    void new_local(const key_type& key, const mapped_type& value) {
        auto& current_locals = locals.back();

        auto e = current_locals.find(key);
        if (e == current_locals.cend()) {
            throw bad_name_add(key);
        }
        current_locals.emplace(key, value);

    }

    const mapped_type& lookup(const key_type& key) {
        auto from_globals = globals->find(key);
        if (from_globals != globals->cend()) {
            return from_globals->second;
        }
        auto from_locals = locals.back().find(key);
        if (from_locals != locals->cend()) {
            return from_locals->second;
        }
        throw bad_name_lookup(key);
    }

    void push() {
        locals.emplace_back();
    }

    void pop() {
        if (!locals.size()) {
            throw std::out_of_range{};
        }
        locals.pop_back();
    };
};
}
