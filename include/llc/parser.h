#ifndef LLC_PARSER_H
#define LLC_PARSER_H

#include <llc/defines.h>
#include <llc/tokenizer.h>
#include <llc/types.h>

namespace llc {

struct Parser {
    void parse(std::string source, std::vector<Token> tokens) {
        this->tokens = std::move(tokens);
        scope.reset();
        lines.clear();

        std::stringstream sstream(std::move(source));
        std::string line;
        while (std::getline(sstream, line))
            lines.push_back(line);

        try {
            parse_tokens();
            construct_recursively(scope);
        } catch (Exception e) {
            handle_exception(e);
        }
    }

    void execute() {
        LLC_CHECK(scope != nullptr);
        try {
            scope->execute();
        } catch (Exception e) {
            handle_exception(e);
        }
    }

  private:
    std::shared_ptr<Expression> parse_elementary_group(ElementaryGroup eg) {
        if (eg.size() > 1) {
            int highest_precedence = 0;
            for (auto& element : eg)
                highest_precedence = std::max(highest_precedence, element->precendence);

            for (int precedence = highest_precedence; precedence >= 0; precedence--) {
                for (int i = 0; i < (int)eg.size(); i++) {
                    if (eg[i]->precendence == precedence) {
                        std::vector<int> merged = eg[i]->construct(*scope, eg, i);
                        std::sort(merged.begin(), merged.end(), std::greater<int>());
                        for (int index : merged) {
                            eg.erase(eg.begin() + index);
                            if (index <= i)
                                i--;
                        }
                    }
                }
            }
        }

        LLC_CHECK(eg.size() == 1);
        return eg[0];
    }

    void parse_tokens() {
        scope = std::make_shared<Scope>();
        ElementaryGroup eg;
        int precendence_bias = 0;

        int struct_decl_pos = -1;

        for (const auto& token : tokens) {
            if (token.type == TokenType::Semicolon) {
                if (eg.size())
                    scope->statements.emplace_back(parse_elementary_group(std::move(eg)));
            } else if (token.type == TokenType::LeftCurlyBracket) {
                if (eg.size())
                    scope->statements.emplace_back(parse_elementary_group(std::move(eg)));

                std::shared_ptr<Scope> sub_scope(new Scope);
                sub_scope->parent_scope = scope;
                scope->statements.emplace_back(sub_scope);
                scope = sub_scope;

            } else if (token.type == TokenType::RightCurlyBracket) {
                LLC_CHECK(eg.size() == 0);
                scope = scope->parent_scope;
                if (struct_decl_pos != -1) {
                    LLC_CHECK(scope->statements[struct_decl_pos].is_struct_decl());
                    std::vector<int> merged =
                        scope->statements[struct_decl_pos].struct_decl->construct(
                            *scope, scope->statements, struct_decl_pos);

                    std::sort(merged.begin(), merged.end(), std::greater<int>());
                    for (int index : merged) {
                        scope->statements.erase(scope->statements.begin() + index);
                    }

                    struct_decl_pos = -1;
                }
            } else if (token.type == TokenType::LeftParenthesis) {
                precendence_bias += max_precedence;
            } else if (token.type == TokenType::RightParenthesis) {
                precendence_bias -= max_precedence;
            } else if (is_controlflow(token)) {
                std::shared_ptr<ControlFlow> controlflow(ControlFlow::create(token));
                scope->statements.emplace_back(controlflow);
            } else if (token.type == TokenType::StructDecl) {
                std::shared_ptr<StructDecl> struct_decl(StructDecl::create(token));
                struct_decl_pos = (int)scope->statements.size();
                scope->statements.emplace_back(struct_decl);
            } else {
                eg.emplace_back(Elementary::create(token, precendence_bias));
            }
        }
    }

    void construct_recursively(std::shared_ptr<Scope> scope) {
        for (int i = 0; i < (int)scope->statements.size(); i++)
            if (scope->statements[i].is_scope())
                construct_recursively(scope->statements[i].scope);

        for (int i = 0; i < (int)scope->statements.size(); i++) {
            if (scope->statements[i].is_controlflow()) {
                std::vector<int> merged =
                    scope->statements[i].controlflow->construct(*scope, scope->statements, i);
                std::sort(merged.begin(), merged.end(), std::greater<int>());
                for (int index : merged) {
                    scope->statements.erase(scope->statements.begin() + index);
                    if (index <= i)
                        i--;
                }
            }
        }
    }

    void handle_exception(Exception e) const {
        print(lines[e.line]);
        print(std::string(std::max(e.column - 1, 0), ' '), std::string(2, '~'));
        fatal(e.message, "(line: ", e.line, ")");
    }

    std::vector<Token> tokens;
    std::shared_ptr<Scope> scope;
    std::vector<std::string> lines;
};

}  // namespace llc

#endif  // LLC_PARSER_H