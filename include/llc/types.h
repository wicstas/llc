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
    Char = 1ul << 23,
    String = 1ul << 24,
    LeftSquareBracket = 1ul << 25,
    RightSquareBracket = 1ul << 26,
    PlusEqual = 1ul << 27,
    MinusEqual = 1ul << 28,
    MultiplyEqual = 1ul << 29,
    DivideEqual = 1ul << 30,
    Invalid = 1ul << 31,
    Eof = 1ul << 32,
    NumTokens = 1ul << 33
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
    char value_c;
    std::string value_s;
    std::string id;
};

struct Scope;
struct Expression;

extern std::map<size_t, std::string> type_id_to_name;

template <typename T>
std::string get_type_name() {
    using Ty = std::decay_t<T>;
    auto it = type_id_to_name.find(typeid(Ty).hash_code());
    if (it == type_id_to_name.end())
        throw_exception("cannot get name of unregistered type T, typeid(T).name(): \"",
                        typeid(Ty).name(), "\"");
    return it->second;
}

struct Object;
struct Function;

struct BreakLoop {};

struct BaseFunction {
    virtual ~BaseFunction() = default;
    virtual BaseFunction* clone() const = 0;
    virtual std::optional<Object> run(const Scope& scope,
                                      const std::vector<Expression>& exprs) const = 0;
};

struct BaseObject {
    BaseObject() = default;
    BaseObject(std::string type_name) : type_name_(type_name){};
    virtual ~BaseObject() = default;
    virtual BaseObject* clone() const = 0;
    virtual BaseObject* alloc() const = 0;

    virtual Object construct(const std::vector<Object>& objects) = 0;

    virtual void* ptr() const = 0;
    virtual void assign(const BaseObject* rhs) = 0;
    virtual void add(BaseObject* rhs) = 0;
    virtual void sub(BaseObject* rhs) = 0;
    virtual void mul(BaseObject* rhs) = 0;
    virtual void div(BaseObject* rhs) = 0;
    virtual Object neg() const = 0;
    virtual void increment() = 0;
    virtual void decrement() = 0;
    virtual bool less_than(BaseObject* rhs) const = 0;
    virtual bool less_equal(BaseObject* rhs) const = 0;
    virtual bool greater_than(BaseObject* rhs) const = 0;
    virtual bool greater_equal(BaseObject* rhs) const = 0;
    virtual bool equal(BaseObject* rhs) const = 0;
    virtual bool not_equal(BaseObject* rhs) const = 0;

    virtual Object get_element(size_t index) const = 0;
    virtual void set_element(size_t index, Object object) = 0;

    template <typename T>
    T as() const {
        if constexpr (std::is_arithmetic<T>::value) {
            if (type_name() == "int")
                return T(*(int*)ptr());
            else if (type_name() == "bool")
                return T(*(bool*)ptr());
            else if (type_name() == "uint8_t")
                return T(*(uint8_t*)ptr());
            else if (type_name() == "uint16_t")
                return T(*(uint16_t*)ptr());
            else if (type_name() == "uint32_t")
                return T(*(uint32_t*)ptr());
            else if (type_name() == "uint64_t")
                return T(*(uint64_t*)ptr());
            else if (type_name() == "float")
                return T(*(float*)ptr());
        }

        if (type_name() != get_type_name<T>())
            throw_exception("cannot convert type \"", type_name(), "\" to type \"",
                            get_type_name<std::decay_t<T>>(), "\"");

        return *(std::decay_t<T>*)ptr();
    }

    template <typename T>
    std::optional<T> as_opt() const {
        if constexpr (std::is_arithmetic<T>::value) {
            if (type_name() == "int")
                return T(*(int*)ptr());
            else if (type_name() == "bool")
                return T(*(bool*)ptr());
            else if (type_name() == "uint8_t")
                return T(*(uint8_t*)ptr());
            else if (type_name() == "uint16_t")
                return T(*(uint16_t*)ptr());
            else if (type_name() == "uint32_t")
                return T(*(uint32_t*)ptr());
            else if (type_name() == "uint64_t")
                return T(*(uint64_t*)ptr());
            else if (type_name() == "float")
                return T(*(float*)ptr());
        }

        if (type_name() != get_type_name<T>())
            return std::nullopt;

        return *(std::decay_t<T>*)ptr();
    }

    std::string type_name() const {
        return type_name_;
    }
    Object& get_member(std::string name);

