#ifndef LLC_TYPES_H
#define LLC_TYPES_H

#include <llc/defines.h>
#include <llc/misc.h>

#include <optional>
#include <string>
#include <memory>
#include <vector>
#include <map>

namespace llc {

struct Location {
    Location() = default;
    Location(int line, int column, int length) : line(line), column(column), length(length){};

    friend Location operator+(Location lhs, Location rhs) {
        lhs.length += rhs.length;
        return lhs;
    }

    std::string operator()(const std::string& source);

    int line = -1;
    int column = -1;
    int length = -1;
};

enum class TokenType {
    Number,
    Increment,
    Decrement,
    Plus,
    Minus,
    Star,
    ForwardSlash,
    LeftParenthese,
    RightParenthese,
    LeftCurlyBracket,
    RightCurlyBracket,
    Semicolon,
    Identifier,
    Dot,
    Comma,
    LessThan,
    LessEqual,
    GreaterThan,
    GreaterEqual,
    Equal,
    NotEqual,
    Assign,
    Exclaimation,
    Invalid,
    NumTokens
};

inline std::string enum_to_string(TokenType type) {
    static const char* map[] = {
        "number", "++", "--", "+",          "-", "*",       "/",         "(",  ")",
        "{",      "}",  ";",  "identifier", ".", ",",       "<",         "<=", ">",
        ">=",     "==", "!=", "=",          "!", "invalid", "num_tokens"};
    return map[(int)type];
}

struct Token {
    TokenType type = TokenType::Invalid;
    Location location;

    float value;
    std::string id;
};

struct Scope;
struct Expression;

struct Struct {
    Struct() = default;
    Struct(int valuei) : valuei(valuei){};

    friend Struct operator+(Struct a, Struct b) {
        a.valuei += b.valuei;
        return a;
    }
    friend Struct operator-(Struct a, Struct b) {
        a.valuei -= b.valuei;
        return a;
    }
    friend Struct operator*(Struct a, Struct b) {
        a.valuei *= b.valuei;
        return a;
    }
    friend Struct operator/(Struct a, Struct b) {
        a.valuei /= b.valuei;
        return a;
    }
    friend bool operator<(Struct a, Struct b) {
        return a.valuei < b.valuei;
    }
    friend bool operator<=(Struct a, Struct b) {
        return a.valuei <= b.valuei;
    }
    friend bool operator>(Struct a, Struct b) {
        return a.valuei > b.valuei;
    }
    friend bool operator>=(Struct a, Struct b) {
        return a.valuei >= b.valuei;
    }
    friend bool operator==(Struct a, Struct b) {
        return a.valuei == b.valuei;
    }
    friend bool operator!=(Struct a, Struct b) {
        return a.valuei != b.valuei;
    }
    Struct operator++() {
        valuei++;
        return *this;
    }
    Struct operator--() {
        valuei--;
        return *this;
    }
    Struct operator++(int) {
        auto temp = *this;
        valuei++;
        return temp;
    }
    Struct operator--(int) {
        auto temp = *this;
        valuei--;
        return temp;
    }

    operator bool() const {
        return (bool)valuei;
    }

    int valuei = 0;
};

struct Function {
    Struct run(Scope& scope, std::vector<Expression> args) const;

    Struct return_type;
    std::shared_ptr<Scope> definition;
    std::vector<std::string> parameters;
};

////////////////////////////////
////////////////////////////////
// Statement level
struct Statement {
    virtual ~Statement() = default;

    virtual Struct run(Scope& scope) = 0;
};

struct Scope : Statement {
    Scope() {
        types["int"] = {};
        types["void"] = {};
    }

    Struct run(Scope&) {
        for (const auto& statement : statements)
            statement->run(*this);
        return {};
    }

