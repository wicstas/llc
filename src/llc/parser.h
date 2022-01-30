#ifndef LLC_PARSER_H
#define LLC_PARSER_H

#include <llc/defines.h>
#include <llc/types.h>
#include <llc/misc.h>

namespace llc {

template <typename Return, typename... Args>
struct FunctionInstance : ExternalFunction {
    using F = Return (*)(Args...);
    FunctionInstance(F f) : f(f){};

    Struct invoke(std::vector<Struct> args) const override {
        LLC_CHECK(args.size() == sizeof...(Args));

        if constexpr (std::is_same<Return, void>::value) {
            if constexpr (sizeof...(Args) == 0)
                f();
            else if constexpr (sizeof...(Args) == 1)
                f(args[0].value);
            else if constexpr (sizeof...(Args) == 2)
                f(args[0].value, args[1].value);
            else if constexpr (sizeof...(Args) == 3)
                f(args[0].value, args[1].value, args[2].value);
            else if constexpr (sizeof...(Args) == 4)
                f(args[0].value, args[1].value, args[2].value, args[3].value);
            else
                fatal("too many arguments, only support <= 4");

        } else {
            if constexpr (sizeof...(Args) == 0)
                return f();
            else if constexpr (sizeof...(Args) == 1)
                return f(args[0].value);
            else if constexpr (sizeof...(Args) == 2)
                return f(args[0].value, args[1].value);
            else if constexpr (sizeof...(Args) == 3)
                return f(args[0].value, args[1].value, args[2].value);
            else if constexpr (sizeof...(Args) == 4)
                return f(args[0].value, args[1].value, args[2].value, args[3].value);
            else
                fatal("too many arguments, only support <= 4");
        }

        return {};
    }

    F f;
};

struct Parser {
    Program parse(std::string source, std::vector<Token> tokens) {
        this->source = source;
        this->tokens = tokens;
        pos = 0;

        std::shared_ptr<Scope> scope = std::make_shared<Scope>();
        parse_recursively(scope);
        return scope;
    }

    template <typename Return, typename... Args>
    void register_function(std::string name, Return (*func)(Args...)) {
        registered_functions[name] = std::make_shared<FunctionInstance<Return, Args...>>(func);
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
    FunctionCall build_functioncall(std::shared_ptr<Scope> scope, std::shared_ptr<Function> function);
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

    std::map<std::string, std::shared_ptr<ExternalFunction>> registered_functions;
};

}  // namespace llc

#endif  // LLC_PARSER_H