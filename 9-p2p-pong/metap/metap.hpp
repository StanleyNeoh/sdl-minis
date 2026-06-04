#ifndef METAP_HEADERS_HPP
#define METAP_HEADERS_HPP

#include <type_traits>

namespace MetaP {
    template <typename T>
    struct TV_True {
        constexpr static bool value = true;
    };

    template <typename T>
    struct TT_Identity {
        template <typename U>
        static decltype(auto) f(U&& arg) {
            return std::forward<U>(arg);
        }
    };
}

#include "typelist.hpp"
#include "variant.hpp"
#include "functional.hpp"
#include "functions.hpp"

#endif