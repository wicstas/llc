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

        std::shared_ptr<Scope> scope = std::make_shared<Scope>();
        parse_recursively(scope);
        return scope;
    }

    void parse_recursively(std::shared_ptr<Scope> scope);
    std::shared_ptr<Scope> parse_recursively_topdown(std::shared_ptr<Scope> parent) {
        std::shared_ptr<Scope> scope = std::make_shared<Scope>();
        scope->parent = parent;
        parse_recursively(scope);
        return scope;
    }

    void declare_variable(std::shared_ptr<Scope> scope);
    void declare_function(std::shared_ptr<Scope> scope);
    FunctionCall build_functioncall(std::shared_ptr<Scope> scope);
    Expression build_expression(std::shared_ptr<Scope> scope);

    std::optional<Token> match(TokenType type);
    std::optional<Token> detect(TokenType type);
    Token must_match(TokenType type);
    
    template <typename T>
    T must_has(T value, Token token) {
        if (!value)
            fatal("cannot find \"", token.id, "\":\n", token.location(source));
        return value;
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