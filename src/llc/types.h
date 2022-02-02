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

inline TokenType operator|(TokenType a, TokenType b) {
    return (TokenType)((uint64_t)a | (uint64_t)b);
}
inline uint64_t operator&(TokenType a, TokenType b) {
    return (uint64_t)a & (uint64_t)b;
}

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

template <typename T>
std::string get_type_name() {
    auto it = type_id_to_name.find(typeid(T).hash_code());
    if (it == type_id_to_name.end())
        throw_exception("unknown type \"", typeid(T).name(), "\"");
    return it->second;
}

struct Object;

struct BaseObject {
    BaseObject(std::string type_name) : type_name_(type_name){};
    virtual ~BaseObject() = default;
    virtual BaseObject* clone() const = 0;
    virtual void* ptr() const = 0;
    virtual void assign(const BaseObject* src) = 0;
    virtual void add(BaseObject* rhs) = 0;
    virtual void sub(BaseObject* rhs) = 0;
    virtual void mul(BaseObject* rhs) = 0;
    virtual void div(BaseObject* rhs) = 0;
    virtual bool less_than(BaseObject* rhs) const = 0;
    virtual bool less_equal(BaseObject* rhs) const = 0;
    virtual bool greater_than(BaseObject* rhs) const = 0;
    virtual bool greater_equal(BaseObject* rhs) const = 0;
    virtual bool equal(BaseObject* rhs) const = 0;
    virtual bool not_equal(BaseObject* rhs) const = 0;

    std::string type_name() const {
        return type_name_;
    }
    Object& operator[](std::string name);

    mutable std::map<std::string, Object> members;

  private:
    std::string type_name_;
};

struct Object {
    Object() : base(nullptr){};
    explicit Object(std::unique_ptr<BaseObject> base) {
        LLC_CHECK(base != nullptr);
        this->base = std::move(base);
    }
    template <typename T, typename = typename std::enable_if<
                              !std::is_convertible<T, std::unique_ptr<BaseObject>>::value>::type>
    explicit Object(T instance);

    Object(const Object& rhs) {
        if (rhs.base != nullptr)
            base.reset(rhs.base->clone());
    }
    Object& operator=(Object rhs) {
        std::swap(base, rhs.base);
        return *this;
    }

    void assign(const Object& rhs) {
        LLC_CHECK(base != nullptr);
        LLC_CHECK(rhs.base != nullptr);
        if (type_name() != rhs.type_name())
            throw_exception("cannot assign type \"", rhs.type_name(), "\" to type \"", type_name(),
                            "\"");
        base->assign(rhs.base.get());
    }

    template <typename T>
    T as() const {
        LLC_CHECK(base != nullptr);
        if (type_name() != get_type_name<T>())
            throw_exception("cannot convert type \"", type_name(), "\" to type \"",
                            get_type_name<std::decay_t<T>>(), "\"");
        return *(std::decay_t<T>*)base->ptr();
    }

    template <typename T>
    std::optional<T> as_opt() const {
        LLC_CHECK(base != nullptr);
        if (type_name() != get_type_name<T>())
            return std::nullopt;
        return *(std::decay_t<T>*)base->ptr();
    }

    std::string type_name() const {
        LLC_CHECK(base != nullptr);
        return base->type_name();
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
    friend Object operator+(Object lhs, const Object& rhs) {
        return lhs += rhs;
    }
    friend Object operator-(Object lhs, const Object& rhs) {
        return lhs -= rhs;
    }
    friend Object operator*(Object lhs, const Object& rhs) {
        return lhs *= rhs;
    }
    friend Object operator/(Object lhs, const Object& rhs) {
        return lhs /= rhs;
    }
    friend bool operator<(const Object& lhs, const Object& rhs) {
        LLC_CHECK(lhs.base != nullptr);
        LLC_CHECK(rhs.base != nullptr);
        return lhs.base->less_than(rhs.base.get());
    }
    friend bool operator<=(const Object& lhs, const Object& rhs) {
        LLC_CHECK(lhs.base != nullptr);
        LLC_CHECK(rhs.base != nullptr);
        return lhs.base->less_equal(rhs.base.get());
    }
    friend bool operator>(const Object& lhs, const Object& rhs) {
        LLC_CHECK(lhs.base != nullptr);
        LLC_CHECK(rhs.base != nullptr);
        return lhs.base->greater_than(rhs.base.get());
    }
    friend bool operator>=(const Object& lhs, const Object& rhs) {
        LLC_CHECK(lhs.base != nullptr);
        LLC_CHECK(rhs.base != nullptr);
        return lhs.base->greater_equal(rhs.base.get());
    }
    friend bool operator==(const Object& lhs, const Object& rhs) {
        LLC_CHECK(lhs.base != nullptr);
        LLC_CHECK(rhs.base != nullptr);
        return lhs.base->equal(rhs.base.get());
    }
    friend bool operator!=(const Object& lhs, const Object& rhs) {
        LLC_CHECK(lhs.base != nullptr);
        LLC_CHECK(rhs.base != nullptr);
        return lhs.base->not_equal(rhs.base.get());
    }

    Object& operator[](std::string name) {
        LLC_CHECK(base != nullptr);
        return (*base)[name];
    }

    std::unique_ptr<BaseObject> base;
};

inline Object& BaseObject::operator[](std::string name) {
    if (members.find(name) == members.end())
        throw_exception("cannot find member \"", name, "\"");
    return members[name];
}

template <typename T>
struct ConcreteObject : BaseObject {
    ConcreteObject(T value) : BaseObject(get_type_name<T>()), value(value){};

