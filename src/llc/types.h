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
    String = 1ul << 23,
    LeftSquareBracket = 1ul << 24,
    RightSquareBracket = 1ul << 25,
    Invalid = 1ul << 26,
    Eof = 1ul << 27,
    NumTokens = 1ul << 28
};

inline TokenType operator|(TokenType a, TokenType b) { return (TokenType)((uint64_t)a | (uint64_t)b); }
inline uint64_t operator&(TokenType a, TokenType b) { return (uint64_t)a & (uint64_t)b; }

std::string enum_to_string(TokenType type);

struct Token {
    TokenType type = TokenType::Invalid;
    Location location;

    float value;
    std::string value_s;
    std::string id;
};

struct Scope;
struct Expression;

extern std::map<size_t, std::string> type_id_to_name;

struct BaseObject {
    BaseObject(std::string type_name) : type_name_(type_name){};
    virtual ~BaseObject() = default;
    virtual BaseObject* clone() const = 0;
    virtual void copy(void* ptr, std::string type_name) const = 0;
    virtual void assign(const BaseObject* src) = 0;
    virtual void add(BaseObject* rhs) = 0;
    virtual void sub(BaseObject* rhs) = 0;
    virtual void mul(BaseObject* rhs) = 0;
    virtual void div(BaseObject* rhs) = 0;
    virtual BaseObject* get_member(std::string name) = 0;
    std::string type_name() const { return type_name_; }

  private:
    std::string type_name_;
};

template <typename T>
struct ConcreteObject : BaseObject {
    ConcreteObject(T value, std::string type_name) : BaseObject(type_name), value(value){};
    virtual BaseObject* clone() const override { return new ConcreteObject<T>(*this); }

    void copy(void* ptr, std::string type_name) const override {
        using Ty = typename std::decay<T>::type;
        if (type_name != this->type_name())
            fatal("cannot convert type \"", this->type_name(), "\" to type \"", type_name, "\"");
        *((Ty*)ptr) = value;
    }
    void assign(const BaseObject* src) override {
        if (!std::is_reference<T>::value)
            fatal("assign to rvalue");
        src->copy(&value, type_name());
    }

    void add(BaseObject* rhs) override {
        auto ptr = dynamic_cast<ConcreteObject<T>*>(rhs);
        LLC_CHECK(ptr != nullptr);

        if constexpr (HasOperatorAdd<T>::value)
            value += ptr->value;
        else
            fatal("type \"", type_name(), "\" does not have operator \"+\"");
    }
    void sub(BaseObject* rhs) override {
        auto ptr = dynamic_cast<ConcreteObject<T>*>(rhs);
        LLC_CHECK(ptr != nullptr);

        if constexpr (HasOperatorSub<T>::value)
            value -= ptr->value;
        else
            fatal("type \"", type_name(), "\" does not have operator \"-\"");
    }
    void mul(BaseObject* rhs) override {
        auto ptr = dynamic_cast<ConcreteObject<T>*>(rhs);
        LLC_CHECK(ptr != nullptr);

        if constexpr (HasOperatorMul<T>::value)
            value -= ptr->value;
        else
            fatal("type \"", type_name(), "\" does not have operator \"*\"");
    }
    void div(BaseObject* rhs) override {
        auto ptr = dynamic_cast<ConcreteObject<T>*>(rhs);
        LLC_CHECK(ptr != nullptr);

        if constexpr (HasOperatorDiv<T>::value)
            value -= ptr->value;
        else
            fatal("type \"", type_name(), "\" does not have operator \"/\"");
    }

    struct Accessor {
        virtual ~Accessor() = default;
        virtual BaseObject* access(T& object) const = 0;
    };

    template <typename M>
    struct ConcreteAccessor : Accessor {
        ConcreteAccessor(M T::*ptr, std::string type_name) : ptr(ptr), type_name(type_name) {}

        BaseObject* access(T& object) const override {
            return new ConcreteObject<M&>(object.*ptr, type_name);
        }

        M T::*ptr;
        std::string type_name;
    };

    BaseObject* get_member(std::string name) override {
        auto it = accessors.find(name);
        LLC_CHECK(it != accessors.end());
        return it->second->access(value);
    }

    T value;
    std::map<std::string, std::shared_ptr<Accessor>> accessors;
};

