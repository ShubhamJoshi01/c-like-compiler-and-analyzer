#pragma once
#include "lexer.hpp"
#include "ast.hpp"
#include <stdexcept>

class Parser {
    std::vector<Token> tokens;
    size_t pos = 0;

    Token current() {
        if (pos >= tokens.size()) return {TokenType::Eof, "", 0};
        return tokens[pos];
    }

    void advance() {
        if (pos < tokens.size()) pos++;
    }

    bool match(TokenType type) {
        if (current().type == type) {
            advance();
            return true;
        }
        return false;
    }

    Token expect(TokenType type, const std::string& err) {
        if (current().type == type) {
            Token t = current();
            advance();
            return t;
        }
        throw std::runtime_error("Syntax Error line " + std::to_string(current().line) + ": " + err);
    }

public:
    Parser(std::vector<Token> t) : tokens(t) {}

    std::unique_ptr<ProgramNode> parse() {
        auto prog = std::make_unique<ProgramNode>(current().line);
        while (current().type != TokenType::Eof) {
            prog->statements.push_back(parseGlobalStatement());
        }
        return prog;
    }

    std::unique_ptr<ASTNode> parseGlobalStatement() {
        if (current().type == TokenType::Int || current().type == TokenType::Char) {
            std::string type_val = current().value;
            int start_line = current().line;
            advance(); // consume type
            Token id = expect(TokenType::Identifier, "Expected identifier after type");
            
            if (current().type == TokenType::LParen) {
                // Function decl
                advance(); // consume (
                std::vector<Param> params;
                if (current().type != TokenType::RParen) {
                    params.push_back(parseParam());
                    while (match(TokenType::Comma)) {
                        params.push_back(parseParam());
                    }
                }
                expect(TokenType::RParen, "Expected ')' after parameters");
                auto block = parseBlock();
                return std::make_unique<FunctionDeclNode>(type_val, id.value, params, std::move(block), start_line);
            } else {
                // Var decl
                return parseVarDeclRest(type_val, id.value, start_line);
            }
        }
        throw std::runtime_error("Syntax Error line " + std::to_string(current().line) + ": Expected global declaration");
    }

    Param parseParam() {
        if (current().type == TokenType::Int || current().type == TokenType::Char) {
            std::string type_val = current().value;
            advance();
            Token id = expect(TokenType::Identifier, "Expected identifier in parameter");
            return {type_val, id.value};
        }
        throw std::runtime_error("Syntax Error line " + std::to_string(current().line) + ": Expected parameter type");
    }

    std::unique_ptr<ASTNode> parseVarDeclRest(std::string type_val, std::string id, int line) {
        std::unique_ptr<ASTNode> init = nullptr;
        if (match(TokenType::Assign)) {
            init = parseExpression();
        }
        expect(TokenType::Semi, "Expected ';' after variable declaration");
        return std::make_unique<VarDeclNode>(type_val, id, std::move(init), line);
    }

    std::unique_ptr<ASTNode> parseBlock() {
        Token start = expect(TokenType::LBrace, "Expected '{'");
        auto block = std::make_unique<BlockNode>(start.line);
        while (current().type != TokenType::RBrace && current().type != TokenType::Eof) {
            block->statements.push_back(parseStatement());
        }
        expect(TokenType::RBrace, "Expected '}'");
        return block;
    }

    std::unique_ptr<ASTNode> parseStatement() {
        if (current().type == TokenType::LBrace) return parseBlock();

        if (current().type == TokenType::Int || current().type == TokenType::Char) {
            std::string tval = current().value;
            int line = current().line;
            advance();
            Token id = expect(TokenType::Identifier, "Expected identifier");
            return parseVarDeclRest(tval, id.value, line);
        }

        if (match(TokenType::If)) {
            int line = current().line;
            expect(TokenType::LParen, "Expected '(' after if");
            auto cond = parseExpression();
            expect(TokenType::RParen, "Expected ')' after if condition");
            auto t_block = parseStatement();
            std::unique_ptr<ASTNode> f_block = nullptr;
            if (match(TokenType::Else)) {
                f_block = parseStatement();
            }
            return std::make_unique<IfNode>(std::move(cond), std::move(t_block), std::move(f_block), line);
        }

        if (match(TokenType::While)) {
            int line = current().line;
            expect(TokenType::LParen, "Expected '(' after while");
            auto cond = parseExpression();
            expect(TokenType::RParen, "Expected ')'");
            auto block = parseStatement();
            return std::make_unique<WhileNode>(std::move(cond), std::move(block), line);
        }

        if (match(TokenType::For)) {
            int line = current().line;
            expect(TokenType::LParen, "Expected '(' after for");
            
            std::unique_ptr<ASTNode> init = nullptr;
            if (current().type == TokenType::Int || current().type == TokenType::Char) {
                std::string tval = current().value;
                int l = current().line; advance();
                Token id = expect(TokenType::Identifier, "Expected identifier");
                init = parseVarDeclRest(tval, id.value, l);
            } else {
                Token id = expect(TokenType::Identifier, "Expected identifier");
                expect(TokenType::Assign, "Expected '='");
                auto expr = parseExpression();
                expect(TokenType::Semi, "Expected ';'");
                init = std::make_unique<AssignNode>(id.value, std::move(expr), id.line);
            }

            auto cond = parseExpression();
            expect(TokenType::Semi, "Expected ';'");
            
            Token id2 = expect(TokenType::Identifier, "Expected identifier in update");
            expect(TokenType::Assign, "Expected '='");
            auto expr2 = parseExpression();
            auto update = std::make_unique<AssignNode>(id2.value, std::move(expr2), id2.line);
            
            expect(TokenType::RParen, "Expected ')'");
            auto block = parseStatement();
            return std::make_unique<ForNode>(std::move(init), std::move(cond), std::move(update), std::move(block), line);
        }

        if (match(TokenType::Return)) {
            int line = current().line;
            std::unique_ptr<ASTNode> expr = nullptr;
            if (current().type != TokenType::Semi) {
                expr = parseExpression();
            }
            expect(TokenType::Semi, "Expected ';'");
            return std::make_unique<ReturnNode>(std::move(expr), line);
        }

        if (match(TokenType::Print)) {
            int line = current().line;
            expect(TokenType::LParen, "Expected '(' after print");
            auto expr = parseExpression();
            expect(TokenType::RParen, "Expected ')'");
            expect(TokenType::Semi, "Expected ';'");
            return std::make_unique<PrintNode>(std::move(expr), line);
        }

        if (current().type == TokenType::Identifier) {
            Token id = current();
            advance();
            if (match(TokenType::Assign)) {
                auto expr = parseExpression();
                expect(TokenType::Semi, "Expected ';'");
                return std::make_unique<AssignNode>(id.value, std::move(expr), id.line);
            }
            if (match(TokenType::LParen)) {
                std::vector<std::unique_ptr<ASTNode>> args;
                if (current().type != TokenType::RParen) {
                    args.push_back(parseExpression());
                    while (match(TokenType::Comma)) {
                        args.push_back(parseExpression());
                    }
                }
                expect(TokenType::RParen, "Expected ')'");
                expect(TokenType::Semi, "Expected ';'");
                auto call = std::make_unique<FunctionCallNode>(id.value, id.line);
                call->args = std::move(args);
                return call;
            }
        }

        throw std::runtime_error("Syntax Error line " + std::to_string(current().line) + ": Invalid statement");
    }

