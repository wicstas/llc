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
        : message(message), line(line), column(column){};

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
        *this = rhs;
    }
    Struct& operator=(const Struct& rhs);
    const Struct& operator[](std::string name) const;

    bool is_fundamental() const {
        return !is_void && members.size() == 0;
    }

    std::string id;
    bool is_void = false;

    float value = 0.0f;
    std::map<std::string, Struct> members;
};

struct Statement;
struct Elementary;
struct ControlFlow;
struct StructDecl;
struct Scope;

using StatementGroup = std::vector<Statement>;
using ElementaryGroup = std::vector<std::shared_ptr<Elementary>>;

using Expression = Elementary;
using ReturnType = Struct;

struct Statement {
    Statement(std::shared_ptr<Expression> expr)
        : expression(expr),
          controlflow(nullptr),
          struct_decl(nullptr),
          scope(nullptr),
          type(Type::Expression){};
    Statement(std::shared_ptr<ControlFlow> cf)
        : expression(nullptr),
          controlflow(cf),
          struct_decl(nullptr),
          scope(nullptr),
          type(Type::ControlFlow){};
    Statement(std::shared_ptr<StructDecl> sd)
        : expression(nullptr),
          controlflow(nullptr),
          struct_decl(sd),
          scope(nullptr),
          type(Type::StructDecl){};
    Statement(std::shared_ptr<Scope> scope)
        : expression(nullptr),
          controlflow(nullptr),
          struct_decl(nullptr),
          scope(scope),
          type(Type::Scope){};

    bool is_expression() const {
        return type == Type::Expression;
    }
    bool is_controlflow() const {
        return type == Type::ControlFlow;
    }
    bool is_struct_decl() const {
        return type == Type::StructDecl;
    }
    bool is_scope() const {
        return type == Type::Scope;
    }

    std::shared_ptr<Expression> expression;
    std::shared_ptr<ControlFlow> controlflow;
    std::shared_ptr<StructDecl> struct_decl;
    std::shared_ptr<Scope> scope;
    enum class Type { Expression, ControlFlow, StructDecl, Scope } type;
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

struct StructDecl {
    static StructDecl* create(Token token) {
        return new StructDecl(token.line, token.column);
    }

    StructDecl(int line, int column) : line(line), column(column) {
    }

    std::vector<int> construct(Scope& scope, const StatementGroup& sg, int index);

    int line, column;
};

struct Scope {
    Scope() {
        types["int"] = Struct(0);
        types["float"] = Struct(0.0f);
    }
    void execute();

    Struct* search_variable(std::string name);
    Struct declare_varible(std::string type, std::string name);

    std::shared_ptr<Scope> parent_scope;

    std::vector<Statement> statements;

    std::map<std::string, Struct> types;
    std::map<std::string, Struct> variables;
};

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

    virtual std::vector<int> construct(Scope&, const ElementaryGroup& eg, int index) override;

  protected:
    std::shared_ptr<Elementary> right;
};

struct RightUnaryOp : Elementary {
    using Elementary::Elementary;
    virtual std::vector<int> construct(Scope&, const ElementaryGroup& eg, int index) override;

  protected:
    std::shared_ptr<Elementary> left;
};

struct BinaryOp : Elementary {
    using Elementary::Elementary;
    virtual std::vector<int> construct(Scope&, const ElementaryGroup& eg, int index) override;

  protected:
    std::shared_ptr<Elementary> left;
    std::shared_ptr<Elementary> right;
};

struct Identifier : Elementary {
    Identifier(std::string name, int precendence, int line, int column)
        : Elementary(precendence, line, column), name(name) {
    }
    virtual std::vector<int> construct(Scope& scope, const ElementaryGroup&, int) override;

    virtual ReturnType execute(Scope& scope) const override;

    std::string name;
};

struct Type : Elementary {
    Type(std::string type_name, int precendence, int line, int column)
        : Elementary(precendence, line, column), type_name(type_name){};

    virtual std::vector<int> construct(Scope& scope, const ElementaryGroup& eg, int index) override;
    virtual ReturnType execute(Scope&) const override;

  protected:
    std::string type_name;
};

struct Assign : BinaryOp {
    using BinaryOp::BinaryOp;

