#pragma once
#include "ast.hpp"
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>

struct Value {
    std::string type; // "int" or "char" or "void" (or "return_int" etc for control flow)
    int int_val = 0;
    char char_val = 0;
};

class Interpreter {
    std::vector<std::map<std::string, Value>> env; // Call stack of environments
    std::map<std::string, FunctionDeclNode*> functions;

    struct ReturnException {
        Value val;
    };

public:
    std::vector<std::string> console_output;

    Interpreter() {
        env.push_back({}); // global scope
    }

    void enterScope() { env.push_back({}); }
    void exitScope() { env.pop_back(); }

    void setVar(const std::string& name, Value val) {
        for (int i = env.size() - 1; i >= 0; --i) {
            if (env[i].count(name) || i == 0 /* declare globally if nowhere else, simple heuristic */) {
                env[i][name] = val;
                return;
            }
        }
    }

    Value getVar(const std::string& name) {
        for (int i = env.size() - 1; i >= 0; --i) {
            if (env[i].count(name)) return env[i][name];
        }
        return {"int", 0, 0}; // Should not happen if semantics checked
    }

    void visitProgram(ASTNode* node) {
        if (auto p = dynamic_cast<ProgramNode*>(node)) {
            // First pass: register functions
            for (auto& stmt : p->statements) {
                if (auto f = dynamic_cast<FunctionDeclNode*>(stmt.get())) {
                    functions[f->name] = f;
                }
            }
            // Second pass: exec global statements and call main if exists
            for (auto& stmt : p->statements) {
                if (!dynamic_cast<FunctionDeclNode*>(stmt.get())) {
                    visitNode(stmt.get());
                }
            }
            // Call main
            if (functions.count("main")) {
                callFunction("main", {});
            }
        }
    }

    Value callFunction(const std::string& name, std::vector<Value> args) {
        auto f = functions[name];
        std::map<std::string, Value> local_env;
        for (size_t i = 0; i < args.size(); ++i) {
            local_env[f->params[i].name] = args[i];
        }
        env.push_back(local_env);
        
        try {
            visitNode(f->block.get());
        } catch (ReturnException& ret) {
            env.pop_back();
            return ret.val;
        }
        
        env.pop_back();
        return {"void", 0, 0};
    }

    void visitNode(ASTNode* node) {
        if (!node) return;

        if (auto b = dynamic_cast<BlockNode*>(node)) {
            enterScope();
            for (auto& stmt : b->statements) visitNode(stmt.get());
            exitScope();
        }
        else if (auto vd = dynamic_cast<VarDeclNode*>(node)) {
            Value val = {vd->val_type, 0, 0};
            if (vd->initializer) {
                val = evalExpr(vd->initializer.get());
            }
            env.back()[vd->name] = val;
        }
        else if (auto a = dynamic_cast<AssignNode*>(node)) {
            Value val = evalExpr(a->expr.get());
            setVar(a->name, val);
        }
        else if (auto ifn = dynamic_cast<IfNode*>(node)) {
            Value cond = evalExpr(ifn->condition.get());
            bool truthy = (cond.type == "int" && cond.int_val != 0) || (cond.type == "char" && cond.char_val != 0);
            if (truthy) {
                visitNode(ifn->true_block.get());
            } else if (ifn->false_block) {
                visitNode(ifn->false_block.get());
            }
        }
        else if (auto wn = dynamic_cast<WhileNode*>(node)) {
            while (true) {
                Value cond = evalExpr(wn->condition.get());
                bool truthy = (cond.type == "int" && cond.int_val != 0) || (cond.type == "char" && cond.char_val != 0);
                if (!truthy) break;
                visitNode(wn->block.get());
            }
        }
        else if (auto fn = dynamic_cast<ForNode*>(node)) {
            enterScope();
            if (fn->init) visitNode(fn->init.get());
            while (true) {
                Value cond = evalExpr(fn->condition.get());
                bool truthy = (cond.type == "int" && cond.int_val != 0) || (cond.type == "char" && cond.char_val != 0);
                if (!truthy) break;
                visitNode(fn->block.get());
                if (fn->update) visitNode(fn->update.get());
            }
            exitScope();
        }
        else if (auto ret = dynamic_cast<ReturnNode*>(node)) {
            Value val = {"void", 0, 0};
            if (ret->expr) val = evalExpr(ret->expr.get());
            throw ReturnException{val};
        }
        else if (auto pn = dynamic_cast<PrintNode*>(node)) {
            Value val = evalExpr(pn->expr.get());
            if (val.type == "int") {
                console_output.push_back(std::to_string(val.int_val));
            } else if (val.type == "char") {
                std::string s(1, val.char_val);
                console_output.push_back(s);
            }
        }
        else if (auto fc = dynamic_cast<FunctionCallNode*>(node)) {
            evalExpr(node); // Call function ignoring return value
        }
    }

    Value evalExpr(ASTNode* node) {
        if (!node) return {"void", 0, 0};

        if (auto lit = dynamic_cast<LiteralNode*>(node)) {
            if (lit->val_type == "int") return {"int", std::stoi(lit->value), 0};
            if (lit->val_type == "char") return {"char", 0, lit->value[0]};
        }
        else if (auto id = dynamic_cast<IdentifierNode*>(node)) {
            return getVar(id->name);
        }
        else if (auto fc = dynamic_cast<FunctionCallNode*>(node)) {
            std::vector<Value> args;
            for (auto& a : fc->args) args.push_back(evalExpr(a.get()));
            return callFunction(fc->name, args);
        }
        else if (auto bin = dynamic_cast<BinaryOpNode*>(node)) {
            Value l = evalExpr(bin->left.get());
            Value r = evalExpr(bin->right.get());
            
            int lv = l.type == "int" ? l.int_val : (int)l.char_val;
            int rv = r.type == "int" ? r.int_val : (int)r.char_val;
            
            if (bin->op == "+") return {"int", lv + rv, 0};
            if (bin->op == "-") return {"int", lv - rv, 0};
            if (bin->op == "*") return {"int", lv * rv, 0};
            if (bin->op == "/") return {"int", rv != 0 ? lv / rv : 0, 0};
            if (bin->op == "==") return {"int", lv == rv ? 1 : 0, 0};
            if (bin->op == "!=") return {"int", lv != rv ? 1 : 0, 0};
            if (bin->op == "<") return {"int", lv < rv ? 1 : 0, 0};
            if (bin->op == ">") return {"int", lv > rv ? 1 : 0, 0};
            if (bin->op == "<=") return {"int", lv <= rv ? 1 : 0, 0};
            if (bin->op == ">=") return {"int", lv >= rv ? 1 : 0, 0};
        }
        return {"int", 0, 0};
    }
};
