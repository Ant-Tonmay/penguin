#pragma once

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include "lexer/lexer.h"

struct ASTNode {
    SourceLocation loc;
    virtual ~ASTNode() = default;
};

enum class AccessModifier {
    PUBLIC,
    PRIVATE,
    PROTECTED,
    SHARED
};

enum class TraitAccessModifier {
    SHARED,
    PROTECTED,
    PUBLIC
};


struct Expr : ASTNode {};

struct BinaryExpr : Expr {
    std::unique_ptr<Expr> left;
    std::string op;
    std::unique_ptr<Expr> right;

    BinaryExpr(std::unique_ptr<Expr> left, std::string op, std::unique_ptr<Expr> right)
        : left(std::move(left)), op(op), right(std::move(right)) {}
};

struct NumberExpr : Expr {
    std::string value;
    explicit NumberExpr(std::string value) : value(value) {}
};

struct StringExpr : Expr {
    std::string value;
    explicit StringExpr(std::string value) : value(value) {}
};

struct BoolExpr : Expr {
    bool value;
    explicit BoolExpr(bool value) : value(value) {}
};

struct VarExpr : Expr {
    std::string name;
    explicit VarExpr(std::string name) : name(name) {}
};

struct Stmt : ASTNode {};

struct Block : Stmt {
    std::vector<std::unique_ptr<Stmt>> statements;
};

struct PrintStmt : Stmt {
    std::unique_ptr<Expr> expression;
    explicit PrintStmt(std::unique_ptr<Expr> expression) 
        : expression(std::move(expression)) {}
};

struct PrintlnStmt : Stmt {
    std::unique_ptr<Expr> expression;
    explicit PrintlnStmt(std::unique_ptr<Expr> expression) 
        : expression(std::move(expression)) {}
};

struct Assignment {
    std::unique_ptr<Expr> target;
    TokenType op;      
    std::unique_ptr<Expr> value;

    Assignment(std::unique_ptr<Expr> target,
               TokenType op,
               std::unique_ptr<Expr> value)
        : target(std::move(target)), op(op), value(std::move(value)) {}
};


struct AssignmentStmt : Stmt {
    std::vector<Assignment> assignments;
    
    explicit AssignmentStmt(std::vector<Assignment> assignments)
        : assignments(std::move(assignments)) {}
};

struct ForStmt : Stmt {
    std::unique_ptr<AssignmentStmt> init;
    std::unique_ptr<Expr> condition;
    std::unique_ptr<AssignmentStmt> increment; 
    std::unique_ptr<Block> body;

    ForStmt(std::unique_ptr<AssignmentStmt> init,
            std::unique_ptr<Expr> condition,
            std::unique_ptr<AssignmentStmt> increment,
            std::unique_ptr<Block> body)
        : init(std::move(init)), 
          condition(std::move(condition)), 
          increment(std::move(increment)), 
          body(std::move(body)) {}
};

struct WhileStmt : Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Block> body;

    WhileStmt(std::unique_ptr<Expr> condition,
              std::unique_ptr<Block> body)
        : condition(std::move(condition)),
          body(std::move(body)){}


};
struct IfStmt : Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Block> thenBranch;
    std::unique_ptr<Stmt> elseBranch;

    IfStmt(std::unique_ptr<Expr> condition,
           std::unique_ptr<Block> thenBranch,
           std::unique_ptr<Stmt> elseBranch)
        : condition(std::move(condition)),
          thenBranch(std::move(thenBranch)),
          elseBranch(std::move(elseBranch)) {}
};

struct CatchClause {
    std::string exceptionType;
    std::string variableName; 
    std::unique_ptr<Block> block;
    
    CatchClause(std::string type, std::string var, std::unique_ptr<Block> blk)
        : exceptionType(std::move(type)), variableName(std::move(var)), block(std::move(blk)) {}
};


struct TryCatchStmt : Stmt {
    std::unique_ptr<Block> tryBlock;
    std::vector<CatchClause> catchBlocks;
    std::unique_ptr<Block> finallyBlock;

    TryCatchStmt(std::unique_ptr<Block> tryBlock,
                 std::vector<CatchClause> catchBlocks,
                 std::unique_ptr<Block> finallyBlock)
        : tryBlock(std::move(tryBlock)), catchBlocks(std::move(catchBlocks)), finallyBlock(std::move(finallyBlock)) {}
};

struct ThrowStmt : Stmt {
    std::unique_ptr<Expr> expression;
    explicit ThrowStmt(std::unique_ptr<Expr> expression)
        : expression(std::move(expression)) {}
};


struct Param {
    std::string name;
    bool isRef;

    Param(std::string name, bool isRef)
        : name(std::move(name)), isRef(isRef) {}
};

struct Function : ASTNode {
    std::string name;
    std::vector<Param> params;
    std::unique_ptr<Block> body;

    Function(std::string name, std::vector<Param> params, std::unique_ptr<Block> body)
        : name(std::move(name)), params(std::move(params)), body(std::move(body)) {}
};


struct UnaryExpr : Expr {
    std::string op;
    std::unique_ptr<Expr> right;

    UnaryExpr(std::string op, std::unique_ptr<Expr> right)
        : op(std::move(op)), right(std::move(right)) {}
};

struct ArrayExpr : Expr {
    std::vector<std::unique_ptr<Expr>> elements;

    explicit ArrayExpr(std::vector<std::unique_ptr<Expr>> elements)
        : elements(std::move(elements)) {}
};

struct IndexExpr : Expr {
    std::unique_ptr<Expr> array;
    std::unique_ptr<Expr> index;

