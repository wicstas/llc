#ifndef LLC_TOKENIZER_H
#define LLC_TOKENIZER_H

#include <llc/defines.h>
#include <llc/types.h>

namespace llc {

struct Tokenizer {
    std::vector<Token> tokenize(const Program& program);

  private:
    char next();
    void putback();

    void skip(const char*& ptr);
    float scan_value(char c);
    std::string scan_string(char c);

    const char* text = nullptr;
    int line = 0;
    int column = 0;
    int current_char_offset = 0;
    int source_char_count = 0;
};

}  // namespace llc

#endif  // LLC_TOKENIZER_H
