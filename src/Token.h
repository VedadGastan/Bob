#ifndef TOKEN_H
#define TOKEN_H

#include <string>
#include <map>

enum class TokenType {
	NUMBER, STRING, TRUE, FALSE, NIL,
	IDENTIFIER,
	LET, FUNC, IF, ELSE, WHILE, FOR, RETURN, BREAK, CONTINUE,
	PRINT, IN, AND, OR, NOT,

	PLUS, MINUS, STAR, SLASH, PERCENT,
	STAR_STAR,

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
	{"let", TokenType::LET}, 
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
	{"end", TokenType::END}
};

#endif