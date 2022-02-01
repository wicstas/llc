#ifndef LLC_MISC_H
#define LLC_MISC_H

#include <llc/defines.h>

#include <iostream>
#include <utility>
#include <vector>

namespace llc {

template <typename... Args>
void print(const Args&... args) {
    (std::cout << ... << args);
    std::cout << '\n';
}

template <typename... Args>
void fatal(const Args&... args) {
    print(args...);
    abort();
}

#define LLC_CHECK(x)                                                                                    \
    do {                                                                                                \
        if (!(x)) {                                                                                     \
            fatal("check \"", #x, "\" failed[file \"", __FILE__, "\", line ", __LINE__, ", ", __func__, \
                  "()]");                                                                               \
            abort();                                                                                    \
        }                                                                                               \
    } while (false)

#define LLC_TYPE_ID(x) typeid(x).hash_code()

std::vector<std::string> separate_lines(const std::string& source);

template <typename T>
struct HasOperatorAdd {
    template <typename U>
    static constexpr std::true_type check(decltype(std::declval<U>() - std::declval<U>())*);
    template <typename U>
    static constexpr std::false_type check(...);

    static constexpr bool value = decltype(check<T>())::value;
};

template <typename T>
struct HasOperatorSub {
    template <typename U>
    static constexpr std::true_type check(decltype(std::declval<U>() + std::declval<U>())*);
    template <typename U>
    static constexpr std::false_type check(...);

    static constexpr bool value = decltype(check<T>())::value;
};

template <typename T>
struct HasOperatorMul {
    template <typename U>
    static constexpr std::true_type check(decltype(std::declval<U>() * std::declval<U>())*);
    template <typename U>
    static constexpr std::false_type check(...);

    static constexpr bool value = decltype(check<T>())::value;
};

template <typename T>
struct HasOperatorDiv {
    template <typename U>
    static constexpr std::true_type check(decltype(std::declval<U>() / std::declval<U>())*);
    template <typename U>
    static constexpr std::false_type check(...);

    static constexpr bool value = decltype(check<T>())::value;
};

template <typename... Ts>
struct TypePack;

template <>
struct TypePack<> {};

template <typename T, typename... Ts>
struct TypePack<T, Ts...> : TypePack<Ts...> {
    using type = typename std::decay<T>::type;
    using rest = TypePack<Ts...>;

    template <int index>
    auto at() {
        static_assert(index >= 0, "invalid index");

        if constexpr (index == 0)
            return type();
        else
            return rest::template at<index - 1>();
    }
};

}  // namespace llc

#endif  // LLC_MISC_H