#include "lexer.hpp"
#include "parser.hpp"
#include "semantic.hpp"
#include "interpreter.hpp"
#include <iostream>
#include <sstream>

std::string escapeJson(const std::string& s) {
    std::ostringstream o;
    for (auto c : s) {
        if (c == '"') o << "\\\"";
        else if (c == '\\') o << "\\\\";
        else if (c == '\b') o << "\\b";
        else if (c == '\f') o << "\\f";
        else if (c == '\n') o << "\\n";
        else if (c == '\r') o << "\\r";
        else if (c == '\t') o << "\\t";
        else o << c;
    }
    return o.str();
}

int main() {
    std::string source;
    std::string line;
    while (std::getline(std::cin, line)) {
        source += line + "\n";
    }

    std::cout << "{" << std::endl;
    
    try {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();

        Parser parser(tokens);
        auto ast = parser.parse();

        SemanticAnalyzer analyzer;
        analyzer.visit(ast.get());
        
        std::vector<std::string> output_logs;
        // Only run interpreter if no semantic errors
        if (analyzer.errors.empty()) {
            Interpreter interpreter;
            interpreter.visitProgram(ast.get());
            output_logs = interpreter.console_output;
        }

        // Finalize score bounds
        int score = analyzer.score;
        if (score < 0) score = 0;

        std::cout << "  \"status\": \"success\"," << std::endl;
        std::cout << "  \"score\": " << score << "," << std::endl;
        std::cout << "  \"total_funcs\": " << analyzer.totalFuncs << "," << std::endl;
        std::cout << "  \"total_vars\": " << analyzer.totalVars << "," << std::endl;
        std::cout << "  \"total_tokens\": " << tokens.size() << "," << std::endl;
        std::cout << "  \"ast\": " << ast->toJson() << "," << std::endl;
        
        std::cout << "  \"tokens\": [" << std::endl;
        for (size_t i = 0; i < tokens.size(); ++i) {
            std::string t_type = "UNKNOWN";
            // Map token enum types to strings roughly mapping to what's in screenshot (KEYWORD, IDENTIFIER, NUMBER, SYMBOL, OPERATOR)
            switch(tokens[i].type) {
                case TokenType::Int: case TokenType::Char: case TokenType::If: case TokenType::Else: 
                case TokenType::While: case TokenType::For: case TokenType::Return: case TokenType::Print: t_type = "KEYWORD"; break;
                case TokenType::Identifier: t_type = "IDENTIFIER"; break;
                case TokenType::IntegerLit: case TokenType::CharLit: t_type = "NUMBER"; break;
                case TokenType::Plus: case TokenType::Minus: case TokenType::Star: case TokenType::Slash: 
                case TokenType::Assign: case TokenType::Eq: case TokenType::Neq: case TokenType::Lt: 
                case TokenType::Gt: case TokenType::Lte: case TokenType::Gte:
                case TokenType::AmpAmp: case TokenType::PipePipe: case TokenType::Bang: t_type = "OPERATOR"; break;
                case TokenType::LParen: case TokenType::RParen: case TokenType::LBrace: case TokenType::RBrace: 
                case TokenType::Semi: case TokenType::Comma: t_type = "SYMBOL"; break;
                case TokenType::Eof: t_type = "EOF"; break;
                default: t_type = "INVALID"; break;
            }
            std::cout << "    { \"line\": " << tokens[i].line << ", \"type\": \"" << t_type << "\", \"value\": \"" << escapeJson(tokens[i].value) << "\" }";
            if (i < tokens.size() - 1) std::cout << ",";
            std::cout << std::endl;
        }
        std::cout << "  ]," << std::endl;
        
        std::cout << "  \"console_output\": [" << std::endl;
        for (size_t i = 0; i < output_logs.size(); ++i) {
            std::cout << "    \"" << escapeJson(output_logs[i]) << "\"";
            if (i < output_logs.size() - 1) std::cout << ",";
            std::cout << std::endl;
        }
        std::cout << "  ]," << std::endl;
        
        std::cout << "  \"errors\": [" << std::endl;
        for (size_t i = 0; i < analyzer.errors.size(); ++i) {
            auto& err = analyzer.errors[i];
            std::cout << "    { \"line\": " << err.line << ", \"type\": \"" << escapeJson(err.type) << "\", \"message\": \"" << escapeJson(err.message) << "\" }";
            if (i < analyzer.errors.size() - 1) std::cout << ",";
            std::cout << std::endl;
        }
        std::cout << "  ]," << std::endl;
        
        std::cout << "  \"warnings\": [" << std::endl;
        for (size_t i = 0; i < analyzer.warnings.size(); ++i) {
            auto& err = analyzer.warnings[i];
            std::cout << "    { \"line\": " << err.line << ", \"type\": \"" << escapeJson(err.type) << "\", \"message\": \"" << escapeJson(err.message) << "\" }";
            if (i < analyzer.warnings.size() - 1) std::cout << ",";
            std::cout << std::endl;
        }
        std::cout << "  ]" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "  \"status\": \"error\"," << std::endl;
        std::cout << "  \"score\": 0," << std::endl;
        std::cout << "  \"errors\": [ { \"line\": 0, \"type\": \"SyntaxError\", \"message\": \"" << escapeJson(e.what()) << "\" } ]," << std::endl;
        std::cout << "  \"warnings\": []" << std::endl;
    }
    std::cout << "}" << std::endl;

    return 0;
}
