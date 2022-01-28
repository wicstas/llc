#include <llc/types.h>

#include <algorithm>

namespace llc {

std::string Location::operator()(const std::string& source) {
    LLC_CHECK(line >= 0);
    LLC_CHECK(column >= 0);
    LLC_CHECK(length > 0);
    std::vector<std::string> lines = separate_lines(source);

    std::string pos = std::to_string(line) + ':' + std::to_string(column) + ':';
    std::string raw = lines[line];
    std::string underline(raw.size(), ' ');
    for (int i = 0; i < length; i++) {
        LLC_CHECK(column + i < (int)underline.size());
        underline[column + i] = '~';
    }
    raw = pos + raw;
    underline = std::string(pos.size(), ' ') + underline;
    return raw + '\n' + underline;
}

Scope::Scope() {
    types["int"] = {};
    types["float"] = {};
    types["bool"] = {};
    types["void"] = {};
}
Struct Scope::run(Scope&) {
    for (const auto& statement : statements)
        LLC_CHECK(statement != nullptr);

    for (const auto& statement : statements) {
        auto result = statement->run(*this);
        if (result.is_return)
            return result;
        else if (dynamic_cast<Return*>(statement.get())) {
            result.is_return = true;
            return result;
        }
    }
    return {};
}
std::optional<Struct> Scope::find_type(std::string name) const {
    auto it = types.find(name);
    if (it == types.end())
        return parent ? parent->find_type(name) : std::nullopt;
    else
        return it->second;
}
std::shared_ptr<Struct> Scope::find_variable(std::string name) const {
    auto it = variables.find(name);
    if (it == variables.end())
        return parent ? parent->find_variable(name) : nullptr;
    else {
        if (it->second == nullptr)
            fatal("cannot find definition of variable \"", name, "\"");
        return it->second;
    }
}
std::shared_ptr<Function> Scope::find_function(std::string name) const {
    auto it = functions.find(name);
    if (it == functions.end())
        return parent ? parent->find_function(name) : nullptr;
    else {
        return it->second;
    }
}

Struct Function::run(Scope& scope, std::vector<Expression> args) const {
    LLC_CHECK(parameters.size() == args.size());
    LLC_CHECK(definition != nullptr);

    for (int i = 0; i < (int)args.size(); i++)
        LLC_CHECK(definition->variables.find(parameters[i]) != definition->variables.end());

    for (int i = 0; i < (int)args.size(); i++)
        definition->variables[parameters[i]] = std::make_shared<Struct>(args[i](scope));

    return definition->run(scope);
}

void Expression::apply_parenthese() {
    int highest_prec = 0;
    for (const auto& operand : operands)
        highest_prec = std::max(highest_prec, operand->get_precedence());

    std::vector<int> parenthese_indices;
    int depth = 0;

    for (int i = 0; i < (int)operands.size(); i++) {
        if (dynamic_cast<LeftParenthese*>(operands[i].get())) {
            depth++;
            parenthese_indices.push_back(i);
        } else if (dynamic_cast<RightParenthese*>(operands[i].get())) {
            depth--;
            parenthese_indices.push_back(i);
        } else {
            operands[i]->set_precedence(operands[i]->get_precedence() + depth * highest_prec);
        }
    }

    for (int i = (int)parenthese_indices.size() - 1; i >= 0; i--)
        operands.erase(operands.begin() + parenthese_indices[i]);
}

void Expression::collapse() {
    apply_parenthese();

    int highest_prec = 0;
    for (const auto& operand : operands)
        highest_prec = std::max(highest_prec, operand->get_precedence());

    for (int prec = highest_prec; prec >= 0; prec--) {
        for (int i = 0; i < (int)operands.size(); i++) {
            if (operands[i]->get_precedence() == prec) {
                std::vector<int> merged = operands[i]->collapse(operands, i);
                std::sort(merged.begin(), merged.end(), std::greater<int>());
                for (int index : merged) {
                    LLC_CHECK(index >= 0);
                    LLC_CHECK(index < (int)operands.size());
                    operands.erase(operands.begin() + index);
                    if (index <= i)
                        i--;
                }
            }
        }
    }
}

Struct IfElseChain::run(Scope& scope) {
    LLC_CHECK(conditions.size() == actions.size() || conditions.size() == actions.size() - 1);
    for (int i = 0; i < (int)actions.size(); i++)
        LLC_CHECK(actions[i] != nullptr);

    for (int i = 0; i < (int)conditions.size(); i++)
        if (conditions[i](scope))
            return actions[i]->run(scope);

    if (conditions.size() == actions.size() - 1)
        return actions.back()->run(scope);
    return {};
}

Struct For::run(Scope& scope) {
    LLC_CHECK(action != nullptr);
    for (; condition(*internal_scope); updation(*internal_scope)) {
        auto result = action->run(scope);

        if (result.is_return)
            return result;
    }
    return {};
}

Struct While::run(Scope& scope) {
    LLC_CHECK(action != nullptr);

    while (condition(scope)) {
        auto result = action->run(scope);
        if (result.is_return)
            return result;
    }
    return {};
}

Struct Print::run(Scope& scope) {
    auto print_recursively = [&](auto me, Struct value, std::string name) -> void {
        if (name != "")
            print(name, ':', value.value);
        else if (value.members.size() == 0)
            print(value.value);
        for (auto member : value.members)
            me(me, *member.second, member.first);
    };

    print_recursively(print_recursively, expression(scope), "");

    return {};
}

}  // namespace llc