struct Object {
    Object() : base(nullptr){};
    explicit Object(std::unique_ptr<BaseObject> base) : base(std::move(base)) {
        LLC_CHECK(this->base != nullptr);
    };
    template <typename T>
    explicit Object(T instance, std::string type_name)
        : base(std::make_unique<ConcreteObject<T>>(instance, type_name)) {}

    Object(const Object& rhs) {
        if (rhs.base != nullptr)
            base.reset(rhs.base->clone());
    };
    Object(Object&&) = default;
    Object& operator=(Object rhs) {
        swap(rhs);
        return *this;
    }
    void swap(Object& rhs) { std::swap(base, rhs.base); }

    void assign(const Object& rhs) {
        LLC_CHECK(base != nullptr);
        base->assign(rhs.base.get());
    }

    template <typename T>
    T as() const {
        LLC_CHECK(base != nullptr);
        T value;
        auto it = type_id_to_name.find(LLC_TYPE_ID(T));
        if (it == type_id_to_name.end())
            fatal("unknown type \"", typeid(T).name(), "\"");
        base->copy(&value, it->second);
        return value;
    }

    std::string type_name() const {
        LLC_CHECK(base != nullptr);
        return base->type_name();
    }

    Object operator[](std::string member_id) {
        LLC_CHECK(base != nullptr);
        std::unique_ptr<BaseObject> object(base->get_member(member_id));
        return Object(std::move(object));
    }

    Object& operator+=(const Object& rhs) {
        LLC_CHECK(rhs.base != nullptr);
        base->add(rhs.base.get());
        return *this;
    }
    Object& operator-=(const Object& rhs) {
        LLC_CHECK(rhs.base != nullptr);
        base->sub(rhs.base.get());
        return *this;
    }
    Object& operator*=(const Object& rhs) {
        LLC_CHECK(rhs.base != nullptr);
        base->mul(rhs.base.get());
        return *this;
    }
    Object& operator/=(const Object& rhs) {
        LLC_CHECK(rhs.base != nullptr);
        base->div(rhs.base.get());
        return *this;
    }
    friend Object operator+(Object lhs, const Object& rhs) { return lhs += rhs; }
    friend Object operator-(Object lhs, const Object& rhs) { return lhs -= rhs; }
    friend Object operator*(Object lhs, const Object& rhs) { return lhs *= rhs; }
    friend Object operator/(Object lhs, const Object& rhs) { return lhs /= rhs; }

  private:
    std::unique_ptr<BaseObject> base;
};

struct BaseFunction {
    virtual ~BaseFunction() = default;
    virtual BaseFunction* clone() const = 0;
    virtual Object run(const Scope& scope, const std::vector<Expression>& exprs) const = 0;
};

struct InternalFunction : BaseFunction {
    BaseFunction* clone() const override { return new InternalFunction(*this); }
    Object run(const Scope& scope, const std::vector<Expression>& exprs) const override;

    Object return_type;
    std::shared_ptr<Scope> definition;
    std::vector<std::string> parameters;
};

struct ExternalFunction : BaseFunction {
    Object run(const Scope& scope, const std::vector<Expression>& exprs) const override;

  protected:
    virtual Object invoke(const std::vector<Object>& args) const = 0;
};

template <typename Return, typename... Args>
struct FunctionInstance : ExternalFunction {
    using F = Return (*)(Args...);
    FunctionInstance(F f) : f(f){};

    BaseFunction* clone() const override { return new FunctionInstance<Return, Args...>(*this); }

    Object invoke(const std::vector<Object>& args) const override {
        LLC_CHECK(args.size() == sizeof...(Args));
        TypePack<Args...> types;

        if constexpr (std::is_same<Return, void>::value) {
            if constexpr (sizeof...(Args) == 0)
                f();
            else if constexpr (sizeof...(Args) == 1)
                f(args[0].as<decltype(types.template at<0>())>());
            else if constexpr (sizeof...(Args) == 2)
                f(args[0].as<decltype(types.template at<0>())>(),
                  args[0].as<decltype(types.template at<1>())>());
            else
                fatal("too many arguments, only support <= 4");

        } else {
            if constexpr (sizeof...(Args) == 0)
                return f();
            else if constexpr (sizeof...(Args) == 1)
                return f(args[0].as<decltype(types.template at<0>())>());
            else if constexpr (sizeof...(Args) == 2)
                return f(args[0].as<decltype(types.template at<0>())>(),
                         args[0].as<decltype(types.template at<1>())>());
            else
                fatal("too many arguments, only support <= 4");
        }

        return {};
    }

