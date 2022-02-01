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

    Program parse(std::string source, std::vector<Token> tokens) {
        this->source = source;
        this->tokens = tokens;
        pos = 0;

        std::shared_ptr<Scope> scope = std::make_shared<Scope>();
        for (const auto& type : registered_types)
            scope->types[type.first] = type.second;

        parse_recursively(scope);
        return scope;
    }

    template <typename Return, typename... Args>
    void bind(std::string name, Return (*func)(Args...)) {
        registered_functions[name] = (Function)std::make_unique<FunctionInstance<Return, Args...>>(func);
    }

    template <typename T>
    struct TypeBindHelper {
        TypeBindHelper(std::string name, Parser& parser) : name(name), parser(parser) {
            parser.registered_types[name] = Object(T(), name);
        };

        template <typename M>
        TypeBindHelper bind(std::string name, M T::*ptr) {
            (void)name;
            (void)ptr;
            return *this;
        }

        std::string name;
        Parser& parser;
    };

    template <typename T>
    TypeBindHelper<T> bind(std::string name) {
        return TypeBindHelper<T>(name, *this);
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
    FunctionCall build_functioncall(std::shared_ptr<Scope> scope, Function function);
    Expression build_expression(std::shared_ptr<Scope> scope);

    std::optional<Token> match(TokenType type);
    Token must_match(TokenType type);

    template <typename T>
    T must_has(T value, Token token) {
        if (!value)
            fatal("cannot find \"", token.id, "\":\n", token.location(source));
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

    std::map<std::string, Function> registered_functions;
    std::map<std::string, Object> registered_types;

    friend struct Compiler;
};

}  // namespace llc

#endif  // LLC_PARSER_H