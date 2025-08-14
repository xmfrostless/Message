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
#include <cstdint>
#include <algorithm>

// debug config
#ifndef NDEBUG
#include <cassert>
#include <iostream>

#define MESSAGE_WARNING(__INFO__, __DETAIL__) do {\
    std::cerr << "\033[33m" << "[Message] Warn: " << __FUNCTION__ << " / " << __INFO__ << " / " << __DETAIL__ << "\033[0m" << std::endl;\
} while(false)

#define MESSAGE_ERROR(__INFO__, __DETAIL__) do {\
    std::cerr << "\033[35m" << "[Message] Error: " <<  __FUNCTION__ << " / " << __INFO__ << " / " << __DETAIL__ << "\033[0m" << std::endl;\
} while(false)

#define MESSAGE_ASSERT(__INFO__, __DETAIL__) do {\
    MESSAGE_ERROR(__INFO__, __DETAIL__);\
    assert(false);\
} while(false)

#define MESSAGE_INVOKE_PUSH(__STACK__, __CUR__) do {\
    if (__STACK__.size() > 200u) {\
        MESSAGE_ERROR(__CUR__.name(), "The number of message recursion exceeds the upper limit");\
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
#define MESSAGE_ERROR(__INFO__, __DETAIL__) (void(0))
#define MESSAGE_ASSERT(__INFO__, __DETAIL__) (void(0))
#define MESSAGE_INVOKE_PUSH(__STACK__, __CUR__) (void(0))
#define MESSAGE_INVOKE_POP(__STACK__) (void(0))
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
            MESSAGE_WARNING(Type<_Ty>::TYPE.name(), "The binder is null!");
            return;
        }
        if (!func) {
            MESSAGE_WARNING(Type<_Ty>::TYPE.name(), "Func is null!");
            return;
        }
        auto message_code { Type<_Ty>::TYPE_CODE };
        auto binder_key { reinterpret_cast<std::intptr_t>(binder) };
        auto& vec { _listener_map[message_code] };
        auto removeIndexesIte = _remove_indexes.find(message_code);
        for (auto i { 0u }; i < vec.size(); ++i) {
            if (vec[i]->binder_key == binder_key) {
                if (removeIndexesIte == _remove_indexes.end() ||
                    removeIndexesIte->second.find(i) == removeIndexesIte->second.end()) {
                    MESSAGE_WARNING(Type<_Ty>::TYPE.name(), "The binder is exist!");
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
            MESSAGE_WARNING(Type<_Ty>::TYPE.name(), "The binder is null!");
            return;
        }
        auto message_code { Type<_Ty>::TYPE_CODE };
        auto& vec { _listener_map[message_code] };
        if (vec.empty()) {
            MESSAGE_WARNING(Type<_Ty>::TYPE.name(), "The listener list is empty!");
            return;
        }
        auto binder_key { reinterpret_cast<std::intptr_t>(binder) };
        if (_invoke_level == 0) {
            auto it = std::find_if(vec.begin(), vec.end(), [&binder_key](auto& item) {
                return item->binder_key == binder_key;
            });
            if (it != vec.end()) {
                vec.erase(it);
                return;
            }
        } else {
            auto& remove_indexes { _remove_indexes[message_code] };
            for (auto i { 0u }; i < vec.size(); ++i) {
                if (vec[i]->binder_key == binder_key) {
                    if (remove_indexes.find(i) == remove_indexes.end()) {
                        remove_indexes.emplace(i);
                        _should_remove = true;
                        return;
                    }
                }
            }
        }
        MESSAGE_WARNING(Type<_Ty>::TYPE.name(), "The binder not found!");
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
        auto& remove_indexes { _remove_indexes[message_code] };
        for (auto i { 0u }; i < size; ++i) {
            if (remove_indexes.find(i) != remove_indexes.end()) {
                continue;
            }
            static_cast<Listener<_Ty>*>(listeners[i].get())->call(message);
        }
        --_invoke_level;
        MESSAGE_INVOKE_POP(_invoke_stack);

        if (_invoke_level == 0 && _should_remove) {
            for (auto& item : _remove_indexes) {
                if (item.second.empty()) {
                    continue;
                }
                auto& vec { _listener_map[item.first] };
                if (!vec.empty()) {
                    auto tail { vec.size() };
                    for (auto index : item.second) {
                        std::swap(vec[index], vec[--tail]);
                    }
                    vec.resize(tail);
                }
                item.second.clear();
            }
            _should_remove = false;
        }
    }

private:
    std::unordered_map<std::size_t, std::vector<std::unique_ptr<ListenerBase>>> _listener_map;
    std::unordered_map<std::size_t, std::unordered_set<std::size_t>> _remove_indexes;
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

#pragma warning(pop)