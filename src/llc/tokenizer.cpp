#include <llc/tokenizer.h>

#include <algorithm>

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
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

char Tokenizer::next() {
    if (current_char_offset == source_char_count)
        throw Exception("Tokenizer::next(): try to access beyond string end");
    current_char_offset++;
    column++;
    return *(text++);
}
void Tokenizer::putback() {
    current_char_offset--;
    column--;
    text--;
}

static const std::map<char, char> escape_char_map = {
    {'n', '\n'}, {'t', '\r'}, {'r', '\r'}, {'b', '\b'}, {'v', '\v'}, {'f', '\f'}, {'a', '\a'}};

std::vector<Token> Tokenizer::tokenize(const Program& program) {
    std::vector<Token> tokens;
    text = program.source.c_str();
    line = 0;
    column = 0;
    current_char_offset = 0;
    int start_offset = 0;
    source_char_count = (int)program.source.size();
    bool comment = false;

    skip(text);
    while (char c = next()) {
        Token token;
        switch (c) {
        case EOF: token.type = TokenType::Eof; break;
        case '+': {
            c = next();
            if (c == '+')
                token.type = TokenType::Increment;
            else if (c == '=')
                token.type = TokenType::PlusEqual;
            else {
                token.type = TokenType::Plus;
                putback();
            }
            break;
        }

        case '-': {
            c = next();
            if (c == '-')
                token.type = TokenType::Decrement;
            else if (c == '=')
                token.type = TokenType::MinusEqual;
            else {
                token.type = TokenType::Minus;
                putback();
            }
            break;
        }

        case '*': {
            char c = next();
            if (c == '=')
                token.type = TokenType::MultiplyEqual;
            else {
                token.type = TokenType::Star;
                putback();
            }
            break;
        }

        case '/': {
            if (next() == '/') {
                comment = true;
                while (!is_newline(next())) {
                }
                putback();
            } else {
                putback();
                if (next() == '=')
                    token.type = TokenType::DivideEqual;
                else {
                    token.type = TokenType::ForwardSlash;
                    putback();
                }
            }
            break;
        }

        case '(': token.type = TokenType::LeftParenthese; break;
        case ')': token.type = TokenType::RightParenthese; break;
        case '[': token.type = TokenType::LeftSquareBracket; break;
        case ']': token.type = TokenType::RightSquareBracket; break;
        case '{': token.type = TokenType::LeftCurlyBracket; break;
        case '}': token.type = TokenType::RightCurlyBracket; break;
        case ';': token.type = TokenType::Semicolon; break;
        case '.': token.type = TokenType::Dot; break;
        case ',': token.type = TokenType::Comma; break;
        case '<': {
            if (next() == '=') {
                token.type = TokenType::LessEqual;
            } else {
                token.type = TokenType::LessThan;
                putback();
            }
            break;
        }
        case '>': {
            if (next() == '=') {
                token.type = TokenType::GreaterEqual;
            } else {
                token.type = TokenType::GreaterThan;
                putback();
            }
            break;
        }
        case '=': {
            if (next() == '=')
                token.type = TokenType::Equal;
            else {
                putback();
                token.type = TokenType::Assign;
            }
            break;
        }
        case '!': {
            if (next() == '=')
                token.type = TokenType::NotEqual;
            else {
                putback();
                token.type = TokenType::Exclaimation;
            }
            break;
        }
        case '"': {
            token.type = TokenType::String;
            c = next();
            while (c != '"') {
                if (c == '\\') {
                    char e = next();
                    auto it = escape_char_map.find(e);
                    if (it == escape_char_map.end())
                        throw Exception(to_string("use of unknown escape character \"", e, "\""),
                                        Location(line, column, current_char_offset - start_offset));
                    c = it->second;
                }
                token.value_s += c;
                c = next();
                if (c == '\0')
                    throw Exception("missing \"",
                                    Location(line, column, current_char_offset - start_offset));
            }
            break;
        }
        case '\'': {
            token.type = TokenType::Char;
            c = next();
            if (c == '\\') {
                c = next();
                auto it = escape_char_map.find(c);
                if (it == escape_char_map.end())
                    throw Exception(to_string("use of unknown escape character \"", c, "\""),
                                    Location(line, column, current_char_offset - start_offset));
                token.value_c = it->second;
            } else {
                token.value_c = c;
            }
            LLC_CHECK(next() == '\'');
            break;
        }
        default: {
            if (is_digit(c)) {
                token.type = TokenType::Number;
                token.value = scan_value(c);
            } else {
                token.type = TokenType::Identifier;
                token.id = scan_string(c);
                if (token.id == "true") {
                    token.type = TokenType::Number;
                    token.value = 1;
                }
                if (token.id == "false") {
                    token.type = TokenType::Number;
                    token.value = 0;
                }
            }
            break;
        }
        }

        if (!comment) {
            int length = current_char_offset - start_offset;
            token.location = Location(line, column - length, length, program.filepath);
            tokens.push_back(token);
        }
        comment = false;

        if (tokens.size() && tokens.back().type == TokenType::Eof)
            break;

        skip(text);
        start_offset = current_char_offset;
    }

    if (tokens.size() == 0 || tokens.back().type != TokenType::Eof) {
        Token token;
        token.type = TokenType::Eof;
        tokens.push_back(token);
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
        while (is_digit(c)) {
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
