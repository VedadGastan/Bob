#ifndef PARSER_H
#define PARSER_H

#include "Token.h"
#include "AST.h"
#include <vector>
#include <stdexcept>
#include <memory>

class ParserError : public std::runtime_error {
public:
	Token token;
	ParserError(const Token& t, const std::string& message)
		: std::runtime_error(message), token(t) {}
};


class Parser {
private:
	std::vector<Token> tokens;
	size_t current = 0;

	bool isAtEnd() { return peek().type == TokenType::END_OF_FILE; }
	Token peek() { return tokens[current]; }
	Token previous() { return tokens[current - 1]; }

	Token advance() {
		if (!isAtEnd()) current++;
		return previous();
	}

	bool check(TokenType type) {
		if (isAtEnd()) return false;
		return peek().type == type;
	}

	bool match(TokenType type) {
		if (check(type)) {
			advance();
			return true;
		}
		return false;
	}

	void skipNewlines() {
		while (match(TokenType::NEWLINE)) {}
	}

	Token consume(TokenType type, const std::string& message) {
		if (check(type)) return advance();
		throw ParserError(peek(), message);
	}

	ExprPtr expression() { return assignment(); }

	ExprPtr assignment() {
		ExprPtr expr = logicalOr();

		if (match(TokenType::EQUAL)) {
			ExprPtr value = assignment();

			if (auto var = std::dynamic_pointer_cast<VariableExpr>(expr)) {
				return std::make_shared<AssignmentExpr>(var->name, value);
			}

			if (auto index = std::dynamic_pointer_cast<IndexExpr>(expr)) {
				return std::make_shared<IndexAssignExpr>(index->array, index->index, value);
			}

			throw ParserError(previous(), "Invalid assignment target");
		}

		if (match(TokenType::PLUS_EQUAL) || match(TokenType::MINUS_EQUAL) ||
			match(TokenType::STAR_EQUAL) || match(TokenType::SLASH_EQUAL) ||
			match(TokenType::PERCENT_EQUAL)) {
			Token op = previous();
			ExprPtr value = assignment();

			if (auto var = std::dynamic_pointer_cast<VariableExpr>(expr)) {
				return std::make_shared<CompoundAssignExpr>(var->name, op, value);
			}

			throw ParserError(previous(), "Invalid assignment target");
		}

		return expr;
	}

	ExprPtr logicalOr() {
		ExprPtr expr = logicalAnd();
		while (match(TokenType::OR)) {
			Token op = previous();
			ExprPtr right = logicalAnd();
			expr = std::make_shared<BinaryExpr>(expr, op, right);
		}
		return expr;
	}

	ExprPtr logicalAnd() {
		ExprPtr expr = equality();
		while (match(TokenType::AND)) {
			Token op = previous();
			ExprPtr right = equality();
			expr = std::make_shared<BinaryExpr>(expr, op, right);
		}
		return expr;
	}

	ExprPtr equality() {
		ExprPtr expr = comparison();
		while (match(TokenType::EQUAL_EQUAL) || match(TokenType::BANG_EQUAL)) {
			Token op = previous();
			ExprPtr right = comparison();
			expr = std::make_shared<BinaryExpr>(expr, op, right);
		}
		return expr;
	}

	ExprPtr comparison() {
		ExprPtr expr = addition();
		while (match(TokenType::GREATER) || match(TokenType::GREATER_EQUAL) ||
			match(TokenType::LESS) || match(TokenType::LESS_EQUAL) ||
			match(TokenType::IN)) {
			Token op = previous();
			ExprPtr right = addition();
			expr = std::make_shared<BinaryExpr>(expr, op, right);
		}
		return expr;
	}

	ExprPtr addition() {
		ExprPtr expr = multiplication();
		while (match(TokenType::MINUS) || match(TokenType::PLUS)) {
			Token op = previous();
			ExprPtr right = multiplication();
			expr = std::make_shared<BinaryExpr>(expr, op, right);
		}
		return expr;
	}

	ExprPtr multiplication() {
		ExprPtr expr = exponentiation();
		while (match(TokenType::SLASH) || match(TokenType::STAR) || match(TokenType::PERCENT)) {
			Token op = previous();
			ExprPtr right = exponentiation();
			expr = std::make_shared<BinaryExpr>(expr, op, right);
		}
		return expr;
	}

	ExprPtr exponentiation() {
		ExprPtr expr = unary();
		while (match(TokenType::STAR_STAR)) {
			Token op = previous();
			ExprPtr right = unary();
			expr = std::make_shared<BinaryExpr>(expr, op, right);
		}
		return expr;
	}

	ExprPtr unary() {
		if (match(TokenType::NOT) || match(TokenType::MINUS)) {
			Token op = previous();
			ExprPtr right = unary();
			return std::make_shared<UnaryExpr>(op, right);
		}
		return call();
	}