    mutable std::map<std::string, Object> members;
    mutable std::map<std::string, Function> functions;

    std::string type_name_;
};

struct Object {
    static Object construct(Object type, std::vector<Object> args) {
        LLC_CHECK(type.base != nullptr);
        return type.base->construct(args);
    }

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

    Object alloc() const {
        return Object(std::unique_ptr<BaseObject>(base->alloc()));
    }

    void assign(const Object& rhs) {
        LLC_CHECK(base != nullptr);
        LLC_CHECK(rhs.base != nullptr);
        base->assign(rhs.base.get());
    }

    template <typename T>
    T as() const {
        if (base == nullptr)
            throw_exception("cannot cast \"void\" to type \"", get_type_name<T>(), "\"");
        return base->as<T>();
    }

    template <typename T>
    std::optional<T> as_opt() const {
        if (base == nullptr)
            throw_exception("cannot cast \"void\" to type \"", get_type_name<T>(), "\"");
        return base->as_opt<T>();
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
    Object operator-() {
        return base->neg();
    }
    Object& operator++() {
        LLC_CHECK(base != nullptr);
        base->increment();
        return *this;
    }
    Object operator++(int) {
        LLC_CHECK(base != nullptr);
        Object temp = *this;
        base->increment();
        return temp;
    }
    Object& operator--() {
        LLC_CHECK(base != nullptr);
        base->decrement();
        return *this;
    }
    Object operator--(int) {
        LLC_CHECK(base != nullptr);
        Object temp = *this;
        base->decrement();
        return temp;
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

    Object& operator[](std::string name) & {
        LLC_CHECK(base != nullptr);
        return base->get_member(name);
    }
    Object& operator[](std::string name) && {
        LLC_CHECK(base != nullptr);
        throw_exception("cannot get member(which store a reference to part of that temporary "
                        "object) of temporary object");
        return base->get_member(name);
    }

    struct ArrayElementProxy {
        ArrayElementProxy(BaseObject* base, size_t index) : base(base), index(index) {
        }

        operator Object() const {
            return base->get_element(index);
        }
        ArrayElementProxy& operator=(Object object) {
            base->set_element(index, object);
            return *this;
        }

        BaseObject* base;
        size_t index;
    };
    ArrayElementProxy operator[](size_t index) {
        LLC_CHECK(base != nullptr);
        return ArrayElementProxy(base.get(), index);
    }

    std::unique_ptr<BaseObject> base;
};

template <typename T>
struct ConcreteObject : BaseObject {
    ConcreteObject() = default;
    ConcreteObject(T value) : BaseObject(get_type_name<T>()), value(value){};

    BaseObject* clone() const override;
    BaseObject* alloc() const override {
        using Ty = std::decay_t<T>;
        if constexpr (!std::is_pointer<T>::value) {
            type_id_to_name[typeid(Ty*).hash_code()] = get_type_name<Ty>() + "*";
            return new ConcreteObject<Ty*>(new Ty(value));
        } else {
            throw_exception("only one level of indirection is supported");
            return nullptr;
        }
    }

    Object construct(const std::vector<Object>& objects) override {
        if (constructors.size() == 0)
            throw_exception("no constructor was binded for type \"", type_name(), "\"");
        for (const auto& ctor : constructors)
            if (ctor->is_viable(objects))
                return ctor->construct(objects);
        throw_exception("no viable constructor found for type \"", type_name(), "\"");
        return {};
    }

    void* ptr() const override {
        return (void*)&value;
    }
    void assign(const BaseObject* rhs) override {
        value = rhs->as<typename std::decay_t<T>>();
    }

    void bind_members() {
        for (const auto& accessor : accessors)
            members[accessor.first] = accessor.second->access(value);
    }

    void add(BaseObject* rhs) override {
        if constexpr (HasOperatorAdd<T>::value)
            value += rhs->as<T>();
        else
            throw_exception("type \"", type_name(), "\" does not have operator \"+\"");
    }
    void sub(BaseObject* rhs) override {
        if constexpr (HasOperatorSub<T>::value && !std::is_pointer<T>::value)
            value -= rhs->as<T>();
        else
            throw_exception("type \"", type_name(), "\" does not have operator \"-\"");
    }
    void mul(BaseObject* rhs) override {
        if constexpr (HasOperatorMul<T>::value)
            value *= rhs->as<T>();
        else
            throw_exception("type \"", type_name(), "\" does not have operator \"*\"");
    }
    void div(BaseObject* rhs) override {
        if constexpr (HasOperatorDiv<T>::value)
            value /= rhs->as<T>();
        else
            throw_exception("type \"", type_name(), "\" does not have operator \"/\"");
    }
    Object neg() const override {
        if constexpr (HasOperatorNegation<T>::value)
            return Object(-value);
        else
            throw_exception("type \"", type_name(), "\" does not have operator \"/\"");
        return {};
    }
    void increment() override {
        if constexpr (HasOperatorPreIncrement<T>::value)
            ++value;
        else
            throw_exception("type \"", type_name(), "\" does not have operator \"++\"");
    }
    void decrement() override {
        if constexpr (HasOperatorPreIncrement<T>::value)
            --value;
        else
            throw_exception("type \"", type_name(), "\" does not have operator \"--\"");
    }
    bool less_than(BaseObject* rhs) const override {
        if constexpr (HasOperatorLT<T>::value)
            return value < rhs->as<T>();
        else
            throw_exception("type \"", type_name(), "\" does not have operator \"<\"");
        return {};
    }
    bool less_equal(BaseObject* rhs) const override {
        if constexpr (HasOperatorLE<T>::value)
            return value <= rhs->as<T>();
        else
            throw_exception("type \"", type_name(), "\" does not have operator \"<=\"");
        return {};
    }
    bool greater_than(BaseObject* rhs) const override {
        if constexpr (HasOperatorGT<T>::value)
            return value > rhs->as<T>();
        else
            throw_exception("type \"", type_name(), "\" does not have operator \">\"");
        return {};
    }
    bool greater_equal(BaseObject* rhs) const override {
        if constexpr (HasOperatorGE<T>::value)
            return value >= rhs->as<T>();
        else
            throw_exception("type \"", type_name(), "\" does not have operator \">=\"");
        return {};
    }
    bool equal(BaseObject* rhs) const override {
        if constexpr (HasOperatorEQ<T>::value)
            return value == rhs->as<T>();
        else
            throw_exception("type \"", type_name(), "\" does not have operator \"==\"");
        return {};
    }
    bool not_equal(BaseObject* rhs) const override {
        if constexpr (HasOperatorNE<T>::value)
            return value != rhs->as<T>();
        else
            throw_exception("type \"", type_name(), "\" does not have operator \"!=\"");
        return {};
    }
    Object get_element(size_t index) const override {
        if constexpr (HasOperatorArrayAccess<const T>::value)
            return (Object)std::make_unique<ConcreteObject<std::decay_t<decltype(value[index])>>>(
                value[index]);
        else
            throw_exception("type \"", type_name(), "\" does not have operator \"[]\" const");
        return {};
    }
    void set_element(size_t index, Object object) override {
        if constexpr (HasOperatorArrayAccess<T>::value)
            value[index] = object.as<std::decay_t<decltype(value[index])>>();
        else
            throw_exception("type \"", type_name(), "\" does not have operator \"[]\"");
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

    struct Constructor {
        virtual ~Constructor() = default;
        virtual Object construct(const std::vector<Object>& objects) const = 0;
        virtual bool is_viable(const std::vector<Object>& objects) const = 0;
    };

    template <int index, typename... Args>
    static bool objects_to_args(std::tuple<Args...>& args, const std::vector<Object>& objects) {
        if (auto arg = objects[index].as_opt<std::tuple_element_t<index, std::tuple<Args...>>>())
            std::get<index>(args) = *arg;
        else
            return false;

        if constexpr (index != sizeof...(Args) - 1)
            return objects_to_args<index + 1>(args, objects);
        else
            return true;
    }

    template <typename... Args>
    struct ConcreteConstructor : Constructor {
        Object construct(const std::vector<Object>& objects) const override {
            LLC_CHECK(objects.size() == sizeof...(Args));
            std::tuple<Args...> args;
            objects_to_args<0>(args, objects);
            return Object(std::make_from_tuple<T>(args));
        }
        bool is_viable(const std::vector<Object>& objects) const override {
            if (objects.size() != sizeof...(Args))
                return false;
            std::tuple<Args...> args;
            return objects_to_args<0>(args, objects);
        }
    };

    T value;
    std::map<std::string, std::shared_ptr<Accessor>> accessors;
    std::vector<std::shared_ptr<Constructor>> constructors;
};

struct InternalObject : BaseObject {
    using BaseObject::BaseObject;

    BaseObject* clone() const override;
    BaseObject* alloc() const override {
        throw_exception("internal object does not support \"new\"");
        return nullptr;
    }

    Object construct(const std::vector<Object>& objects) override {
        LLC_CHECK(objects.size() == members.size());
        throw_exception("internal object does not support constructor yet");
        return {};
    }

    void* ptr() const override {
        throw_exception("cannot get pointer to internal type");
        return nullptr;
    };
    void assign(const BaseObject* rhs) override {
        auto ptr = dynamic_cast<const InternalObject*>(rhs);
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
            throw_exception("subtract internal type by external type is not allowed");
        for (auto& member : ptr->members)
            members[member.first] -= member.second;
    }
    void mul(BaseObject* rhs) override {
        auto ptr = dynamic_cast<const InternalObject*>(rhs);
        if (ptr == nullptr)
            throw_exception("multiply internal type by external type is not allowed");
        for (auto& member : ptr->members)
            members[member.first] *= member.second;
    }
    void div(BaseObject* rhs) override {
        auto ptr = dynamic_cast<const InternalObject*>(rhs);
        if (ptr == nullptr)
            throw_exception("divide internal type by external type is not allowed");
        for (auto& member : ptr->members)
            members[member.first] /= member.second;
    }
    Object neg() const override {
        std::unique_ptr<BaseObject> copy(clone());
        InternalObject* ptr = dynamic_cast<InternalObject*>(copy.get());
        for (auto& member : ptr->members)
            ptr->members[member.first] = -member.second;
        return Object(std::move(copy));
    }
    void increment() override {
        for (auto& member : members)
            ++member.second;
    }
    void decrement() override {
        for (auto& member : members)
            --member.second;
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
    Object get_element(size_t) const override {
        throw_exception("internal type does not support operator []");
        return {};
    }
    void set_element(size_t, Object) override {
        throw_exception("internal type does not support operator []");
    }
};

template <typename T, typename>
Object::Object(T instance) : base(std::make_unique<ConcreteObject<T>>(instance)) {
}

struct InternalFunction : BaseFunction {
    BaseFunction* clone() const override {
        return new InternalFunction(*this);
    }
    std::optional<Object> run(const Scope& scope,
                              const std::vector<Expression>& exprs) const override;

    std::optional<Object> return_type;
    std::shared_ptr<Scope> definition;
    std::map<std::string, Object*> this_scope;
    std::vector<std::string> parameters;
};

struct ExternalFunction : BaseFunction {
    std::optional<Object> run(const Scope& scope,
                              const std::vector<Expression>& exprs) const override;
    virtual void bind_object(BaseObject*) {
    }

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
            else if constexpr (sizeof...(Args) == 3)
                f(args[0].as<decltype(types.template at<0>())>(),
                  args[1].as<decltype(types.template at<1>())>(),
                  args[2].as<decltype(types.template at<2>())>());
            else if constexpr (sizeof...(Args) == 4)
                f(args[0].as<decltype(types.template at<0>())>(),
                  args[1].as<decltype(types.template at<1>())>(),
                  args[2].as<decltype(types.template at<2>())>(),
                  args[3].as<decltype(types.template at<3>())>());
            else if constexpr (sizeof...(Args) == 5)
                f(args[0].as<decltype(types.template at<0>())>(),
                  args[1].as<decltype(types.template at<1>())>(),
                  args[2].as<decltype(types.template at<2>())>(),
                  args[3].as<decltype(types.template at<3>())>(),
                  args[4].as<decltype(types.template at<4>())>());
            else if constexpr (sizeof...(Args) == 6)
                f(args[0].as<decltype(types.template at<0>())>(),
                  args[1].as<decltype(types.template at<1>())>(),
                  args[2].as<decltype(types.template at<2>())>(),
                  args[3].as<decltype(types.template at<3>())>(),
                  args[4].as<decltype(types.template at<4>())>(),
                  args[5].as<decltype(types.template at<5>())>());
            else if constexpr (sizeof...(Args) == 7)
                f(args[0].as<decltype(types.template at<0>())>(),
                  args[1].as<decltype(types.template at<1>())>(),
                  args[2].as<decltype(types.template at<2>())>(),
                  args[3].as<decltype(types.template at<3>())>(),
                  args[4].as<decltype(types.template at<4>())>(),
                  args[5].as<decltype(types.template at<5>())>(),
                  args[6].as<decltype(types.template at<6>())>());
            else if constexpr (sizeof...(Args) == 8)
                f(args[0].as<decltype(types.template at<0>())>(),
                  args[1].as<decltype(types.template at<1>())>(),
                  args[2].as<decltype(types.template at<2>())>(),
                  args[3].as<decltype(types.template at<3>())>(),
                  args[4].as<decltype(types.template at<4>())>(),
                  args[5].as<decltype(types.template at<5>())>(),
                  args[6].as<decltype(types.template at<6>())>(),
                  args[7].as<decltype(types.template at<7>())>());
            else
                throw_exception("too many arguments, only support <= 8");

        } else {
            if constexpr (sizeof...(Args) == 0)
                return f();
            else if constexpr (sizeof...(Args) == 1)
                return f(args[0].as<decltype(types.template at<0>())>());
            else if constexpr (sizeof...(Args) == 2)
                return f(args[0].as<decltype(types.template at<0>())>(),
                         args[1].as<decltype(types.template at<1>())>());
            else if constexpr (sizeof...(Args) == 3)
                return f(args[0].as<decltype(types.template at<0>())>(),
                         args[1].as<decltype(types.template at<1>())>(),
                         args[2].as<decltype(types.template at<2>())>());
            else if constexpr (sizeof...(Args) == 4)
                return f(args[0].as<decltype(types.template at<0>())>(),
                         args[1].as<decltype(types.template at<1>())>(),
                         args[2].as<decltype(types.template at<2>())>(),
                         args[3].as<decltype(types.template at<3>())>());
            else if constexpr (sizeof...(Args) == 5)
                return f(args[0].as<decltype(types.template at<0>())>(),
                         args[1].as<decltype(types.template at<1>())>(),
                         args[2].as<decltype(types.template at<2>())>(),
                         args[3].as<decltype(types.template at<3>())>(),
                         args[4].as<decltype(types.template at<4>())>());
            else if constexpr (sizeof...(Args) == 6)
                return f(args[0].as<decltype(types.template at<0>())>(),
                         args[1].as<decltype(types.template at<1>())>(),
                         args[2].as<decltype(types.template at<2>())>(),
                         args[3].as<decltype(types.template at<3>())>(),
                         args[4].as<decltype(types.template at<4>())>(),
                         args[5].as<decltype(types.template at<5>())>());
            else if constexpr (sizeof...(Args) == 7)
                return f(args[0].as<decltype(types.template at<0>())>(),
                         args[1].as<decltype(types.template at<1>())>(),
                         args[2].as<decltype(types.template at<2>())>(),
                         args[3].as<decltype(types.template at<3>())>(),
                         args[4].as<decltype(types.template at<4>())>(),
                         args[5].as<decltype(types.template at<5>())>(),
                         args[6].as<decltype(types.template at<6>())>());
            else if constexpr (sizeof...(Args) == 8)
                return f(args[0].as<decltype(types.template at<0>())>(),
                         args[1].as<decltype(types.template at<1>())>(),
                         args[2].as<decltype(types.template at<2>())>(),
                         args[3].as<decltype(types.template at<3>())>(),
                         args[4].as<decltype(types.template at<4>())>(),
                         args[5].as<decltype(types.template at<5>())>(),
                         args[6].as<decltype(types.template at<6>())>(),
                         args[7].as<decltype(types.template at<7>())>());
            else
                throw_exception("too many arguments, only support <= 8");
        }

        return {};
    }

    F f;
};

template <typename T, typename R, typename... Args>
struct ConcreteMemberFunction : ExternalFunction {
    using F = R (T::*)(Args...);
    ConcreteMemberFunction(ConcreteObject<T>* object, F f) : object(object), f(f){};

    BaseFunction* clone() const override {
        return new ConcreteMemberFunction<T, R, Args...>(*this);
    }
    void bind_object(BaseObject* ptr) override {
        object = dynamic_cast<ConcreteObject<T>*>(ptr);
        LLC_CHECK(object != nullptr);
    }
    Object invoke(const std::vector<Object>& args) const override {
        LLC_CHECK(args.size() == sizeof...(Args));
        TypePack<Args...> types;

        LLC_CHECK(object != nullptr);

        if constexpr (std::is_same<R, void>::value) {
            if constexpr (sizeof...(Args) == 0)
                (object->value.*f)();
            else if constexpr (sizeof...(Args) == 1)
                (object->value.*f)(args[0].as<decltype(types.template at<0>())>());
            else if constexpr (sizeof...(Args) == 2)
                (object->value.*f)(args[0].as<decltype(types.template at<0>())>(),
                                   args[1].as<decltype(types.template at<1>())>());
            else
                throw_exception("too many arguments, only support <= 4");

        } else {
            if constexpr (sizeof...(Args) == 0)
                return (Object)(object->value.*f)();
            else if constexpr (sizeof...(Args) == 1)
                return (Object)(object->value.*f)(args[0].as<decltype(types.template at<0>())>());
            else if constexpr (sizeof...(Args) == 2)
                return (Object)(object->value.*f)(args[0].as<decltype(types.template at<0>())>(),
                                                  args[1].as<decltype(types.template at<1>())>());
            else
                throw_exception("too many arguments, only support <= 4");
        }

        return {};
    }

    ConcreteObject<T>* object;
    F f;
};

template <typename T>
BaseObject* ConcreteObject<T>::clone() const {
    ConcreteObject<T>* object = new ConcreteObject<T>(*this);
    object->bind_members();
    for (auto& f : object->functions) {
        dynamic_cast<ExternalFunction*>(f.second.base.get())->bind_object(object);
    }
    return object;
}

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
    virtual Object& original(const Scope&) const {
        throw_exception("Operand::original is unimplmented for this class");
        static Object null_object;
        return null_object;
    }

    virtual Object assign(const Scope&, const Object&) {
        throw_exception("Operand::assign is unimplmented for this class");
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

struct CharLiteral : BaseOp {
    CharLiteral(char value) : value(value){};

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
    char value;
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

    Object& original(const Scope& scope) const override {
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
        auto member = dynamic_cast<ObjectMember*>(b.get());
        LLC_CHECK(member != nullptr);
        return a->original(scope)[member->name];
    }
    Object& original(const Scope& scope) const override {
        auto member = dynamic_cast<ObjectMember*>(b.get());
        LLC_CHECK(member != nullptr);
        return a->original(scope)[member->name];
    }
    Object assign(const Scope& scope, const Object& value) override {
        auto member = dynamic_cast<ObjectMember*>(b.get());
        LLC_CHECK(member != nullptr);
        a->original(scope)[member->name].assign(value);
        return a->original(scope)[member->name];
    }

    int get_precedence() const override {
        return precedence;
    }
    void set_precedence(int prec) override {
        precedence = prec;
    }
    int precedence = 10;
};

struct MemberFunctionCall : PostUnaryOp {
    Object evaluate(const Scope& scope) const override;
    Object assign(const Scope&, const Object&) override {
        throw_exception("cannot assign a member function");
        return {};
    }

    int get_precedence() const override {
        return precedence;
    }
    void set_precedence(int prec) override {
        precedence = prec;
    }
    int precedence = 10;

    std::string function_name;
    std::vector<Expression> arguments;
};

struct ArrayAccess : BinaryOp {
    Object evaluate(const Scope& scope) const override {
        Object arr = a->evaluate(scope);
        size_t index = (int)b->evaluate(scope).as<size_t>();
        return arr[index];
    }
    Object assign(const Scope& scope, const Object& object) override {
        Object& arr = ((VariableOp*)a.get())->original(scope);
        size_t index = (int)b->evaluate(scope).as<size_t>();
        return arr[index] = object;
    }

    int get_precedence() const override {
        return precedence;
    }
    void set_precedence(int prec) override {
        precedence = prec;
    }

    int precedence = 10;
};

struct TypeOp : BaseOp {
    TypeOp(Object type) : type(type){};

    Object evaluate(const Scope& scope) const override;

    int get_precedence() const override {
        return precedence;
    }
    void set_precedence(int prec) override {
        precedence = prec;
    }

    int precedence = 8;
    Object type;
    std::vector<Expression> arguments;
};

struct NewOp : PreUnaryOp {
    Object evaluate(const Scope& scope) const override {
        return operand->evaluate(scope).alloc();
    }

    int get_precedence() const override {
        return precedence;
    }
    void set_precedence(int prec) override {
        precedence = prec;
    }

    int precedence = 8;
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

struct AddEqual : BinaryOp {
    Object evaluate(const Scope& scope) const override {
        return a->original(scope) += b->evaluate(scope);
    }

    int get_precedence() const override {
        return precedence;
    }
    void set_precedence(int prec) override {
        precedence = prec;
    }
    int precedence = 3;
};

struct SubtractEqual : BinaryOp {
    Object evaluate(const Scope& scope) const override {
        return a->original(scope) -= b->evaluate(scope);
    }

    int get_precedence() const override {
        return precedence;
    }
    void set_precedence(int prec) override {
        precedence = prec;
    }
    int precedence = 3;
};

struct MultiplyEqual : BinaryOp {
    Object evaluate(const Scope& scope) const override {
        return a->original(scope) *= b->evaluate(scope);
    }

    int get_precedence() const override {
        return precedence;
    }
    void set_precedence(int prec) override {
        precedence = prec;
    }
    int precedence = 3;
};

struct DivideEqual : BinaryOp {
    Object evaluate(const Scope& scope) const override {
        return a->original(scope) /= b->evaluate(scope);
    }

    int get_precedence() const override {
        return precedence;
    }
    void set_precedence(int prec) override {
        precedence = prec;
    }
    int precedence = 3;
};

struct PostIncrement : PostUnaryOp {
    Object evaluate(const Scope& scope) const override {
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
    Object evaluate(const Scope& scope) const override {
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
    Object evaluate(const Scope& scope) const override {
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
    Object evaluate(const Scope& scope) const override {
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

struct Negation : PreUnaryOp {
    Object evaluate(const Scope& scope) const override {
        return -operand->evaluate(scope);
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

struct LeftSquareBracket : Operand {
    std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>&, int) override {
        throw_exception("LeftSquareBracket::collapse() shall not be called");
        return {};
    }
    Object evaluate(const Scope&) const override {
        throw_exception("LeftSquareBracket::evaluate() shall not be called");
        return {};
    }
    int get_precedence() const override {
        return 0;
    }
    void set_precedence(int) override {
    }
};

struct RightSquareBracket : Operand {
    std::vector<int> collapse(const std::vector<std::shared_ptr<Operand>>&, int) override {
        throw_exception("RightSquareBracket::collapse() shall not be called");
        return {};
    }
    Object evaluate(const Scope&) const override {
        throw_exception("RightSquareBracket::evaluate() shall not be called");
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
        if (auto func = scope.find_function(function_name))
            return func->run(scope, arguments);
        else
            throw_exception("cannot find function \"", function_name, "\"");
        return std::nullopt;
    }

    std::string function_name;
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

struct Break : Statement {
    std::optional<Object> run(const Scope&) const override {
        throw BreakLoop();
        return std::nullopt;
    }
};

struct IfElseChain : Statement {
    IfElseChain(std::vector<Expression> conditions, std::vector<std::shared_ptr<Scope>> actions)
        : conditions(conditions), actions(actions){};

    std::optional<Object> run(const Scope& scope) const override;

    std::vector<Expression> conditions;
    std::vector<std::shared_ptr<Scope>> actions;
};

struct For : Statement {
    For(Expression initialization, Expression condition, Expression updation,
        std::shared_ptr<Scope> internal_scope, std::shared_ptr<Scope> action)
        : initialization(initialization),
          condition(condition),
          updation(updation),
          internal_scope(internal_scope),
          action(action){};

    std::optional<Object> run(const Scope& scope) const override;

    Expression initialization, condition, updation;
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
    template <typename T, typename = typename std::enable_if_t<!std::is_function_v<T>>>
    void bind(std::string name, const T& var) {
        using Ty = std::decay_t<T>;
        if constexpr (std::is_pointer_v<Ty>)
            type_id_to_name[typeid(Ty).hash_code()] =
                get_type_name<decltype(*(std::declval<Ty>()))>() + "*";
        variables[name] = Object(Ty(var));
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

        template <typename M,
                  typename = typename std::enable_if_t<!std::is_member_function_pointer_v<M T::*>>>
        TypeBindHelper& bind(std::string id, M T::*ptr) {
            using U = typename ConcreteObject<T>::template ConcreteAccessor<M>;
            object->accessors[id] = std::make_shared<U>(ptr);
            return *this;
        }
        template <typename F>
        TypeBindHelper& bind(std::string id, F&& func) {
            bind_func_impl(id, func);
            return *this;
        }

        template <typename... Args>
        TypeBindHelper& ctor() {
            using U = typename ConcreteObject<T>::template ConcreteConstructor<Args...>;
            object->constructors.push_back(std::make_shared<U>());
            return *this;
        }

      private:
        template <typename R = void, typename... Args>
        void bind_func_impl(std::string id, R (T::*func)(Args...)) {
            object->functions[id] =
                (Function)std::make_unique<ConcreteMemberFunction<T, R, Args...>>(
                    object.get(), (R(T::*)(Args...))func);
        }
        template <typename R = void, typename... Args>
        void bind_func_impl(std::string id, R (T::*func)(Args...) const) {
            object->functions[id] =
                (Function)std::make_unique<ConcreteMemberFunction<T, R, Args...>>(
                    object.get(), (R(T::*)(Args...))func);
        }
        template <typename R = void, typename... Args>
        void bind_func_impl(std::string id, R (T::*func)(Args...) &) {
            object->functions[id] =
                (Function)std::make_unique<ConcreteMemberFunction<T, R, Args...>>(
                    object.get(), (R(T::*)(Args...))func);
        }
        template <typename R = void, typename... Args>
        void bind_func_impl(std::string id, R (T::*func)(Args...) const&) {
            object->functions[id] =
                (Function)std::make_unique<ConcreteMemberFunction<T, R, Args...>>(
                    object.get(), (R(T::*)(Args...))func);
        }

        std::string type_name;
        std::unique_ptr<ConcreteObject<T>> object;
        std::map<std::string, Object>& types;
    };

    template <typename T, typename = typename std::enable_if_t<!std::is_pointer_v<T>>>
    TypeBindHelper<T> bind(std::string name) {
        return TypeBindHelper<T>(name, types);
    }
    template <typename T, typename = typename std::enable_if_t<std::is_pointer_v<T>>>
    void bind(std::string name) {
        type_id_to_name[typeid(T).hash_code()] = name;
    }

    void run() {
        try {
            scope->run(*scope);
        } catch (const std::optional<Object>&) {
        }
    }

    struct Proxy {
        Proxy() = default;
        Proxy(std::shared_ptr<Scope> scope, Object& object) : scope(scope), object(&object) {
        }
        Proxy(std::shared_ptr<Scope> scope, Function func) : scope(scope), func(func) {
        }

        template <typename T>
        T as() {
            LLC_CHECK(object != nullptr);
            return object->as<T>();
        }

        template <typename T>
        std::optional<T> as_opt() {
            LLC_CHECK(object != nullptr);
            return object->as_opt<T>();
        }

        template <typename T>
        struct expr_helper_struct : BaseOp {
            expr_helper_struct(T value) : object(value) {
            }

            Object evaluate(const Scope&) const {
                return object;
            }
            Object& original(const Scope&) const {
                return object;
            }
            Object assign(const Scope&, const Object& object) {
                return this->object = object;
            }
            int get_precedence() const {
                return 10;
            }
            void set_precedence(int) {
            }

            mutable Object object;
        };

        template <typename T, typename... Args>
        void expr_helper(std::vector<Expression>& exprs, T first, Args... rest) const {
            Expression expr;
            expr.operands.push_back(std::make_shared<expr_helper_struct<T>>(first));
            exprs.push_back(expr);
            if constexpr (sizeof...(rest) > 0)
                expr_helper(exprs, rest...);
        }

        template <typename... Args>
        Object operator()(Args... args) const {
            LLC_CHECK(object == nullptr);

            std::vector<Expression> exprs;
            if constexpr (sizeof...(args) != 0)
                expr_helper(exprs, args...);
            auto object = func.run(*scope, exprs);
            if (object.has_value())
                return *object;
            else
                return {};
        }

        Proxy operator[](std::string name) {
            LLC_CHECK(object != nullptr);
            if (object->base->members.find(name) != object->base->members.end())
                return Proxy(scope, object->base->members[name]);

            LLC_CHECK(object->base->functions.find(name) != object->base->functions.end());
            return Proxy(scope, object->base->functions[name]);
        }

        std::shared_ptr<Scope> scope = nullptr;
        Object* object = nullptr;
        Function func;
    };

    Proxy operator[](std::string name) const {
        if (scope->find_variable(name))
            return Proxy(scope, scope->variables[name]);
        else if (scope->find_function(name))
            return Proxy(scope, scope->functions[name]);
        else
            throw_exception("\"", name, " is neither a function nor a variable");
        return {};
    }

    std::string source;
    std::string filepath;

  private:
    std::shared_ptr<Scope> scope;
    std::map<std::string, Function> functions;
    std::map<std::string, Object> types;
    std::map<std::string, Object> variables;

    friend struct Compiler;
    friend struct Parser;
};

}  // namespace llc

#endif  // LLC_TYPES_H