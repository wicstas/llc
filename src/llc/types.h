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

enum class TokenType : uint64_t {
    Number = 1ul << 0,
    Increment = 1ul << 1,
    Decrement = 1ul << 2,
    Plus = 1ul << 3,
    Minus = 1ul << 4,
    Star = 1ul << 5,
    ForwardSlash = 1ul << 6,
    LeftParenthese = 1ul << 7,
    RightParenthese = 1ul << 8,
    LeftCurlyBracket = 1ul << 9,
    RightCurlyBracket = 1ul << 10,
    Semicolon = 1ul << 11,
    Identifier = 1ul << 12,
    Dot = 1ul << 13,
    Comma = 1ul << 14,
    LessThan = 1ul << 15,
    LessEqual = 1ul << 16,
    GreaterThan = 1ul << 17,
    GreaterEqual = 1ul << 18,
    Equal = 1ul << 19,
    NotEqual = 1ul << 20,
    Assign = 1ul << 21,
    Exclaimation = 1ul << 22,
    Invalid = 1ul << 23,
    Eof = 1ul << 24,
    NumTokens = 1ul << 25
};

inline TokenType operator|(TokenType a, TokenType b) { return (TokenType)((uint64_t)a | (uint64_t)b); }
inline uint64_t operator&(TokenType a, TokenType b) { return (uint64_t)a & (uint64_t)b; }

inline std::string enum_to_string(TokenType type) {
    static const char* map[] = {"number", "++", "--", "+",          "-", "*",       "/",   "(",         ")",
                                "{",      "}",  ";",  "identifier", ".", ",",       "<",   "<=",        ">",
                                ">=",     "==", "!=", "=",          "!", "invalid", "eof", "num_tokens"};
    std::string str;
    for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); i++)
        if ((uint64_t(type) >> i) & 1ul)
            str += (std::string)map[i] + "|";

    if (str.back() == '|')
        str.pop_back();
    return str;
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
    Struct(float value) : value(value){};

    friend Struct operator+(Struct a, Struct b) {
        a.value += b.value;
        return a;
    }
    friend Struct operator-(Struct a, Struct b) {
        a.value -= b.value;
        return a;
    }
    friend Struct operator*(Struct a, Struct b) {
        a.value *= b.value;
        return a;
    }
    friend Struct operator/(Struct a, Struct b) {
        a.value /= b.value;
        return a;
    }
    friend bool operator<(Struct a, Struct b) { return a.value < b.value; }
    friend bool operator<=(Struct a, Struct b) { return a.value <= b.value; }
    friend bool operator>(Struct a, Struct b) { return a.value > b.value; }
    friend bool operator>=(Struct a, Struct b) { return a.value >= b.value; }
    friend bool operator==(Struct a, Struct b) { return a.value == b.value; }
    friend bool operator!=(Struct a, Struct b) { return a.value != b.value; }
    Struct operator++() {
        value++;
        return *this;
    }
    Struct operator--() {
        value--;
        return *this;
    }
    Struct operator++(int) {
        auto temp = *this;
        value++;
        return temp;
    }
    Struct operator--(int) {
        auto temp = *this;
        value--;
        return temp;
    }

    operator bool() const { return (bool)value; }

    bool is_return = false;
    float value = 0.0f;
    std::map<std::string, std::shared_ptr<Struct>> members;
};

struct Function {
    Struct run(Scope& scope, std::vector<Expression> args) const;

    Struct return_type;
    std::shared_ptr<Scope> definition;
    std::vector<std::string> parameters;
};

struct Statement {
    virtual ~Statement() = default;

    virtual Struct run(Scope& scope) = 0;
};

struct Scope : Statement {
    Scope();

    Struct run(Scope& scope);

    std::optional<Struct> find_type(std::string name) const;
    std::shared_ptr<Struct> find_variable(std::string name) const;
    std::shared_ptr<Function> find_function(std::string name) const;

    std::shared_ptr<Scope> parent;
    std::vector<std::shared_ptr<Statement>> statements;
    std::map<std::string, Struct> types;
    std::map<std::string, std::shared_ptr<Struct>> variables;
    std::map<std::string, std::shared_ptr<Function>> functions;
};

