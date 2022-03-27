#pragma once

#include <iostream>
#include <utility>
#include <type_traits>
#include <tuple>

#define CONSTEXPR_STRING(x)                                                    \
    sw_constexpr::string_builder([](std::size_t i) constexpr { return x[i]; }, \
                 std::make_index_sequence<sizeof(x)>{})

namespace sw_constexpr {

    template <char... c>
    class String
    {
    private:
        template <std::size_t ... Is>
        static constexpr auto indexSequenceReverse(std::index_sequence<Is...> const&) -> decltype(std::index_sequence<sizeof...(Is) - 1U - Is...>{});

        template <std::size_t N>
        using makeIndexSequenceReverse = decltype(indexSequenceReverse(std::make_index_sequence<N>{}));

    public:
        template<char... Values>
        using tupleValue = std::tuple<std::integral_constant<decltype(Values), Values>...>;

        constexpr operator const char* () const { return buffer; }
        static constexpr const char* data() { return buffer; }
        static constexpr size_t length() { return sizeof...(c) - 1; }
        static constexpr size_t capacity() { return sizeof...(c); }
        static constexpr bool empty() { return length() == 0; }

        template<size_t pos, int len = -1>
        static constexpr auto substr() {
            if constexpr (pos >= length()) {
                return String<'\0'>{};
            }
            else {
                constexpr auto correct_len = len < 0 ? length() - pos : (length() - pos >= len ? len : length() - pos);
                return substr_helper<pos, correct_len>();
            }
        }

        template<class F, int pos = 0>
        static constexpr auto find(F f) {
            if constexpr (pos < 0 || pos >= length()) {
                return -1;
            }
            else {
                constexpr auto lam = [f](size_t i) constexpr {return f(buffer[pos + i]); };
                constexpr auto result = find_helper(lam, std::make_index_sequence<length() - pos>{});
                return result >= 0 ? pos + result : -1;
            }
        }

        template<class F, int pos = 0>
        static constexpr auto find_last(F f) {
            if constexpr (pos < 0 || pos >= length()) {
                return -1;
            }
            else {
                constexpr auto lam = [f](size_t i) constexpr {return f(buffer[pos + i]); };
                constexpr auto result = find_helper(lam, makeIndexSequenceReverse<length() - pos>{});
                return result >= 0 ? pos + result : -1;
            }
        }

        template<class F, int pos = length() - 1>
        static constexpr auto rfind(F f) {
            if constexpr (pos <= 0 || pos >= length()) {
                return -1;
            }
            else {
                constexpr auto lam = [f](size_t i) constexpr {return f(buffer[i]); };
                constexpr auto result = find_helper(lam, makeIndexSequenceReverse<pos + 1>{});
                return result >= 0 ? result : -1;
            }
        }

        template<char sym, int pos = 0>
        static constexpr auto find() {
            constexpr auto lam = [](char val) constexpr {return val == sym; };
            return find<decltype(lam), pos>(lam);
        }

        template<char sym, int pos = 0>
        static constexpr int find_last() {
            constexpr auto lam = [](char val) constexpr {return val == sym; };
            return find_last<decltype(lam), pos>(lam);
        }

        template<char sym, int pos = length() - 1>
        static constexpr int rfind() {
            constexpr auto lam = [](char val) constexpr {return val == sym; };
            return rfind<decltype(lam), pos>(lam);
        }

        template<class F>
        static constexpr auto filter(F f) {
            constexpr auto lam = [](F fun, auto...ts) constexpr {
                return std::tuple_cat(std::conditional_t<fun(decltype(ts)::value),
                    std::tuple<decltype(ts)>,
                    std::tuple<>>{}...);
            };

            constexpr auto result = my_apply(lam, f, toTuple());
            return fromTuple(result);
        }

        template<class T>
        static constexpr auto concat(T other) {
            constexpr auto filtered = filter([](char sym) constexpr {return  sym != '\0'; });
            constexpr auto tuple = std::tuple_cat(filtered.toTuple(), other.toTuple());
            return fromTuple(tuple);
        }

        template<class F, size_t pos = 0>
        static constexpr auto split(F f) {
            constexpr auto sym_pos = find<F, pos>(f);
            if constexpr (sym_pos < 0) {
                if constexpr (pos < length()) {
                    return std::tuple<decltype(substr<pos>())>{};
                }
                else {
                    return std::tuple<>{};
                }
            }
            else {
                if constexpr (pos == sym_pos) { //to skip the same symbols
                    using right_substr = decltype(split<F, sym_pos + 1>(f));
                    return right_substr{};
                }
                else {
                    using left_substr = decltype(substr<pos, sym_pos - pos>());
                    using right_substr = decltype(split<F, sym_pos + 1>(f));
                    if constexpr (std::is_same<right_substr, std::tuple<>>::value) {
                        return std::tuple<left_substr>{};
                    }
                    else {
                        return std::tuple_cat(std::tuple<left_substr>{}, right_substr{});
                    }
                }
            }
        }

        template<char sym, size_t pos = 0>
        static constexpr auto split() {
            constexpr auto lam = [](char val) constexpr {return val == sym; };
            return split<decltype(lam), pos>(lam);
        }

        static constexpr tupleValue<c...> toTuple() { return tupleValue<c...>{}; }

        template<class... T>
        static constexpr auto fromTuple(const std::tuple<T...>& tuple) {
            return String<T::value...>{};
        }

    private:
        template<size_t pos, int len>
        static constexpr auto substr_helper() {
            constexpr auto lam = [](size_t i) constexpr {return i < len ? buffer[pos + i] : '\0'; };
            return string_builder(lam, std::make_index_sequence<len + 1>{});
        }

        template <class F, class D, class Tuple, std::size_t... I>
        static constexpr decltype(auto) my_apply_impl(F f, D d, Tuple&& t, std::index_sequence<I...>)
        {
            return f(d, std::get<I>(std::forward<Tuple>(t))...);
        }

        template <class F, class D, class Tuple>
        static constexpr decltype(auto) my_apply(F f, D d, Tuple&& t)
        {
            return my_apply_impl(
                f, d, std::forward<Tuple>(t),
                std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
        }

        template <class F, std::size_t ... I>
        static constexpr auto string_builder(F f, std::index_sequence<I...>)
        {
            return String<f(I)...>{};
        }

        template <class F>
        static constexpr auto find_helper_impl(F f)
        {
            return -1;
        }

        template <class F, std::size_t Index, std::size_t ... I>
        static constexpr auto find_helper_impl(F f)
        {
            return f(Index) ? Index : find_helper_impl<F, I...>(f);
        }

        template <class F, std::size_t ... I>
        static constexpr auto find_helper(F f, std::index_sequence<I...>)
        {
            return find_helper_impl<F, I...>(f);
        }

        static inline char constexpr buffer[sizeof...(c)] = { c... };
    };

    template <class F, std::size_t ... I>
    constexpr auto string_builder(F f, std::index_sequence<I...>)
    {
        return String<f(I)...>{};
    }
}
