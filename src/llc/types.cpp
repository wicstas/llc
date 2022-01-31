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
    types["string"] = {};
    types["void"] = {};
}
Object Scope::run(const Scope&) const {
    for (const auto& statement : statements)
        LLC_CHECK(statement != nullptr);

    for (const auto& statement : statements) {
        try {
            statement->run(*this);
        } catch (const Object& result) {
            return result;
        }
    }
    return {};
}
std::optional<Object> Scope::find_type(std::string name) const {
    auto it = types.find(name);
    if (it == types.end())
        return parent ? parent->find_type(name) : std::nullopt;
    else
        return it->second;
}
std::optional<Object> Scope::find_variable(std::string name) const {
    auto it = variables.find(name);
    if (it == variables.end())
        return parent ? parent->find_variable(name) : std::nullopt;
    else
        return it->second;
}
std::optional<Function> Scope::find_function(std::string name) const {
    auto it = functions.find(name);
    if (it == functions.end())
        return parent ? parent->find_function(name) : std::nullopt;
    else
        return it->second;
}

Object InternalFunction::run(const Scope& scope, const std::vector<Expression>& exprs) const {
    LLC_CHECK(parameters.size() == exprs.size());
    LLC_CHECK(definition != nullptr);

    for (int i = 0; i < (int)exprs.size(); i++)
        LLC_CHECK(definition->variables.find(parameters[i]) != definition->variables.end());

    for (int i = 0; i < (int)exprs.size(); i++)
        definition->variables[parameters[i]] = exprs[i](scope);

    return definition->run(scope);
}

Object ExternalFunction::run(const Scope& scope, const std::vector<Expression>& exprs) const {
    std::vector<Object> arguments;
    for (auto& expr : exprs)
        arguments.push_back(expr(scope));
    return invoke(arguments);
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

Object IfElseChain::run(const Scope&) const {
    LLC_CHECK(conditions.size() == actions.size() || conditions.size() == actions.size() - 1);
    for (int i = 0; i < (int)actions.size(); i++)
        LLC_CHECK(actions[i] != nullptr);

    // for (int i = 0; i < (int)conditions.size(); i++)
    //     if (conditions[i](scope))
    //         return actions[i]->run(scope);

    // if (conditions.size() == actions.size() - 1)
    //     return actions.back()->run(scope);
    return {};
}

Object For::run(const Scope&) const {
    LLC_CHECK(action != nullptr);
    // for (; condition(*internal_scope); updation(*internal_scope)) {
    //     auto result = action->run(scope);

    //     if (result.is_return)
    //         return result;
    // }
    return {};
}

Object While::run(const Scope&) const {
    LLC_CHECK(action != nullptr);

    // while (condition(scope)) {
    //     auto result = action->run(scope);
    //     if (result.is_return)
    //         return result;
    // }
    return {};
}

}  // namespace llc