    virtual BaseObject* clone() const override {
        ConcreteObject<T>* object = new ConcreteObject<T>(*this);
        object->bind_members();
        return object;
    }

    void* ptr() const override {
        return (void*)&value;
    }
    void assign(const BaseObject* src) override {
        using Ty = typename std::decay_t<T>;
        value = *(Ty*)src->ptr();
    }

    void bind_members() {
        for (const auto& accessor : accessors)
            members[accessor.first] = accessor.second->access(value);
    }

    void add(BaseObject* rhs) override {
        auto ptr = dynamic_cast<ConcreteObject<T>*>(rhs);
        LLC_CHECK(ptr != nullptr);

        if constexpr (HasOperatorAdd<T>::value)
            value += ptr->value;
        else
            throw_exception("type \"", type_name(), "\" does not have operator \"+\"");
    }
    void sub(BaseObject* rhs) override {
        auto ptr = dynamic_cast<ConcreteObject<T>*>(rhs);
        LLC_CHECK(ptr != nullptr);

        if constexpr (HasOperatorSub<T>::value)
            value -= ptr->value;
        else
            throw_exception("type \"", type_name(), "\" does not have operator \"-\"");
    }
    void mul(BaseObject* rhs) override {
        auto ptr = dynamic_cast<ConcreteObject<T>*>(rhs);
        LLC_CHECK(ptr != nullptr);

        if constexpr (HasOperatorMul<T>::value)
            value -= ptr->value;
        else
            throw_exception("type \"", type_name(), "\" does not have operator \"*\"");
    }
    void div(BaseObject* rhs) override {
        auto ptr = dynamic_cast<ConcreteObject<T>*>(rhs);
        LLC_CHECK(ptr != nullptr);

        if constexpr (HasOperatorDiv<T>::value)
            value -= ptr->value;
        else
            throw_exception("type \"", type_name(), "\" does not have operator \"/\"");
    }
    virtual bool less_than(BaseObject* rhs) const override {
        auto ptr = dynamic_cast<ConcreteObject<T>*>(rhs);
        LLC_CHECK(ptr != nullptr);

        if constexpr (HasOperatorLT<T>::value)
            return value < ptr->value;
        else
            throw_exception("type \"", type_name(), "\" does not have operator \"<\"");
        return {};
    }
    virtual bool less_equal(BaseObject* rhs) const override {
        auto ptr = dynamic_cast<ConcreteObject<T>*>(rhs);
        LLC_CHECK(ptr != nullptr);

        if constexpr (HasOperatorLE<T>::value)
            return value <= ptr->value;
        else
            throw_exception("type \"", type_name(), "\" does not have operator \"<=\"");
        return {};
    }
    virtual bool greater_than(BaseObject* rhs) const override {
        auto ptr = dynamic_cast<ConcreteObject<T>*>(rhs);
        LLC_CHECK(ptr != nullptr);

        if constexpr (HasOperatorGT<T>::value)
            return value > ptr->value;
        else
            throw_exception("type \"", type_name(), "\" does not have operator \">\"");
        return {};
    }
    virtual bool greater_equal(BaseObject* rhs) const override {
        auto ptr = dynamic_cast<ConcreteObject<T>*>(rhs);
        LLC_CHECK(ptr != nullptr);

        if constexpr (HasOperatorGE<T>::value)
            return value >= ptr->value;
        else
            throw_exception("type \"", type_name(), "\" does not have operator \">=\"");
        return {};
    }
    virtual bool equal(BaseObject* rhs) const override {
        auto ptr = dynamic_cast<ConcreteObject<T>*>(rhs);
        LLC_CHECK(ptr != nullptr);

        if constexpr (HasOperatorEQ<T>::value)
            return value == ptr->value;
        else
            throw_exception("type \"", type_name(), "\" does not have operator \"==\"");
        return {};
    }
    virtual bool not_equal(BaseObject* rhs) const override {
        auto ptr = dynamic_cast<ConcreteObject<T>*>(rhs);
        LLC_CHECK(ptr != nullptr);

        if constexpr (HasOperatorNE<T>::value)
            return value != ptr->value;
        else
            throw_exception("type \"", type_name(), "\" does not have operator \"!=\"");
        return {};
    }

