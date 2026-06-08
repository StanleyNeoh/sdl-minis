#ifndef METAP_FUNCTIONS_HEADER_HPP
#define METAP_FUNCTIONS_HEADER_HPP

namespace MetaP {
    template <typename T>
    struct TT_Args;

    template <typename T>
    struct TT_FirstArg;

    template <typename... Fs>
    struct Callbacks;

    // Tell compiler how to deduce class templates (CTAD)
    template <typename... Fs>
    Callbacks(Fs&&...) -> Callbacks<Fs...>;
}

#endif
