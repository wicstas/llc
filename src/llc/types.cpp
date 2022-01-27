#include <llc/types.h>

#include <algorithm>

namespace llc {

std::string Location::operator()(const std::string& source) {
    LLC_CHECK(line != -1);
    LLC_CHECK(column != -1);
    LLC_CHECK(length != 0);
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

}  // namespace llc