    F f;
};

struct Function {
    Function() : base(nullptr){};
    Function(std::unique_ptr<BaseFunction> base) : base(std::move(base)) {
        LLC_CHECK(this->base != nullptr);
    };

    Function(const Function& rhs) {
        if (rhs.base != nullptr)
            base.reset(rhs.base->clone());
    };
    Function(Function&&) = default;
    Function& operator=(Function rhs) {
        swap(rhs);
        return *this;
    }
    void swap(Function& rhs) { std::swap(base, rhs.base); }

    Object run(const Scope& scope, const std::vector<Expression>& exprs) const {
        LLC_CHECK(base != nullptr);
        return base->run(scope, exprs);
    }

  private:
    std::unique_ptr<BaseFunction> base;
};

struct Statement {
    virtual ~Statement() = default;

    virtual Object run(const Scope& scope) const = 0;
};

struct Scope : Statement {
    Scope();

    Object run(const Scope& scope) const override;

    std::optional<Object> find_type(std::string name) const;
    std::optional<Object> find_variable(std::string name) const;
    std::optional<Function> find_function(std::string name) const;

    std::shared_ptr<Scope> parent;
    std::vector<std::shared_ptr<Statement>> statements;
    mutable std::map<std::string, Object> types;
    mutable std::map<std::string, Object> variables;
    mutable std::map<std::string, Function> functions;
};

struct Operand {
    virtual ~Operand() = default;

    virtual std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>& operands, int index) = 0;

    virtual Object evaluate(const Scope& scope) const = 0;

    virtual Object assign(const Scope&, const Object&) {
        fatal("Operand::assign shall not be called");
        return {};
    }

    virtual int get_precedence() const = 0;
    virtual void set_precedence(int prec) = 0;
};

struct BaseOp : Operand {
    std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>&, int) override { return {}; };
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

struct NumberLiteral : BaseOp {
    NumberLiteral(float value) : value(value){};

    Object evaluate(const Scope&) const override { return Object(value, "float"); }

    int get_precedence() const override { return precedence; }
    void set_precedence(int prec) override { precedence = prec; }

    int precedence = 10;
    float value;
};

struct StringLiteral : BaseOp {
    StringLiteral(std::string value) : value(value){};

    Object evaluate(const Scope&) const override { return Object(value, "string"); }

    int get_precedence() const override { return precedence; }
    void set_precedence(int prec) override { precedence = prec; }

    int precedence = 10;
    std::string value;
};

struct VariableOp : BaseOp {
    VariableOp(std::string name) : name(name){};

    Object evaluate(const Scope& scope) const override {
        LLC_CHECK(scope.find_variable(name).has_value());
        return scope.variables[name];
    }

    Object assign(const Scope& scope, const Object& value) override {
        LLC_CHECK(scope.find_variable(name).has_value());
        return scope.variables[name] = value;
    }

    Object& original(const Scope& scope) const {
        LLC_CHECK(scope.find_variable(name).has_value());
        return scope.variables[name];
    }

    int get_precedence() const override { return precedence; }
    void set_precedence(int prec) override { precedence = prec; }
    int precedence = 10;
    std::string name;
};

struct ObjectMember : Operand {
    ObjectMember(std::string name) : name(name){};

    std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>&, int) override {
        fatal("ObjectMember::collapse() shall not be called");
        return {};
    }
    Object evaluate(const Scope&) const override {
        fatal("ObjectMember::evaluate() shall not be called");
        return {};
    }
    int get_precedence() const override { return 0; }
    void set_precedence(int) override {}

    std::string name;
};

