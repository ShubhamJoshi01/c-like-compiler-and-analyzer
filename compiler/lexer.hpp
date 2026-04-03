#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cctype>
#include <iostream>

enum class TokenType {
    Int, Char, If, Else, While, For, Return, Print,
    Identifier, IntegerLit, CharLit,
    Plus, Minus, Star, Slash, Assign, Eq, Neq, Lt, Gt, Lte, Gte,
    LParen, RParen, LBrace, RBrace, Semi, Comma,
    Eof, Invalid
};

struct Token {
    TokenType type;
    std::string value;
    int line;
};

class Lexer {
    std::string source;
    size_t pos_ = 0;
    int line_ = 1;
    
    std::unordered_map<std::string, TokenType> keywords_ = {
        {"int", TokenType::Int}, {"char", TokenType::Char},
        {"if", TokenType::If}, {"else", TokenType::Else},
        {"while", TokenType::While}, {"for", TokenType::For},
        {"return", TokenType::Return}, {"print", TokenType::Print}
    };

    char peek() {
        if (pos_ >= source.length()) return '\0';
        return source[pos_];
    }
    
    char peek_next() {
        if (pos_ + 1 >= source.length()) return '\0';
        return source[pos_ + 1];
    }

    char advance() {
        char c = peek();
        pos_++;
        if (c == '\n') line_++;
        return c;
    }

    void skipWhitespaceAndComments() {
        while (true) {
            char c = peek();
            if (std::isspace(c)) {
                advance();
            } else if (c == '/' && peek_next() == '/') {
                while (peek() != '\n' && peek() != '\0') advance();
            } else if (c == '/' && peek_next() == '*') {
                advance(); advance(); // skip /*
                while (peek() != '\0' && !(peek() == '*' && peek_next() == '/')) {
                    advance();
                }
                if (peek() != '\0') {
                    advance(); advance(); // skip */
                }
            } else {
                break;
            }
        }
    }

    Token makeToken(TokenType type, std::string value) {
        return Token{type, value, line_};
    }

public:
    Lexer(const std::string& src) : source(src) {}

    Token nextToken() {
        skipWhitespaceAndComments();
        
        char c = peek();
        if (c == '\0') {
            return makeToken(TokenType::Eof, "");
        }

        if (std::isalpha(c) || c == '_') {
            std::string val = "";
            while (std::isalnum(peek()) || peek() == '_') {
                val += advance();
            }
            if (keywords_.count(val)) {
                return makeToken(keywords_[val], val);
            }
            return makeToken(TokenType::Identifier, val);
        }

        if (std::isdigit(c)) {
            std::string val = "";
            while (std::isdigit(peek())) {
                val += advance();
            }
            // simplified error tokenization line_ state
            int start_line = line_;
            return Token{TokenType::IntegerLit, val, start_line};
        }

        if (c == '\'') {
            int start_line = line_;
            advance(); // skip '
            if (peek() == '\0') return Token{TokenType::Invalid, "'", start_line};
            std::string val = "";
            val += advance(); 
            if (peek() == '\'') {
                advance(); // skip '
                return Token{TokenType::CharLit, val, start_line};
            }
            return Token{TokenType::Invalid, "char_lit_err", start_line};
        }

        int start_line = line_;
        c = advance();
        
        switch (c) {
            case '+': return Token{TokenType::Plus, "+", start_line};
            case '-': return Token{TokenType::Minus, "-", start_line};
            case '*': return Token{TokenType::Star, "*", start_line};
            case '/': return Token{TokenType::Slash, "/", start_line};
            case '(': return Token{TokenType::LParen, "(", start_line};
            case ')': return Token{TokenType::RParen, ")", start_line};
            case '{': return Token{TokenType::LBrace, "{", start_line};
            case '}': return Token{TokenType::RBrace, "}", start_line};
            case ';': return Token{TokenType::Semi, ";", start_line};
            case ',': return Token{TokenType::Comma, ",", start_line};
            case '=':
                if (peek() == '=') { advance(); return Token{TokenType::Eq, "==", start_line}; }
                return Token{TokenType::Assign, "=", start_line};
            case '!':
                if (peek() == '=') { advance(); return Token{TokenType::Neq, "!=", start_line}; }
                return Token{TokenType::Invalid, "!", start_line};
            case '<':
                if (peek() == '=') { advance(); return Token{TokenType::Lte, "<=", start_line}; }
                return Token{TokenType::Lt, "<", start_line};
            case '>':
                if (peek() == '=') { advance(); return Token{TokenType::Gte, ">=", start_line}; }
                return Token{TokenType::Gt, ">", start_line};
        }
        
        return Token{TokenType::Invalid, std::string(1, c), start_line};
    }

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        while (true) {
            Token t = nextToken();
            if (t.type == TokenType::Eof) break;
            tokens.push_back(t);
        }
        return tokens;
    }
};
