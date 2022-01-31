#ifndef LLC_PARSER_H
#define LLC_PARSER_H

#include <llc/defines.h>
#include <llc/types.h>
#include <llc/misc.h>

#include <tuple>

namespace llc {

struct Parser;

struct TypeMemberBuilder {
    virtual ~TypeMemberBuilder() = default;
    virtual void build(void* buffer, Struct object) const = 0;
};

template <typename T, typename M>
struct ConcreteTypeMemberBuilder : TypeMemberBuilder {
    ConcreteTypeMemberBuilder(std::string id, M T::*ptr) : id(id), ptr(ptr){};

    void build(void* object, Struct input) const override { ((T*)object)->*ptr = input.members[id]->value; }

    std::string id;
    M T::*ptr;
};

struct TypeBuilder {
    virtual ~TypeBuilder() = default;

    template <typename T>
    T build(Struct input) {
        T object;
        for (const auto& builder : builders)
            builder.second->build(&object, input);
        return object;
    }

    std::map<std::string, std::shared_ptr<TypeMemberBuilder>> builders;
};

template <typename T>
struct ConcreteTypeBuilder : TypeBuilder {
    ConcreteTypeBuilder(Struct& object) : object(object){};

    template <typename M>
    ConcreteTypeBuilder& bind(std::string id, M T::*ptr) {
        object.members[id] = std::make_shared<Struct>();
        builders[id] = std::make_shared<ConcreteTypeMemberBuilder<T, M>>(id, ptr);

        return *this;
    }

    Struct& object;
};

template <int index, typename... Args>
void helper(std::tuple<std::decay_t<Args>...>& tuple, std::vector<Struct> args,
            std::map<uint64_t, std::shared_ptr<TypeBuilder>> builders) {
    using T = std::decay_t<decltype(std::get<index>(tuple))>;
    if constexpr (std::is_same<T, int>::value) {
        std::get<index>(tuple) = args[index].value;
    } else if constexpr (std::is_same<T, float>::value) {
        std::get<index>(tuple) = args[index].value;
    } else if constexpr (std::is_same<T, std::string>::value) {
        std::get<index>(tuple) = args[index].value_s;
    } else if constexpr (std::is_same<T, const char*>::value) {
        std::get<index>(tuple) = args[index].value_s.c_str();
    } else {
        LLC_CHECK(builders[typeid(T).hash_code()] != nullptr);
        std::get<index>(tuple) = builders[typeid(T).hash_code()]->build<T>(args[index]);
    }
}

template <typename Return, typename... Args>
struct FunctionInstance : ExternalFunction {
    using F = Return (*)(Args...);
    FunctionInstance(F f, std::map<uint64_t, std::shared_ptr<TypeBuilder>>& builders)
        : f(f), builders(builders){};

    Struct invoke(std::vector<Struct> args) const override {
        LLC_CHECK(args.size() == sizeof...(Args));

        std::tuple<std::decay_t<Args>...> tuple = {};
        if constexpr (sizeof...(Args) >= 1)
            helper<0, Args...>(tuple, args, builders);
        if constexpr (sizeof...(Args) >= 2)
            helper<1, Args...>(tuple, args, builders);

        if constexpr (std::is_same<Return, void>::value) {
            if constexpr (sizeof...(Args) == 0)
                f();
            else if constexpr (sizeof...(Args) == 1)
                f(std::get<0>(tuple));
            else if constexpr (sizeof...(Args) == 2)
                f(std::get<0>(tuple), std::get<1>(tuple));
            else
                fatal("too many arguments, only support <= 4");

        } else {
            if constexpr (sizeof...(Args) == 0)
                return f();
            else if constexpr (sizeof...(Args) == 1)
                return f(std::get<0>(tuple));
            else if constexpr (sizeof...(Args) == 2)
                return f(std::get<0>(tuple), std::get<1>(tuple));
            else
                fatal("too many arguments, only support <= 4");
        }

        return {};
    }

    F f;
    std::map<uint64_t, std::shared_ptr<TypeBuilder>>& builders;
};

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
        registered_functions[name] = std::make_shared<FunctionInstance<Return, Args...>>(func, type_builders);
    }

    template <typename T>
    ConcreteTypeBuilder<T>& bind(std::string name) {
        registered_types.push_back({name, Struct()});
        auto builder = std::make_shared<ConcreteTypeBuilder<T>>(registered_types.back().second);
        type_builders[typeid(T).hash_code()] = builder;
        return *builder;
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
    std::vector<std::pair<std::string, Struct>> registered_types;
    std::map<uint64_t, std::shared_ptr<TypeBuilder>> type_builders;

    friend struct Compiler;
};

}  // namespace llc

#endif  // LLC_PARSER_H