struct Operand {
    virtual ~Operand() = default;

    virtual std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>& operands, int index) = 0;

    virtual Struct evaluate(Scope& scope) = 0;

    virtual Struct assign(Scope&, const Struct&) {
        fatal("Operand::assign shall not be called");
        return {};
    }

    virtual int get_precedence() const = 0;
    virtual void set_precedence(int prec) = 0;
};

struct BinaryOp : Operand {
    std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>& operands, int index) override {
        LLC_CHECK(index - 1 >= 0);
        LLC_CHECK(index + 1 < (int)operands.size());
        a = operands[index - 1];
        b = operands[index + 1];
        return {index - 1, index + 1};
    }

    std::shared_ptr<Operand> a, b;
};

struct PreUnaryOp : Operand {
    std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>& operands, int index) override {
        LLC_CHECK(index + 1 >= 0);
        LLC_CHECK(index + 1 < (int)operands.size());
        operand = operands[index + 1];
        return {index + 1};
    }

    std::shared_ptr<Operand> operand;
};

struct PostUnaryOp : Operand {
    std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>& operands, int index) override {
        LLC_CHECK(index - 1 >= 0);
        LLC_CHECK(index - 1 < (int)operands.size());
        operand = operands[index - 1];
        return {index - 1};
    }

    std::shared_ptr<Operand> operand;
};

struct NumberLiteral : Operand {
    NumberLiteral(float value) : value(value){};

    std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>&, int) override { return {}; }

    Struct evaluate(Scope&) override { return value; }

    int get_precedence() const override { return precedence; }
    void set_precedence(int prec) override { precedence = prec; }

    int precedence = 10;
    float value;
};

struct Assignable {
    virtual std::shared_ptr<Struct> get(Scope& scope) const = 0;
};

struct VariableOp : Operand, Assignable {
    VariableOp(std::string name) : name(name){};

    std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>&, int) override { return {}; }

    Struct evaluate(Scope& scope) override {
        if (auto var = scope.find_variable(name))
            return *var;
        LLC_CHECK(false);
        return {};
    }

    int get_precedence() const override { return precedence; }
    void set_precedence(int prec) override { precedence = prec; }

    Struct assign(Scope& scope, const Struct& value) override {
        if (auto var = scope.find_variable(name))
            return *var = value;
        LLC_CHECK(false);
        return {};
    }
    std::shared_ptr<Struct> get(Scope& scope) const override {
        if (auto var = scope.find_variable(name))
            return var;
        LLC_CHECK(false);
        return nullptr;
    }

    int precedence = 10;
    std::string name;
};

struct StructMember : Operand {
    StructMember(std::string name) : name(name){};

    std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>&, int) override {
        fatal("StructMember::collapse() shall not be called");
        return {};
    }
    Struct evaluate(Scope&) override {
        fatal("StructMember::evaluate() shall not be called");
        return {};
    }
    int get_precedence() const override { return 0; }
    void set_precedence(int) override {}

    std::string name;
};

struct MemberAccess : BinaryOp, Assignable {
    std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>& operands, int index) override {
        LLC_CHECK(index - 1 >= 0);
        LLC_CHECK(index + 1 < (int)operands.size());
        a = operands[index - 1];
        b = operands[index + 1];
        return {index - 1, index + 1};
    }

    Struct evaluate(Scope& scope) override {
        auto variable = dynamic_cast<Assignable*>(a.get());
        auto member = dynamic_cast<StructMember*>(b.get());
        LLC_CHECK(variable != nullptr);
        LLC_CHECK(member != nullptr);
        LLC_CHECK(variable->get(scope)->members[member->name] != nullptr);
        return *variable->get(scope)->members[member->name];
    }
    Struct assign(Scope& scope, const Struct& value) override {
        auto variable = dynamic_cast<Assignable*>(a.get());
        auto member = dynamic_cast<StructMember*>(b.get());
        LLC_CHECK(variable != nullptr);
        LLC_CHECK(member != nullptr);
        LLC_CHECK(variable->get(scope)->members[member->name] != nullptr);
        return *variable->get(scope)->members[member->name] = value;
    }
    std::shared_ptr<Struct> get(Scope& scope) const override {
        auto variable = dynamic_cast<Assignable*>(a.get());
        auto member = dynamic_cast<StructMember*>(b.get());
        LLC_CHECK(variable != nullptr);
        LLC_CHECK(member != nullptr);
        LLC_CHECK(variable->get(scope)->members[member->name] != nullptr);
        return variable->get(scope)->members[member->name];
    }

    int get_precedence() const override { return precedence; }
    void set_precedence(int prec) override { precedence = prec; }
    int precedence = 8;
};

