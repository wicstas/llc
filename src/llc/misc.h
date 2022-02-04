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

template <typename T, typename... Args>
std::string to_string(const T& first, const Args&... args) {
    static_assert(std::is_convertible<T, std::string>::value || std::is_arithmetic<T>::value,
                  "unable to convert type T to std::string");

    std::string str;
    if constexpr (std::is_convertible<T, std::string>::value)
        str += std::string(first);
    else if constexpr (std::is_arithmetic<T>::value)
        str += std::to_string(first);

    if constexpr (sizeof...(args) != 0)
        str += to_string(args...);
    return str;
}

struct Location {
    Location() = default;
    Location(int line, int column, int length) : line(line), column(column), length(length){};

    std::string operator()(const std::string& source) const;

    friend Location operator+(Location lhs, Location rhs) {
        lhs.length += rhs.length;
        return lhs;
    }

    int line = -1;
    int column = -1;
    int length = -1;
};

struct Exception : std::exception {
    Exception(std::string message, Location location = {}) : message(message), location(location){};

    const char* what() const noexcept override {
        return message.c_str();
    }

    std::string operator()(std::string source) const {
        if (location.line != -1)
            return message + ":\n" + location(source);
        else
            return message;
    }

    std::string message;
    Location location;
};

template <typename... Args>
void throw_exception(const Args&... args) {
    throw Exception(to_string(args...));
}

#define LLC_CHECK(x)                                                                            \
    do {                                                                                        \
        if (!(x)) {                                                                             \
            throw Exception(to_string("internal error: check \"", #x, "\" failed[file \"",      \
                                      __FILE__, "\", line ", __LINE__, ", ", __func__, "()]")); \
        }                                                                                       \
    } while (false)

std::vector<std::string> separate_lines(const std::string& source);

#define DefineCheckOperator(name, op)                                                             \
    template <typename T>                                                                         \
    struct HasOperator##name {                                                                    \
        template <typename U>                                                                     \
        static constexpr std::true_type check(decltype(std::declval<U>() op std::declval<U>())*); \
        template <typename U>                                                                     \
        static constexpr std::false_type check(...);                                              \
        static constexpr bool value = decltype(check<T>(0))::value;                               \
    }

DefineCheckOperator(Add, +);
DefineCheckOperator(Sub, -);
DefineCheckOperator(Mul, *);
DefineCheckOperator(Div, /);
DefineCheckOperator(LT, <);
DefineCheckOperator(LE, <=);
DefineCheckOperator(GT, >);
DefineCheckOperator(GE, >=);
DefineCheckOperator(EQ, ==);
DefineCheckOperator(NE, !=);

template <typename T>
struct HasOperatorArrayAccess {
    template <typename U>
    static constexpr std::true_type check(std::decay_t<decltype(std::declval<U>()[0])>*);
    template <typename U>
    static constexpr std::false_type check(...);
    static constexpr bool value = decltype(check<T>(0))::value;
};

template <typename T>
struct HasOperatorPreIncrement {
    template <typename U>
    static constexpr std::true_type check(std::decay_t<decltype(++std::declval<U&>())>*);
    template <typename U>
    static constexpr std::false_type check(...);
    static constexpr bool value = decltype(check<T>(0))::value;
};

template <>
struct HasOperatorPreIncrement<bool> {
    static constexpr bool value = false;
};

template <typename T>
struct HasOperatorPreDecrement {
    template <typename U>
    static constexpr std::true_type check(std::decay_t<decltype(--std::declval<U&>())>*);
    template <typename U>
    static constexpr std::false_type check(...);
    static constexpr bool value = decltype(check<T>(0))::value;
};

template <>
struct HasOperatorPreDecrement<bool> {
    static constexpr bool value = false;
};

template <typename T>
struct HasOperatorNegation {
    template <typename U>
    static constexpr std::true_type check(std::decay_t<decltype(-std::declval<U>())>*);
    template <typename U>
    static constexpr std::false_type check(...);
    static constexpr bool value = decltype(check<T>(0))::value;
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

template <bool val, typename T>
struct deferred_bool {
    static constexpr bool value = val;
};

template <typename... Args, typename T, typename R>
auto overload_cast(R (T::*func)(Args...)) {
    return func;
}

}  // namespace llc

#endif  // LLC_MISC_H