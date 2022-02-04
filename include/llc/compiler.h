#ifndef LLC_COMPILER_H
#define LLC_COMPILER_H

#include <llc/tokenizer.h>
#include <llc/parser.h>

namespace llc {

struct Compiler {
    void compile(Program& program) {
        auto tokens = tokenizer.tokenize(program.source);
        parser.parse(program.source, tokens, &program);
    }

  private:
    Tokenizer tokenizer;
    Parser parser;
};

}  // namespace llc

#endif  // LLC_COMPILER_H