    std::optional<Struct> find_type(std::string name) const {
        auto it = types.find(name);
        if (it == types.end())
            return parent ? parent->find_type(name) : std::nullopt;
        else
            return it->second;
    }
    std::shared_ptr<Struct> find_variable(std::string name) const {
        auto it = variables.find(name);
        if (it == variables.end())
            return parent ? parent->find_variable(name) : nullptr;
        else {
            if (it->second == nullptr)
                fatal("cannot find variable \"", name, "\"", " definition");
            return it->second;
        }
    }
    std::shared_ptr<Function> find_function(std::string name) const {
        auto it = functions.find(name);
        if (it == functions.end())
            return parent ? parent->find_function(name) : nullptr;
        else {
            return it->second;
        }
    }

    std::shared_ptr<Scope> parent;
    std::vector<std::shared_ptr<Statement>> statements;
    std::map<std::string, Struct> types;
    std::map<std::string, std::shared_ptr<Struct>> variables;
    std::map<std::string, std::shared_ptr<Function>> functions;
};

////////////////////////////////
////////////////////////////////
// Expression level
struct Operand {
    virtual ~Operand() = default;

    virtual std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>& operands,
                                      int index) = 0;

    virtual Struct evaluate(const Scope& scope) const = 0;

    virtual Struct assign(const Scope&, const Struct&) {
        fatal("Operand::assign shall not be called");
        return {};
    }

    virtual int get_precedence() const = 0;
    virtual void set_precedence(int prec) = 0;
};

struct BinaryOp : Operand {
    std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>& operands,
                              int index) override {
        a = operands[index - 1];
        b = operands[index + 1];
        return {index - 1, index + 1};
    }

    std::shared_ptr<Operand> a, b;
};

struct PreUnaryOp : Operand {
    std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>& operands,
                              int index) override {
        operand = operands[index + 1];
        return {index + 1};
    }

    std::shared_ptr<Operand> operand;
};

struct PostUnaryOp : Operand {
    std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>& operands,
                              int index) override {
        operand = operands[index - 1];
        return {index - 1};
    }

    std::shared_ptr<Operand> operand;
};

struct NumberLiteral : Operand {
    NumberLiteral(int value) : value(value){};

    std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>&, int) override {
        return {};
    }

    Struct evaluate(const Scope&) const override {
        return value;
    }

    int get_precedence() const override {
        return precedence;
    }
    void set_precedence(int prec) override {
        precedence = prec;
    }

    int precedence = 10;
    int value;
};

struct VariableOp : Operand {
    VariableOp(std::string name) : name(name){};

    std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>&, int) override {
        return {};
    }

    Struct evaluate(const Scope& scope) const override {
        return *scope.find_variable(name);
    }

    int get_precedence() const override {
        return precedence;
    }
    void set_precedence(int prec) override {
        precedence = prec;
    }

    Struct assign(const Scope& scope, const Struct& value) override {
        return *scope.find_variable(name) = value;
    }

    int precedence = 10;
    std::string name;
};

struct Assignment : BinaryOp {
    using BinaryOp::BinaryOp;

    Struct evaluate(const Scope& scope) const override {
        return a->assign(scope, b->evaluate(scope));
    }

    int get_precedence() const override {
        return precedence;
    }
    void set_precedence(int prec) override {
        precedence = prec;
    }
    int precedence = 0;
};

struct Addition : BinaryOp {
    using BinaryOp::BinaryOp;

    Struct evaluate(const Scope& scope) const override {
        return a->evaluate(scope) + b->evaluate(scope);
    }

    int get_precedence() const override {
        return precedence;
    }
    void set_precedence(int prec) override {
        precedence = prec;
    }

    int precedence = 4;
};

struct Subtraction : BinaryOp {
    using BinaryOp::BinaryOp;

    Struct evaluate(const Scope& scope) const override {
        return a->evaluate(scope) - b->evaluate(scope);
    }

    int get_precedence() const override {
        return precedence;
    }
    void set_precedence(int prec) override {
        precedence = prec;
    }

    int precedence = 4;
};

struct Multiplication : BinaryOp {
    using BinaryOp::BinaryOp;

