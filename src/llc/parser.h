#ifndef LLC_PARSER_H
#define LLC_PARSER_H

#include <llc/defines.h>
#include <llc/types.h>
#include <llc/misc.h>

namespace llc {

struct Parser {
    Program parse(std::string source, std::vector<Token> tokens) {
        this->source = source;
        this->tokens = tokens;
        pos = 0;

        return parse_recursively(nullptr);
    }

    std::shared_ptr<Scope> parse_recursively(std::shared_ptr<Scope> parent);

    void declare_variable(std::shared_ptr<Scope> scope);
    Expression build_expression(std::shared_ptr<Scope> scope);

    std::optional<Token> match(TokenType type) {
        auto token = advance();
        if (token.type == type) {
            return token;
        } else {
            putback();
            return std::nullopt;
        }
    }
    Token must_match(TokenType type) {
        if (no_more())
            fatal("expect \"", enum_to_string(type), "\", but no more token is available");
        auto token = advance();
        if (token.type == type) {
            return token;
        } else {
            fatal("token mismatch, expect \"", enum_to_string(type), "\", get \"",
                  enum_to_string(token.type), "\":\n", token.location(source));
            return {};
        }
    }
    template <typename T>
    std::decay_t<decltype(*std::declval<T>())> must_has(T value, Token token) {
        if (value) {
            return *value;
        } else {
            fatal("cannot find \"", token.id, "\":\n", token.location(source));
            return {};
        }
    }

    void putback() {
        pos--;
    }
    Token advance() {
        LLC_CHECK(!no_more());
        return tokens[pos++];
    }

    bool no_more() const {
        return pos >= tokens.size();
    }

    std::string source;
    std::vector<Token> tokens;
    size_t pos;
};

}  // namespace llc

#endif  // LLC_PARSER_H