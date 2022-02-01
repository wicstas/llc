#include <llc/types.h>

#include <algorithm>

namespace llc {

std::map<size_t, std::string> type_id_to_name = {
    {typeid(int).hash_code(), "int"},       {typeid(float).hash_code(), "float"},
    {typeid(double).hash_code(), "double"}, {typeid(std::string).hash_code(), "string"},
    {typeid(bool).hash_code(), "bool"},
};

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

std::string enum_to_string(TokenType type) {
    static const char* map[] = {"number", "++", "--",      "+",   "-",          "*", "/", "(",
                                ")",      "{",  "}",       ";",   "identifier", ".", ",", "<",
                                "<=",     ">",  ">=",      "==",  "!=",         "=", "!", "string",
                                "[",      "]",  "invalid", "eof", "num_tokens"};
    std::string str;
    for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); i++)
        if ((uint64_t(type) >> i) & 1ul)
            str += (std::string)map[i] + "|";

    if (str.back() == '|')
        str.pop_back();
    return str;
}

Scope::Scope() {
    types["int"] = Object(0);
    types["float"] = Object(0.0f);
    types["double"] = Object(0.0);
    types["bool"] = Object(false);
}
std::optional<Object> Scope::run(const Scope&) const {
    for (const auto& statement : statements)
        LLC_CHECK(statement != nullptr);

    for (const auto& statement : statements) {
        try {
            statement->run(*this);
        } catch (const std::optional<Object>& result) {
            return result;
        }
    }
    return std::nullopt;
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

std::optional<Object> InternalFunction::run(const Scope& scope,
                                            const std::vector<Expression>& exprs) const {
    LLC_CHECK(parameters.size() == exprs.size());
    LLC_CHECK(definition != nullptr);

    for (int i = 0; i < (int)exprs.size(); i++)
        LLC_CHECK(definition->variables.find(parameters[i]) != definition->variables.end());

    for (int i = 0; i < (int)exprs.size(); i++)
        if (auto ret = exprs[i](scope))
            definition->variables[parameters[i]] = *ret;
        else
            fatal("void cannot be used as function parameter");

    return definition->run(scope);
}

std::optional<Object> ExternalFunction::run(const Scope& scope,
                                            const std::vector<Expression>& exprs) const {
    std::vector<Object> arguments;

    for (auto& expr : exprs) {
        if (auto ret = expr(scope))
            arguments.push_back(*ret);
        else
            fatal("void cannot be passes as argument to function");
    }
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

std::optional<Object> IfElseChain::run(const Scope& scope) const {
    LLC_CHECK(conditions.size() == actions.size() || conditions.size() == actions.size() - 1);
    for (int i = 0; i < (int)actions.size(); i++)
        LLC_CHECK(actions[i] != nullptr);

    for (int i = 0; i < (int)conditions.size(); i++) {
        try {
            if (conditions[i](scope))
                actions[i]->run(scope);
        } catch (const std::optional<Object> object) {
            return object;
        }
    }

    try {
        if (conditions.size() == actions.size() - 1)
            actions.back()->run(scope);
    } catch (const std::optional<Object> object) {
        return object;
    }
    
    return std::nullopt;
}

std::optional<Object> For::run(const Scope&) const {
    LLC_CHECK(action != nullptr);
    // for (; condition(*internal_scope); updation(*internal_scope)) {
    //     auto result = action->run(scope);

    //     if (result.is_return)
    //         return result;
    // }
    return std::nullopt;
}

std::optional<Object> While::run(const Scope&) const {
    LLC_CHECK(action != nullptr);

    // while (condition(scope)) {
    //     auto result = action->run(scope);
    //     if (result.is_return)
    //         return result;
    // }
    return std::nullopt;
}

}  // namespace llc