	ExprPtr call() {
		ExprPtr expr = primary();

		while (true) {
			if (match(TokenType::LPAREN)) {
				expr = finishCall(expr);
			}
			else if (match(TokenType::LBRACKET)) {
				ExprPtr index = expression();
				consume(TokenType::RBRACKET, "Expect ']' after array index");
				expr = std::make_shared<IndexExpr>(expr, index);
			}
			else if (match(TokenType::PLUS_PLUS) || match(TokenType::MINUS_MINUS)) {
				Token op = previous();
				if (auto var = std::dynamic_pointer_cast<VariableExpr>(expr)) {
					expr = std::make_shared<PostfixExpr>(var->name, op);
				}
				else {
					throw ParserError(op, "Invalid postfix target");
				}
			}
			else {
				break;
			}
		}

		return expr;
	}

	ExprPtr finishCall(ExprPtr callee) {
		std::vector<ExprPtr> arguments;
		if (!check(TokenType::RPAREN)) {
			do {
				arguments.push_back(expression());
			} while (match(TokenType::COMMA));
		}
		consume(TokenType::RPAREN, "Expect ')' after arguments");
		return std::make_shared<CallExpr>(callee, arguments);
	}

	ExprPtr primary() {
		if (match(TokenType::FALSE)) return std::make_shared<LiteralExpr>(Value::Bool(false));
		if (match(TokenType::TRUE)) return std::make_shared<LiteralExpr>(Value::Bool(true));
		if (match(TokenType::NIL)) return std::make_shared<LiteralExpr>(Value::Nil());

		if (match(TokenType::NUMBER)) {
			return std::make_shared<LiteralExpr>(Value::Number(std::stod(previous().lexeme)));
		}

		if (match(TokenType::STRING)) {
			return std::make_shared<LiteralExpr>(Value::String(previous().lexeme));
		}

		if (match(TokenType::IDENTIFIER)) {
			return std::make_shared<VariableExpr>(previous().lexeme);
		}

		if (match(TokenType::LPAREN)) {
			ExprPtr expr = expression();
			consume(TokenType::RPAREN, "Expect ')' after expression");
			return std::make_shared<GroupingExpr>(expr);
		}

		if (match(TokenType::LBRACKET)) {
			std::vector<ExprPtr> elements;
			if (!check(TokenType::RBRACKET)) {
				do {
					elements.push_back(expression());
				} while (match(TokenType::COMMA));
			}
			consume(TokenType::RBRACKET, "Expect ']' after array elements");
			return std::make_shared<ArrayExpr>(elements);
		}

		throw ParserError(peek(), "Expect expression");
	}

	StmtPtr statement() {
		if (match(TokenType::IF)) return ifStatement();
		if (match(TokenType::WHILE)) return whileStatement();
		if (match(TokenType::FOR)) return forStatement();
		if (match(TokenType::PARALLEL)) return parallelStatement();
		if (match(TokenType::RETURN)) return returnStatement();
		if (match(TokenType::LBRACE)) {
			return std::make_shared<BlockStmt>(parseBlockBody());
		}
		return expressionStatement();
	}

	std::vector<StmtPtr> parseBlockBody() {
		std::vector<StmtPtr> statements;
		skipNewlines();
		while (!check(TokenType::RBRACE) && !isAtEnd()) {
			statements.push_back(declaration());
			skipNewlines();
		}
		consume(TokenType::RBRACE, "Expect '}' after block");
		return statements;
	}

	StmtPtr expressionStatement() {
		ExprPtr expr = expression();
		skipNewlines();
		if (check(TokenType::SEMICOLON)) advance();
		return std::make_shared<ExprStmt>(expr);
	}

	StmtPtr ifStatement() {
		consume(TokenType::LPAREN, "Expect '(' after 'if'");
		ExprPtr condition = expression();
		consume(TokenType::RPAREN, "Expect ')' after if condition");

		StmtPtr thenBranch = statement();
		StmtPtr elseBranch = nullptr;

		if (match(TokenType::ELIF)) {
			elseBranch = ifStatement();
		}
		else if (match(TokenType::ELSE)) {
			elseBranch = statement();
		}

		return std::make_shared<IfStmt>(condition, thenBranch, elseBranch);
	}

	StmtPtr whileStatement() {
		consume(TokenType::LPAREN, "Expect '(' after 'while'");
		ExprPtr condition = expression();
		consume(TokenType::RPAREN, "Expect ')' after while condition");
		StmtPtr body = statement();
		return std::make_shared<WhileStmt>(condition, body);
	}

	StmtPtr parallelStatement() {
		consume(TokenType::LPAREN, "Expect '(' after 'parallel'");

		StmtPtr initializer;
		if (match(TokenType::SEMICOLON)) {
			initializer = nullptr;
		}
		else if (match(TokenType::VAR)) {
			Token name = consume(TokenType::IDENTIFIER, "Expect variable name");
			ExprPtr initExpr = nullptr;
			if (match(TokenType::EQUAL)) {
				initExpr = expression();
			}
			initializer = std::make_shared<VarStmt>(name.lexeme, initExpr);
			consume(TokenType::SEMICOLON, "Expect ';' after loop initializer");
		}
		else {
			initializer = std::make_shared<ExprStmt>(expression());
			consume(TokenType::SEMICOLON, "Expect ';' after loop initializer");
		}

		ExprPtr condition = nullptr;
		if (!check(TokenType::SEMICOLON)) {
			condition = expression();
		}
		consume(TokenType::SEMICOLON, "Expect ';' after loop condition");

		ExprPtr increment = nullptr;
		if (!check(TokenType::RPAREN)) {
			increment = expression();
		}
		consume(TokenType::RPAREN, "Expect ')' after clauses");

		StmtPtr body = statement();

		return std::make_shared<ParallelStmt>(initializer, condition, increment, body);
	}

