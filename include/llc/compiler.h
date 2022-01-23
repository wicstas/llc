#ifndef LLC_COMPILER_H
#define LLC_COMPILER_H

#include <llc/tokenizer.h>
#include <llc/parser.h>

namespace llc {

struct Compiler {
    void compile(std::string source) {
        auto tokens = tokenizer.tokenize(source);
        parser.parse(std::move(source), std::move(tokens));
    }
    void execute() {
        parser.execute();
    }

  private:
    Tokenizer tokenizer;
    Parser parser;
};

}  // namespace llc

#endif  // LLC_COMPILER_H