struct MemberAccess : BinaryOp {
    std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>& operands, int index) override {
        LLC_CHECK(index - 1 >= 0);
        LLC_CHECK(index + 1 < (int)operands.size());
        a = operands[index - 1];
        b = operands[index + 1];
        return {index - 1, index + 1};
    }

    Object evaluate(const Scope& scope) const override {
        auto variable = dynamic_cast<VariableOp*>(a.get());
        auto member = dynamic_cast<ObjectMember*>(b.get());
        LLC_CHECK(variable != nullptr);
        LLC_CHECK(member != nullptr);
        Object obj = variable->evaluate(scope)[member->name];
        return variable->evaluate(scope)[member->name];
    }
    Object assign(const Scope& scope, const Object& value) override {
        auto variable = dynamic_cast<VariableOp*>(a.get());
        auto member = dynamic_cast<ObjectMember*>(b.get());
        LLC_CHECK(variable != nullptr);
        LLC_CHECK(member != nullptr);
        variable->original(scope)[member->name].assign(value);
        return variable->evaluate(scope)[member->name];
    }

    int get_precedence() const override { return precedence; }
    void set_precedence(int prec) override { precedence = prec; }
    int precedence = 8;
};

// struct ArrayAccess : BinaryOp {
//     Object evaluate(Scope&) override {
//         // Object arr = a->evaluate(scope);
//         // int index = (int)b->evaluate(scope).value;
//         return {};
//     }
//     Object assign(Scope&, const Object&) override {
//         // Object arr = a->evaluate(scope);
//         // int index = (int)b->evaluate(scope).value;
//         return {};
//     }

//     int get_precedence() const override { return precedence; }
//     void set_precedence(int prec) override { precedence = prec; }

//     int precedence = 2;
// };

struct TypeOp : BaseOp {
    TypeOp(Object type) : type(type){};

    Object evaluate(const Scope&) const override { return type; }

    int get_precedence() const override { return precedence; }
    void set_precedence(int prec) override { precedence = prec; }
    int precedence = 8;
    Object type;
};

struct Assignment : BinaryOp {
    Object evaluate(const Scope& scope) const override { return a->assign(scope, b->evaluate(scope)); }

    int get_precedence() const override { return precedence; }
    void set_precedence(int prec) override { precedence = prec; }
    int precedence = 0;
};

struct Addition : BinaryOp {
    Object evaluate(const Scope& scope) const override { return a->evaluate(scope) + b->evaluate(scope); }

    int get_precedence() const override { return precedence; }
    void set_precedence(int prec) override { precedence = prec; }
    int precedence = 4;
};

struct Subtraction : BinaryOp {
    Object evaluate(const Scope& scope) const override { return a->evaluate(scope) - b->evaluate(scope); }

    int get_precedence() const override { return precedence; }
    void set_precedence(int prec) override { precedence = prec; }
    int precedence = 4;
};

struct Multiplication : BinaryOp {
    Object evaluate(const Scope& scope) const override { return a->evaluate(scope) * b->evaluate(scope); }

    int get_precedence() const override { return precedence; }
    void set_precedence(int prec) override { precedence = prec; }
    int precedence = 5;
};

struct Division : BinaryOp {
    Object evaluate(const Scope& scope) const override { return a->evaluate(scope) / b->evaluate(scope); }

    int get_precedence() const override { return precedence; }
    void set_precedence(int prec) override { precedence = prec; }
    int precedence = 5;
};

// struct PostIncrement : PostUnaryOp {
//     Object evaluate(Scope& scope) override {
//         auto old = operand->evaluate(scope);
//         operand->assign(scope, ++operand->evaluate(scope));
//         return old;
//     }

//     int get_precedence() const override { return precedence; }
//     void set_precedence(int prec) override { precedence = prec; }

//     int precedence = 8;
// };

// struct PostDecrement : PostUnaryOp {
//     Object evaluate(Scope& scope) override {
//         auto old = operand->evaluate(scope);
//         operand->assign(scope, --operand->evaluate(scope));
//         return old;
//     }

//     int get_precedence() const override { return precedence; }
//     void set_precedence(int prec) override { precedence = prec; }

//     int precedence = 8;
// };

// struct PreIncrement : PreUnaryOp {
//     Object evaluate(Scope& scope) override { return operand->assign(scope, ++operand->evaluate(scope)); }

//     int get_precedence() const override { return precedence; }
//     void set_precedence(int prec) override { precedence = prec; }

//     int precedence = 8;
// };

// struct PreDecrement : PreUnaryOp {
//     Object evaluate(Scope& scope) override { return operand->assign(scope, --operand->evaluate(scope)); }

//     int get_precedence() const override { return precedence; }
//     void set_precedence(int prec) override { precedence = prec; }

//     int precedence = 8;
// };

// struct LessThan : BinaryOp {
//     Object evaluate(Scope& scope) override { return a->evaluate(scope) < b->evaluate(scope); }

