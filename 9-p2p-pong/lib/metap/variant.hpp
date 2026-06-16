#ifndef METAP_VARIANT_HPP
#define METAP_VARIANT_HPP

#include "variant.header.hpp"
#include <cstring>
#include <utility>

namespace MetaP {
    struct EmptyStruct {};
    template<typename T, typename... Ts>
    union Variant<TD_List<T, Ts...>> {
        T data;
        std::conditional_t<
            (sizeof...(Ts) > 0),
            Variant<TD_List<Ts...>>,
            EmptyStruct
        > next;
        
        Variant() : data() {}
        ~Variant() {}

        Variant(const Variant& other) {
            std::memcpy(reinterpret_cast<void*>(this), &other, sizeof(Variant));
        }

        Variant(Variant&& other) noexcept {
            std::memcpy(reinterpret_cast<void*>(this), &other, sizeof(Variant));
        }

        Variant& operator=(const Variant& other) {
            if (this != &other) {
                std::memcpy(this, &other, sizeof(Variant));
            }
            return *this;
        }

        Variant& operator=(Variant&& other) noexcept {
            if (this != &other) {
                std::memcpy(reinterpret_cast<void*>(this), &other, sizeof(Variant));
            }
            return *this;
        }

        template <typename A>
        decltype(auto) get() {
            return TO_VariantGet<A, Variant<TD_List<T, Ts...>>>::f(*this);
        }
    }; 

    template<typename A, typename U, typename... Us>
    struct TO_VariantGet<A, Variant<TD_List<U, Us...>>> {
        template<typename V>
        static decltype(auto) f(V&& variant) {
            if constexpr (std::is_same_v<A, U>) {
                return std::forward<V>(variant).data;
            } else if constexpr (sizeof...(Us) > 0) {
                return TO_VariantGet<A, Variant<TD_List<Us...>>>::f(std::forward<V>(variant).next);
            } else {
                static_assert(std::is_same_v<A, U>, "Type not found in variant");
            }
        }
    };

    template <typename T>
    struct TO_VariantCast {
        template <typename V>
        static decltype(auto) f(V&& variant) {
            return std::forward<V>(variant).template get<T>();
        }
    };

    template<
        template<typename> typename TV_Include,
        template<typename> typename TO_Pred, 
        template<typename> typename TO_Op, 
        typename T, 
        typename... Ts
    >
    struct TO_VariantDispatch<TV_Include, TO_Pred, TO_Op, Variant<TD_List<T, Ts...>>> {
        template<typename V, typename K, typename... Args>
        constexpr static decltype(auto) f(V&& member, K&& key, Args&&... args) {
            if constexpr (TV_Include<T>::value) {
                if (TO_Pred<T>::f(std::forward<K>(key))) {
                    return TO_Op<T>::f(&std::forward<V>(member).data, std::forward<Args>(args)...);
                }
            }
            if constexpr (sizeof...(Ts) > 0) {
                return TO_VariantDispatch<TV_Include, TO_Pred, TO_Op, Variant<TD_List<Ts...>>>::f(
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