    IndexExpr(std::unique_ptr<Expr> array,
              std::unique_ptr<Expr> index)
        : array(std::move(array)), index(std::move(index)) {}
};

struct CallExpr : Expr {
    std::unique_ptr<Expr> callee;
    std::vector<std::unique_ptr<Expr>> arguments;

    CallExpr(std::unique_ptr<Expr> callee, std::vector<std::unique_ptr<Expr>> arguments)
        : callee(std::move(callee)), arguments(std::move(arguments)) {}
};

struct MemberExpr : Expr {
    std::unique_ptr<Expr> object;
    std::string name;

    MemberExpr(std::unique_ptr<Expr> object, std::string name)
        : object(std::move(object)), name(name) {}
};
struct ReturnStmt : Stmt {
    std::unique_ptr<Expr> value; 

    explicit ReturnStmt(std::unique_ptr<Expr> value)
        : value(std::move(value)) {}
};

struct ExprStmt : Stmt {
    std::unique_ptr<Expr> expression;
    explicit ExprStmt(std::unique_ptr<Expr> expression)
        : expression(std::move(expression)) {}
};
struct BreakStmt : Stmt {};

struct ContinueStmt : Stmt {};


struct ClassMember : ASTNode {
    virtual ~ClassMember() = default;
};

struct TraitMember : ASTNode
{
    virtual ~TraitMember() = default;
};

struct TraitFieldDecl : TraitMember {
    std::string name;
    std::unique_ptr<Expr> initializer;

    explicit TraitFieldDecl(std::string name, std::unique_ptr<Expr> initializer = nullptr)
        : name(std::move(name)), initializer(std::move(initializer)) {}
};

struct TraitMethodDecl : TraitMember {
    std::string name;
    std::vector<Param> params;

    TraitMethodDecl(std::string name, std::vector<Param> params)
        : name(std::move(name)), params(std::move(params)) {}
};

struct FieldDecl : ClassMember {
    std::string name;
    std::unique_ptr<Expr> initializer;

    explicit FieldDecl(std::string name, std::unique_ptr<Expr> initializer = nullptr)
        : name(std::move(name)), initializer(std::move(initializer)) {}
};

struct MethodDecl : ClassMember {
    std::string name;
    std::vector<Param> params;

    MethodDecl(std::string name, std::vector<Param> params)
        : name(std::move(name)), params(std::move(params)) {}
};

struct MethodDef : ClassMember {
    std::string name;
    std::vector<Param> params;
    std::unique_ptr<Block> body;

    MethodDef(std::string name,
              std::vector<Param> params,
              std::unique_ptr<Block> body)
        : name(std::move(name)),
          params(std::move(params)),
          body(std::move(body)) {}
};

struct ClassSection : ASTNode {
    AccessModifier modifier;
    std::vector<std::unique_ptr<ClassMember>> members;

    ClassSection(AccessModifier modifier,
                 std::vector<std::unique_ptr<ClassMember>> members)
        : modifier(modifier),
          members(std::move(members)) {}
};

struct TraitSection : ASTNode {
    TraitAccessModifier modifier;
    std::vector<std::unique_ptr<TraitMember>> members;

    TraitSection(TraitAccessModifier modifier,
                 std::vector<std::unique_ptr<TraitMember>> members)
        : modifier(modifier),
          members(std::move(members)) {}
};

struct ClassStmt : Stmt {
    std::string name;
    std::string parentName; 
    std::vector<std::string> impls;
    std::vector<std::unique_ptr<ClassSection>> sections;

    ClassStmt(std::string name,
              std::vector<std::unique_ptr<ClassSection>> sections,
              std::string parentName = "",
              std::vector<std::string> impls = {})
        : name(std::move(name)),
          parentName(std::move(parentName)),
          impls(std::move(impls)),
          sections(std::move(sections)) {}
};

struct TraitStmt : Stmt {
    std::string name;
    std::vector<std::string> parents;
    std::vector<std::unique_ptr<TraitSection>> sections;

    TraitStmt(std::string name,
              std::vector<std::unique_ptr<TraitSection>> sections,
              std::vector<std::string> parents = {})
        : name(std::move(name)),
          parents(std::move(parents)),
          sections(std::move(sections)) {}


};

struct IncludeStmt : Stmt {
    std::string name;
    std::vector<std::string> members;
    std::string alias;

    IncludeStmt(std::string name,
                std::vector<std::string> members,
                std::string alias = "")
        : name(std::move(name)),
          members(std::move(members)),
          alias(std::move(alias)) {}
};

struct AliasStmt : Stmt {
    std::string name;
    std::unique_ptr<IncludeStmt> include;
    std::string vname;
    std::vector<std::string> resolvedExports;

    // Constructor for include alias
    AliasStmt(std::string name,
              std::unique_ptr<IncludeStmt> include)
        : name(std::move(name)),
          include(std::move(include)),
          vname("") {}

    // Constructor for variable alias
    AliasStmt(std::string name, std::string vname)
        : name(std::move(name)),
          include(nullptr),
          vname(std::move(vname)) {}
};

struct ExportStmt : ASTNode {
    std::vector<std::string> members;

    ExportStmt(std::vector<std::string> members)
        : members(std::move(members)) {}
};


struct Program : ASTNode {
    std::vector<std::unique_ptr<IncludeStmt>> includes;
    std::vector<std::unique_ptr<AliasStmt>> aliases;
    std::vector<std::unique_ptr<Function>> functions;
    std::vector<std::unique_ptr<ClassStmt>> classes;
    std::vector<std::unique_ptr<TraitStmt>> traits;
    std::vector<std::unique_ptr<ExportStmt>> exports;
};