    virtual std::vector<int> construct(Scope&, const ElementaryGroup& eg, int index) override;
    virtual ReturnType execute(Scope& scope) const override;

    std::string variable_name;
    std::shared_ptr<Expression> expression;
};

struct Add : BinaryOp {
    using BinaryOp::BinaryOp;

    virtual ReturnType execute(Scope& scope) const override;
};

struct Substract : BinaryOp {
    using BinaryOp::BinaryOp;

    virtual ReturnType execute(Scope& scope) const override;
};

struct Multiply : BinaryOp {
    using BinaryOp::BinaryOp;

    virtual ReturnType execute(Scope& scope) const override;
};

struct Divide : BinaryOp {
    using BinaryOp::BinaryOp;

    virtual ReturnType execute(Scope& scope) const override;
};

struct Print : LeftUnaryOp {
    using LeftUnaryOp::LeftUnaryOp;

    virtual ReturnType execute(Scope& scope) const override;
};

struct LessThan : BinaryOp {
    using BinaryOp::BinaryOp;

    virtual ReturnType execute(Scope& scope) const override;
};
struct LessEqual : BinaryOp {
    using BinaryOp::BinaryOp;

    virtual ReturnType execute(Scope& scope) const override;
};
struct GreaterThan : BinaryOp {
    using BinaryOp::BinaryOp;

    virtual ReturnType execute(Scope& scope) const override;
};
struct GreaterEqual : BinaryOp {
    using BinaryOp::BinaryOp;

    virtual ReturnType execute(Scope& scope) const override;
};

struct Equal : BinaryOp {
    using BinaryOp::BinaryOp;

    virtual ReturnType execute(Scope& scope) const override;
};
struct NotEqual : BinaryOp {
    using BinaryOp::BinaryOp;

    virtual ReturnType execute(Scope& scope) const override;
};

struct Increment : Elementary {
    using Elementary::Elementary;

    virtual std::vector<int> construct(Scope&, const ElementaryGroup& eg, int index) override;
    virtual ReturnType execute(Scope& scope) const override;

    bool is_post_increment = false;
    std::string variable_name;
};
struct Decrement : Elementary {
    using Elementary::Elementary;

    virtual std::vector<int> construct(Scope&, const ElementaryGroup& eg, int index) override;
    virtual ReturnType execute(Scope& scope) const override;

    bool is_post_decrement = false;
    std::string variable_name;
};
struct MemberAccess : Elementary {
    using Elementary::Elementary;

    virtual std::vector<int> construct(Scope&, const ElementaryGroup& eg, int index) override;
    virtual ReturnType execute(Scope& scope) const override;

    std::string variable_name;
    std::string field_name;
};

////////////////////////////////////////////////
// Control Flow
struct ElseIf : ControlFlow {
    using ControlFlow::ControlFlow;

    std::vector<int> construct(Scope&, const StatementGroup&, int) override;
    void execute(Scope&) const override;
};
struct Else : ControlFlow {
    using ControlFlow::ControlFlow;

    std::vector<int> construct(Scope&, const StatementGroup&, int) override;
    void execute(Scope&) const override;
};
struct If : ControlFlow {
    using ControlFlow::ControlFlow;

    virtual std::vector<int> construct(Scope&, const StatementGroup& sg, int index) override;
    virtual void execute(Scope& scope) const override;

    std::vector<std::shared_ptr<Expression>> expressions;
    std::vector<std::shared_ptr<Scope>> actions;
};
struct For : ControlFlow {
    using ControlFlow::ControlFlow;

    virtual std::vector<int> construct(Scope&, const StatementGroup& sg, int index) override;
    virtual void execute(Scope& scope) const override;

    std::shared_ptr<Expression> initialization;
    std::shared_ptr<Expression> condition;
    std::shared_ptr<Expression> updation;
    std::shared_ptr<Scope> action;
};
struct While : ControlFlow {
    using ControlFlow::ControlFlow;

    virtual std::vector<int> construct(Scope&, const StatementGroup& sg, int index) override;
    virtual void execute(Scope& scope) const override;

    std::shared_ptr<Expression> condition;
    std::shared_ptr<Scope> action;
};

}  // namespace llc

#include <llc/types_impl.h>

#endif  // LLC_TYPES_H