#ifndef LLC_TYPES_H
#define LLC_TYPES_H

#include <llc/defines.h>

#include <algorithm>
#include <optional>
#include <sstream>
#include <memory>
#include <map>

namespace llc {

constexpr int max_precedence = 10;
constexpr int int_uninitialized = std::numeric_limits<int>::min();

struct Exception {
    Exception(std::string message, int line, int column)
        : message(std::move(message)), line(line), column(column){};

    std::string message;
    int line, column;
};

struct Statement;
struct Elementary;
struct ControlFlow;
struct Scope;

using StatementGroup = std::vector<Statement>;
using ElementaryGroup = std::vector<std::shared_ptr<Elementary>>;
using Expression = Elementary;

struct Statement {
    Statement(std::shared_ptr<Expression> expr)
        : expression(expr), controlflow(nullptr), scope(nullptr), type(Type::Expression){};
    Statement(std::shared_ptr<ControlFlow> cf)
        : expression(nullptr), controlflow(cf), scope(nullptr), type(Type::ControlFlow){};
    Statement(std::shared_ptr<Scope> scope)
        : expression(nullptr), controlflow(nullptr), scope(scope), type(Type::Scope){};

    bool is_expression() const {
        return type == Type::Expression;
    }
    bool is_controlflow() const {
        return type == Type::ControlFlow;
    }
    bool is_scope() const {
        return type == Type::Scope;
    }

    std::shared_ptr<Expression> expression;
    std::shared_ptr<ControlFlow> controlflow;
    std::shared_ptr<Scope> scope;
    enum class Type { Expression, ControlFlow, Scope } type;
};

struct Scope {
    void execute();
    int* search_symbol(const std::string& name);

    std::vector<Statement> statements;
    std::shared_ptr<Scope> parent_scope;
    std::map<std::string, int> symbols;
};

struct Elementary {
    static Elementary* create(Token type, int precedence_bias);

    Elementary(int precendence, int line, int column)
        : precendence(precendence), line(line), column(column) {
    }
    virtual ~Elementary() = default;

    virtual std::vector<int> construct(Scope& scope, const ElementaryGroup& eg, int index) = 0;
    virtual std::optional<int> execute(Scope& scope) const = 0;

    virtual void throw_error(const std::string& message) const {
        throw Exception(message, line, column);
    };

    std::string type_name;
    int precendence;
    int line, column;
};

struct ControlFlow {
    static ControlFlow* create(Token type);

    ControlFlow(int line, int column) : line(line), column(column) {
    }
    virtual ~ControlFlow() = default;

    virtual std::vector<int> construct(Scope& scope, const StatementGroup& sg, int index) = 0;
    virtual void execute(Scope& scope) const = 0;

    virtual void throw_error(const std::string& message) const {
        throw Exception(message, line, column);
    };

    std::string type_name;
    int line, column;
};

void Scope::execute() {
    for (const auto& statement : statements) {
        if (statement.is_expression())
            statement.expression->execute(*this);
        else if (statement.is_controlflow())
            statement.controlflow->execute(*this);
        else if (statement.is_scope())
            statement.scope->execute();
    }
}

int* Scope::search_symbol(const std::string& name) {
    auto iter = symbols.find(name);
    if (iter != symbols.end())
        return &iter->second;
    else if (parent_scope)
        return parent_scope->search_symbol(name);
    else
        return nullptr;
}

////////////////////////////////////////////////
// Elementary
struct Type : Elementary {
    using Elementary::Elementary;

  protected:
    std::shared_ptr<Elementary> variable;
};

struct Number : Elementary {
    Number(int value, int precendence, int line, int column)
        : Elementary(precendence, line, column), value(value){};

    virtual std::vector<int> construct(Scope&, const ElementaryGroup&, int) override {
        return {};
    }
    virtual std::optional<int> execute(Scope&) const override {
        return value;
    }

    int value;
};

struct LeftUnaryOp : Elementary {
    using Elementary::Elementary;

    virtual std::vector<int> construct(Scope&, const ElementaryGroup& eg, int index) override {
        LLC_INVOKE_IF(index + 1 >= (int)eg.size(), throw_error("syntax error: missing rhs"));
        right = eg[index + 1];
        return {index + 1};
    }

  protected:
    std::shared_ptr<Elementary> right;
};

struct RightUnaryOp : Elementary {
    using Elementary::Elementary;
    virtual std::vector<int> construct(Scope&, const ElementaryGroup& eg, int index) override {
        LLC_INVOKE_IF(index - 1 < 0, throw_error("syntax error: missing lhs"));
        left = eg[index - 1];
        return {index - 1};
    }