    Struct evaluate(const Scope& scope) const override {
        return a->evaluate(scope) * b->evaluate(scope);
    }

    int get_precedence() const override {
        return precedence;
    }
    void set_precedence(int prec) override {
        precedence = prec;
    }
    int precedence = 5;
};

struct Division : BinaryOp {
    using BinaryOp::BinaryOp;

    Struct evaluate(const Scope& scope) const override {
        return a->evaluate(scope) / b->evaluate(scope);
    }

    int get_precedence() const override {
        return precedence;
    }
    void set_precedence(int prec) override {
        precedence = prec;
    }
    int precedence = 5;
};

struct PostIncrement : PostUnaryOp {
    using PostUnaryOp::PostUnaryOp;

    Struct evaluate(const Scope& scope) const override {
        auto old = operand->evaluate(scope);
        operand->assign(scope, ++operand->evaluate(scope));
        return old;
    }

    int get_precedence() const override {
        return precedence;
    }
    void set_precedence(int prec) override {
        precedence = prec;
    }

    int precedence = 8;
};

struct PostDecrement : PostUnaryOp {
    using PostUnaryOp::PostUnaryOp;

    Struct evaluate(const Scope& scope) const override {
        auto old = operand->evaluate(scope);
        operand->assign(scope, --operand->evaluate(scope));
        return old;
    }

    int get_precedence() const override {
        return precedence;
    }
    void set_precedence(int prec) override {
        precedence = prec;
    }

    int precedence = 8;
};

struct PreIncrement : PreUnaryOp {
    using PreUnaryOp::PreUnaryOp;

    Struct evaluate(const Scope& scope) const override {
        return operand->assign(scope, ++operand->evaluate(scope));
    }

    int get_precedence() const override {
        return precedence;
    }
    void set_precedence(int prec) override {
        precedence = prec;
    }

    int precedence = 8;
};

struct PreDecrement : PreUnaryOp {
    using PreUnaryOp::PreUnaryOp;

    Struct evaluate(const Scope& scope) const override {
        return operand->assign(scope, --operand->evaluate(scope));
    }

    int get_precedence() const override {
        return precedence;
    }
    void set_precedence(int prec) override {
        precedence = prec;
    }

    int precedence = 8;
};

struct LessThan : BinaryOp {
    using BinaryOp::BinaryOp;

    Struct evaluate(const Scope& scope) const override {
        return a->evaluate(scope) < b->evaluate(scope);
    }

    int get_precedence() const override {
        return precedence;
    }
    void set_precedence(int prec) override {
        precedence = prec;
    }
    int precedence = 2;
};

struct LessEqual : BinaryOp {
    using BinaryOp::BinaryOp;

    Struct evaluate(const Scope& scope) const override {
        return a->evaluate(scope) <= b->evaluate(scope);
    }

    int get_precedence() const override {
        return precedence;
    }
    void set_precedence(int prec) override {
        precedence = prec;
    }
    int precedence = 2;
};

struct GreaterThan : BinaryOp {
    using BinaryOp::BinaryOp;

    Struct evaluate(const Scope& scope) const override {
        return a->evaluate(scope) > b->evaluate(scope);
    }

    int get_precedence() const override {
        return precedence;
    }
    void set_precedence(int prec) override {
        precedence = prec;
    }
    int precedence = 2;
};

struct GreaterEqual : BinaryOp {
    using BinaryOp::BinaryOp;

    Struct evaluate(const Scope& scope) const override {
        return a->evaluate(scope) >= b->evaluate(scope);
    }

    int get_precedence() const override {
        return precedence;
    }
    void set_precedence(int prec) override {
        precedence = prec;
    }
    int precedence = 2;
};

struct Equal : BinaryOp {
    using BinaryOp::BinaryOp;

    Struct evaluate(const Scope& scope) const override {
        return a->evaluate(scope) == b->evaluate(scope);
    }

    int get_precedence() const override {
        return precedence;
    }
    void set_precedence(int prec) override {
        precedence = prec;
    }
    int precedence = 2;
};

