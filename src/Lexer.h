#ifndef LEXER_H
#define LEXER_H

#include "Token.h"
#include <vector>
#include <string>
#include <cctype>
#include <stdexcept>
#include <sstream>

class Lexer {
private:
	std::string source;
	std::vector<Token> tokens;
	size_t start = 0;
	size_t current = 0;
	int line = 1;
	int column = 1;

	bool isAtEnd() { return current >= source.length(); }

	char advance() {
		column++;
		return source[current++];
	}

	char peek() {
		if (isAtEnd()) return '\0';
		return source[current];
	}

	char peekNext() {
		if (current + 1 >= source.length()) return '\0';
		return source[current + 1];
	}

	bool match(char expected) {
		if (isAtEnd() || source[current] != expected) return false;
		current++;
		column++;
		return true;
	}

	void addToken(TokenType type) {
		std::string text = source.substr(start, current - start);
		tokens.push_back(Token(type, text, line, column - static_cast<int>(text.length())));
	}

	void scanString(int startColumn) {
		std::stringstream valueBuilder;

		while (peek() != '"' && !isAtEnd()) {
			char c = advance();

			if (c == '\n') {
				line++;
				column = 1;
				valueBuilder << c;
				continue;
			}

			if (c == '\\') {
				if (isAtEnd()) {
					throw std::runtime_error("Unterminated string at line " + std::to_string(line));
				}
				char next = advance();
				switch (next) {
				case '"':  valueBuilder << '"'; break;
				case '\\': valueBuilder << '\\'; break;
				case 'n':  valueBuilder << '\n'; break;
				case 't':  valueBuilder << '\t'; break;
				case 'r':  valueBuilder << '\r'; break;
				default:
					valueBuilder << '\\' << next;
					break;
				}
			}
			else {
				valueBuilder << c;
			}
		}

		if (isAtEnd()) {
			throw std::runtime_error("Unterminated string at line " + std::to_string(line));
		}

		advance();

		std::string value = valueBuilder.str();
		tokens.push_back(Token(TokenType::STRING, value, line, startColumn));
	}

	void scanNumber() {
		while (isdigit(peek())) advance();

		if (peek() == '.' && isdigit(peekNext())) {
			advance();
			while (isdigit(peek())) advance();
		}

		addToken(TokenType::NUMBER);
	}

	void scanIdentifier() {
		while (isalnum(peek()) || peek() == '_') advance();

		std::string text = source.substr(start, current - start);
		TokenType type = TokenType::IDENTIFIER;

		if (keywords.find(text) != keywords.end()) {
			type = keywords[text];
		}

		tokens.push_back(Token(type, text, line, column - static_cast<int>(text.length())));
	}

	void skipComment() {
		while (peek() != '\n' && !isAtEnd()) advance();
	}

public:
	Lexer(const std::string& src) : source(src) {}

	std::vector<Token> scanTokens() {
		while (!isAtEnd()) {
			start = current;
			char c = advance();

			switch (c) {
			case '(': addToken(TokenType::LPAREN); break;
			case ')': addToken(TokenType::RPAREN); break;
			case '{': addToken(TokenType::LBRACE); break;
			case '}': addToken(TokenType::RBRACE); break;
			case '[': addToken(TokenType::LBRACKET); break;
			case ']': addToken(TokenType::RBRACKET); break;
			case ',': addToken(TokenType::COMMA); break;
			case '.': addToken(TokenType::DOT); break;
			case ':': addToken(TokenType::COLON); break;
			case ';': addToken(TokenType::SEMICOLON); break;
			case '%':
				addToken(match('=') ? TokenType::PERCENT_EQUAL : TokenType::PERCENT);
				break;
			case '+':
				if (match('+')) addToken(TokenType::PLUS_PLUS);
				else if (match('=')) addToken(TokenType::PLUS_EQUAL);
				else addToken(TokenType::PLUS);
				break;
			case '*':
				if (match('*')) addToken(TokenType::STAR_STAR);
				else if (match('=')) addToken(TokenType::STAR_EQUAL);
				else addToken(TokenType::STAR);
				break;
			case '-':
				if (match('>')) addToken(TokenType::ARROW);
				else if (match('-')) addToken(TokenType::MINUS_MINUS);
				else if (match('=')) addToken(TokenType::MINUS_EQUAL);
				else addToken(TokenType::MINUS);
				break;
			case '/':
				if (match('/')) skipComment();
				else if (match('=')) addToken(TokenType::SLASH_EQUAL);
				else addToken(TokenType::SLASH);
				break;
			case '=':
				addToken(match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL);
				break;
			case '!':
				addToken(match('=') ? TokenType::BANG_EQUAL : TokenType::NOT);
				break;
			case '<':
				addToken(match('=') ? TokenType::LESS_EQUAL : TokenType::LESS);
				break;
			case '>':
				addToken(match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER);
				break;
			case ' ':
			case '\r':
			case '\t':
				break;
			case '\n':
				addToken(TokenType::NEWLINE);
				line++;
				column = 1;
				break;
			case '"':
				scanString(column - 1);
				break;
			default:
				if (isdigit(c)) {
					scanNumber();
				}
				else if (isalpha(c) || c == '_') {
					scanIdentifier();
				}
				break;
			}
		}

		tokens.push_back(Token(TokenType::END_OF_FILE, "", line, column));
		return tokens;
	}
};

#endif