#include "src/Lexer.h"
#include "src/Parser.h"
#include "src/Interpreter.h"
#include "src/AST.h"
#include <iostream>
#include <fstream>
#include <sstream>

void run(const std::string& source, Interpreter& interpreter) {
	try {
		Lexer lexer(source);
		std::vector<Token> tokens = lexer.scanTokens();

		Parser parser(tokens);
		std::vector<StmtPtr> statements = parser.parse();

		interpreter.execute(statements);
	}
	catch (const ParserError& e) {
		std::cerr << "[line " << e.token.line << ":" << e.token.column << "] ParseError: " << e.what() << std::endl;
	}
	catch (const RuntimeError& e) {
		std::cerr << "[line " << e.line << "] RuntimeError: " << e.what() << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
}

void runFile(const std::string& filename) {
	std::ifstream file(filename);
	if (!file.is_open()) {
		std::cerr << "Error: Could not open file " << filename << std::endl;
		return;
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string source = buffer.str();

	Interpreter interpreter;
	run(source, interpreter);
}

void runRepl() {
	std::cout << "Bob Language REPL v1.0 (Python/Ruby Syntax)" << std::endl;
	std::cout << "Type 'exit' to quit" << std::endl;
	std::cout << std::endl;

	Interpreter interpreter;
	std::string line;

	while (true) {
		std::cout << ">>> ";
		std::getline(std::cin, line);

		if (line == "exit" || line == "quit") {
			break;
		}
		if (line.empty()) continue;

		run(line, interpreter);
	}
}

int main(int argc, char* argv[]) {
	if (argc > 1) {
		runFile(argv[1]);
	}
	else {
		runRepl();
	}
	return 0;
}