    struct Accessor {
        virtual ~Accessor() = default;
        virtual Object access(T& object) const = 0;
    };

    template <typename M>
    struct ConcreteAccessor : Accessor {
        ConcreteAccessor(M T::*ptr) : ptr(ptr) {
        }

        Object access(T& object) const override {
            return Object(std::make_unique<ConcreteObject<M&>>(object.*ptr));
        }

        M T::*ptr;
    };

    T value;
    std::map<std::string, std::shared_ptr<Accessor>> accessors;
};

struct InternalObject : BaseObject {
    using BaseObject::BaseObject;

    BaseObject* clone() const override {
        return new InternalObject(*this);
    }

    void* ptr() const override {
        throw_exception("cannot get pointer to internal type");
        return nullptr;
    };
    void assign(const BaseObject* src) override {
        auto ptr = dynamic_cast<const InternalObject*>(src);
        if (ptr == nullptr)
            throw_exception("assign external type to internal type is not allowed");
        for (auto& member : ptr->members)
            members[member.first] = member.second;
    }
    void add(BaseObject* rhs) override {
        auto ptr = dynamic_cast<const InternalObject*>(rhs);
        if (ptr == nullptr)
            throw_exception("add external type to internal type is not allowed");
        for (auto& member : ptr->members)
            members[member.first] += member.second;
    }
    void sub(BaseObject* rhs) override {
        auto ptr = dynamic_cast<const InternalObject*>(rhs);
        if (ptr == nullptr)
            throw_exception("subtract internal type by internal type is not allowed");
        for (auto& member : ptr->members)
            members[member.first] -= member.second;
    }
    void mul(BaseObject* rhs) override {
        auto ptr = dynamic_cast<const InternalObject*>(rhs);
        if (ptr == nullptr)
            throw_exception("multiply internal type by internal type is not allowed");
        for (auto& member : ptr->members)
            members[member.first] *= member.second;
    }
    void div(BaseObject* rhs) override {
        auto ptr = dynamic_cast<const InternalObject*>(rhs);
        if (ptr == nullptr)
            throw_exception("divide internal type by internal type is not allowed");
        for (auto& member : ptr->members)
            members[member.first] /= member.second;
    }
    bool less_than(BaseObject* rhs) const override {
        for (const auto& it : rhs->members)
            if (members[it.first] >= it.second)
                return false;
        return true;
    }
    bool less_equal(BaseObject* rhs) const override {
        for (const auto& it : rhs->members)
            if (members[it.first] > it.second)
                return false;
        return true;
    }
    bool greater_than(BaseObject* rhs) const override {
        for (const auto& it : rhs->members)
            if (members[it.first] <= it.second)
                return false;
        return true;
    }
    bool greater_equal(BaseObject* rhs) const override {
        for (const auto& it : rhs->members)
            if (members[it.first] < it.second)
                return false;
        return true;
    }
    bool equal(BaseObject* rhs) const override {
        for (const auto& it : rhs->members)
            if (members[it.first] != it.second)
                return false;
        return true;
    }
    bool not_equal(BaseObject* rhs) const override {
        for (const auto& it : rhs->members)
            if (members[it.first] != it.second)
                return true;
        return false;
    }
};

template <typename T, typename>
Object::Object(T instance) : base(std::make_unique<ConcreteObject<T>>(instance)) {
}

struct BaseFunction {
    virtual ~BaseFunction() = default;
    virtual BaseFunction* clone() const = 0;
    virtual std::optional<Object> run(const Scope& scope,
                                      const std::vector<Expression>& exprs) const = 0;
};

struct InternalFunction : BaseFunction {
    BaseFunction* clone() const override {
        return new InternalFunction(*this);
    }
    std::optional<Object> run(const Scope& scope,
                              const std::vector<Expression>& exprs) const override;

