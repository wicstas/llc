#ifndef LLC_COMPILER_H
#define LLC_COMPILER_H

#include <llc/tokenizer.h>
#include <llc/parser.h>

namespace llc {

struct Compiler {
    template <typename F>
    void register_function(std::string name, F func) {
        parser.register_function(name, func);
    }

    Program compile(std::string source) {
        auto tokens = tokenizer.tokenize(source);
        return parser.parse(source, tokens);
    }

  private:
    Tokenizer tokenizer;
    Parser parser;
};

}  // namespace llc

#endif  // LLC_COMPILER_H