struct NotEqual : BinaryOp {
    using BinaryOp::BinaryOp;

    Struct evaluate(const Scope& scope) const override {
        return a->evaluate(scope) != b->evaluate(scope);
    }

    int get_precedence() const override {
        return precedence;
    }
    void set_precedence(int prec) override {
        precedence = prec;
    }
    int precedence = 2;
};

struct LeftParenthese : Operand {
    std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>&, int) override {
        fatal("LeftParenthese::collapse() shall not be called");
        return {};
    }
    Struct evaluate(const Scope&) const override {
        fatal("LeftParenthese::evaluate() shall not be called");
        return {};
    }
    int get_precedence() const override {
        return 0;
    }
    void set_precedence(int) override {
        fatal("LeftParenthese::set_precedence() shall not be called");
    }
};

struct RightParenthese : Operand {
    std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>&, int) override {
        fatal("RightParenthese::collapse() shall not be called");
        return {};
    }
    Struct evaluate(const Scope&) const override {
        fatal("RightParenthese::evaluate() shall not be called");
        return {};
    }
    int get_precedence() const override {
        return 0;
    }
    void set_precedence(int) override {
        fatal("RightParenthese::set_precedence() shall not be called");
    }
};

////////////////////////////////
////////////////////////////////
// Statement level
struct Expression : Statement {
    void apply_parenthese();
    void collapse();

    Struct operator()(const Scope& scope) const {
        LLC_CHECK(operands.size() == 1);
        return operands[0]->evaluate(scope);
    }

    Struct run(Scope& scope) override {
        return this->operator()(scope);
    }

    std::vector<std::shared_ptr<Operand>> operands;
};

inline Struct Function::run(Scope& scope, std::vector<Expression> args) const {
    LLC_CHECK(parameters.size() == args.size());
    LLC_CHECK(definition != nullptr);

    for (int i = 0; i < (int)args.size(); i++)
        definition->variables[parameters[i]] = std::make_shared<Struct>(args[i](scope));

    return definition->run(scope);
}

struct FunctionCall : Statement {
    Struct run(Scope& scope) override {
        LLC_CHECK(function != nullptr);
        return function->run(scope, arguments);
    }

    std::shared_ptr<Function> function;
    std::vector<Expression> arguments;
};

struct IfElseChain : Statement {
    IfElseChain(std::vector<Expression> conditions, std::vector<std::shared_ptr<Statement>> actions)
        : conditions(conditions), actions(actions){};

    Struct run(Scope& scope) override {
        LLC_CHECK(conditions.size() == actions.size() || conditions.size() == actions.size() - 1);
        for (int i = 0; i < (int)conditions.size(); i++) {
            if (conditions[i](scope)) {
                actions[i]->run(scope);
                return {};
            }
        }
        if (conditions.size() == actions.size() - 1)
            actions.back()->run(scope);
        return {};
    }

    std::vector<Expression> conditions;
    std::vector<std::shared_ptr<Statement>> actions;
};

struct For : Statement {
    For(Expression condition, Expression updation, std::shared_ptr<Statement> action)
        : condition(condition), updation(updation), action(action){};

    Struct run(Scope& scope) override {
        for (; condition(scope); updation(scope))
            action->run(scope);
        return {};
    }

    Expression condition, updation;
    std::shared_ptr<Statement> action;
};

struct While : Statement {
    While(Expression condition, std::shared_ptr<Statement> action)
        : condition(condition), action(action){};

    Struct run(Scope& scope) override {
        while (condition(scope))
            action->run(scope);
        return {};
    }

    Expression condition;
    std::shared_ptr<Statement> action;
};

struct Print : Statement {
    Print(Expression expression) : expression(expression){};

    Struct run(Scope& scope) override {
        print(expression(scope).valuei);
        return {};
    }

    Expression expression;
};

using Program = std::shared_ptr<Scope>;

}  // namespace llc

#endif  // LLC_TYPES_H