  protected:
    std::shared_ptr<Elementary> left;
};

struct BinaryOp : Elementary {
    using Elementary::Elementary;
    virtual std::vector<int> construct(Scope&, const ElementaryGroup& eg, int index) override {
        LLC_INVOKE_IF(index - 1 < 0, throw_error("syntax error: missing lhs"));
        LLC_INVOKE_IF(index + 1 >= (int)eg.size(), throw_error("syntax error: missing rhs"));
        left = eg[index - 1];
        right = eg[index + 1];
        return {index - 1, index + 1};
    }

  protected:
    std::shared_ptr<Elementary> left;
    std::shared_ptr<Elementary> right;
};

struct Variable : Elementary {
    Variable(std::string symbol, int precendence, int line, int column)
        : Elementary(precendence, line, column), symbol(symbol) {
    }
    virtual std::vector<int> construct(Scope& scope, const ElementaryGroup&, int) override {
        if (scope.search_symbol(symbol) == nullptr)
            throw_error("syntax error: variable can only be constructed by type");
        return {};
    }

    virtual std::optional<int> execute(Scope& scope) const override {
        int* value = scope.search_symbol(symbol);
        if (value == nullptr)
            throw_error("variable \"" + symbol + "\" is not declared");
        if (*value == int_uninitialized)
            throw_error("variable \"" + symbol + "\" is used without being initialized");
        return *value;
    }

    std::string symbol;
};

struct Int : Type {
    using Type::Type;

    virtual std::vector<int> construct(Scope& scope, const ElementaryGroup& eg,
                                       int index) override {
        LLC_INVOKE_IF(index + 1 == (int)eg.size(),
                      throw_error("syntax error: expect variable name after type"));
        Variable* variable = dynamic_cast<Variable*>(eg[index + 1].get());
        LLC_INVOKE_IF(variable == nullptr,
                      throw_error("syntax error: expect variable declaration after type"));
        scope.symbols.insert({variable->symbol, int_uninitialized});
        return {index};
    }
    virtual std::optional<int> execute(Scope&) const override {
        throw_error("\"int\" should not be executeed");
        return std::nullopt;
    }
};

struct Assign : BinaryOp {
    using BinaryOp::BinaryOp;

    virtual std::vector<int> construct(Scope&, const ElementaryGroup& eg, int index) override {
        LLC_INVOKE_IF(index == 0, throw_error("syntax error: missing lhs"));
        LLC_INVOKE_IF(index + 1 == (int)eg.size(), throw_error("syntax error: missing rhs"));
        Variable* variable = dynamic_cast<Variable*>(eg[index - 1].get());
        LLC_INVOKE_IF(variable == nullptr, throw_error("syntax error: expect variable on lhs"));
        symbol = variable->symbol;
        expression = eg[index + 1];
        return {index - 1, index + 1};
    }
    virtual std::optional<int> execute(Scope& scope) const override {
        int* value = scope.search_symbol(symbol);
        LLC_CHECK(value != nullptr);
        LLC_CHECK(expression != nullptr);
        if (auto ret = expression->execute(scope))
            *value = *ret;
        else
            throw_error("type error: rhs does not return a value");
        return *value;
    }

    std::string symbol;
    std::shared_ptr<Expression> expression = nullptr;
};

struct Add : BinaryOp {
    using BinaryOp::BinaryOp;

    virtual std::optional<int> execute(Scope& scope) const override {
        if (auto l = left->execute(scope)) {
            if (auto r = right->execute(scope)) {
                return *l + *r;
            } else {
                throw_error("type error: rhs does not return a value");
                return std::nullopt;
            }
        } else {
            throw_error("type error: lhs does not return a value");
            return std::nullopt;
        }
    }
};

struct Substract : BinaryOp {
    using BinaryOp::BinaryOp;

    virtual std::optional<int> execute(Scope& scope) const override {
        if (auto l = left->execute(scope)) {
            if (auto r = right->execute(scope)) {
                return *l - *r;
            } else {
                throw_error("type error: rhs does not return a value");
                return std::nullopt;
            }
        } else {
            throw_error("type error: lhs does not return a value");
            return std::nullopt;
        }
    }
};

struct Multiply : BinaryOp {
    using BinaryOp::BinaryOp;

    virtual std::optional<int> execute(Scope& scope) const override {
        if (auto l = left->execute(scope)) {
            if (auto r = right->execute(scope)) {
                return *l * *r;
            } else {
                throw_error("type error: rhs does not return a value");
                return std::nullopt;
            }
        } else {
            throw_error("type error: lhs does not return a value");
            return std::nullopt;
        }
    }
};

struct Divide : BinaryOp {
    using BinaryOp::BinaryOp;