    std::optional<Object> return_type;
    std::shared_ptr<Scope> definition;
    std::vector<std::string> parameters;
};

struct ExternalFunction : BaseFunction {
    std::optional<Object> run(const Scope& scope,
                              const std::vector<Expression>& exprs) const override;

  protected:
    virtual Object invoke(const std::vector<Object>& args) const = 0;
};

template <typename Return, typename... Args>
struct ConcreteFunction : ExternalFunction {
    using F = Return (*)(Args...);
    ConcreteFunction(F f) : f(f){};

    BaseFunction* clone() const override {
        return new ConcreteFunction<Return, Args...>(*this);
    }

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
                  args[1].as<decltype(types.template at<1>())>());
            else
                throw_exception("too many arguments, only support <= 4");

        } else {
            if constexpr (sizeof...(Args) == 0)
                return f();
            else if constexpr (sizeof...(Args) == 1)
                return f(args[0].as<decltype(types.template at<0>())>());
            else if constexpr (sizeof...(Args) == 2)
                return f(args[0].as<decltype(types.template at<0>())>(),
                         args[1].as<decltype(types.template at<1>())>());
            else
                throw_exception("too many arguments, only support <= 4");
        }

        return {};
    }

    F f;
};

struct Function {
    Function() : base(nullptr){};
    Function(std::unique_ptr<BaseFunction> base) : base(std::move(base)) {
        LLC_CHECK(this->base != nullptr);
    }

    Function(const Function& rhs) {
        if (rhs.base != nullptr)
            base.reset(rhs.base->clone());
    }
    Function(Function&&) = default;
    Function& operator=(Function rhs) {
        swap(rhs);
        return *this;
    }
    void swap(Function& rhs) {
        std::swap(base, rhs.base);
    }

    std::optional<Object> run(const Scope& scope, const std::vector<Expression>& exprs) const {
        LLC_CHECK(base != nullptr);
        return base->run(scope, exprs);
    }

  private:
    std::unique_ptr<BaseFunction> base;
};

struct Statement {
    virtual ~Statement() = default;

    virtual std::optional<Object> run(const Scope& scope) const = 0;
};

struct Scope : Statement {
    Scope();

    std::optional<Object> run(const Scope& scope) const override;

    std::optional<Object> find_type(std::string name) const;
    std::optional<Object> find_variable(std::string name) const;
    std::optional<Function> find_function(std::string name) const;

    Object& get_variable(std::string name) const;

    std::shared_ptr<Scope> parent;
    std::vector<std::shared_ptr<Statement>> statements;
    mutable std::map<std::string, Object> types;
    mutable std::map<std::string, Object> variables;
    mutable std::map<std::string, Function> functions;
};

struct Operand {
    virtual ~Operand() = default;

    virtual std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>& operands,
                                      int index) = 0;

    virtual Object evaluate(const Scope& scope) const = 0;

    virtual Object assign(const Scope&, const Object&) {
        throw_exception("Operand::assign shall not be called");
        return {};
    }

    virtual int get_precedence() const = 0;
    virtual void set_precedence(int prec) = 0;
};

struct BaseOp : Operand {
    std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>&, int) override {
        return {};
    };
};

struct BinaryOp : Operand {
    std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>& operands,
                              int index) override {
        LLC_CHECK(index - 1 >= 0);
        LLC_CHECK(index + 1 < (int)operands.size());
        a = operands[index - 1];
        b = operands[index + 1];
        return {index - 1, index + 1};
    }

    std::shared_ptr<Operand> a, b;
};

struct PreUnaryOp : Operand {
    std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>& operands,
                              int index) override {
        LLC_CHECK(index + 1 >= 0);
        LLC_CHECK(index + 1 < (int)operands.size());
        operand = operands[index + 1];
        return {index + 1};
    }

    std::shared_ptr<Operand> operand;
};

struct PostUnaryOp : Operand {
    std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>& operands,
                              int index) override {
        LLC_CHECK(index - 1 >= 0);
        LLC_CHECK(index - 1 < (int)operands.size());
        operand = operands[index - 1];
        return {index - 1};
    }

    std::shared_ptr<Operand> operand;
};

