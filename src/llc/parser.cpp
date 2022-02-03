#include <llc/parser.h>

namespace llc {

void Parser::parse_recursively(std::shared_ptr<Scope> scope, bool end_on_new_line) {
    Token prev;

    while (!no_more()) {
        if (match(TokenType::RightCurlyBracket)) {
            putback();
            break;
        }
        if (auto token = match(TokenType::Identifier)) {
            if (auto type = scope->find_type(token->id) || token->id == "void") {
                auto next0 = advance();
                auto next1 = advance();
                putback();
                putback();
                putback();
                if (next1.type == TokenType::LeftParenthese) {
                    declare_function(scope);
                } else {
                    if (token->id == "void")
                        throw_exception("cannot declare variable of type \"void\":\n",
                                        token->location(source));
                    declare_variable(scope);
                }

            } else if (auto var = scope->find_variable(token->id)) {
                putback();
                scope->statements.push_back(std::make_shared<Expression>(build_expression(scope)));

            } else if (auto function = scope->find_function(token->id)) {
                scope->statements.push_back(
                    std::make_shared<FunctionCall>(build_functioncall(scope, token->id)));

            } else if (token->id == "struct") {
                declare_struct(scope);
                must_match(TokenType::Semicolon);

            } else if (token->id == "return") {
                scope->statements.push_back(std::make_shared<Return>(build_expression(scope)));

            } else if (token->id == "if") {
                std::vector<Expression> exprs;
                std::vector<std::shared_ptr<Scope>> actions;
                must_match(TokenType::LeftParenthese);
                exprs.push_back(build_expression(scope));
                must_match(TokenType::RightParenthese);

                if (match(TokenType::LeftCurlyBracket)) {
                    actions.push_back(parse_recursively_topdown(scope));
                    must_match(TokenType::RightCurlyBracket);
                } else {
                    actions.push_back(parse_recursively_topdown(scope, true));
                }

                while (true) {
                    auto next1 = advance();
                    auto next2 = advance();
                    if (next1.id != "else" || next2.id != "if") {
                        putback();
                        putback();
                        break;
                    }

                    must_match(TokenType::LeftParenthese);

                    exprs.push_back(build_expression(scope));
                    must_match(TokenType::RightParenthese);
                    if (match(TokenType::LeftCurlyBracket)) {
                        actions.push_back(parse_recursively_topdown(scope));
                        must_match(TokenType::RightCurlyBracket);
                    } else {
                        actions.push_back(parse_recursively_topdown(scope, true));
                    }
                }

                if (advance().id != "else") {
                    putback();
                } else {
                    if (match(TokenType::LeftCurlyBracket)) {
                        actions.push_back(parse_recursively_topdown(scope));
                        must_match(TokenType::RightCurlyBracket);
                    } else {
                        actions.push_back(parse_recursively_topdown(scope, true));
                    }
                }
                scope->statements.push_back(std::make_shared<IfElseChain>(exprs, actions));

            } else if (token->id == "for") {
                auto for_scope = std::make_shared<Scope>();
                for_scope->parent = scope;
                must_match(TokenType::LeftParenthese);
                declare_variable(for_scope);
                must_match(TokenType::Semicolon);
                Expression condtion = build_expression(for_scope);
                must_match(TokenType::Semicolon);
                Expression updation = build_expression(for_scope);
                must_match(TokenType::RightParenthese);

                std::shared_ptr<Scope> sub_scope;
                if (match(TokenType::LeftCurlyBracket)) {
                    sub_scope = parse_recursively_topdown(for_scope);
                    must_match(TokenType::RightCurlyBracket);
                } else {
                    sub_scope = parse_recursively_topdown(for_scope, true);
                }
                scope->statements.push_back(
                    std::make_shared<For>(condtion, updation, for_scope, sub_scope));

            } else if (token->id == "while") {
                must_match(TokenType::LeftParenthese);
                Expression condtion = build_expression(scope);
                must_match(TokenType::RightParenthese);

                // FIXME
                std::shared_ptr<Scope> sub_scope;
                if (match(TokenType::LeftCurlyBracket)) {
                    sub_scope = parse_recursively_topdown(scope);
                    must_match(TokenType::RightCurlyBracket);
                } else {
                    sub_scope = parse_recursively_topdown(scope, true);
                }
                scope->statements.push_back(std::make_shared<While>(condtion, sub_scope));

            } else {
                putback();
                scope->statements.push_back(std::make_shared<Expression>(build_expression(scope)));
                // throw_exception("unrecognized token: \"", token->id, "\":\n",
                //                 token->location(source));
            }

        } else if (auto token = match(TokenType::Semicolon)) {
            if (end_on_new_line)
                return;

        } else if (match(TokenType::Star)) {
            putback();
            scope->statements.push_back(std::make_shared<Expression>(build_expression(scope)));

        } else if (auto token = match(TokenType::Eof)) {
            break;

        } else {
            token = advance();
            throw_exception("unrecognized token: \"", enum_to_string(token->type), "\":\n",
                            token->location(source));
        }

        putback();
        prev = advance();
    }
}

void Parser::declare_variable(std::shared_ptr<Scope> scope) {
    auto type_token = must_match(TokenType::Identifier);
    auto type = must_has(scope->find_type(type_token.id), type_token);
    auto var_token = must_match(TokenType::Identifier);
    auto var = scope->variables[var_token.id] = *type;

    if (match(TokenType::Assign)) {
        putback();
        putback();
        scope->statements.push_back(std::make_shared<Expression>(build_expression(scope)));
    }
}

void Parser::declare_function(std::shared_ptr<Scope> scope) {
    auto return_type_token = must_match(TokenType::Identifier);
    auto func_token = must_match(TokenType::Identifier);
    auto func = std::make_unique<InternalFunction>();
    scope->functions[func_token.id] = {};

    func->return_type = return_type_token.id == "void"
                            ? std::nullopt
                            : std::optional<Object>(*must_has(
                                  scope->find_type(return_type_token.id), return_type_token));

    must_match(TokenType::LeftParenthese);
    while (!match(TokenType::RightParenthese)) {
        auto type_token = must_match(TokenType::Identifier);
        must_has(scope->find_type(type_token.id), type_token);
        auto var_token = must_match(TokenType::Identifier);
        func->parameters.push_back(var_token.id);
        if (must_match(TokenType::Comma | TokenType::RightParenthese).type ==
            TokenType::RightParenthese)
            break;
    }

    if (match(TokenType::LeftCurlyBracket)) {
        if (func->definition)
            throw_exception("function \"", func_token.id, "\" redefined:\n",
                            func_token.location(source));
        func->definition = std::make_shared<Scope>();
        func->definition->parent = scope;
        for (auto param : func->parameters)
            func->definition->variables.insert({param, Object()});
        parse_recursively(func->definition);
        must_match(TokenType::RightCurlyBracket);
    } else {
        must_match(TokenType::Semicolon);
    }

    scope->functions[func_token.id] = Function(std::move(func));
}

void Parser::declare_struct(std::shared_ptr<Scope> scope) {
    auto type_name = must_match(TokenType::Identifier);
    must_match(TokenType::LeftCurlyBracket);
    auto definition = parse_recursively_topdown(scope);
    LLC_CHECK(definition != nullptr);
    must_match(TokenType::RightCurlyBracket);
    std::unique_ptr<InternalObject> object = std::make_unique<InternalObject>(type_name.id);
    for (auto& var : definition->variables)
        object->members[var.first] = var.second;
    for (auto& func : definition->functions)
        object->functions[func.first] = func.second;

    for (auto& func : object->functions) {
        for (auto& var : object->members)
            dynamic_cast<InternalFunction*>(func.second.base.get())->this_scope[var.first] =
                &var.second;
    }

    scope->types[type_name.id] = Object(std::move(object));
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
        } else if (token.type == TokenType::Number)
            expression.operands.push_back(std::make_shared<NumberLiteral>(token.value));
        else if (token.type == TokenType::String)
            expression.operands.push_back(std::make_shared<StringLiteral>(token.value_s));
        else if (token.type == TokenType::Dot) {
            auto function_name = advance();
            auto next1 = advance();
            if (next1.type == TokenType::LeftParenthese) {
                std::shared_ptr<MemberFunctionCall> call = std::make_shared<MemberFunctionCall>();
                call->function_name = function_name.id;

                while (!match(TokenType::RightParenthese)) {
                    call->arguments.emplace_back(build_expression(scope));
                    if (must_match(TokenType::Comma | TokenType::RightParenthese).type ==
                        TokenType::RightParenthese)
                        break;
                }
                expression.operands.push_back(call);
            } else {
                putback();
                putback();
                expression.operands.push_back(std::make_shared<MemberAccess>());
            }
        } else if (token.type == TokenType::Assign)
            expression.operands.push_back(std::make_shared<Assignment>());
        else if (token.type == TokenType::Increment) {
            if (prev.type & (TokenType::Identifier | TokenType::RightSquareBracket))
                expression.operands.push_back(std::make_shared<PostIncrement>());
            else
                expression.operands.push_back(std::make_shared<PreIncrement>());
        } else if (token.type == TokenType::Decrement) {
            if (prev.type & (TokenType::Identifier | TokenType::RightSquareBracket))
                expression.operands.push_back(std::make_shared<PostDecrement>());
            else
                expression.operands.push_back(std::make_shared<PreDecrement>());
        } else if (token.type == TokenType::Plus) {
            if (prev.type & (TokenType::Number | TokenType::RightSquareBracket |
                             TokenType::RightParenthese | TokenType::Identifier))
                expression.operands.push_back(std::make_shared<Addition>());
        } else if (token.type == TokenType::Minus) {
            if (prev.type & (TokenType::Number | TokenType::RightSquareBracket |
                             TokenType::RightParenthese | TokenType::Identifier))
                expression.operands.push_back(std::make_shared<Subtraction>());
            else
                expression.operands.push_back(std::make_shared<Negation>());
        } else if (token.type == TokenType::Star)
            expression.operands.push_back(std::make_shared<Multiplication>());
        else if (token.type == TokenType::ForwardSlash)
            expression.operands.push_back(std::make_shared<Division>());
        else if (token.type == TokenType::LeftSquareBracket) {
            expression.operands.push_back(std::make_shared<ArrayAccess>());
            expression.operands.push_back(std::make_shared<LeftSquareBracket>());
        } else if (token.type == TokenType::RightSquareBracket) {
            expression.operands.push_back(std::make_shared<RightSquareBracket>());
        } else if (token.type == TokenType::LessThan)
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
            if (auto type = scope->find_type(token.id))
                expression.operands.push_back(std::make_shared<TypeOp>(*type));
            else if (prev.type == TokenType::Dot)
                expression.operands.push_back(std::make_shared<ObjectMember>(token.id));
            else if (scope->find_variable(token.id))
                expression.operands.push_back(std::make_shared<VariableOp>(token.id));
            else if (auto function = scope->find_function(token.id)) {
                expression.operands.push_back(
                    std::make_shared<FunctionCallOp>(build_functioncall(scope, token.id)));
            } else {
                expression.operands.push_back(std::make_shared<VariableOp>(token.id));
                // throw_exception("\"", token.id, "\" is neither a function nor a variable:\n",
                //                 token.location(source));
            }
        } else {
            throw_exception("unrecognized operand \"", enum_to_string(token.type), "\":\n",
                            token.location(source));
        }
        prev = token;
    }

    expression.collapse();

    return expression;
}

FunctionCall Parser::build_functioncall(std::shared_ptr<Scope> scope, std::string function_name) {
    FunctionCall call;

    call.function_name = function_name;
    must_match(TokenType::LeftParenthese);

    while (!match(TokenType::RightParenthese)) {
        call.arguments.emplace_back(build_expression(scope));
        if (must_match(TokenType::Comma | TokenType::RightParenthese).type ==
            TokenType::RightParenthese)
            break;
    }

    return call;
}

std::optional<Token> Parser::match(TokenType type) {
    auto token = advance();
    if (token.type & type) {
        return token;
    } else {
        putback();
        return std::nullopt;
    }
}

Token Parser::must_match(TokenType type) {
    if (no_more())
        throw_exception("expect \"", enum_to_string(type), "\", but no more token is available");
    auto token = advance();
    if (token.type & type) {
        return token;
    } else {
        throw_exception("token mismatch, expect \"", enum_to_string(type), "\", get \"",
                        token.type == TokenType::Identifier ? token.id : enum_to_string(token.type),
                        "\":\n", token.location(source));
        return {};
    }
}

}  // namespace llc