    std::unique_ptr<ASTNode> parseExpression() {
        return parseLogicalOr();
    }

    std::unique_ptr<ASTNode> parseLogicalOr() {
        auto left = parseLogicalAnd();
        while (current().type == TokenType::PipePipe) {
            Token op = current();
            advance();
            auto right = parseLogicalAnd();
            left = std::make_unique<BinaryOpNode>(op.value, std::move(left), std::move(right), op.line);
        }
        return left;
    }

    std::unique_ptr<ASTNode> parseLogicalAnd() {
        auto left = parseComparison();
        while (current().type == TokenType::AmpAmp) {
            Token op = current();
            advance();
            auto right = parseComparison();
            left = std::make_unique<BinaryOpNode>(op.value, std::move(left), std::move(right), op.line);
        }
        return left;
    }

    std::unique_ptr<ASTNode> parseComparison() {
        auto left = parseAdditive();
        while (current().type == TokenType::Eq || current().type == TokenType::Neq || 
               current().type == TokenType::Lt || current().type == TokenType::Gt || 
               current().type == TokenType::Lte || current().type == TokenType::Gte) {
            Token op = current();
            advance();
            auto right = parseAdditive();
            left = std::make_unique<BinaryOpNode>(op.value, std::move(left), std::move(right), op.line);
        }
        return left;
    }

    std::unique_ptr<ASTNode> parseAdditive() {
        auto left = parseTerm();
        while (current().type == TokenType::Plus || current().type == TokenType::Minus) {
            Token op = current();
            advance();
            auto right = parseTerm();
            left = std::make_unique<BinaryOpNode>(op.value, std::move(left), std::move(right), op.line);
        }
        return left;
    }

    std::unique_ptr<ASTNode> parseTerm() {
        auto left = parseFactor();
        while (current().type == TokenType::Star || current().type == TokenType::Slash) {
            Token op = current();
            advance();
            auto right = parseFactor();
            left = std::make_unique<BinaryOpNode>(op.value, std::move(left), std::move(right), op.line);
        }
        return left;
    }

    std::unique_ptr<ASTNode> parseFactor() {
        Token t = current();
        if (match(TokenType::IntegerLit)) return std::make_unique<LiteralNode>("int", t.value, t.line);
        if (match(TokenType::CharLit)) return std::make_unique<LiteralNode>("char", t.value, t.line);
        
        if (match(TokenType::Bang)) {
            auto expr = parseFactor();
            return std::make_unique<UnaryOpNode>("!", std::move(expr), t.line);
        }
        
        if (match(TokenType::Identifier)) {
            if (match(TokenType::LParen)) {
                std::vector<std::unique_ptr<ASTNode>> args;
                if (current().type != TokenType::RParen) {
                    args.push_back(parseExpression());
                    while (match(TokenType::Comma)) {
                        args.push_back(parseExpression());
                    }
                }
                expect(TokenType::RParen, "Expected ')'");
                auto call = std::make_unique<FunctionCallNode>(t.value, t.line);
                call->args = std::move(args);
                return call;
            }
            return std::make_unique<IdentifierNode>(t.value, t.line);
        }
        
        if (match(TokenType::LParen)) {
            auto expr = parseExpression();
            expect(TokenType::RParen, "Expected ')'");
            return expr;
        }

        throw std::runtime_error("Syntax Error line " + std::to_string(current().line) + ": Expected expression");
    }
};
