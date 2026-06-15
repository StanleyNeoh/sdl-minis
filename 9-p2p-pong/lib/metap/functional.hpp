#ifndef METAP_FUNCTIONAL_HPP
#define METAP_FUNCTIONAL_HPP

#include "functional.header.hpp"
namespace MetaP {
    template <
        template<typename> typename TV
    >
    struct TT_TVToTO {
        template<typename T>
        struct type {
            constexpr static auto f(auto&&... args) {
                return TV<T>::value;
            }
        };
    };

    template <
        template<typename> typename TV
    >
    struct TT_TVIsEquals {
        template<typename T>
        struct type {
            constexpr static bool f(auto&& key) {
                if constexpr (std::is_void_v<T>) {
                    return false;
                }
                return TV<T>::value == key;
            }
        };
    };
}

#endif