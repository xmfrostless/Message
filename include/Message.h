/*
    https://github.com/xmfrostless/Message
*/

#pragma once

#include <typeindex>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>

// debug config
#ifndef NDEBUG
#include <cassert>
#include <iostream>

#define MESSAGE_ASSERT(__INFO__, __MSG__) do {\
    std::cerr << "[Message Error]: " << __FUNCTION__ << " / " << __INFO__ << " / " << __MSG__ << std::endl;\
    assert(false);\
} while(false)

#define MESSAGE_INVOKE_ASSERT(__STACK__, __CUR__) do {\
    if (__STACK__.size() > 200u) {\
        std::cerr << "[Message Error]: " << "Invoke stack:" << std::endl;\
        for (auto& item : __STACK__) {\
            std::cerr << item.name() << " -> ";\
        }\
        std::cerr << "(*)" << __CUR__.name() << std::endl;\
        MESSAGE_ASSERT("The number of message recursion exceeds the upper limit", "");\
    }\
} while(false)

#else
#define MESSAGE_ASSERT(__INFO__, __MSG__) (void(0))
#define MESSAGE_INVOKE_ASSERT(__STACK__) (void(0))
#endif

#pragma warning(push)
#pragma warning(disable: 4514)

namespace Message {

class Dispatcher {
private:
    template <typename T>
    struct Type final {
        static const std::type_index TYPE;
        static const std::size_t TYPE_CODE;
    };

    struct ListenerBase {
        virtual ~ListenerBase() = default;
        std::intptr_t binder_key { 0 };
    };

    template<typename _Ty>
    struct Listener final: public ListenerBase {
        Listener(std::intptr_t key, std::function<void(const _Ty&)> func): call(func) {
            binder_key = key;
        }
        std::function<void(const _Ty&)> call;
    };

public:
    //
    template<typename _Ty>
    void AddListener(const void* binder, std::function<void(const _Ty&)> func) {
        if (!binder) {
            MESSAGE_ASSERT("The binder is null!", Type<_Ty>::TYPE.name());
            return;
        }
        if (!func) {
            MESSAGE_ASSERT("Func is null!", Type<_Ty>::TYPE.name());
            return;
        }
        auto message_code { Type<_Ty>::TYPE_CODE };
        auto binder_key { reinterpret_cast<std::intptr_t>(binder) };
        auto& vec { _listener_map[message_code] };
        for (auto i { 0u }; i < vec.size(); ++i) {
            if (vec[i]->binder_key == binder_key) {
                if (_remove_indexes.find(i) == _remove_indexes.end()) {
                    MESSAGE_ASSERT("The binder is exist!", Type<_Ty>::TYPE.name());
                    return;
                }
            }
        }
        vec.emplace_back(std::make_unique<Listener<_Ty>>(binder_key, func));
    }

    //
    template<typename _Ty>
    void RemoveListener(const void* binder) {
        if (!binder) {
            MESSAGE_ASSERT("The binder is null!", Type<_Ty>::TYPE.name());
            return;
        }
        auto message_code { Type<_Ty>::TYPE_CODE };
        auto binder_key { reinterpret_cast<std::intptr_t>(binder) };
        auto& vec { _listener_map[message_code] };
        for (auto i { 0u }; i < vec.size(); ++i) {
            if (vec[i]->binder_key == binder_key) {
                if (_remove_indexes.find(i) == _remove_indexes.end()) {
                    _remove_indexes.emplace(i);
                    return;
                }
            }
        }
    }

    //
    template <typename _Ty>
    void Send(const _Ty& message) {
        std::size_t message_code { Type<_Ty>::TYPE_CODE };
        auto& vec { _listener_map[message_code] };
        if (vec.empty()) {
            return;
        }

        MESSAGE_INVOKE_ASSERT(_invoke_stack, Type<_Ty>::TYPE);
        _invoke_stack.push_back(Type<_Ty>::TYPE);
        const auto size { vec.size() };
        for (auto i { 0u }; i < size; ++i) {
            if (_remove_indexes.find(i) != _remove_indexes.end()) {
                continue;
            }
            static_cast<Listener<_Ty>*>(vec[i].get())->call(message);
        }
        _invoke_stack.pop_back();

        if (_invoke_stack.empty()) {
            if (!_remove_indexes.empty()) {
                auto tail { vec.size() };
                for (auto index : _remove_indexes) {
                    std::swap(vec[index], vec[--tail]);
                }
                vec.resize(tail);
                _remove_indexes.clear();
            }
        }
    }

private:
    std::unordered_map<std::size_t, std::vector<std::unique_ptr<ListenerBase>>> _listener_map;
    std::unordered_set<std::size_t> _remove_indexes;
    std::vector<std::type_index> _invoke_stack;
};

template <typename T>
const std::type_index Dispatcher::Type<T>::TYPE { typeid(T) };

template <typename T>
const std::size_t Dispatcher::Type<T>::TYPE_CODE { Type<T>::TYPE.hash_code() };
}

#pragma warning(pop)