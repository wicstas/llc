#include <llc/parser.h>

namespace llc {

void Parser::parse_recursively(std::shared_ptr<Scope> scope) {
    while (!no_more()) {
        if (match(TokenType::RightCurlyBracket)) {
            putback();
            break;
        }
        if (auto token = match(TokenType::Identifier)) {
            if (auto type = scope->find_type(token->id)) {
                auto next0 = advance();
                auto next1 = advance();
                putback();
                putback();
                putback();
                if (next0.type != TokenType::Identifier)
                    fatal("expect identifier after type name:\n", next0.location(source));
                if (next1.type == TokenType::LeftParenthese) {
                    declare_function(scope);
                } else {
                    declare_variable(scope);
                    must_match(TokenType::Semicolon);
                }

            } else if (auto var = scope->find_variable(token->id)) {
                putback();
                scope->statements.push_back(std::make_shared<Expression>(build_expression(scope)));

            } else if (auto function = scope->find_function(token->id)) {
                putback();
                scope->statements.push_back(
                    std::make_shared<FunctionCall>(build_functioncall(scope)));

            } else if (token->id == "return") {
                scope->statements.push_back(std::make_shared<Return>(build_expression(scope)));
                must_match(TokenType::Semicolon);

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
                actions.push_back(parse_recursively_topdown(scope));
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
                    actions.push_back(parse_recursively_topdown(scope));
                    must_match(TokenType::RightCurlyBracket);
                }
                if (advance().id != "else") {
                    putback();
                } else {
                    must_match(TokenType::LeftCurlyBracket);
                    actions.push_back(parse_recursively_topdown(scope));
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
                auto sub_scope = parse_recursively_topdown(scope);
                must_match(TokenType::RightCurlyBracket);
                scope->statements.push_back(std::make_shared<For>(condtion, updation, sub_scope));

            } else if (token->id == "while") {
                must_match(TokenType::LeftParenthese);
                Expression condtion = build_expression(scope);
                must_match(TokenType::RightParenthese);

                must_match(TokenType::LeftCurlyBracket);
                auto sub_scope = parse_recursively_topdown(scope);
                must_match(TokenType::RightCurlyBracket);
                scope->statements.push_back(std::make_shared<While>(condtion, sub_scope));

            } else {
                fatal("unrecognized token: \"", token->id, "\":\n", token->location(source));
            }

        } else if (auto token = match(TokenType::Semicolon)) {
            continue;

        } else if (auto token = match(TokenType::Eof)) {
            break;

        } else {
            token = advance();
            fatal("unrecognized token: \"", enum_to_string(token->type), "\":\n",
                  token->location(source));
        }
    }
}

void Parser::declare_variable(std::shared_ptr<Scope> scope) {
    auto type_token = must_match(TokenType::Identifier);
    auto type = must_has(scope->find_type(type_token.id), type_token);
    auto var_token = must_match(TokenType::Identifier);
    auto& var = scope->variables[var_token.id];

    if (match(TokenType::Assign)) {
        if (var)
            fatal("variable \"", var_token.id, "\" redefinition:\n", var_token.location(source));

        var = std::make_shared<Struct>(*type);

        putback();
        putback();
        Expression expression = build_expression(scope);
        scope->statements.push_back(std::make_shared<Expression>(std::move(expression)));
    }
}

void Parser::declare_function(std::shared_ptr<Scope> scope) {
    auto return_type_token = must_match(TokenType::Identifier);
    auto return_type = must_has(scope->find_type(return_type_token.id), return_type_token);
    auto func_token = must_match(TokenType::Identifier);
    auto& func = scope->functions[func_token.id] = std::make_shared<Function>();

    func->return_type = *return_type;

    must_match(TokenType::LeftParenthese);
    if (!detect(TokenType::RightParenthese)) {
        while (true) {  // TODO while -> for
            auto type_token = must_match(TokenType::Identifier);
            auto type = must_has(scope->find_type(type_token.id), type_token);
            (void)type;
            auto var_token = must_match(TokenType::Identifier);
            func->parameters.push_back(var_token.id);
            if (detect(TokenType::RightParenthese))
                break;
            else
                must_match(TokenType::Comma);
        }
    }
    must_match(TokenType::RightParenthese);

    if (match(TokenType::LeftCurlyBracket)) {
        if (func->definition)
            fatal("function \"", func_token.id, "\" redefinition:\n", func_token.location(source));
        func->definition = std::make_shared<Scope>();
        func->definition->parent = scope;
        for (auto param : func->parameters)
            func->definition->variables.insert({param, std::make_shared<Struct>()});
        parse_recursively(func->definition);
        must_match(TokenType::RightCurlyBracket);
    } else {
        must_match(TokenType::Semicolon);
    }
}

Expression Parser::build_expression(std::shared_ptr<Scope> scope) {
    Expression expression;

    int depth = 0;
    Token prev;

    while (true) {
        auto token = advance();

        LLC_CHECK(token.type != TokenType::Invalid);
        if (token.type == TokenType::Eof || token.type == TokenType::Semicolon ||
            token.type == TokenType::Comma) {
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
            if (depth == 0) {
                putback();
                break;
            } else {
                expression.operands.push_back(std::make_shared<RightParenthese>());
                depth--;
            }
        } else if (token.type == TokenType::Identifier) {
            if (scope->find_variable(token.id))
                expression.operands.push_back(std::make_shared<VariableOp>(token.id));
            else {
                must_has(scope->find_function(token.id), token);
                putback();
                expression.operands.push_back(
                    std::make_shared<FunctionCallOp>(build_functioncall(scope)));
            }
        } else {
            fatal("unrecognized operand \"", enum_to_string(token.type), "\":\n",
                  token.location(source));
        }
        prev = token;
    }

    expression.collapse();

    return expression;
}

FunctionCall Parser::build_functioncall(std::shared_ptr<Scope> scope) {
    FunctionCall call;

    auto func_token = must_match(TokenType::Identifier);
    call.function = must_has(scope->find_function(func_token.id), func_token);
    must_match(TokenType::LeftParenthese);

    if (!detect(TokenType::RightParenthese)) {
        while (true) {  // TODO:while -> for
            call.arguments.emplace_back(build_expression(scope));

            if (detect(TokenType::RightParenthese))
                break;
            else
                must_match(TokenType::Comma);
        }
    }
    must_match(TokenType::RightParenthese);

    return call;
}

}  // namespace llc