struct Assignment : BinaryOp {
    Struct evaluate(Scope& scope) override { return a->assign(scope, b->evaluate(scope)); }

    int get_precedence() const override { return precedence; }
    void set_precedence(int prec) override { precedence = prec; }
    int precedence = 0;
};

struct Addition : BinaryOp {
    Struct evaluate(Scope& scope) override { return a->evaluate(scope) + b->evaluate(scope); }

    int get_precedence() const override { return precedence; }
    void set_precedence(int prec) override { precedence = prec; }

    int precedence = 4;
};

struct Subtraction : BinaryOp {
    Struct evaluate(Scope& scope) override { return a->evaluate(scope) - b->evaluate(scope); }

    int get_precedence() const override { return precedence; }
    void set_precedence(int prec) override { precedence = prec; }

    int precedence = 4;
};

struct Multiplication : BinaryOp {
    Struct evaluate(Scope& scope) override { return a->evaluate(scope) * b->evaluate(scope); }

    int get_precedence() const override { return precedence; }
    void set_precedence(int prec) override { precedence = prec; }
    int precedence = 5;
};

struct Division : BinaryOp {
    Struct evaluate(Scope& scope) override { return a->evaluate(scope) / b->evaluate(scope); }

    int get_precedence() const override { return precedence; }
    void set_precedence(int prec) override { precedence = prec; }
    int precedence = 5;
};

struct PostIncrement : PostUnaryOp {
    Struct evaluate(Scope& scope) override {
        auto old = operand->evaluate(scope);
        operand->assign(scope, ++operand->evaluate(scope));
        return old;
    }

    int get_precedence() const override { return precedence; }
    void set_precedence(int prec) override { precedence = prec; }

    int precedence = 8;
};

struct PostDecrement : PostUnaryOp {
    Struct evaluate(Scope& scope) override {
        auto old = operand->evaluate(scope);
        operand->assign(scope, --operand->evaluate(scope));
        return old;
    }

    int get_precedence() const override { return precedence; }
    void set_precedence(int prec) override { precedence = prec; }

    int precedence = 8;
};

struct PreIncrement : PreUnaryOp {
    Struct evaluate(Scope& scope) override { return operand->assign(scope, ++operand->evaluate(scope)); }

    int get_precedence() const override { return precedence; }
    void set_precedence(int prec) override { precedence = prec; }

    int precedence = 8;
};

struct PreDecrement : PreUnaryOp {
    Struct evaluate(Scope& scope) override { return operand->assign(scope, --operand->evaluate(scope)); }

    int get_precedence() const override { return precedence; }
    void set_precedence(int prec) override { precedence = prec; }

    int precedence = 8;
};

struct LessThan : BinaryOp {
    Struct evaluate(Scope& scope) override { return a->evaluate(scope) < b->evaluate(scope); }

    int get_precedence() const override { return precedence; }
    void set_precedence(int prec) override { precedence = prec; }
    int precedence = 2;
};

struct LessEqual : BinaryOp {
    Struct evaluate(Scope& scope) override { return a->evaluate(scope) <= b->evaluate(scope); }

    int get_precedence() const override { return precedence; }
    void set_precedence(int prec) override { precedence = prec; }
    int precedence = 2;
};

struct GreaterThan : BinaryOp {
    Struct evaluate(Scope& scope) override { return a->evaluate(scope) > b->evaluate(scope); }

