#include <llc/tokenizer.h>

namespace llc {

static inline bool is_digit(char c) {
    return '0' <= c && c <= '9';
}
static inline bool is_space(char c) {
    return c == ' ' || c == '\t' || c == '\f';
}
static inline bool is_newline(char c) {
    return c == '\n' || c == '\r';
}
static inline bool is_alpha(char c) {
    return c >= 'A' && c <= 'z';
}

char Tokenizer::next() {
    current_char_offset++;
    column++;
    return *(text++);
}
void Tokenizer::putback() {
    current_char_offset--;
    column--;
    text--;
}

std::vector<Token> Tokenizer::tokenize(const std::string& source) {
    std::vector<Token> tokens;
    text = source.c_str();
    line = 0;
    column = 0;
    current_char_offset = 0;
    int start_offset = 0;

    skip(text);
    while (char c = next()) {
        Token token;
        switch (c) {
        case EOF: break;
        case '+':
            if (next() == '+') {
                token.type = TokenType::Increment;
            } else {
                token.type = TokenType::Plus;
                putback();
            }
            break;
        case '-':
            if (next() == '-') {
                token.type = TokenType::Decrement;
            } else {
                token.type = TokenType::Minus;
                putback();
            }
            break;
        case '*': token.type = TokenType::Star; break;
        case '/': token.type = TokenType::ForwardSlash; break;
        case '(': token.type = TokenType::LeftParenthese; break;
        case ')': token.type = TokenType::RightParenthese; break;
        case '{': token.type = TokenType::LeftCurlyBracket; break;
        case '}': token.type = TokenType::RightCurlyBracket; break;
        case ';': token.type = TokenType::Semicolon; break;
        case '.': token.type = TokenType::Dot; break;
        case ',': token.type = TokenType::Comma; break;
        case '<':
            if (next() == '=') {
                token.type = TokenType::LessEqual;
            } else {
                token.type = TokenType::LessThan;
                putback();
            }
            break;
        case '>':
            if (next() == '=') {
                token.type = TokenType::GreaterEqual;
            } else {
                token.type = TokenType::GreaterThan;
                putback();
            }
            break;
        case '=':
            if (next() == '=')
                token.type = TokenType::Equal;
            else {
                putback();
                token.type = TokenType::Assign;
            }
            break;
        case '!':
            if (next() == '=')
                token.type = TokenType::NotEqual;
            else {
                putback();
                token.type = TokenType::Exclaimation;
            }
            break;
        default:
            if (is_digit(c)) {
                token.type = TokenType::Number;
                token.value = scan_value(c);
            } else {
                token.type = TokenType::Identifier;
                token.id = scan_string(c);
            }
            break;
        }

        int length = current_char_offset - start_offset;
        token.location = Location(line, column - length, length);
        tokens.push_back(token);

        skip(text);
        start_offset = current_char_offset;
    }

    return tokens;
}

float Tokenizer::scan_value(char c) {
    float number = 0.0f;
    do {
        number = number * 10.0f + c - '0';
        c = next();
    } while (is_digit(c));

    if (c == '.') {
        c = next();

        float scale = 0.1f;
        while(is_digit(c)){
            number += (c - '0') * scale;
            scale /= 10.0f;
            c = next();
        };
    }
    if (c != 'f')
        putback();

    return number;
}

std::string Tokenizer::scan_string(char c) {
    std::string str;
    do {
        str += c;
        c = next();
    } while (is_alpha(c) || is_digit(c) || c == '_');

    putback();
    return str;
}

void Tokenizer::skip(const char*& ptr) {
    while (is_space(*ptr) || is_newline(*ptr)) {
        column++;
        if (is_newline(*ptr)) {
            line++;
            column = 0;
        }
        ptr++;
    }
}

}  // namespace llc
