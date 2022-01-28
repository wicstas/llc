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

}  // namespace llc

#endif  // LLC_MISC_H