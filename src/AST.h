#ifndef AST_H
#define AST_H

#include "Token.h"
#include "Value.h"
#include <memory>
#include <vector>

class Expr;
class Stmt;

using ExprPtr = std::shared_ptr<Expr>;
using StmtPtr = std::shared_ptr<Stmt>;

class Expr {
public:
	virtual ~Expr() = default;
};

class LiteralExpr : public Expr {
public:
	Value value;
	LiteralExpr(const Value& v) : value(v) {}
};

class VariableExpr : public Expr {
public:
	std::string name;
	VariableExpr(const std::string& n) : name(n) {}
};

class AssignmentExpr : public Expr {
public:
	std::string name;
	ExprPtr value;
	AssignmentExpr(const std::string& n, ExprPtr v) : name(n), value(v) {}
};

class CompoundAssignExpr : public Expr {
public:
	std::string name;
	Token op;
	ExprPtr value;
	CompoundAssignExpr(const std::string& n, const Token& o, ExprPtr v)
		: name(n), op(o), value(v) {
	}
};

class PostfixExpr : public Expr {
public:
	std::string name;
	Token op;
	PostfixExpr(const std::string& n, const Token& o) : name(n), op(o) {}
};

class GroupingExpr : public Expr {
public:
	ExprPtr expression;
	GroupingExpr(ExprPtr expr) : expression(expr) {}
};

class BinaryExpr : public Expr {
public:
	ExprPtr left;
	Token op;
	ExprPtr right;
	BinaryExpr(ExprPtr l, const Token& o, ExprPtr r)
		: left(l), op(o), right(r) {
	}
};

class UnaryExpr : public Expr {
public:
	Token op;
	ExprPtr right;
	UnaryExpr(const Token& o, ExprPtr r) : op(o), right(r) {}
};

class ArrayExpr : public Expr {
public:
	std::vector<ExprPtr> elements;
	ArrayExpr(const std::vector<ExprPtr>& elems) : elements(elems) {}
};

class IndexExpr : public Expr {
public:
	ExprPtr array;
	ExprPtr index;
	IndexExpr(ExprPtr a, ExprPtr i) : array(a), index(i) {}
};

class IndexAssignExpr : public Expr {
public:
	ExprPtr array;
	ExprPtr index;
	ExprPtr value;
	IndexAssignExpr(ExprPtr a, ExprPtr i, ExprPtr v)
		: array(a), index(i), value(v) {
	}
};

class CallExpr : public Expr {
public:
	ExprPtr callee;
	std::vector<ExprPtr> arguments;
	CallExpr(ExprPtr c, const std::vector<ExprPtr>& args)
		: callee(c), arguments(args) {
	}
};

class Stmt {
public:
	virtual ~Stmt() = default;
};

class ExprStmt : public Stmt {
public:
	ExprPtr expression;
	ExprStmt(ExprPtr expr) : expression(expr) {}
};

class PrintStmt : public Stmt {
public:
	ExprPtr expression;
	PrintStmt(ExprPtr expr) : expression(expr) {}
};

class VarStmt : public Stmt {
public:
	std::string name;
	ExprPtr initializer;
	VarStmt(const std::string& n, ExprPtr init)
		: name(n), initializer(init) {
	}
};

class BlockStmt : public Stmt {
public:
	std::vector<StmtPtr> statements;
	BlockStmt(const std::vector<StmtPtr>& stmts) : statements(stmts) {}
};

class IfStmt : public Stmt {
public:
	ExprPtr condition;
	StmtPtr thenBranch;
	StmtPtr elseBranch;
	IfStmt(ExprPtr cond, StmtPtr then, StmtPtr els = nullptr)
		: condition(cond), thenBranch(then), elseBranch(els) {
	}
};

class WhileStmt : public Stmt {
public:
	ExprPtr condition;
	StmtPtr body;
	WhileStmt(ExprPtr cond, StmtPtr b) : condition(cond), body(b) {}
};

class ParallelForStmt : public Stmt {
public:
	std::string varName;
	ExprPtr start;
	ExprPtr end;
	StmtPtr body;
	ParallelForStmt(const std::string& var, ExprPtr s, ExprPtr e, StmtPtr b)
		: varName(var), start(s), end(e), body(b) {
	}
};

class FunctionStmt : public Stmt {
public:
	std::string name;
	std::vector<std::string> params;
	std::vector<StmtPtr> body;
	FunctionStmt(const std::string& n, const std::vector<std::string>& p,
		const std::vector<StmtPtr>& b)
		: name(n), params(p), body(b) {
	}
};

class ReturnStmt : public Stmt {
public:
	ExprPtr value;
	ReturnStmt(ExprPtr v) : value(v) {}
};

#endif