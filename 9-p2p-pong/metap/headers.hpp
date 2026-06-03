#ifndef METAP_HEADERS_HPP
#define METAP_HEADERS_HPP

#include <type_traits>

namespace MetaP {
    template <typename T>
    struct TV_True {
        constexpr static bool value = true;
    };

    template <typename... T>
    struct TD_List;

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
        template<typename> typename TO_OP,
        typename _List
    >
    struct TO_FindAndOperateList;

    template<typename _List>
    union VariantL;

    template<typename... Ts>
    using Variant = VariantL<TD_List<Ts...>>;

    template<typename T, typename _Iter>
    struct TO_GetFirst;

    template<
        template<typename> typename TV_Include,
        template<typename> typename TO_Pred, 
        template<typename> typename TOOp, 
        typename _Variant
    >
    struct TO_FindAndOperateVariant;

}

#endif