#ifndef LLC_PARSER_H
#define LLC_PARSER_H

#include <llc/defines.h>
#include <llc/types.h>
#include <llc/misc.h>

#include <tuple>

namespace llc {

struct Parser {
    Parser() = default;
    Parser(const Parser&) = delete;
    Parser& operator=(const Parser&) = delete;

    void parse(Program& program, std::vector<Token> tokens) {
        this->source = program.source;
        this->tokens = std::move(tokens);
        this->pos = 0;

        program.scope = std::make_shared<Scope>();
        for (const auto& type : program.types)
            program.scope->types[type.first] = type.second;
        for (const auto& var : program.variables)
            program.scope->variables[var.first] = var.second;
        for (const auto& function : program.functions)
            program.scope->functions[function.first] = function.second;
        parse_recursively(program.scope);
    }

  private:
    void parse_recursively(std::shared_ptr<Scope> scope, bool end_on_new_line = false);

    std::shared_ptr<Scope> parse_recursively_topdown(std::shared_ptr<Scope> parent,
                                                     bool end_on_new_line = false) {
        std::shared_ptr<Scope> scope = std::make_shared<Scope>();
        scope->parent = parent;
        parse_recursively(scope, end_on_new_line);
        return scope;
    }

    void declare_variable(std::shared_ptr<Scope> scope);
    void declare_function(std::shared_ptr<Scope> scope);
    void declare_struct(std::shared_ptr<Scope> scope);
    FunctionCall build_functioncall(std::shared_ptr<Scope> scope, std::string function_name);
    Expression build_expression(std::shared_ptr<Scope> scope);

    std::optional<Token> match(TokenType type);
    Token must_match(TokenType type);

    template <typename T>
    T must_has(T value, Token token) {
        if (!value)
            throw_exception("cannot find \"", token.id, "\":\n", token.location(source));
        return value;
    }

    void putback() {
        LLC_CHECK(pos != 0);
        pos--;
    }
    Token advance() {
        LLC_CHECK(!no_more());
        return tokens[pos++];
    }
    bool no_more() const {
        LLC_CHECK(pos <= tokens.size());
        return pos == tokens.size();
    }

    std::string source;
    std::vector<Token> tokens;
    size_t pos;
};

}  // namespace llc

#endif  // LLC_PARSER_H