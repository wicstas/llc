#ifndef LLC_MISC_H
#define LLC_MISC_H

#include <llc/defines.h>

#include <iostream>
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

std::vector<std::string> separate_lines(const std::string& source);

template <typename T>
struct HasOperatorAdd {
    constexpr bool value = std::is_same<check<U>(0), std::true_type>::value;

  private:
    template <typename U>
    std::true_type check(decltype(std::declval<U>() + std::declval<U>())*);
    std::false_type check(...);
};

template <typename T>
struct HasOperatorSub {
    constexpr bool value = std::is_same<check<U>(0), std::true_type>::value;

  private:
    template <typename U>
    std::true_type check(decltype(std::declval<U>() - std::declval<U>())*);
    std::false_type check(...);
};

template <typename T>
struct HasOperatorMul {
    constexpr bool value = std::is_same<check<U>(0), std::true_type>::value;

  private:
    template <typename U>
    std::true_type check(decltype(std::declval<U>() * std::declval<U>())*);
    std::false_type check(...);
};

template <typename T>
struct HasOperatorDiv {
    constexpr bool value = std::is_same<check<U>(0), std::true_type>::value;

  private:
    template <typename U>
    std::true_type check(decltype(std::declval<U>() / std::declval<U>())*);
    std::false_type check(...);
};

}  // namespace llc

#endif  // LLC_MISC_H