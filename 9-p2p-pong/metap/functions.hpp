#ifndef METAP_FUNCTIONS_HPP
#define METAP_FUNCTIONS_HPP

#include "typelist.header.hpp"
#include "functions.header.hpp"

namespace MetaP {
    template <typename R, typename Class, typename... Ts>
    struct TT_Args<R(Class::*)(Ts...) const> {
        using type = TD_List<Ts...>;
    };
    template <typename R, typename Class, typename... Ts>
    struct TT_Args<R(Class::*)(Ts...)> {
        using type = TD_List<Ts...>;
    };
    template<typename R, typename... Ts>
    struct TT_Args<R(*)(Ts...)> {
        using type = TD_List<Ts...>;
    };

    template <typename F>
    struct TT_Args {
        using type = typename TT_Args<decltype(&F::operator())>::type;
    };

    template <typename F>
    struct TT_FirstArg {
        using type = typename TT_First<typename TT_Args<F>::type>::type;
    };
}

#endif
