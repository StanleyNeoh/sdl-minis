#ifndef METAP_TYPELIST_HPP
#define METAP_TYPELIST_HPP

#include "typelist.header.hpp"
#include <utility>

namespace MetaP {

    template <> 
    struct TT_First<TD_List<>> {};
    template <typename T, typename... Ts> 
    struct TT_First<TD_List<T, Ts...>> {
        using type = T;
    };

    template <typename T, typename... Ts>
    struct TT_CondPrepend<true, T, TD_List<Ts...>> {
        using type = TD_List<T, Ts...>;
    };
    template <typename T, typename... Ts>
    struct TT_CondPrepend<false, T, TD_List<Ts...>> {
        using type = TD_List<Ts...>;
    };

    template <
        template<typename> typename TV_Pred
    >
    struct TT_Filter<TV_Pred, TD_List<>> {
        using type = TD_List<>;
    };
    template <
        template<typename> typename TV_Pred, 
        typename T, 
        typename... Ts
    >
    struct TT_Filter<TV_Pred, TD_List<T, Ts...>> {
        using type = typename TT_CondPrepend<
            TV_Pred<T>::value,
            T,
            typename TT_Filter<TV_Pred, TD_List<Ts...>>::type
        >::type;
    };

    template <typename A>
    struct TV_Contains<A, TD_List<>> {
        constexpr static bool value = false;
    };
    template <typename A, typename T, typename... Ts>
    struct TV_Contains<A, TD_List<T, Ts...>> {
        constexpr static bool value = std::is_same_v<A, T> || TV_Contains<A, TD_List<Ts...>>::value;
    };

    template <
        template<typename> typename TO_Pred,
        template<typename> typename TO_Op,
        typename T,
        typename... Ts
    >
    struct TO_Dispatch<TO_Pred, TO_Op, TD_List<T, Ts...>> {
        template<typename K, typename... Args>
        constexpr static decltype(auto) f(K&& key, Args&&... args) {
            if (TO_Pred<T>::f(std::forward<K>(key))) {
                return TO_Op<T>::f(std::forward<Args>(args)...);
            }
            if constexpr (sizeof...(Ts) > 0) {
                return TO_Dispatch<TO_Pred, TO_Op, TD_List<Ts...>>::f(std::forward<K>(key), std::forward<Args>(args)...);
            }
            return TO_Op<void>::f(std::forward<Args>(args)...);
        }
    };
};

#endif