    virtual std::optional<int> execute(Scope& scope) const override {
        if (auto l = left->execute(scope)) {
            if (auto r = right->execute(scope)) {
                return *l / *r;
            } else {
                throw_error("type error: rhs does not return a value");
                return std::nullopt;
            }
        } else {
            throw_error("type error: lhs does not return a value");
            return std::nullopt;
        }
    }
};

struct Print : LeftUnaryOp {
    using LeftUnaryOp::LeftUnaryOp;

    virtual std::optional<int> execute(Scope& scope) const override {
        if (auto r = right->execute(scope))
            print(*r);
        else
            throw_error("type error: rhs does not return a value");
        return std::nullopt;
    }
};

struct LessThan : BinaryOp {
    using BinaryOp::BinaryOp;

    virtual std::optional<int> execute(Scope& scope) const override {
        if (auto l = left->execute(scope)) {
            if (auto r = right->execute(scope)) {
                return *l < *r;
            } else {
                throw_error("type error: rhs does not return a value");
                return std::nullopt;
            }
        } else {
            throw_error("type error: lhs does not return a value");
            return std::nullopt;
        }
    }
};
struct LessEqual : BinaryOp {
    using BinaryOp::BinaryOp;

    virtual std::optional<int> execute(Scope& scope) const override {
        if (auto l = left->execute(scope)) {
            if (auto r = right->execute(scope)) {
                return *l <= *r;
            } else {
                throw_error("type error: rhs does not return a value");
                return std::nullopt;
            }
        } else {
            throw_error("type error: lhs does not return a value");
            return std::nullopt;
        }
    }
};
struct GreaterThan : BinaryOp {
    using BinaryOp::BinaryOp;

    virtual std::optional<int> execute(Scope& scope) const override {
        if (auto l = left->execute(scope)) {
            if (auto r = right->execute(scope)) {
                return *l > *r;
            } else {
                throw_error("type error: rhs does not return a value");
                return std::nullopt;
            }
        } else {
            throw_error("type error: lhs does not return a value");
            return std::nullopt;
        }
    }
};
struct GreaterEqual : BinaryOp {
    using BinaryOp::BinaryOp;

    virtual std::optional<int> execute(Scope& scope) const override {
        if (auto l = left->execute(scope)) {
            if (auto r = right->execute(scope)) {
                return *l >= *r;
            } else {
                throw_error("type error: rhs does not return a value");
                return std::nullopt;
            }
        } else {
            throw_error("type error: lhs does not return a value");
            return std::nullopt;
        }
    }
};
struct Equal : BinaryOp {
    using BinaryOp::BinaryOp;

    virtual std::optional<int> execute(Scope& scope) const override {
        if (auto l = left->execute(scope)) {
            if (auto r = right->execute(scope)) {
                return *l == *r;
            } else {
                throw_error("type error: rhs does not return a value");
                return std::nullopt;
            }
        } else {
            throw_error("type error: lhs does not return a value");
            return std::nullopt;
        }
    }
};
struct NotEqual : BinaryOp {
    using BinaryOp::BinaryOp;

    virtual std::optional<int> execute(Scope& scope) const override {
        if (auto l = left->execute(scope)) {
            if (auto r = right->execute(scope)) {
                return *l != *r;
            } else {
                throw_error("type error: rhs does not return a value");
                return std::nullopt;
            }
        } else {
            throw_error("type error: lhs does not return a value");
            return std::nullopt;
        }
    }
};
struct Increment : Elementary {
    using Elementary::Elementary;

    virtual std::vector<int> construct(Scope&, const ElementaryGroup& eg, int index) override {
        if (index + 1 < (int)eg.size() && dynamic_cast<Variable*>(eg[index + 1].get())) {
            is_post_increment = true;
            symbol = dynamic_cast<Variable*>(eg[index + 1].get())->symbol;
            return {index + 1};
        } else if (index - 1 >= 0 && dynamic_cast<Variable*>(eg[index - 1].get())) {
            is_post_increment = false;
            symbol = dynamic_cast<Variable*>(eg[index - 1].get())->symbol;
            return {index - 1};
        } else {
            throw_error("missing variable");
            return {};
        }
    }
    virtual std::optional<int> execute(Scope& scope) const override {
        if (int* value = scope.search_symbol(symbol)) {
            return is_post_increment ? (*value)++ : ++(*value);
        } else {
            throw_error("cannot find symbol \"" + symbol + "\"");
            return std::nullopt;
        }
    }

