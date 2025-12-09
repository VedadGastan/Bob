#ifndef TOKEN_H
#define TOKEN_H

#include <string>
#include <map>

enum class TokenType {
	NUMBER, STRING, TRUE, FALSE, NIL,
	IDENTIFIER,
	VAR, FUNC, IF, ELSE, WHILE, FOR, RETURN, BREAK, CONTINUE,
	PRINT, IN, AND, OR, NOT,
	PARALLEL, ASYNC, AWAIT,

	PLUS, MINUS, STAR, SLASH, PERCENT,
	STAR_STAR,
	PLUS_PLUS, MINUS_MINUS,
	PLUS_EQUAL, MINUS_EQUAL, STAR_EQUAL, SLASH_EQUAL, PERCENT_EQUAL,

	EQUAL, EQUAL_EQUAL, BANG_EQUAL,
	LESS, LESS_EQUAL, GREATER, GREATER_EQUAL,
	DOT, ARROW,
	LPAREN, RPAREN, LBRACKET, RBRACKET,
	COMMA, COLON, SEMICOLON, NEWLINE,

	END,
	END_OF_FILE, INVALID
};

struct Token {
	TokenType type;
	std::string lexeme;
	int line;
	int column;

	Token(TokenType t, const std::string& lex, int ln, int col) : type(t), lexeme(lex), line(ln), column(col) {
	}
};

static std::map<std::string, TokenType> keywords = {
	{"var", TokenType::VAR},
	{"func", TokenType::FUNC},
	{"if", TokenType::IF},
	{"else", TokenType::ELSE},
	{"while", TokenType::WHILE},
	{"for", TokenType::FOR},
	{"return", TokenType::RETURN},
	{"break", TokenType::BREAK},
	{"continue", TokenType::CONTINUE},
	{"print", TokenType::PRINT},
	{"in", TokenType::IN},
	{"and", TokenType::AND},
	{"or", TokenType::OR},
	{"not", TokenType::NOT},
	{"true", TokenType::TRUE},
	{"false", TokenType::FALSE},
	{"nil", TokenType::NIL},
	{"end", TokenType::END},
	{"parallel", TokenType::PARALLEL},
	{"async", TokenType::ASYNC},
	{"await", TokenType::AWAIT}
};

#endif