	StmtPtr forStatement() {
		consume(TokenType::LPAREN, "Expect '(' after 'for'");

		StmtPtr initializer;
		if (match(TokenType::SEMICOLON)) {
			initializer = nullptr;
		}
		else if (match(TokenType::VAR)) {
			Token name = consume(TokenType::IDENTIFIER, "Expect variable name");
			ExprPtr initExpr = nullptr;
			if (match(TokenType::EQUAL)) {
				initExpr = expression();
			}
			initializer = std::make_shared<VarStmt>(name.lexeme, initExpr);
			consume(TokenType::SEMICOLON, "Expect ';' after loop initializer");
		}
		else {
			initializer = std::make_shared<ExprStmt>(expression());
			consume(TokenType::SEMICOLON, "Expect ';' after loop initializer");
		}

		ExprPtr condition = nullptr;
		if (!check(TokenType::SEMICOLON)) {
			condition = expression();
		}
		consume(TokenType::SEMICOLON, "Expect ';' after loop condition");

		ExprPtr increment = nullptr;
		if (!check(TokenType::RPAREN)) {
			increment = expression();
		}
		consume(TokenType::RPAREN, "Expect ')' after for clauses");

		StmtPtr body = statement();

		if (increment != nullptr) {
			std::vector<StmtPtr> bodyStmts;
			if (auto block = std::dynamic_pointer_cast<BlockStmt>(body)) {
				bodyStmts = block->statements;
			}
			else {
				bodyStmts.push_back(body);
			}
			bodyStmts.push_back(std::make_shared<ExprStmt>(increment));
			body = std::make_shared<BlockStmt>(bodyStmts);
		}

		if (condition == nullptr) {
			condition = std::make_shared<LiteralExpr>(Value::Bool(true));
		}
		StmtPtr whileLoop = std::make_shared<WhileStmt>(condition, body);

		if (initializer != nullptr) {
			std::vector<StmtPtr> blockStmts;
			blockStmts.push_back(initializer);
			blockStmts.push_back(whileLoop);
			return std::make_shared<BlockStmt>(blockStmts);
		}

		return whileLoop;
	}


	StmtPtr functionStatement() {
		Token name = consume(TokenType::IDENTIFIER, "Expect function name");
		consume(TokenType::LPAREN, "Expect '(' after function name");

		std::vector<std::string> params;
		if (!check(TokenType::RPAREN)) {
			do {
				params.push_back(consume(TokenType::IDENTIFIER, "Expect parameter name").lexeme);
			} while (match(TokenType::COMMA));
		}

		consume(TokenType::RPAREN, "Expect ')' after parameters");

		consume(TokenType::LBRACE, "Expect '{' before function body");
		std::vector<StmtPtr> body = parseBlockBody();

		return std::make_shared<FunctionStmt>(name.lexeme, params, body);
	}

	StmtPtr returnStatement() {
		ExprPtr value = nullptr;
		if (!check(TokenType::SEMICOLON) && !check(TokenType::NEWLINE) && !check(TokenType::RBRACE)) {
			value = expression();
		}
		if (check(TokenType::SEMICOLON)) advance();
		return std::make_shared<ReturnStmt>(value);
	}

	StmtPtr declaration() {
		try {
			if (match(TokenType::VAR)) {
				Token name = consume(TokenType::IDENTIFIER, "Expect variable name");
				ExprPtr initializer = nullptr;

				if (match(TokenType::EQUAL)) {
					initializer = expression();
				}

				if (check(TokenType::SEMICOLON)) advance();
				return std::make_shared<VarStmt>(name.lexeme, initializer);
			}

			if (match(TokenType::FUNC)) {
				return functionStatement();
			}

			return statement();
		}
		catch (const ParserError& e) {
			while (!isAtEnd()) {
				if (previous().type == TokenType::NEWLINE || previous().type == TokenType::SEMICOLON) break;
				switch (peek().type) {
				case TokenType::FUNC:
				case TokenType::VAR:
				case TokenType::FOR:
				case TokenType::IF:
				case TokenType::WHILE:
				case TokenType::RETURN:
					return nullptr;
				default:
					advance();
				}
			}
			throw;
		}
	}

public:
	Parser(const std::vector<Token>& toks) : tokens(toks) {}

	std::vector<StmtPtr> parse() {
		std::vector<StmtPtr> statements;
		skipNewlines();
		while (!isAtEnd()) {
			StmtPtr stmt = declaration();
			if (stmt) statements.push_back(stmt);
			skipNewlines();
		}
		return statements;
	}
};

#endif