    bool is_post_increment = false;
    std::string symbol;
};
struct Decrement : Elementary {
    using Elementary::Elementary;

    virtual std::vector<int> construct(Scope&, const ElementaryGroup& eg, int index) override {
        if (index + 1 < (int)eg.size() && dynamic_cast<Variable*>(eg[index + 1].get())) {
            is_post_decrement = true;
            symbol = dynamic_cast<Variable*>(eg[index + 1].get())->symbol;
            return {index + 1};
        } else if (index - 1 >= 0 && dynamic_cast<Variable*>(eg[index - 1].get())) {
            is_post_decrement = false;
            symbol = dynamic_cast<Variable*>(eg[index - 1].get())->symbol;
            return {index - 1};
        } else {
            throw_error("missing variable");
            return {};
        }
    }
    virtual std::optional<int> execute(Scope& scope) const override {
        if (int* value = scope.search_symbol(symbol)) {
            return is_post_decrement ? (*value)-- : --(*value);
        } else {
            throw_error("cannot find symbol \"" + symbol + "\"");
            return std::nullopt;
        }
    }

    bool is_post_decrement = false;
    std::string symbol;
};

Elementary* Elementary::create(Token token, int bias) {
    Elementary* ptr = nullptr;
    switch (token.type) {
    case TokenType::Number:
        ptr = new Number(token.value, 10 + bias, token.line, token.column);
        break;
    case TokenType::Plus: ptr = new Add(5 + bias, token.line, token.column); break;
    case TokenType::Minus: ptr = new Substract(5 + bias, token.line, token.column); break;
    case TokenType::Star: ptr = new Multiply(6 + bias, token.line, token.column); break;
    case TokenType::ForwardSlash: ptr = new Divide(6 + bias, token.line, token.column); break;
    case TokenType::Print: ptr = new Print(0 + bias, token.line, token.column); break;
    case TokenType::Int: ptr = new Int(10 + bias, token.line, token.column); break;
    case TokenType::Variable:
        ptr = new Variable(token.identifier, 10 + bias, token.line, token.column);
        break;
    case TokenType::Assign: ptr = new Assign(0 + bias, token.line, token.column); break;
    case TokenType::GreaterThan: ptr = new GreaterThan(2 + bias, token.line, token.column); break;
    case TokenType::GreaterEqual: ptr = new GreaterEqual(2 + bias, token.line, token.column); break;
    case TokenType::LessThan: ptr = new LessThan(2 + bias, token.line, token.column); break;
    case TokenType::LessEqual: ptr = new LessEqual(2 + bias, token.line, token.column); break;
    case TokenType::NotEqual: ptr = new NotEqual(2 + bias, token.line, token.column); break;
    case TokenType::Equal: ptr = new Equal(2 + bias, token.line, token.column); break;
    case TokenType::Increment: ptr = new Increment(8 + bias, token.line, token.column); break;
    case TokenType::Decrement: ptr = new Decrement(8 + bias, token.line, token.column); break;
    default:
        throw Exception("invaild token type \"" + std::string(enum_to_name(token.type)) + "\"",
                        token.line, token.column);
    }
    ptr->type_name = enum_to_name(token.type);
    return ptr;
}

////////////////////////////////////////////////
// Control Flow
struct ElseIf : ControlFlow {
    using ControlFlow::ControlFlow;

    virtual std::vector<int> construct(Scope&, const StatementGroup&, int) override {
        throw_error("else if cannot appear alone");
        return {};
    }
    virtual void execute(Scope&) const override {
        throw_error("else if cannot appear alone");
    }
};
struct Else : ControlFlow {
    using ControlFlow::ControlFlow;

    virtual std::vector<int> construct(Scope&, const StatementGroup&, int) override {
        throw_error("else cannot appear alone");
        return {};
    }
    virtual void execute(Scope&) const override {
        throw_error("else cannot appear alone");
    }
};
struct If : ControlFlow {
    using ControlFlow::ControlFlow;