struct NumberLiteral : BaseOp {
    NumberLiteral(float value) : value(value){};

    Object evaluate(const Scope&) const override {
        return Object(value);
    }

    int get_precedence() const override {
        return precedence;
    }
    void set_precedence(int prec) override {
        precedence = prec;
    }

    int precedence = 10;
    float value;
};

struct StringLiteral : BaseOp {
    StringLiteral(std::string value) : value(value){};

    Object evaluate(const Scope&) const override {
        return Object(value);
    }

    int get_precedence() const override {
        return precedence;
    }
    void set_precedence(int prec) override {
        precedence = prec;
    }

    int precedence = 10;
    std::string value;
};

struct VariableOp : BaseOp {
    VariableOp(std::string name) : name(name){};

    Object evaluate(const Scope& scope) const override {
        LLC_CHECK(scope.find_variable(name).has_value());
        return scope.get_variable(name);
    }

    Object assign(const Scope& scope, const Object& value) override {
        LLC_CHECK(scope.find_variable(name).has_value());
        scope.get_variable(name).assign(value);
        return scope.get_variable(name);
    }

    Object& original(const Scope& scope) const {
        LLC_CHECK(scope.find_variable(name).has_value());
        return scope.get_variable(name);
    }

    int get_precedence() const override {
        return precedence;
    }
    void set_precedence(int prec) override {
        precedence = prec;
    }
    int precedence = 10;
    std::string name;
};

struct ObjectMember : Operand {
    ObjectMember(std::string name) : name(name){};

    std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>&, int) override {
        throw_exception("ObjectMember::collapse() shall not be called");
        return {};
    }
    Object evaluate(const Scope&) const override {
        throw_exception("ObjectMember::evaluate() shall not be called");
        return {};
    }
    int get_precedence() const override {
        return 0;
    }
    void set_precedence(int) override {
    }

    std::string name;
};

struct MemberAccess : BinaryOp {
    std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>& operands,
                              int index) override {
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

    int get_precedence() const override {
        return precedence;
    }
    void set_precedence(int prec) override {
        precedence = prec;
    }
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

    Object evaluate(const Scope&) const override {
        return type;
    }

    int get_precedence() const override {
        return precedence;
    }
    void set_precedence(int prec) override {
        precedence = prec;
    }
    int precedence = 8;
    Object type;
};

