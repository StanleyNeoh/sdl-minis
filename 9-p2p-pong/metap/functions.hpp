#ifndef METAP_FUNCTIONS_HPP
#define METAP_FUNCTIONS_HPP

#include "typelist.header.hpp"
#include "functions.header.hpp"
#include "variant.header.hpp"

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

    template <typename F, typename... Fs>
    struct Callbacks<F, Fs...> {
        std::tuple<F, Fs...> funcs;
        Callbacks(F&& f, Fs&&... fs): funcs(std::forward<F>(f), std::forward<Fs>(fs)...) {}

        template <typename... Ts>
        decltype(auto) call(Ts&&... args) const {
            return _call<0>(funcs, std::forward<Ts>(args)...);
        }

        template <
            template<typename> typename TO_Pred,
            template<typename> typename TT_FirstArgCast,
            typename T,
            typename U,
            typename... Us
        >
        bool dispatch(T&& key, U&& arg, Us&&... args) const {
            return _dispatch<0, TO_Pred, TT_FirstArgCast>(funcs, std::forward<T>(key), std::forward<U>(arg), std::forward<Us>(args)...);
        }

    private:
        template<int I, typename CT, typename T, typename... Ts>
        static decltype(auto) _call(CT&& funcs, T&& first, Ts&&... rest) {
            if constexpr (I < std::tuple_size_v<std::decay_t<CT>>) {
                using FuncType = std::decay_t<std::tuple_element_t<I, std::decay_t<CT>>>;
                using FirstArgType = std::decay_t<typename TT_FirstArg<FuncType>::type>;
                if constexpr (std::is_same_v<std::decay_t<T>, FirstArgType>) {
                    return std::get<I>(std::forward<CT>(funcs))(std::forward<T>(first), std::forward<Ts>(rest)...);
                } else {
                    return _call<I+1>(std::forward<CT>(funcs), std::forward<T>(first), std::forward<Ts>(rest)...);
                }
            }
        }

        template <
            int I,
            template<typename> typename TO_Pred,
            template<typename> typename TT_FirstArgCast,
            typename CT,
            typename T,
            typename U,
            typename... Us
        >
        static bool _dispatch(CT&& funcs, T&& key, U&& arg, Us&&... args) {
            if constexpr (I < std::tuple_size_v<std::decay_t<CT>>) {
                using FuncType = std::decay_t<std::tuple_element_t<I, std::decay_t<CT>>>;
                using FirstArgType = std::decay_t<typename TT_FirstArg<FuncType>::type>;
                if (TO_Pred<FirstArgType>::f(std::forward<T>(key))) {
                    // Delegate to call() which will dispatch based on type
                    _call<I>(std::forward<CT>(funcs), TT_FirstArgCast<FirstArgType>::f(arg), std::forward<Us>(args)...);
                    return true;
                }
                return _dispatch<I+1, TO_Pred, TT_FirstArgCast>(std::forward<CT>(funcs), std::forward<T>(key), std::forward<U>(arg), std::forward<Us>(args)...);
            }
            return false;
        }
    };

    // Deduction guide for Callbacks
    template <typename... Fs>
    Callbacks(Fs&&...) -> Callbacks<Fs...>;
}

#endif