    virtual std::vector<int> construct(Scope&, const StatementGroup& sg, int index) override {
        std::vector<int> merged;
        merged.push_back(index + 1);
        merged.push_back(index + 2);
        LLC_CHECK(index + 2 < (int)sg.size());
        LLC_CHECK(sg[index + 1].is_expression());
        LLC_CHECK(sg[index + 2].is_scope());
        expressions.push_back(sg[index + 1].expression);
        actions.push_back(sg[index + 2].scope);

        bool last_is_scope = true;
        bool last_is_controlflow = false;
        for (int i = index + 3; i < (int)sg.size(); i++) {
            if (sg[i].is_scope()) {
                if (last_is_scope)
                    break;
                last_is_controlflow = false;
                last_is_scope = true;
                merged.push_back(i);
                actions.push_back(sg[i].scope);
            } else if (sg[i].is_controlflow()) {
                if (last_is_controlflow)
                    throw_error("syntax error: two consecutive if/else");
                last_is_scope = false;
                merged.push_back(i);
                i++;
                if (sg[i].is_expression()) {
                    expressions.push_back(sg[i].expression);
                } else {
                    actions.push_back(sg[i].scope);
                }
                merged.push_back(i);
                last_is_controlflow = true;
            }
        }
        return merged;
    }
    virtual void execute(Scope& scope) const override {
        LLC_CHECK(actions.size() == expressions.size() || actions.size() == expressions.size() + 1);
        for (int i = 0; i < (int)expressions.size(); i++) {
            if (*expressions[i]->execute(scope)) {
                actions[i]->execute();
                return;
            }
        }
        if (actions.size() == expressions.size() + 1)
            actions.back()->execute();
    }

    std::vector<std::shared_ptr<Expression>> expressions;
    std::vector<std::shared_ptr<Scope>> actions;
};
struct For : ControlFlow {
    using ControlFlow::ControlFlow;

    virtual std::vector<int> construct(Scope&, const StatementGroup& sg, int index) override {
        LLC_INVOKE_IF(index + 1 >= (int)sg.size() || !sg[index + 1].is_expression(),
                      throw_error("expect expression after \"for\""));
        LLC_INVOKE_IF(index + 2 >= (int)sg.size() || !sg[index + 2].is_expression(),
                      throw_error("expect expression after \"for\""));
        LLC_INVOKE_IF(index + 3 >= (int)sg.size() || !sg[index + 3].is_expression(),
                      throw_error("expect expression after \"for\""));
        LLC_INVOKE_IF(index + 4 >= (int)sg.size() || !sg[index + 4].is_scope(),
                      throw_error("expect scope after \"for\""));
        initialization = sg[index + 1].expression;
        condition = sg[index + 2].expression;
        updation = sg[index + 3].expression;
        action = sg[index + 4].scope;
        return {index + 1, index + 2, index + 3, index + 4};
    }
    virtual void execute(Scope& scope) const override {
        LLC_CHECK(initialization != nullptr);
        LLC_CHECK(condition != nullptr);
        LLC_CHECK(updation != nullptr);
        LLC_CHECK(action != nullptr);
        for (initialization->execute(scope); *condition->execute(scope);
             *updation->execute(scope)) {
            action->execute();
        }
    }

    std::shared_ptr<Expression> initialization;
    std::shared_ptr<Expression> condition;
    std::shared_ptr<Expression> updation;
    std::shared_ptr<Scope> action;
};
struct While : ControlFlow {
    using ControlFlow::ControlFlow;

    virtual std::vector<int> construct(Scope&, const StatementGroup& sg, int index) override {
        LLC_INVOKE_IF(index + 1 >= (int)sg.size() || !sg[index + 1].is_expression(),
                      throw_error("expect expression after \"while\""));
        LLC_INVOKE_IF(index + 2 >= (int)sg.size() || !sg[index + 2].is_scope(),
                      throw_error("expect scope after \"while\""));
        condition = sg[index + 1].expression;
        action = sg[index + 2].scope;
        return {index + 1, index + 2};
    }
    virtual void execute(Scope& scope) const override {
        LLC_CHECK(condition != nullptr);
        LLC_CHECK(action != nullptr);
        while (*condition->execute(scope)) {
            action->execute();
        }
    }

    std::shared_ptr<Expression> condition;
    std::shared_ptr<Scope> action;
};

ControlFlow* ControlFlow::create(Token token) {
    ControlFlow* ptr = nullptr;
    switch (token.type) {
    case TokenType::If: ptr = new If(token.line, token.column); break;
    case TokenType::ElseIf: ptr = new ElseIf(token.line, token.column); break;
    case TokenType::Else: ptr = new Else(token.line, token.column); break;
    case TokenType::For: ptr = new For(token.line, token.column); break;
    case TokenType::While: ptr = new While(token.line, token.column); break;
    default:
        throw Exception("invaild token type \"" + std::string(enum_to_name(token.type)) + "\"",
                        token.line, token.column);
    }

    ptr->type_name = enum_to_name(token.type);
    return ptr;
}

}  // namespace llc

#endif  // LLC_TYPES_H