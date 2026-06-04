#ifndef METAP_FUNCTIONS_HPP
#define METAP_FUNCTIONS_HPP

#include "functions.header.hpp"

namespace MetaP {
    template <typename R, typename Class, typename T, typename... Ts>
    struct TT_FirstArg<R(Class::*)(T, Ts...) const> {
        using type = T;
    };
    template <typename R, typename Class, typename T, typename... Ts>
    struct TT_FirstArg<R(Class::*)(T, Ts...)> {
        using type = T;
    };
}

#endif
