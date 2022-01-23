#ifndef LLC_TYPES_H
#define LLC_TYPES_H

#include <llc/defines.h>

#include <algorithm>
#include <optional>
#include <sstream>
#include <memory>
#include <vector>
#include <map>

namespace llc {

constexpr int max_precedence = 20;

struct Exception {
    Exception(std::string message, int line, int column)
        : message(std::move(message)), line(line), column(column){};

    std::string message;
    int line, column;
};

struct Struct {
    static Struct null() {
        Struct s;
        s.is_void = true;
        return s;
    }

    Struct() = default;
    Struct(float value) : value(value){};
    Struct(const Struct& rhs) {
        if (rhs.is_void)
            fatal("cannot copy from void");
        id = rhs.id;
        value = rhs.value;
        ints = rhs.ints;
        floats = rhs.floats;
        members = rhs.members;
    }
    Struct& operator=(const Struct& rhs) {
        if (rhs.is_void)
            fatal("cannot copy from void");
        id = rhs.id;
        value = rhs.value;
        ints = rhs.ints;
        floats = rhs.floats;
        members = rhs.members;
        return *this;
    }

    int get_int(std::string name) {
        auto iter = ints.find(name);
        if (iter == ints.end())
            fatal(id, " does not have a member named ", name);
        return iter->second;
    }
    float get_float(std::string name) {
        auto iter = floats.find(name);
        if (iter == floats.end())
            fatal(id, " does not have a member named ", name);
        return iter->second;
    }
    const Struct& get_member(std::string name) {
        auto iter = members.find(name);
        if (iter == members.end())
            fatal(id, " does not have a member named ", name);
        return iter->second;
    }

    bool is_fundamental() const {
        return !is_void && ints.size() == 0 && floats.size() == 0 && members.size() == 0;
    }

    bool is_void = false;
    std::string id;
    float value = 0.0f;
    std::map<std::string, int> ints;
    std::map<std::string, float> floats;
    std::map<std::string, Struct> members;
};

struct Statement;
struct Elementary;
struct ControlFlow;
struct Scope;

using StatementGroup = std::vector<Statement>;
using ElementaryGroup = std::vector<std::shared_ptr<Elementary>>;
using Expression = Elementary;

using ReturnType = Struct;

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
    Scope() {
        types["int"] = Struct(0);
        types["float"] = Struct(0.0f);
    }
    void execute();
    Struct* search_variable(std::string name);
    Struct declare_varible(std::string type, std::string name);

    std::vector<Statement> statements;
    std::shared_ptr<Scope> parent_scope;

    std::map<std::string, Struct> types;
    std::map<std::string, Struct> variables;
};

struct Elementary {
    static Elementary* create(Token type, int precedence_bias);

    Elementary(int precendence, int line, int column)
        : precendence(precendence), line(line), column(column) {
    }
    virtual ~Elementary() = default;

    virtual std::vector<int> construct(Scope& scope, const ElementaryGroup& eg, int index) = 0;
    virtual ReturnType execute(Scope& scope) const = 0;

    virtual void syntax_error(const std::string& message) const {
        throw_error("syntax error:" + message);
    }
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

    virtual void syntax_error(const std::string& message) const {
        throw_error("syntax error:" + message);
    }
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

Struct* Scope::search_variable(std::string name) {
    auto iter = variables.find(name);
    if (iter != variables.end())
        return &iter->second;
    else if (parent_scope)
        return parent_scope->search_variable(name);
    else
        return nullptr;
}
Struct Scope::declare_varible(std::string type, std::string name) {
    auto iter = types.find(type);
    if (iter != types.end()) {
        if (variables.find(name) != variables.end())
            fatal("variable \"" + name + "\" is already declared at this scope");
        variables[name] = iter->second;
        return iter->second;
    } else if (parent_scope)
        return parent_scope->declare_varible(type, name);
    else {
        fatal("cannot declare variable with type \"", type, "\"");
        return {};
    }
}

////////////////////////////////////////////////
// Elementary
struct Number : Elementary {
    Number(float value, int precendence, int line, int column)
        : Elementary(precendence, line, column), value(value){};

    virtual std::vector<int> construct(Scope&, const ElementaryGroup&, int) override {
        return {};
    }
    virtual ReturnType execute(Scope&) const override {
        return value;
    }

    float value;
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
    Variable(std::string name, int precendence, int line, int column)
        : Elementary(precendence, line, column), name(name) {
    }
    virtual std::vector<int> construct(Scope& scope, const ElementaryGroup&, int) override {
        if (scope.search_variable(name) == nullptr)
            throw_error("syntax error: variable can only be constructed by type");
        return {};
    }

