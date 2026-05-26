#pragma once
#include "ast.hpp"
#include <map>
#include <vector>
#include <string>
#include <iostream>

struct SemanticError {
    int line;
    std::string type;
    std::string message;
};

struct VarInfo {
    std::string type;
    int line;
    bool used = false;
    bool initialized = false;
};

struct FuncInfo {
    std::string returnType;
    std::vector<std::string> paramTypes;
    bool used = false;
};

class SemanticAnalyzer {
    std::vector<std::map<std::string, VarInfo>> scopes;
    std::map<std::string, FuncInfo> functions;
    
public:
    std::vector<SemanticError> errors;
    std::vector<SemanticError> warnings;
    int score = 100;
    int totalVars = 0;
    int totalFuncs = 0;

    SemanticAnalyzer() {
        scopes.push_back({}); // global scope
    }

    void enterScope() { scopes.push_back({}); }
    
    void exitScope() {
        // Check unused vars
        for (auto const& pair : scopes.back()) {
            std::string name = pair.first;
            VarInfo info = pair.second;
            if (!info.used) {
                warnings.push_back({info.line, "UnusedVariable", "Variable '" + name + "' is declared but never used"});
                score -= 5;
            }
        }
        scopes.pop_back();
    }

    void error(int line, const std::string& msg) {
        errors.push_back({line, "SemanticError", msg});
        score -= 15;
    }

    void declareVar(const std::string& name, const std::string& type, int line, bool initialized) {
        if (scopes.back().count(name)) {
            error(line, "Variable '" + name + "' already declared in this scope");
        } else {
            scopes.back()[name] = {type, line, false, initialized};
            totalVars++;
        }
    }

    VarInfo* lookupVar(const std::string& name) {
        for (int i = scopes.size() - 1; i >= 0; --i) {
            if (scopes[i].count(name)) return &scopes[i][name];
        }
        return nullptr;
    }

