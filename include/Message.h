/*
    https://github.com/xmfrostless/Message
*/

#pragma once

#include <typeindex>
#include <functional>
#include <unordered_map>
#include <set>
#include <vector>
#include <memory>
#include <cstdint>
#include <algorithm>

// debug config
#ifndef NDEBUG
#include <cassert>
#include <iostream>

#define MESSAGE_WARNING(__INFO__, __DETAIL__) do {\
    std::cerr << "\n\033[33m" << "[Message] Warn: " << __FUNCTION__ << " / " << __INFO__ << " / " << __DETAIL__ << "\033[0m\n" << std::endl;\
} while(false)

#define MESSAGE_INVOKE_PUSH(__STACK__, __CUR__) do {\
    if (__STACK__.size() > 200u) {\
        MESSAGE_WARNING(__CUR__.name(), "The recursive number of messages exceeds 200");\
        std::cerr << "\033[35m" << "Invoke stack:" << std::endl;\
        for (auto& item : __STACK__) {\
            std::cerr << "\033[35m" << item.name() << " -> ";\
        }\
        std::cerr << "\033[35m" << "(*)" << __CUR__.name() << "\033[0m" << std::endl;\
        assert(false);\
    }\
    __STACK__.push_back(__CUR__);\
} while(false)

#define MESSAGE_INVOKE_POP(__STACK__) do {\
    __STACK__.pop_back();\
} while(false)

#else
#define MESSAGE_WARNING(__INFO__, __DETAIL__) (void(0))
#define MESSAGE_INVOKE_PUSH(__STACK__, __CUR__) (void(0))
#define MESSAGE_INVOKE_POP(__STACK__) (void(0))
#endif

namespace Message {

class Dispatcher {
public:
    //
    template<typename _Ty>
    void AddListener(const void* binder, std::function<void(const _Ty&)> func) {
        if (!binder) {
            MESSAGE_WARNING(Type<_Ty>::TYPE.name(), "The binder is null!");
            return;
        }
        if (!func) {
            MESSAGE_WARNING(Type<_Ty>::TYPE.name(), "Func is null!");
            return;
        }
        auto message_code { Type<_Ty>::TYPE_CODE };
        auto binder_key { reinterpret_cast<std::intptr_t>(binder) };
        auto& listeners { _listener_map[message_code] };
        auto& remove_indexes { _remove_indexes[message_code] };
        for (auto i { 0u }; i < listeners.size(); ++i) {
            if (listeners[i]->binder_key == binder_key) {
                if (remove_indexes.find(i) == remove_indexes.end()) {
                    MESSAGE_WARNING(Type<_Ty>::TYPE.name(), "The binder is exist!");
                    return;
                }
            }
        }
        listeners.emplace_back(std::make_unique<Listener<_Ty>>(binder_key, func));
    }

    //
    template<typename _Ty>
    void RemoveListener(const void* binder) {
        if (!binder) {
            MESSAGE_WARNING(Type<_Ty>::TYPE.name(), "The binder is null!");
            return;
        }
        if (!_RemoveListener(Type<_Ty>::TYPE_CODE, reinterpret_cast<std::intptr_t>(binder))) {
            MESSAGE_WARNING(Type<_Ty>::TYPE.name(), "The binder not found!");
        }
    }

    //
    void RemoveAllListeners(const void* binder) {
        if (!binder) {
            MESSAGE_WARNING("The binder is null!", "");
            return;
        }
        auto binder_key { reinterpret_cast<std::intptr_t>(binder) };
        bool success { false };
        for (auto& item : _listener_map) {
            success |= _RemoveListener(item.first, binder_key);
        }
        if (!success) {
            MESSAGE_WARNING("The binder not found!", "");
        }
    }

    //
    template <typename _Ty>
    void Send(const _Ty& message) {
        std::size_t message_code { Type<_Ty>::TYPE_CODE };
        auto& listeners { _listener_map[message_code] };
        if (listeners.empty()) {
            return;
        }

        MESSAGE_INVOKE_PUSH(_invoke_stack, Type<_Ty>::TYPE);
        ++_invoke_level;
        const auto size { listeners.size() };
        auto& indexes { _remove_indexes[message_code] };
        for (auto i { 0u }; i < size; ++i) {
            if (indexes.find(i) == indexes.end()) {
                static_cast<Listener<_Ty>*>(listeners[i].get())->call(message);
            }
        }
        --_invoke_level;
        MESSAGE_INVOKE_POP(_invoke_stack);

        if (_invoke_level == 0 && _should_remove) {
            for (auto& [rm_code, rm_indexes] : _remove_indexes) {
                if (rm_indexes.empty()) {
                    continue;
                }
                auto& rm_listeners { _listener_map[rm_code] };
                if (!rm_listeners.empty()) {
                    auto tail { rm_listeners.size() };
                    for (auto index : rm_indexes) {
                        std::swap(rm_listeners[index], rm_listeners[--tail]);
                    }
                    rm_listeners.resize(tail);
                }
                rm_indexes.clear();
            }
            _should_remove = false;
        }
    }

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

    bool _RemoveListener(std::size_t message_code, std::intptr_t binder_key) {
        auto& listeners { _listener_map[message_code] };
        if (listeners.empty()) {
            return false;
        }
        if (_invoke_level == 0) {
            auto it { std::remove_if(listeners.begin(), listeners.end(), [&binder_key](auto& item) {
                return item->binder_key == binder_key;
            }) };
            if (it != listeners.end()) {
                listeners.resize(std::distance(listeners.begin(), it));
                return true;
            }
        } else {
            auto& indexes { _remove_indexes[message_code] };
            for (auto i { 0u }; i < listeners.size(); ++i) {
                if (listeners[i]->binder_key == binder_key) {
                    if (indexes.find(i) == indexes.end()) {
                        indexes.emplace(i);
                        _should_remove = true;
                        return true;
                    }
                }
            }
        }
        return false;
    }

private:
    std::unordered_map<std::size_t, std::vector<std::unique_ptr<ListenerBase>>> _listener_map;
    std::unordered_map<std::size_t, std::set<std::size_t, std::greater<std::size_t>>> _remove_indexes;
#ifndef NDEBUG
    std::vector<std::type_index> _invoke_stack;
#endif
    int _invoke_level { 0 };
    bool _should_remove { false };
};

template <typename T>
const std::type_index Dispatcher::Type<T>::TYPE { typeid(T) };

template <typename T>
const std::size_t Dispatcher::Type<T>::TYPE_CODE { Type<T>::TYPE.hash_code() };
}