//     int get_precedence() const override { return precedence; }
//     void set_precedence(int prec) override { precedence = prec; }
//     int precedence = 2;
// };

// struct LessEqual : BinaryOp {
//     Object evaluate(Scope& scope) override { return a->evaluate(scope) <= b->evaluate(scope); }

//     int get_precedence() const override { return precedence; }
//     void set_precedence(int prec) override { precedence = prec; }
//     int precedence = 2;
// };

// struct GreaterThan : BinaryOp {
//     Object evaluate(Scope& scope) override { return a->evaluate(scope) > b->evaluate(scope); }

//     int get_precedence() const override { return precedence; }
//     void set_precedence(int prec) override { precedence = prec; }
//     int precedence = 2;
// };

// struct GreaterEqual : BinaryOp {
//     Object evaluate(Scope& scope) override { return a->evaluate(scope) >= b->evaluate(scope); }

//     int get_precedence() const override { return precedence; }
//     void set_precedence(int prec) override { precedence = prec; }
//     int precedence = 2;
// };

// struct Equal : BinaryOp {
//     Object evaluate(Scope& scope) override { return a->evaluate(scope) == b->evaluate(scope); }

//     int get_precedence() const override { return precedence; }
//     void set_precedence(int prec) override { precedence = prec; }
//     int precedence = 2;
// };

// struct NotEqual : BinaryOp {
//     Object evaluate(Scope& scope) override { return a->evaluate(scope) != b->evaluate(scope); }

//     int get_precedence() const override { return precedence; }
//     void set_precedence(int prec) override { precedence = prec; }
//     int precedence = 2;
// };

struct LeftParenthese : Operand {
    std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>&, int) override {
        fatal("LeftParenthese::collapse() shall not be called");
        return {};
    }
    Object evaluate(const Scope&) const override {
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
    Object evaluate(const Scope&) const override {
        fatal("RightParenthese::evaluate() shall not be called");
        return {};
    }
    int get_precedence() const override { return 0; }
    void set_precedence(int) override {}
};

struct Expression : Statement {
    void apply_parenthese();
    void collapse();

    Object operator()(const Scope& scope) const {
        if (operands.size() == 0)
            return {};
        LLC_CHECK(operands.size() == 1);
        return operands[0]->evaluate(scope);
    }

    Object run(const Scope& scope) const override { return this->operator()(scope); }

    std::vector<std::shared_ptr<Operand>> operands;
};

struct FunctionCall : Statement {
    Object run(const Scope& scope) const override { return function.run(scope, arguments); }

    Function function;
    std::vector<Expression> arguments;
};

struct FunctionCallOp : BaseOp {
    FunctionCallOp(FunctionCall function) : function(function){};

    Object evaluate(const Scope& scope) const override { return function.run(scope); }

    int get_precedence() const override { return precedence; }
    void set_precedence(int prec) override { precedence = prec; }

    int precedence = 10;
    FunctionCall function;
};

struct Return : Statement {
    Return(Expression expression) : expression(expression){};

    Object run(const Scope& scope) const override {
        auto result = expression(scope);
        throw result;
        return result;
    }

    Expression expression;
};

struct IfElseChain : Statement {
    IfElseChain(std::vector<Expression> conditions, std::vector<std::shared_ptr<Scope>> actions)
        : conditions(conditions), actions(actions){};

    Object run(const Scope& scope) const override;

    std::vector<Expression> conditions;
    std::vector<std::shared_ptr<Scope>> actions;
};

struct For : Statement {
    For(Expression condition, Expression updation, std::shared_ptr<Scope> internal_scope,
        std::shared_ptr<Scope> action)
        : condition(condition), updation(updation), internal_scope(internal_scope), action(action){};

    Object run(const Scope& scope) const override;

    Expression condition, updation;
    std::shared_ptr<Scope> internal_scope, action;
};

struct While : Statement {
    While(Expression condition, std::shared_ptr<Scope> action) : condition(condition), action(action){};

    Object run(const Scope& scope) const override;

    Expression condition;
    std::shared_ptr<Scope> action;
};

struct Program {
    Program(std::shared_ptr<Scope> scope) : scope(scope){};

    void run() { scope->run(*scope); }

  private:
    std::shared_ptr<Scope> scope;
};

}  // namespace llc

#endif  // LLC_TYPES_H