    int get_precedence() const override { return precedence; }
    void set_precedence(int prec) override { precedence = prec; }
    int precedence = 2;
};

struct GreaterEqual : BinaryOp {
    Struct evaluate(Scope& scope) override { return a->evaluate(scope) >= b->evaluate(scope); }

    int get_precedence() const override { return precedence; }
    void set_precedence(int prec) override { precedence = prec; }
    int precedence = 2;
};

struct Equal : BinaryOp {
    Struct evaluate(Scope& scope) override { return a->evaluate(scope) == b->evaluate(scope); }

    int get_precedence() const override { return precedence; }
    void set_precedence(int prec) override { precedence = prec; }
    int precedence = 2;
};

struct NotEqual : BinaryOp {
    Struct evaluate(Scope& scope) override { return a->evaluate(scope) != b->evaluate(scope); }

    int get_precedence() const override { return precedence; }
    void set_precedence(int prec) override { precedence = prec; }
    int precedence = 2;
};

struct LeftParenthese : Operand {
    std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>&, int) override {
        fatal("LeftParenthese::collapse() shall not be called");
        return {};
    }
    Struct evaluate(Scope&) override {
        fatal("LeftParenthese::evaluate() shall not be called");
        return {};
    }
    int get_precedence() const override { return 0; }
    void set_precedence(int) override {}
};

struct RightParenthese : Operand {
    std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>&, int) override {
        fatal("RightParenthese::collapse() shall not be called");
        return {};
    }
    Struct evaluate(Scope&) override {
        fatal("RightParenthese::evaluate() shall not be called");
        return {};
    }
    int get_precedence() const override { return 0; }
    void set_precedence(int) override {}
};

struct Expression : Statement {
    void apply_parenthese();
    void collapse();

    Struct operator()(Scope& scope) const {
        if (operands.size() == 0)
            return {};
        LLC_CHECK(operands.size() == 1);
        return operands[0]->evaluate(scope);
    }

    Struct run(Scope& scope) override { return this->operator()(scope); }

    std::vector<std::shared_ptr<Operand>> operands;
};

struct FunctionCall : Statement {
    Struct run(Scope& scope) override {
        LLC_CHECK(function != nullptr);
        return function->run(scope, arguments);
    }

    std::shared_ptr<Function> function;
    std::vector<Expression> arguments;
};

struct FunctionCallOp : Operand {
    FunctionCallOp(FunctionCall function) : function(function){};

    std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>&, int) override { return {}; }

    Struct evaluate(Scope& scope) override { return function.run(scope); }

    int get_precedence() const override { return precedence; }
    void set_precedence(int prec) override { precedence = prec; }

    int precedence = 10;
    FunctionCall function;
};

struct Return : Statement {
    Return(Expression expression) : expression(expression){};

    Struct run(Scope& scope) override { return expression(scope); }

    Expression expression;
};

struct IfElseChain : Statement {
    IfElseChain(std::vector<Expression> conditions, std::vector<std::shared_ptr<Scope>> actions)
        : conditions(conditions), actions(actions){};

    Struct run(Scope& scope) override;

    std::vector<Expression> conditions;
    std::vector<std::shared_ptr<Scope>> actions;
};

struct For : Statement {
    For(Expression condition, Expression updation, std::shared_ptr<Scope> internal_scope,
        std::shared_ptr<Scope> action)
        : condition(condition), updation(updation), internal_scope(internal_scope), action(action){};

    Struct run(Scope& scope) override;

    Expression condition, updation;
    std::shared_ptr<Scope> internal_scope, action;
};

struct While : Statement {
    While(Expression condition, std::shared_ptr<Scope> action) : condition(condition), action(action){};

    Struct run(Scope& scope) override;

    Expression condition;
    std::shared_ptr<Scope> action;
};

struct Print : Statement {
    Print(Expression expression) : expression(expression){};

    Struct run(Scope& scope) override;

    Expression expression;
};

using Program = std::shared_ptr<Scope>;

}  // namespace llc

#endif  // LLC_TYPES_H