    virtual ReturnType execute(Scope& scope) const override {
        Struct* value = scope.search_variable(name);
        if (value == nullptr)
            throw_error("variable \"" + name + "\" is not declared");
        if (value->is_void)
            throw_error("variable \"" + name + "\" is void");
        return *value;
    }

    std::string name;
};

struct Type : Elementary {
    Type(std::string type_name, int precendence, int line, int column)
        : Elementary(precendence, line, column), type_name(type_name){};

    virtual std::vector<int> construct(Scope& scope, const ElementaryGroup& eg,
                                       int index) override {
        LLC_INVOKE_IF(index + 1 == (int)eg.size(),
                      throw_error("syntax error: expect variable name after type"));
        Variable* variable = dynamic_cast<Variable*>(eg[index + 1].get());
        LLC_INVOKE_IF(variable == nullptr,
                      throw_error("syntax error: expect variable declaration after type"));
        scope.declare_varible(type_name, variable->name);
        return {index};
    }
    virtual ReturnType execute(Scope&) const override {
        throw_error("\"type\" is not executable");
        return {};
    }

  protected:
    std::string type_name;
};

struct Assign : BinaryOp {
    using BinaryOp::BinaryOp;

    virtual std::vector<int> construct(Scope&, const ElementaryGroup& eg, int index) override {
        LLC_INVOKE_IF(index == 0, throw_error("syntax error: missing lhs"));
        LLC_INVOKE_IF(index + 1 == (int)eg.size(), throw_error("syntax error: missing rhs"));
        expression = eg[index + 1];
        Variable* variable = dynamic_cast<Variable*>(eg[index - 1].get());
        LLC_INVOKE_IF(variable == nullptr, throw_error("syntax error: expect variable on lhs"));
        variable_name = variable->name;
        return {index - 1, index + 1};
    }
    virtual ReturnType execute(Scope& scope) const override {
        Struct* value = scope.search_variable(variable_name);
        LLC_CHECK(value != nullptr);
        LLC_CHECK(expression != nullptr);
        *value = expression->execute(scope);
        return *value;
    }

    std::string variable_name;
    std::shared_ptr<Expression> expression;
};

struct Add : BinaryOp {
    using BinaryOp::BinaryOp;

    virtual ReturnType execute(Scope& scope) const override {
        auto value0 = left->execute(scope);
        auto value1 = right->execute(scope);
        if (!value0.is_fundamental() || !value1.is_fundamental())
            fatal("cannot add two struct");
        value0.value = value0.value + value1.value;
        return value0;
    }
};

struct Substract : BinaryOp {
    using BinaryOp::BinaryOp;

    virtual ReturnType execute(Scope& scope) const override {
        auto value0 = left->execute(scope);
        auto value1 = right->execute(scope);
        if (!value0.is_fundamental() || !value1.is_fundamental())
            fatal("cannot substract two struct");
        value0.value = value0.value - value1.value;
        return value0;
    }
};

struct Multiply : BinaryOp {
    using BinaryOp::BinaryOp;

    virtual ReturnType execute(Scope& scope) const override {
        auto value0 = left->execute(scope);
        auto value1 = right->execute(scope);
        if (!value0.is_fundamental() || !value1.is_fundamental())
            fatal("cannot multiply two struct");
        value0.value = value0.value * value1.value;
        return value0;
    }
};

struct Divide : BinaryOp {
    using BinaryOp::BinaryOp;

    virtual ReturnType execute(Scope& scope) const override {
        auto value0 = left->execute(scope);
        auto value1 = right->execute(scope);
        if (!value0.is_fundamental() || !value1.is_fundamental())
            fatal("cannot divide two struct");
        value0.value = value0.value / value1.value;
        return value0;
    }
};

struct Print : LeftUnaryOp {
    using LeftUnaryOp::LeftUnaryOp;

    virtual ReturnType execute(Scope& scope) const override {
        ReturnType ret = right->execute(scope);
        if (ret.is_fundamental())
            print(ret.value);
        else
            throw_error("type error: rhs does not return a value");
        return {};
    }
};

struct LessThan : BinaryOp {
    using BinaryOp::BinaryOp;

    virtual ReturnType execute(Scope& scope) const override {
        ReturnType value0 = left->execute(scope);
        ReturnType value1 = right->execute(scope);
        if (!value0.is_fundamental() || !value1.is_fundamental())
            fatal("cannot compare two struct");
        return value0.value < value1.value;
    }
};
struct LessEqual : BinaryOp {
    using BinaryOp::BinaryOp;

