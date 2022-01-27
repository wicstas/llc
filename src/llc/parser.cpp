#include <llc/parser.h>

namespace llc {

std::shared_ptr<Scope> Parser::parse_recursively(std::shared_ptr<Scope> parent) {
    std::shared_ptr<Scope> scope = std::make_shared<Scope>();
    scope->parent = parent;

    while (!no_more()) {
        if (match(TokenType::RightCurlyBracket)) {
            putback();
            break;
        }
        if (auto token = match(TokenType::Identifier)) {
            if (auto type = scope->find_type(token->id)) {
                putback();
                declare_variable(scope);

            } else if (auto var = scope->find_varaible(token->id)) {
                putback();
                scope->statements.push_back(std::make_shared<Expression>(build_expression(scope)));

            } else if (token->id == "print") {
                must_match(TokenType::LeftParenthese);
                scope->statements.push_back(std::make_shared<Print>(build_expression(scope)));
                must_match(TokenType::RightParenthese);
                must_match(TokenType::Semicolon);

            } else if (token->id == "if") {
                std::vector<Expression> expressions;
                std::vector<std::shared_ptr<Statement>> actions;
                must_match(TokenType::LeftParenthese);
                expressions.push_back(build_expression(scope));
                must_match(TokenType::RightParenthese);
                must_match(TokenType::LeftCurlyBracket);
                actions.push_back(parse_recursively(scope));
                must_match(TokenType::RightCurlyBracket);
                while (true) {
                    auto next1 = advance();
                    auto next2 = advance();
                    if (next1.id != "else" || next2.id != "if") {
                        putback();
                        putback();
                        break;
                    }
                    must_match(TokenType::LeftParenthese);
                    expressions.push_back(build_expression(scope));
                    must_match(TokenType::RightParenthese);
                    must_match(TokenType::LeftCurlyBracket);
                    actions.push_back(parse_recursively(scope));
                    must_match(TokenType::RightCurlyBracket);
                }
                if (advance().id != "else") {
                    putback();
                } else {
                    must_match(TokenType::LeftCurlyBracket);
                    actions.push_back(parse_recursively(scope));
                    must_match(TokenType::RightCurlyBracket);
                }
                scope->statements.push_back(std::make_shared<IfElseChain>(expressions, actions));

            } else if (token->id == "for") {
                must_match(TokenType::LeftParenthese);
                declare_variable(scope);
                must_match(TokenType::Semicolon);
                Expression condtion = build_expression(scope);
                must_match(TokenType::Semicolon);
                Expression updation = build_expression(scope);
                must_match(TokenType::RightParenthese);

                must_match(TokenType::LeftCurlyBracket);
                auto sub_scope = parse_recursively(scope);
                must_match(TokenType::RightCurlyBracket);
                scope->statements.push_back(std::make_shared<For>(condtion, updation, sub_scope));

            } else if (token->id == "while") {
                must_match(TokenType::LeftParenthese);
                Expression condtion = build_expression(scope);
                must_match(TokenType::RightParenthese);

                must_match(TokenType::LeftCurlyBracket);
                auto sub_scope = parse_recursively(scope);
                must_match(TokenType::RightCurlyBracket);
                scope->statements.push_back(std::make_shared<While>(condtion, sub_scope));
                
            } else {
                fatal("unrecognized token: \"", enum_to_string(token->type), "\":\n",
                      token->location(source));
            }

        } else if (auto token = match(TokenType::Semicolon)) {
            continue;

        } else {
            fatal("unrecognized token: \"", enum_to_string(token->type), "\":\n",
                  token->location(source));
        }
    }

    return scope;
}

void Parser::declare_variable(std::shared_ptr<Scope> scope) {
    auto token = match(TokenType::Identifier);

    if (auto type = scope->find_type(token->id)) {
        auto var_token = must_match(TokenType::Identifier);
        auto var = scope->variables[var_token.id] = std::make_shared<Struct>(*type);

        if (match(TokenType::Assign)) {
            Expression expression = build_expression(scope);
            scope->statements.push_back(std::make_shared<Expression>(std::move(expression)));
            var->is_initialized = true;
        } else {
            must_match(TokenType::Semicolon);
        }
    }
}

Expression Parser::build_expression(std::shared_ptr<Scope> scope) {
    Expression expression;

    int depth = 0;
    Token prev;

    while (true) {
        auto token = advance();

        if (token.type == TokenType::Semicolon) {
            putback();
            break;
        }
        if (token.type == TokenType::Number)
            expression.operands.push_back(std::make_shared<NumberLiteral>(token.value));
        else if (token.type == TokenType::Assign)
            expression.operands.push_back(std::make_shared<Assignment>());
        else if (token.type == TokenType::Increment && prev.type == TokenType::Identifier)
            expression.operands.push_back(std::make_shared<PostIncrement>());
        else if (token.type == TokenType::Increment && prev.type != TokenType::Identifier)
            expression.operands.push_back(std::make_shared<PreIncrement>());
        else if (token.type == TokenType::Decrement && prev.type == TokenType::Identifier)
            expression.operands.push_back(std::make_shared<PostIncrement>());
        else if (token.type == TokenType::Decrement && prev.type != TokenType::Identifier)
            expression.operands.push_back(std::make_shared<PreIncrement>());
        else if (token.type == TokenType::Plus)
            expression.operands.push_back(std::make_shared<Addition>());
        else if (token.type == TokenType::Minus)
            expression.operands.push_back(std::make_shared<Subtraction>());
        else if (token.type == TokenType::Star)
            expression.operands.push_back(std::make_shared<Multiplication>());
        else if (token.type == TokenType::ForwardSlash)
            expression.operands.push_back(std::make_shared<Division>());
        else if (token.type == TokenType::LessThan)
            expression.operands.push_back(std::make_shared<LessThan>());
        else if (token.type == TokenType::LessEqual)
            expression.operands.push_back(std::make_shared<LessEqual>());
        else if (token.type == TokenType::GreaterThan)
            expression.operands.push_back(std::make_shared<GreaterThan>());
        else if (token.type == TokenType::GreaterEqual)
            expression.operands.push_back(std::make_shared<GreaterEqual>());
        else if (token.type == TokenType::Equal)
            expression.operands.push_back(std::make_shared<Equal>());
        else if (token.type == TokenType::NotEqual)
            expression.operands.push_back(std::make_shared<NotEqual>());
        else if (token.type == TokenType::LeftParenthese) {
            depth++;
            expression.operands.push_back(std::make_shared<LeftParenthese>());
        } else if (token.type == TokenType::RightParenthese) {
            if (depth == 0)
                putback();
            depth--;
            expression.operands.push_back(std::make_shared<RightParenthese>());
            if (depth <= 0)
                break;
        } else if (token.type == TokenType::Identifier) {
            auto var = scope->find_varaible(token.id);
            must_has(var, token);
            if (!var->is_initialized)
                fatal("variable \"", token.id, "\" is used before been initialized:\n",
                      token.location(source));
            expression.operands.push_back(std::make_shared<VaraibleOp>(token.id));
        } else {
            fatal("unrecognized operand \"", enum_to_string(token.type), "\":\n",
                  token.location(source));
        }
        prev = token;
    }

    expression.collapse();

    return expression;
}

}  // namespace llc