struct Assignment : BinaryOp {
    Object evaluate(const Scope& scope) const override {
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
    Object evaluate(const Scope& scope) const override {
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
    Object evaluate(const Scope& scope) const override {
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
    Object evaluate(const Scope& scope) const override {
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
    Object evaluate(const Scope& scope) const override {
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
//     Object evaluate(Scope& scope) override { return operand->assign(scope,
//     ++operand->evaluate(scope)); }

//     int get_precedence() const override { return precedence; }
//     void set_precedence(int prec) override { precedence = prec; }

//     int precedence = 8;
// };

// struct PreDecrement : PreUnaryOp {
//     Object evaluate(Scope& scope) override { return operand->assign(scope,
//     --operand->evaluate(scope)); }

//     int get_precedence() const override { return precedence; }
//     void set_precedence(int prec) override { precedence = prec; }

//     int precedence = 8;
// };

struct LessThan : BinaryOp {
    Object evaluate(const Scope& scope) const override {
        return Object(a->evaluate(scope) < b->evaluate(scope));
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
    Object evaluate(const Scope& scope) const override {
        return Object(a->evaluate(scope) <= b->evaluate(scope));
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
    Object evaluate(const Scope& scope) const override {
        return Object(a->evaluate(scope) > b->evaluate(scope));
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
    Object evaluate(const Scope& scope) const override {
        return Object(a->evaluate(scope) >= b->evaluate(scope));
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
    Object evaluate(const Scope& scope) const override {
        return Object(a->evaluate(scope) == b->evaluate(scope));
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
    Object evaluate(const Scope& scope) const override {
        return Object(a->evaluate(scope) != b->evaluate(scope));
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
        throw_exception("LeftParenthese::collapse() shall not be called");
        return {};
    }
    Object evaluate(const Scope&) const override {
        throw_exception("LeftParenthese::evaluate() shall not be called");
        return {};
    }
    int get_precedence() const override {
        return 0;
    }
    void set_precedence(int) override {
    }
};

struct RightParenthese : Operand {
    std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>&, int) override {
        throw_exception("RightParenthese::collapse() shall not be called");
        return {};
    }
    Object evaluate(const Scope&) const override {
        throw_exception("RightParenthese::evaluate() shall not be called");
        return {};
    }
    int get_precedence() const override {
        return 0;
    }
    void set_precedence(int) override {
    }
};

struct Expression : Statement {
    void apply_parenthese();
    void collapse();

    std::optional<Object> operator()(const Scope& scope) const {
        if (operands.size() == 0)
            return std::nullopt;
        LLC_CHECK(operands.size() == 1);
        return operands[0]->evaluate(scope);
    }

    std::optional<Object> run(const Scope& scope) const override {
        return this->operator()(scope);
    }

    std::vector<std::shared_ptr<Operand>> operands;
};

struct FunctionCall : Statement {
    std::optional<Object> run(const Scope& scope) const override {
        return function.run(scope, arguments);
    }

    Function function;
    std::vector<Expression> arguments;
};

struct FunctionCallOp : BaseOp {
    FunctionCallOp(FunctionCall function) : function(function){};

    Object evaluate(const Scope& scope) const override {
        if (auto result = function.run(scope))
            return *result;
        throw_exception("function returns void, which cannnot appear in expression");
        return {};
    }

    int get_precedence() const override {
        return precedence;
    }
    void set_precedence(int prec) override {
        precedence = prec;
    }

    int precedence = 10;
    FunctionCall function;
};

struct Return : Statement {
    Return(Expression expression) : expression(expression){};

    std::optional<Object> run(const Scope& scope) const override {
        auto result = expression(scope);
        throw result;
        return result;
    }

    Expression expression;
};

struct IfElseChain : Statement {
    IfElseChain(std::vector<Expression> conditions, std::vector<std::shared_ptr<Scope>> actions)
        : conditions(conditions), actions(actions){};

    std::optional<Object> run(const Scope& scope) const override;

    std::vector<Expression> conditions;
    std::vector<std::shared_ptr<Scope>> actions;
};

struct For : Statement {
    For(Expression condition, Expression updation, std::shared_ptr<Scope> internal_scope,
        std::shared_ptr<Scope> action)
        : condition(condition),
          updation(updation),
          internal_scope(internal_scope),
          action(action){};

    std::optional<Object> run(const Scope& scope) const override;

    Expression condition, updation;
    std::shared_ptr<Scope> internal_scope, action;
};

struct While : Statement {
    While(Expression condition, std::shared_ptr<Scope> action)
        : condition(condition), action(action){};

    std::optional<Object> run(const Scope& scope) const override;

    Expression condition;
    std::shared_ptr<Scope> action;
};

struct Program {
    template <typename Return, typename... Args>
    void bind(std::string name, Return (*func)(Args...)) {
        functions[name] = (Function)std::make_unique<ConcreteFunction<Return, Args...>>(func);
    }

    template <typename T>
    struct TypeBindHelper {
        TypeBindHelper(std::string type_name, std::map<std::string, Object>& types)
            : type_name(type_name), types(types) {
            type_id_to_name[typeid(T).hash_code()] = type_name;
            object = std::make_unique<ConcreteObject<T>>(T());
        };
        ~TypeBindHelper() {
            object->bind_members();
            types[type_name] = Object(std::move(object));
        }

        template <typename M>
        TypeBindHelper& bind(std::string id, M T::*ptr) {
            using U = typename ConcreteObject<T>::template ConcreteAccessor<M>;
            object->accessors[id] = std::make_shared<U>(ptr);
            return *this;
        }

        std::string type_name;
        std::unique_ptr<ConcreteObject<T>> object;
        std::map<std::string, Object>& types;
    };

    template <typename T>
    TypeBindHelper<T> bind(std::string name) {
        return TypeBindHelper<T>(name, types);
    }

    void run() {
        try {
            scope->run(*scope);
        } catch (const Exception& throw_exception) {
            print(throw_exception(source));
        }
    }

    Object& operator[](std::string name) const {
        return scope->variables[name];
    }

    std::string source;

  private:
    std::shared_ptr<Scope> scope;
    std::map<std::string, Function> functions;
    std::map<std::string, Object> types;

    friend struct Compiler;
    friend struct Parser;
};

}  // namespace llc

#endif  // LLC_TYPES_H