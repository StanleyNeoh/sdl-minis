#ifndef METAP_VARIANT_HEADER_HPP
#define METAP_VARIANT_HEADER_HPP

namespace MetaP {
    template<typename _List>
    union VariantL;

    template<typename... Ts>
    using Variant = VariantL<TD_List<Ts...>>;
    template <typename T>
    struct TT_VariantCast {
        template <typename V>
        static decltype(auto) f(V&& variant) {
            return std::forward<V>(variant).template get<T>();
        }
    };

    template<
        template<typename> typename TV_Include,
        template<typename> typename TO_Pred, 
        template<typename> typename TOOp, 
        typename _Variant
    >
    struct TO_DispatchVariant;
}

#endif