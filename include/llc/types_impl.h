#ifndef LLC_TYPES_IMPL_H
#define LLC_TYPES_IMPL_H

#include <llc/types.h>

namespace llc {

Struct& Struct::operator=(const Struct& rhs) {
    if (rhs.is_void)
        fatal("cannot copy from void");
    id = rhs.id;
    value = rhs.value;
    members = rhs.members;
    return *this;
}

const Struct& Struct::operator[](std::string name) const {
    auto iter = members.find(name);
    if (iter == members.end())
        fatal(id, " does not have a member named ", name);
    return iter->second;
}

std::vector<int> StructDecl::construct(Scope& scope, const StatementGroup& sg, int index) {
    LLC_CHECK(sg[index + 1].is_expression());
    LLC_CHECK(sg[index + 2].is_scope());
    Identifier* identifier = dynamic_cast<Identifier*>(sg[index + 1].expression.get());
    LLC_CHECK(identifier != nullptr);
    Struct new_struct;
    new_struct.id = identifier->name;
    for (const auto& variable : sg[index + 2].scope->variables)
        new_struct.members[variable.first] = variable.second;
    scope.types[identifier->name] = new_struct;
    return {index, index + 1, index + 2};
}

void Scope::execute() {
    for (const auto& statement : statements) {
        if (statement.is_expression())
            statement.expression->execute(*this);
        else if (statement.is_controlflow())
            statement.controlflow->execute(*this);
        else if (statement.is_struct_decl())
            fatal("find unsolved struct declaration");
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
        fatal("cannot find type \"", type, "\"");
        return {};
    }
}

////////////////////////////////////////////////
// Elementary
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
    case TokenType::Identifier:
        ptr = new Identifier(token.identifier, 10 + bias, token.line, token.column);
        break;
    case TokenType::Type:
        ptr = new Type(token.identifier, 10 + bias, token.line, token.column);
        break;
    case TokenType::Assign: ptr = new Assign(0 + bias, token.line, token.column); break;
    case TokenType::Dot: ptr = new MemberAccess(11 + bias, token.line, token.column); break;
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

std::vector<int> LeftUnaryOp::construct(Scope&, const ElementaryGroup& eg, int index) {
    LLC_INVOKE_IF(index + 1 >= (int)eg.size(), throw_error("syntax error: missing rhs"));
    right = eg[index + 1];
    return {index + 1};
}

std::vector<int> RightUnaryOp::construct(Scope&, const ElementaryGroup& eg, int index) {
    LLC_INVOKE_IF(index - 1 < 0, throw_error("syntax error: missing lhs"));
    left = eg[index - 1];
    return {index - 1};
}

std::vector<int> BinaryOp::construct(Scope&, const ElementaryGroup& eg, int index) {
    LLC_INVOKE_IF(index - 1 < 0, throw_error("syntax error: missing lhs"));
    LLC_INVOKE_IF(index + 1 >= (int)eg.size(), throw_error("syntax error: missing rhs"));
    left = eg[index - 1];
    right = eg[index + 1];
    return {index - 1, index + 1};
}

std::vector<int> Identifier::construct(Scope& scope, const ElementaryGroup&, int) {
    if (scope.search_variable(name) == nullptr)
        throw_error("syntax error: variable can only be constructed by type");
    return {};
}

ReturnType Identifier::execute(Scope& scope) const {
    Struct* value = scope.search_variable(name);
    if (value == nullptr)
        throw_error("variable \"" + name + "\" is not declared");
    if (value->is_void)
        throw_error("variable \"" + name + "\" is void");
    return *value;
}

std::vector<int> Type::construct(Scope& scope, const ElementaryGroup& eg, int index) {
    LLC_INVOKE_IF(index + 1 == (int)eg.size(),
                  throw_error("syntax error: expect variable name after type"));
    Identifier* variable = dynamic_cast<Identifier*>(eg[index + 1].get());
    LLC_INVOKE_IF(variable == nullptr,
                  throw_error("syntax error: expect variable declaration after type"));
    scope.declare_varible(type_name, variable->name);
    return {index};
}

ReturnType Type::execute(Scope&) const {
    throw_error("\"type\" is not executable");
    return {};
}

std::vector<int> Assign::construct(Scope&, const ElementaryGroup& eg, int index) {
    LLC_INVOKE_IF(index == 0, throw_error("syntax error: missing lhs"));
    LLC_INVOKE_IF(index + 1 == (int)eg.size(), throw_error("syntax error: missing rhs"));
    expression = eg[index + 1];
    Identifier* variable = dynamic_cast<Identifier*>(eg[index - 1].get());
    LLC_INVOKE_IF(variable == nullptr, throw_error("syntax error: expect variable on lhs"));
    variable_name = variable->name;
    return {index - 1, index + 1};
}

ReturnType Assign::execute(Scope& scope) const {
    Struct* value = scope.search_variable(variable_name);
    LLC_CHECK(value != nullptr);
    LLC_CHECK(expression != nullptr);
    *value = expression->execute(scope);
    return *value;
}

ReturnType Add::execute(Scope& scope) const {
    auto value0 = left->execute(scope);
    auto value1 = right->execute(scope);
    if (!value0.is_fundamental() || !value1.is_fundamental())
        fatal("cannot add two struct");
    value0.value = value0.value + value1.value;
    return value0;
}

ReturnType Substract::execute(Scope& scope) const {
    auto value0 = left->execute(scope);
    auto value1 = right->execute(scope);
    if (!value0.is_fundamental() || !value1.is_fundamental())
        fatal("cannot substract two struct");
    value0.value = value0.value - value1.value;
    return value0;
}

ReturnType Multiply::execute(Scope& scope) const {
    auto value0 = left->execute(scope);
    auto value1 = right->execute(scope);
    if (!value0.is_fundamental() || !value1.is_fundamental())
        fatal("cannot multiply two struct");
    value0.value = value0.value * value1.value;
    return value0;
}

ReturnType Divide::execute(Scope& scope) const {
    auto value0 = left->execute(scope);
    auto value1 = right->execute(scope);
    if (!value0.is_fundamental() || !value1.is_fundamental())
        fatal("cannot divide two struct");
    value0.value = value0.value / value1.value;
    return value0;
}

ReturnType Print::execute(Scope& scope) const {
    ReturnType ret = right->execute(scope);
    if (ret.is_fundamental())
        print(ret.value);
    else
        throw_error("type error: rhs does not return a value");
    return {};
}

ReturnType LessThan::execute(Scope& scope) const {
    ReturnType value0 = left->execute(scope);
    ReturnType value1 = right->execute(scope);
    if (!value0.is_fundamental() || !value1.is_fundamental())
        fatal("cannot compare two struct");
    return value0.value < value1.value;
}

ReturnType LessEqual::execute(Scope& scope) const {
    ReturnType value0 = left->execute(scope);
    ReturnType value1 = right->execute(scope);
    if (!value0.is_fundamental() || !value1.is_fundamental())
        fatal("cannot compare two struct");
    return value0.value <= value1.value;
}

ReturnType GreaterThan::execute(Scope& scope) const {
    ReturnType value0 = left->execute(scope);
    ReturnType value1 = right->execute(scope);
    if (!value0.is_fundamental() || !value1.is_fundamental())
        fatal("cannot compare two struct");
    return value0.value > value1.value;
}

ReturnType GreaterEqual::execute(Scope& scope) const {
    ReturnType value0 = left->execute(scope);
    ReturnType value1 = right->execute(scope);
    if (!value0.is_fundamental() || !value1.is_fundamental())
        fatal("cannot compare two struct");
    return value0.value >= value1.value;
}

ReturnType Equal::execute(Scope& scope) const {
    ReturnType value0 = left->execute(scope);
    ReturnType value1 = right->execute(scope);
    if (!value0.is_fundamental() || !value1.is_fundamental())
        fatal("cannot compare two struct");
    return value0.value == value1.value;
}

ReturnType NotEqual::execute(Scope& scope) const {
    ReturnType value0 = left->execute(scope);
    ReturnType value1 = right->execute(scope);
    if (!value0.is_fundamental() || !value1.is_fundamental())
        fatal("cannot compare two struct");
    return value0.value == value1.value;
}

std::vector<int> Increment::construct(Scope&, const ElementaryGroup& eg, int index) {
    if (index + 1 < (int)eg.size() && dynamic_cast<Identifier*>(eg[index + 1].get())) {
        is_post_increment = true;
        variable_name = dynamic_cast<Identifier*>(eg[index + 1].get())->name;
        return {index + 1};
    } else if (index - 1 >= 0 && dynamic_cast<Identifier*>(eg[index - 1].get())) {
        is_post_increment = false;
        variable_name = dynamic_cast<Identifier*>(eg[index - 1].get())->name;
        return {index - 1};
    } else {
        throw_error("missing variable");
        return {};
    }
}
ReturnType Increment::execute(Scope& scope) const {
    Struct* object = scope.search_variable(variable_name);
    if (object == nullptr)
        throw_error("cannot find variable of name \"" + variable_name + "\"");
    return is_post_increment ? (object->value)++ : ++(object->value);
}

std::vector<int> Decrement::construct(Scope&, const ElementaryGroup& eg, int index) {
    if (index + 1 < (int)eg.size() && dynamic_cast<Identifier*>(eg[index + 1].get())) {
        is_post_decrement = true;
        variable_name = dynamic_cast<Identifier*>(eg[index + 1].get())->name;
        return {index + 1};
    } else if (index - 1 >= 0 && dynamic_cast<Identifier*>(eg[index - 1].get())) {
        is_post_decrement = false;
        variable_name = dynamic_cast<Identifier*>(eg[index - 1].get())->name;
        return {index - 1};
    } else {
        throw_error("missing variable");
        return {};
    }
}
ReturnType Decrement::execute(Scope& scope) const {
    Struct* object = scope.search_variable(variable_name);
    if (object == nullptr)
        throw_error("cannot find variable of name \"" + variable_name + "\"");
    return is_post_decrement ? (object->value)-- : --(object->value);
}

std::vector<int> MemberAccess::construct(Scope&, const ElementaryGroup& eg, int index) {
    Identifier* identifier = dynamic_cast<Identifier*>(eg[index - 1].get());
    Identifier* field = dynamic_cast<Identifier*>(eg[index + 1].get());
    LLC_CHECK(identifier != nullptr);
    variable_name = identifier->name;
    field_name = field->name;
    return {index - 1, index + 1};
}
ReturnType MemberAccess::execute(Scope& scope) const {
    Struct* variable = scope.search_variable(variable_name);
    LLC_CHECK(variable != nullptr);
    return (*variable)[field_name];
}

////////////////////////////////////////////////
// Control Flow
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

std::vector<int> ElseIf::construct(Scope&, const StatementGroup&, int) {
    throw_error("else if cannot appear alone");
    return {};
}
void ElseIf::execute(Scope&) const {
    throw_error("else if cannot appear alone");
}

std::vector<int> Else::construct(Scope&, const StatementGroup&, int) {
    throw_error("else cannot appear alone");
    return {};
}
void Else::execute(Scope&) const {
    throw_error("else cannot appear alone");
}

std::vector<int> If::construct(Scope&, const StatementGroup& sg, int index) {
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
void If::execute(Scope& scope) const {
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

std::vector<int> For::construct(Scope&, const StatementGroup& sg, int index) {
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
void For::execute(Scope& scope) const {
    LLC_CHECK(initialization != nullptr);
    LLC_CHECK(condition != nullptr);
    LLC_CHECK(updation != nullptr);
    LLC_CHECK(action != nullptr);
    for (initialization->execute(scope); condition->execute(scope).value;
         (void)updation->execute(scope).value) {
        action->execute();
    }
}

std::vector<int> While::construct(Scope&, const StatementGroup& sg, int index) {
    LLC_INVOKE_IF(index + 1 >= (int)sg.size() || !sg[index + 1].is_expression(),
                  throw_error("expect expression after \"while\""));
    LLC_INVOKE_IF(index + 2 >= (int)sg.size() || !sg[index + 2].is_scope(),
                  throw_error("expect scope after \"while\""));
    condition = sg[index + 1].expression;
    action = sg[index + 2].scope;
    return {index + 1, index + 2};
}
void While::execute(Scope& scope) const {
    LLC_CHECK(condition != nullptr);
    LLC_CHECK(action != nullptr);
    while (condition->execute(scope).value) {
        action->execute();
    }
}

}  // namespace llc

#endif  // LLC_TYPES_IMPL_H