    virtual ReturnType execute(Scope& scope) const override {
        ReturnType value0 = left->execute(scope);
        ReturnType value1 = right->execute(scope);
        if (!value0.is_fundamental() || !value1.is_fundamental())
            fatal("cannot compare two struct");
        return value0.value <= value1.value;
    }
};
struct GreaterThan : BinaryOp {
    using BinaryOp::BinaryOp;

    virtual ReturnType execute(Scope& scope) const override {
        ReturnType value0 = left->execute(scope);
        ReturnType value1 = right->execute(scope);
        if (!value0.is_fundamental() || !value1.is_fundamental())
            fatal("cannot compare two struct");
        return value0.value > value1.value;
    }
};
struct GreaterEqual : BinaryOp {
    using BinaryOp::BinaryOp;

    virtual ReturnType execute(Scope& scope) const override {
        ReturnType value0 = left->execute(scope);
        ReturnType value1 = right->execute(scope);
        if (!value0.is_fundamental() || !value1.is_fundamental())
            fatal("cannot compare two struct");
        return value0.value >= value1.value;
    }
};

struct Equal : BinaryOp {
    using BinaryOp::BinaryOp;

    virtual ReturnType execute(Scope& scope) const override {
        ReturnType value0 = left->execute(scope);
        ReturnType value1 = right->execute(scope);
        if (!value0.is_fundamental() || !value1.is_fundamental())
            fatal("cannot compare two struct");
        return value0.value == value1.value;
    }
};
struct NotEqual : BinaryOp {
    using BinaryOp::BinaryOp;

    virtual ReturnType execute(Scope& scope) const override {
        ReturnType value0 = left->execute(scope);
        ReturnType value1 = right->execute(scope);
        if (!value0.is_fundamental() || !value1.is_fundamental())
            fatal("cannot compare two struct");
        return value0.value != value1.value;
    }
};

struct Increment : Elementary {
    using Elementary::Elementary;

    virtual std::vector<int> construct(Scope&, const ElementaryGroup& eg, int index) override {
        if (index + 1 < (int)eg.size() && dynamic_cast<Variable*>(eg[index + 1].get())) {
            is_post_increment = true;
            variable_name = dynamic_cast<Variable*>(eg[index + 1].get())->name;
            return {index + 1};
        } else if (index - 1 >= 0 && dynamic_cast<Variable*>(eg[index - 1].get())) {
            is_post_increment = false;
            variable_name = dynamic_cast<Variable*>(eg[index - 1].get())->name;
            return {index - 1};
        } else {
            throw_error("missing variable");
            return {};
        }
    }
    virtual ReturnType execute(Scope& scope) const override {
        Struct* object = scope.search_variable(variable_name);
        if (object == nullptr)
            throw_error("cannot find variable of name \"" + variable_name + "\"");
        return is_post_increment ? (object->value)++ : ++(object->value);
    }

    bool is_post_increment = false;
    std::string variable_name;
};
struct Decrement : Elementary {
    using Elementary::Elementary;

    virtual std::vector<int> construct(Scope&, const ElementaryGroup& eg, int index) override {
        if (index + 1 < (int)eg.size() && dynamic_cast<Variable*>(eg[index + 1].get())) {
            is_post_decrement = true;
            variable_name = dynamic_cast<Variable*>(eg[index + 1].get())->name;
            return {index + 1};
        } else if (index - 1 >= 0 && dynamic_cast<Variable*>(eg[index - 1].get())) {
            is_post_decrement = false;
            variable_name = dynamic_cast<Variable*>(eg[index - 1].get())->name;
            return {index - 1};
        } else {
            throw_error("missing variable");
            return {};
        }
    }
    virtual ReturnType execute(Scope& scope) const override {
        Struct* object = scope.search_variable(variable_name);
        if (object == nullptr)
            throw_error("cannot find variable of name \"" + variable_name + "\"");
        return is_post_decrement ? (object->value)-- : --(object->value);
    }

    bool is_post_decrement = false;
    std::string variable_name;
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
    case TokenType::Variable:
        ptr = new Variable(token.identifier, 10 + bias, token.line, token.column);
        break;
    case TokenType::Int: ptr = new Type("int", 10 + bias, token.line, token.column); break;
    case TokenType::Float: ptr = new Type("float", 10 + bias, token.line, token.column); break;
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
            if (expressions[i]->execute(scope).value) {
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
        for (initialization->execute(scope); condition->execute(scope).value;
             (void)updation->execute(scope).value) {
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
        while (condition->execute(scope).value) {
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