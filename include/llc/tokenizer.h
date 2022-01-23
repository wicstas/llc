#ifndef LLC_TOKENIZER_H
#define LLC_TOKENIZER_H

#include <llc/defines.h>
#include <llc/check.h>

#include <vector>
#include <string.h>

namespace llc {

enum class TokenType {
    Number,
    Plus,
    Minus,
    Star,
    ForwardSlash,
    LeftParenthesis,
    RightParenthesis,
    LeftCurlyBracket,
    RightCurlyBracket,
    Semicolon,
    Print,
    Assign,
    Variable,
    Float,
    Int,
    If,
    ElseIf,
    Else,
    For,
    While,
    LessThan,
    LessEqual,
    GreaterThan,
    GreaterEqual,
    Equal,
    NotEqual,
    Exclamation,
    Increment,
    Decrement,
    Invalid,
    NumTokens
};
static const char* token_type_names[] = {
    "Number",
    "Plus",
    "Minus",
    "Star",
    "ForwardSlash",
    "LeftParenthesis",
    "RightParenthesis",
    "LeftCurlyBracket",
    "RightCurlyBracket",
    "Semicolon",
    "Print",
    "Assign",
    "Variable",
    "Float",
    "Int",
    "If",
    "ElseIf",
    "Else",
    "For",
    "While",
    "LessThan",
    "LessEqual",
    "GreaterThan",
    "GreaterEqual",
    "Equal",
    "NotEqual",
    "Exclamation",
    "Increment",
    "Decrement",
    "Invalid",
};

static_assert(
    (int)TokenType::NumTokens == sizeof(token_type_names) / sizeof(token_type_names[0]),
    "Expect (int)TokenType::NumTokens == sizeof(token_type_names) / sizeof(token_type_names[0])");

inline const char* enum_to_name(TokenType type) {
    return token_type_names[(int)type];
}

struct Token {
    TokenType type = TokenType::Invalid;
    int line = -1;
    int column = -1;

    float value = 0;
    std::string identifier;
};

inline bool is_digit(char c) {
    return '0' <= c && c <= '9';
}
inline bool is_space(char c) {
    return c == ' ' || c == '\t' || c == '\f';
}
inline bool is_newline(char c) {
    return c == '\n' || c == '\r';
}
inline bool is_alpha(char c) {
    return c >= 'A' && c <= 'z';
}
inline bool is_controlflow(Token token) {
    return token.type == TokenType::If || token.type == TokenType::ElseIf ||
           token.type == TokenType::Else || token.type == TokenType::For ||
           token.type == TokenType::While;
}
inline TokenType get_identifier_type(std::string str) {
    if (str == "print")
        return TokenType::Print;
    if (str == "int")
        return TokenType::Int;
    if (str == "float")
        return TokenType::Float;
    if (str == "if")
        return TokenType::If;
    if (str == "else if")
        return TokenType::ElseIf;
    if (str == "else")
        return TokenType::Else;
    if (str == "for")
        return TokenType::For;
    if (str == "while")
        return TokenType::While;
    return TokenType::Variable;
}

struct Tokenizer {
    std::vector<Token> tokenize(const std::string& source) {
        std::vector<Token> tokens;
        text = source.c_str();
        line = 0;
        column = 0;

        skip_whitespace(text);
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
            case '(': token.type = TokenType::LeftParenthesis; break;
            case ')': token.type = TokenType::RightParenthesis; break;
            case '{': token.type = TokenType::LeftCurlyBracket; break;
            case '}': token.type = TokenType::RightCurlyBracket; break;
            case ';': token.type = TokenType::Semicolon; break;
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
                if (next() == '=') {
                    token.type = TokenType::Equal;
                } else {
                    token.type = TokenType::Assign;
                    putback();
                }
                break;
            case '!':
                if (next() == '=') {
                    token.type = TokenType::NotEqual;
                } else {
                    token.type = TokenType::Exclamation;
                    putback();
                }
                break;
            default:
                if (is_digit(c)) {
                    token.type = TokenType::Number;
                    token.value = scan_value(c);
                } else {
                    std::string identifier = scan_string(c);
                    if (identifier == "else") {
                        if (std::string(text).substr(0, 3) == " if") {
                            identifier = "else if";
                            text += 3;
                            column += 3;
                        }
                    }
                    token.type = get_identifier_type(identifier);
                    token.identifier = identifier;
                }
                break;
            }
            token.line = line;
            token.column = column;
            tokens.push_back(token);
            skip_whitespace(text);
        }

        return tokens;
    }

    char next() {
        column++;
        return *(text++);
    }
    void putback() {
        column--;
        text--;
    }
    float scan_value(char c) {
        float number = 0.0f;
        do {
            number = number * 10.0f + c - '0';
            c = next();
        } while (is_digit(c));

        if (c == '.') {
            c = next();

            float scale = 0.1f;
            do {
                number += (c - '0') * scale;
                scale /= 10.0f;
                c = next();
            } while (is_digit(c));
        }
        if (c != 'f')
            putback();

        return number;
    }
    std::string scan_string(char c) {
        std::string str;
        do {
            str += c;
            c = next();
        } while (is_alpha(c) || is_digit(c) || c == '_');

        putback();
        return str;
    }
    void skip_whitespace(const char*& ptr) {
        while (is_space(*ptr) || is_newline(*ptr)) {
            column++;
            if (is_newline(*ptr)) {
                line++;
                column = 0;
            }
            ptr++;
        }
    }

  private:
    const char* text = nullptr;
    int line = 0;
    int column = 0;
};

}  // namespace llc

#endif  // LLC_TOKENIZER_H
