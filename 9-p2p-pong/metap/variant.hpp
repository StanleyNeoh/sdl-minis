#ifndef METAP_UNION_HPP
#define METAP_UNION_HPP

#include "headers.hpp"
#include <cstring>
#include <utility>

namespace MetaP {
    template<typename A, typename... Ts>
    union VariantL<TD_List<A, Ts...>> {
        A data;
        std::conditional_t<
            (sizeof...(Ts) > 0),
            VariantL<TD_List<Ts...>>,
            struct {}
        > next;
        
        VariantL() : data() {}
        ~VariantL() {}

        VariantL(const VariantL& other) {
            std::memcpy(this, &other, sizeof(VariantL));
        }

        VariantL(VariantL&& other) noexcept {
            std::memcpy(this, &other, sizeof(VariantL));
        }

        VariantL& operator=(const VariantL& other) {
            if (this != &other) {
                std::memcpy(this, &other, sizeof(VariantL));
            }
            return *this;
        }

        VariantL& operator=(VariantL&& other) noexcept {
            if (this != &other) {
                std::memcpy(this, &other, sizeof(VariantL));
            }
            return *this;
        }

        template <typename T>
        decltype(auto) get() {
            return TO_GetFirst<T, VariantL<TD_List<A, Ts...>>>::f(*this);
        }
    };

    template<typename T, typename A, typename... Ts>
    struct TO_GetFirst<T, VariantL<TD_List<A, Ts...>>> {
        template<typename V>
        static decltype(auto) f(V&& variant) {
            if constexpr (std::is_same_v<T, A>) {
                return std::forward<V>(variant).data;
            } else if constexpr (sizeof...(Ts) > 0) {
                return TO_GetFirst<T, VariantL<TD_List<Ts...>>>::f(std::forward<V>(variant).next);
            } else {
                static_assert(std::is_same_v<T, A>, "Type not found in variant");
            }
        }
    };

    // Base case: empty list
    template<
        template<typename> typename TV_Include,
        template<typename> typename TO_Pred, 
        template<typename> typename TO_Op
    >
    struct TO_FindAndOperateVariant<TV_Include, TO_Pred, TO_Op, VariantL<TD_List<>>> {
        template<typename V, typename K, typename... Args>
        constexpr static auto f(V&&, K&&, Args&&... args) {
            return TO_Op<void>::f(nullptr, std::forward<Args>(args)...);
        }
    };

    // Recursive case
    template<
        template<typename> typename TV_Include,
        template<typename> typename TO_Pred, 
        template<typename> typename TO_Op, 
        typename T, 
        typename... Ts
    >
    struct TO_FindAndOperateVariant<TV_Include, TO_Pred, TO_Op, VariantL<TD_List<T, Ts...>>> {
        template<typename V, typename K, typename... Args>
        constexpr static auto f(V&& member, K&& key, Args&&... args) {
            if constexpr (TV_Include<T>::value) {
                if (TO_Pred<T>::f(std::forward<K>(key))) {
                    return TO_Op<T>::f(&std::forward<V>(member).data, std::forward<Args>(args)...);
                }
            }
            if constexpr (sizeof...(Ts) > 0) {
                return TO_FindAndOperateVariant<TV_Include, TO_Pred, TO_Op, VariantL<TD_List<Ts...>>>::f(
                    std::forward<V>(member).next, 
                    std::forward<K>(key), 
                    std::forward<Args>(args)...
                );
            }
            return TO_Op<void>::f(nullptr, std::forward<Args>(args)...);
        }
    };


}

#endif