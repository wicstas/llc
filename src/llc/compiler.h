#ifndef LLC_COMPILER_H
#define LLC_COMPILER_H

#include <llc/tokenizer.h>
#include <llc/parser.h>

namespace llc {

struct Compiler {
    void compile(Program& program) {
        try {
            auto tokens = tokenizer.tokenize(program.source);
            parser.parse(program.source, tokens, &program);
        } catch (const Exception& throw_exception) {
            print(throw_exception(program.source));
        }
    }

  private:
    Tokenizer tokenizer;
    Parser parser;
};

}  // namespace llc

#endif  // LLC_COMPILER_H