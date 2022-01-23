#ifndef LLC_CHECK_H
#define LLC_CHECK_H

#include <llc/defines.h>

#include <iostream>

namespace llc {

template <typename... Args>
void print(const Args&... args) {
    std::cout << "[llc]";
    (std::cout << ... << args);
    std::cout << '\n';
}

template <typename... Args>
void fatal(const Args&... args) {
    print(args...);
    abort();
}

void print_failed_check(const char* file, int line, const char* func_name, const char* expression) {
    fatal("check ", expression, " failed[file \"", file, "\", line ", line, ", ", func_name,
          "()]\n");
}

#define LLC_CHECK(x)                                              \
    do {                                                          \
        if (!(x)) {                                               \
            print_failed_check(__FILE__, __LINE__, __func__, #x); \
            abort();                                              \
        }                                                         \
    } while (false)

#define LLC_INVOKE_IF(x, expr) \
    do {                       \
        if (x)                 \
            expr;              \
    } while (false)

}  // namespace llc

#endif  // LLC_CHECK_H