#pragma once
#include <string>
#include <vector>
#include <memory>

enum class NodeType {
    Program, FunctionDecl, VarDecl, Assign, If, While, For, Return, Print,
    Block, BinaryOp, UnaryOp, Identifier, Literal, FunctionCall
};

struct ASTNode {
    NodeType type;
    int line;
    virtual ~ASTNode() = default;
    ASTNode(NodeType t, int l) : type(t), line(l) {}
    virtual std::string toJson() const = 0;
};

struct LiteralNode : public ASTNode {
    std::string val_type; // "int" or "char"
    std::string value;
    LiteralNode(std::string t, std::string v, int l) : ASTNode(NodeType::Literal, l), val_type(t), value(v) {}
    
    std::string toJson() const override {
        return "{ \"type\": \"Literal\", \"line\": " + std::to_string(line) + 
               ", \"val_type\": \"" + val_type + "\", \"value\": \"" + value + "\" }";
    }
};

struct IdentifierNode : public ASTNode {
    std::string name;
    IdentifierNode(std::string n, int l) : ASTNode(NodeType::Identifier, l), name(n) {}
    
    std::string toJson() const override {
        return "{ \"type\": \"Identifier\", \"line\": " + std::to_string(line) + 
               ", \"name\": \"" + name + "\" }";
    }
};

struct BinaryOpNode : public ASTNode {
    std::string op;
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    BinaryOpNode(std::string o, std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r, int ln) 
        : ASTNode(NodeType::BinaryOp, ln), op(o), left(std::move(l)), right(std::move(r)) {}
    
    std::string toJson() const override {
        return "{ \"type\": \"BinaryOp\", \"line\": " + std::to_string(line) + 
               ", \"op\": \"" + op + "\", \"left\": " + (left ? left->toJson() : "null") + 
               ", \"right\": " + (right ? right->toJson() : "null") + " }";
    }
};

struct UnaryOpNode : public ASTNode {
    std::string op;
    std::unique_ptr<ASTNode> expr;
    UnaryOpNode(std::string o, std::unique_ptr<ASTNode> e, int ln) 
        : ASTNode(NodeType::UnaryOp, ln), op(o), expr(std::move(e)) {}
    
    std::string toJson() const override {
        return "{ \"type\": \"UnaryOp\", \"line\": " + std::to_string(line) + 
               ", \"op\": \"" + op + "\", \"expr\": " + (expr ? expr->toJson() : "null") + " }";
    }
};

struct BlockNode : public ASTNode {
    std::vector<std::unique_ptr<ASTNode>> statements;
    BlockNode(int l) : ASTNode(NodeType::Block, l) {}
    
    std::string toJson() const override {
        std::string res = "{ \"type\": \"Block\", \"line\": " + std::to_string(line) + ", \"statements\": [";
        for (size_t i = 0; i < statements.size(); ++i) {
            res += statements[i]->toJson();
            if (i < statements.size() - 1) res += ", ";
        }
        res += "] }";
        return res;
    }
};

struct FunctionCallNode : public ASTNode {
    std::string name;
    std::vector<std::unique_ptr<ASTNode>> args;
    FunctionCallNode(std::string n, int l) : ASTNode(NodeType::FunctionCall, l), name(n) {}
    
    std::string toJson() const override {
        std::string res = "{ \"type\": \"FunctionCall\", \"line\": " + std::to_string(line) + 
                          ", \"name\": \"" + name + "\", \"args\": [";
        for (size_t i = 0; i < args.size(); ++i) {
            res += args[i]->toJson();
            if (i < args.size() - 1) res += ", ";
        }
        res += "] }";
        return res;
    }
};

struct PrintNode : public ASTNode {
    std::unique_ptr<ASTNode> expr;
    PrintNode(std::unique_ptr<ASTNode> e, int l) : ASTNode(NodeType::Print, l), expr(std::move(e)) {}
    
    std::string toJson() const override {
        return "{ \"type\": \"Print\", \"line\": " + std::to_string(line) + 
               ", \"expr\": " + (expr ? expr->toJson() : "null") + " }";
    }
};

struct ReturnNode : public ASTNode {
    std::unique_ptr<ASTNode> expr; // can be null
    ReturnNode(std::unique_ptr<ASTNode> e, int l) : ASTNode(NodeType::Return, l), expr(std::move(e)) {}
    
    std::string toJson() const override {
        return "{ \"type\": \"Return\", \"line\": " + std::to_string(line) + 
               ", \"expr\": " + (expr ? expr->toJson() : "null") + " }";
    }
};

struct AssignNode : public ASTNode {
    std::string name;
    std::unique_ptr<ASTNode> expr;
    AssignNode(std::string n, std::unique_ptr<ASTNode> e, int l) : ASTNode(NodeType::Assign, l), name(n), expr(std::move(e)) {}
    
