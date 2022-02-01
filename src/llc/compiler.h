#ifndef LLC_COMPILER_H
#define LLC_COMPILER_H

#include <llc/tokenizer.h>
#include <llc/parser.h>

namespace llc {

struct Compiler {
    void compile(Program& program, std::string source) {
        auto tokens = tokenizer.tokenize(source);
        parser.parse(source, tokens, &program);
    }

  private:
    Tokenizer tokenizer;
    Parser parser;
};

}  // namespace llc

#endif  // LLC_COMPILER_H