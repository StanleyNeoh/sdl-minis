#ifndef METAP_VARIANT_HEADER_HPP
#define METAP_VARIANT_HEADER_HPP

namespace MetaP {
    template<typename _List>
    union Variant;

    template<typename A, typename _Iter>
    struct TO_VariantGet;

    template <typename T>
    struct TO_VariantCast;

    template<
        template<typename> typename TV_Include,
        template<typename> typename TO_Pred, 
        template<typename> typename TOOp, 
        typename _Variant
    >
    struct TO_VariantDispatch;
}

#endif