    std::string toJson() const override {
        return "{ \"type\": \"Assign\", \"line\": " + std::to_string(line) + 
               ", \"name\": \"" + name + "\", \"expr\": " + (expr ? expr->toJson() : "null") + " }";
    }
};

struct VarDeclNode : public ASTNode {
    std::string val_type; // "int" or "char"
    std::string name;
    std::unique_ptr<ASTNode> initializer; // can be null
    VarDeclNode(std::string vt, std::string n, std::unique_ptr<ASTNode> init, int l) 
        : ASTNode(NodeType::VarDecl, l), val_type(vt), name(n), initializer(std::move(init)) {}
    
    std::string toJson() const override {
        return "{ \"type\": \"VarDecl\", \"line\": " + std::to_string(line) + 
               ", \"val_type\": \"" + val_type + "\", \"name\": \"" + name + 
               "\", \"initializer\": " + (initializer ? initializer->toJson() : "null") + " }";
    }
};

struct IfNode : public ASTNode {
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> true_block;
    std::unique_ptr<ASTNode> false_block; // can be null
    IfNode(std::unique_ptr<ASTNode> c, std::unique_ptr<ASTNode> t, std::unique_ptr<ASTNode> f, int l)
        : ASTNode(NodeType::If, l), condition(std::move(c)), true_block(std::move(t)), false_block(std::move(f)) {}
    
    std::string toJson() const override {
        return "{ \"type\": \"If\", \"line\": " + std::to_string(line) + 
               ", \"condition\": " + (condition ? condition->toJson() : "null") + 
               ", \"true_block\": " + (true_block ? true_block->toJson() : "null") + 
               ", \"false_block\": " + (false_block ? false_block->toJson() : "null") + " }";
    }
};

struct WhileNode : public ASTNode {
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> block;
    WhileNode(std::unique_ptr<ASTNode> c, std::unique_ptr<ASTNode> b, int l)
        : ASTNode(NodeType::While, l), condition(std::move(c)), block(std::move(b)) {}
    
    std::string toJson() const override {
        return "{ \"type\": \"While\", \"line\": " + std::to_string(line) + 
               ", \"condition\": " + (condition ? condition->toJson() : "null") + 
               ", \"block\": " + (block ? block->toJson() : "null") + " }";
    }
};

struct ForNode : public ASTNode {
    std::unique_ptr<ASTNode> init;
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> update;
    std::unique_ptr<ASTNode> block;
    ForNode(std::unique_ptr<ASTNode> i, std::unique_ptr<ASTNode> c, std::unique_ptr<ASTNode> u, std::unique_ptr<ASTNode> b, int l)
        : ASTNode(NodeType::For, l), init(std::move(i)), condition(std::move(c)), update(std::move(u)), block(std::move(b)) {}
    
    std::string toJson() const override {
        return "{ \"type\": \"For\", \"line\": " + std::to_string(line) + 
               ", \"init\": " + (init ? init->toJson() : "null") + 
               ", \"condition\": " + (condition ? condition->toJson() : "null") + 
               ", \"update\": " + (update ? update->toJson() : "null") + 
               ", \"block\": " + (block ? block->toJson() : "null") + " }";
    }
};

struct Param {
    std::string type;
    std::string name;
};

struct FunctionDeclNode : public ASTNode {
    std::string return_type;
    std::string name;
    std::vector<Param> params;
    std::unique_ptr<ASTNode> block;
    FunctionDeclNode(std::string rt, std::string n, std::vector<Param> p, std::unique_ptr<ASTNode> b, int l)
        : ASTNode(NodeType::FunctionDecl, l), return_type(rt), name(n), params(p), block(std::move(b)) {}
    
    std::string toJson() const override {
        std::string res = "{ \"type\": \"FunctionDecl\", \"line\": " + std::to_string(line) + 
                          ", \"return_type\": \"" + return_type + "\", \"name\": \"" + name + "\", \"params\": [";
        for (size_t i = 0; i < params.size(); ++i) {
            res += "{ \"type\": \"" + params[i].type + "\", \"name\": \"" + params[i].name + "\" }";
            if (i < params.size() - 1) res += ", ";
        }
        res += "], \"block\": " + (block ? block->toJson() : "null") + " }";
        return res;
    }
};

struct ProgramNode : public ASTNode {
    std::vector<std::unique_ptr<ASTNode>> statements;
    ProgramNode(int l) : ASTNode(NodeType::Program, l) {}
    
    std::string toJson() const override {
        std::string res = "{ \"type\": \"Program\", \"line\": " + std::to_string(line) + ", \"statements\": [";
        for (size_t i = 0; i < statements.size(); ++i) {
            res += statements[i]->toJson();
            if (i < statements.size() - 1) res += ", ";
        }
        res += "] }";
        return res;
    }
};
