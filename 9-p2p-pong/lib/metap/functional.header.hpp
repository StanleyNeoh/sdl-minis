#ifndef METAP_FUNCTIONAL_HEADER_HPP
#define METAP_FUNCTIONAL_HEADER_HPP

#include <type_traits>

namespace MetaP {
    template <
        template<typename> typename TV
    >
    struct TT_TVToTO;

    template <
        template<typename> typename TV
    >
    struct TT_TVIsEquals;
}

#endif