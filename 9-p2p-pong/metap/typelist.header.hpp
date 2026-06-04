#ifndef METAP_TYPELIST_HEADER_HPP
#define METAP_TYPELIST_HEADER_HPP

#include <utility>

namespace MetaP {
    template <typename... T>
    struct TD_List;

    template <typename _List>
    struct TT_First;

    template <bool include, typename T, typename _List>
    struct TT_CondPrepend;

    template <
        template<typename> typename TV_Pred, 
        typename _List
    >
    struct TT_Filter;

    template <typename T, typename _List>
    struct TV_Contains;

    template <
        template<typename> typename TO_Pred,
        template<typename> typename TO_Op,
        typename _List
    >
    struct TO_DispatchList;
};

#endif