#pragma once
#include <string>
#include <vector>
#include <memory>

enum class NodeType {
    Program, FunctionDecl, VarDecl, Assign, If, While, For, Return, Print,
    Block, BinaryOp, Identifier, Literal, FunctionCall
};

struct ASTNode {
    NodeType type;
    int line;
    virtual ~ASTNode() = default;
    ASTNode(NodeType t, int l) : type(t), line(l) {}
};

struct LiteralNode : public ASTNode {
    std::string val_type; // "int" or "char"
    std::string value;
    LiteralNode(std::string t, std::string v, int l) : ASTNode(NodeType::Literal, l), val_type(t), value(v) {}
};

struct IdentifierNode : public ASTNode {
    std::string name;
    IdentifierNode(std::string n, int l) : ASTNode(NodeType::Identifier, l), name(n) {}
};

struct BinaryOpNode : public ASTNode {
    std::string op;
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    BinaryOpNode(std::string o, std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r, int ln) 
        : ASTNode(NodeType::BinaryOp, ln), op(o), left(std::move(l)), right(std::move(r)) {}
};

struct BlockNode : public ASTNode {
    std::vector<std::unique_ptr<ASTNode>> statements;
    BlockNode(int l) : ASTNode(NodeType::Block, l) {}
};

struct FunctionCallNode : public ASTNode {
    std::string name;
    std::vector<std::unique_ptr<ASTNode>> args;
    FunctionCallNode(std::string n, int l) : ASTNode(NodeType::FunctionCall, l), name(n) {}
};

struct PrintNode : public ASTNode {
    std::unique_ptr<ASTNode> expr;
    PrintNode(std::unique_ptr<ASTNode> e, int l) : ASTNode(NodeType::Print, l), expr(std::move(e)) {}
};

struct ReturnNode : public ASTNode {
    std::unique_ptr<ASTNode> expr; // can be null
    ReturnNode(std::unique_ptr<ASTNode> e, int l) : ASTNode(NodeType::Return, l), expr(std::move(e)) {}
};

struct AssignNode : public ASTNode {
    std::string name;
    std::unique_ptr<ASTNode> expr;
    AssignNode(std::string n, std::unique_ptr<ASTNode> e, int l) : ASTNode(NodeType::Assign, l), name(n), expr(std::move(e)) {}
};

struct VarDeclNode : public ASTNode {
    std::string val_type; // "int" or "char"
    std::string name;
    std::unique_ptr<ASTNode> initializer; // can be null
    VarDeclNode(std::string vt, std::string n, std::unique_ptr<ASTNode> init, int l) 
        : ASTNode(NodeType::VarDecl, l), val_type(vt), name(n), initializer(std::move(init)) {}
};

struct IfNode : public ASTNode {
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> true_block;
    std::unique_ptr<ASTNode> false_block; // can be null
    IfNode(std::unique_ptr<ASTNode> c, std::unique_ptr<ASTNode> t, std::unique_ptr<ASTNode> f, int l)
        : ASTNode(NodeType::If, l), condition(std::move(c)), true_block(std::move(t)), false_block(std::move(f)) {}
};

struct WhileNode : public ASTNode {
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> block;
    WhileNode(std::unique_ptr<ASTNode> c, std::unique_ptr<ASTNode> b, int l)
        : ASTNode(NodeType::While, l), condition(std::move(c)), block(std::move(b)) {}
};

struct ForNode : public ASTNode {
    std::unique_ptr<ASTNode> init;
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> update;
    std::unique_ptr<ASTNode> block;
    ForNode(std::unique_ptr<ASTNode> i, std::unique_ptr<ASTNode> c, std::unique_ptr<ASTNode> u, std::unique_ptr<ASTNode> b, int l)
        : ASTNode(NodeType::For, l), init(std::move(i)), condition(std::move(c)), update(std::move(u)), block(std::move(b)) {}
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
};

struct ProgramNode : public ASTNode {
    std::vector<std::unique_ptr<ASTNode>> statements;
    ProgramNode(int l) : ASTNode(NodeType::Program, l) {}
};