    void visit(ASTNode* node) {
        if (!node) return;

        if (auto p = dynamic_cast<ProgramNode*>(node)) {
            for (auto& stmt : p->statements) visit(stmt.get());
        }
        else if (auto b = dynamic_cast<BlockNode*>(node)) {
            enterScope();
            for (auto& stmt : b->statements) visit(stmt.get());
            exitScope();
        }
        else if (auto f = dynamic_cast<FunctionDeclNode*>(node)) {
            if (functions.count(f->name)) {
                error(f->line, "Function '" + f->name + "' already declared");
            } else {
                std::vector<std::string> pTypes;
                for (auto& p : f->params) pTypes.push_back(p.type);
                functions[f->name] = {f->return_type, pTypes, false};
                totalFuncs++;
            }
            enterScope();
            for (auto& p : f->params) {
                declareVar(p.name, p.type, f->line, true);
            }
            visit(f->block.get());
            exitScope();
        }
        else if (auto vd = dynamic_cast<VarDeclNode*>(node)) {
            if (vd->initializer) {
                visit(vd->initializer.get());
                std::string initType = getType(vd->initializer.get());
                if (initType != "" && initType != vd->val_type) {
                    error(vd->line, "Type mismatch: cannot assign " + initType + " to " + vd->val_type);
                }
            }
            declareVar(vd->name, vd->val_type, vd->line, vd->initializer != nullptr);
        }
        else if (auto a = dynamic_cast<AssignNode*>(node)) {
            visit(a->expr.get());
            VarInfo* v = lookupVar(a->name);
            if (!v) {
                error(a->line, "Undeclared variable '" + a->name + "'");
            } else {
                v->initialized = true;
                std::string exprType = getType(a->expr.get());
                if (exprType != "" && exprType != v->type) {
                    error(a->line, "Type mismatch: cannot assign " + exprType + " to " + v->type);
                }
            }
        }
        else if (auto ifn = dynamic_cast<IfNode*>(node)) {
            visit(ifn->condition.get());
            visit(ifn->true_block.get());
            if (ifn->false_block) visit(ifn->false_block.get());
        }
        else if (auto wn = dynamic_cast<WhileNode*>(node)) {
            visit(wn->condition.get());
            visit(wn->block.get());
        }
        else if (auto fn = dynamic_cast<ForNode*>(node)) {
            enterScope();
            if (fn->init) visit(fn->init.get());
            visit(fn->condition.get());
            if (fn->update) visit(fn->update.get());
            visit(fn->block.get());
            exitScope();
        }
        else if (auto ret = dynamic_cast<ReturnNode*>(node)) {
            if (ret->expr) visit(ret->expr.get());
            // basic unreachable code heuristic (if the next statement is in the same block, handled via syntax in a robust compiler)
        }
        else if (auto pn = dynamic_cast<PrintNode*>(node)) {
            visit(pn->expr.get());
        }
        else if (auto un = dynamic_cast<UnaryOpNode*>(node)) {
            visit(un->expr.get());
            std::string t = getType(un->expr.get());
            if (un->op == "!") {
                if (t != "" && t != "int" && t != "char") {
                    error(un->line, "Type error: logical negation operator '!' expects numeric or char operand");
                }
            }
        }
        else if (auto bin = dynamic_cast<BinaryOpNode*>(node)) {
            visit(bin->left.get());
            visit(bin->right.get());
            std::string lt = getType(bin->left.get());
            std::string rt = getType(bin->right.get());
            if (bin->op == "&&" || bin->op == "||") {
                if ((lt != "" && lt != "int" && lt != "char") || (rt != "" && rt != "int" && rt != "char")) {
                    error(bin->line, "Type error: logical operator '" + bin->op + "' expects int or char operands");
                }
            } else {
                if (lt != rt && lt != "" && rt != "") {
                    error(bin->line, "Type mismatch in binary operation between " + lt + " and " + rt);
                }
            }
        }
        else if (auto fc = dynamic_cast<FunctionCallNode*>(node)) {
            for (auto& arg : fc->args) visit(arg.get());
            if (!functions.count(fc->name)) {
                error(fc->line, "Undeclared function '" + fc->name + "'");
            } else {
                functions[fc->name].used = true;
                if (fc->args.size() != functions[fc->name].paramTypes.size()) {
                    error(fc->line, "Function '" + fc->name + "' expects " + std::to_string(functions[fc->name].paramTypes.size()) + " arguments");
                } else {
                    for (size_t i = 0; i < fc->args.size(); i++) {
                        std::string argType = getType(fc->args[i].get());
                        std::string expectedType = functions[fc->name].paramTypes[i];
                        if (argType != "" && argType != expectedType) {
                            error(fc->line, "Type mismatch in argument " + std::to_string(i + 1) + " of '" + fc->name + "'");
                        }
                    }
                }
            }
        }
        else if (auto id = dynamic_cast<IdentifierNode*>(node)) {
            VarInfo* v = lookupVar(id->name);
            if (!v) {
                error(id->line, "Undeclared variable '" + id->name + "'");
            } else {
                v->used = true;
                if (!v->initialized) {
                    error(id->line, "Variable '" + id->name + "' used before assignment");
                }
            }
        }
    }

    std::string getType(ASTNode* node) {
        if (!node) return "";
        if (auto lit = dynamic_cast<LiteralNode*>(node)) return lit->val_type;
        if (auto id = dynamic_cast<IdentifierNode*>(node)) {
            VarInfo* v = lookupVar(id->name);
            if (v) return v->type;
        }
        if (auto fc = dynamic_cast<FunctionCallNode*>(node)) {
            if (functions.count(fc->name)) return functions[fc->name].returnType;
        }
        if (dynamic_cast<UnaryOpNode*>(node)) return "int";
        // In this simple analyzer, arithmetic and logical expressions result in int
        if (dynamic_cast<BinaryOpNode*>(node)) return "int";
        return "";
    }
};
