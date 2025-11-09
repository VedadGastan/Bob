#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "AST.h"
#include "Value.h"
#include "Environment.h"
#include "Callable.h"
#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <memory>
#include <functional>
#include <cmath>

class RuntimeError : public std::runtime_error {
public:
	int line;
	int column;
	RuntimeError(const std::string& message, int ln = 0, int col = 0)
		: std::runtime_error(message), line(ln), column(col) {}
};

class ReturnException : public std::exception {
public:
	Value value;
	ReturnException(const Value& v) : value(v) {}
};


class Interpreter {
private:
	std::shared_ptr<Environment> globals;
	std::shared_ptr<Environment> environment;
	std::map<std::string, std::function<Value(const std::vector<Value>&)>> builtins;

	// Helper for builtins
	void defineBuiltin(const std::string& name,
		std::function<Value(const std::vector<Value>&)> func) {
		builtins[name] = func;
	}

	void checkNumber(const std::string& func, const Value& arg) {
		if (arg.type != ValueType::NUMBER)
			throw RuntimeError(func + " expects a number");
	}

	void initializeBuiltins() {
		defineBuiltin("print", [](const std::vector<Value>& args) -> Value {
			for (size_t i = 0; i < args.size(); i++) {
				if (i > 0) std::cout << " ";
				std::cout << args[i].toString();
			}
			std::cout << std::endl;
			return Value::Nil();
			});

		defineBuiltin("len", [](const std::vector<Value>& args) -> Value {
			if (args.size() != 1) {
				throw RuntimeError("len() expects exactly 1 argument");
			}
			const auto& arg = args[0];
			if (arg.type == ValueType::ARRAY) {
				return Value::Number(static_cast<double>(arg.arrayValue.size()));
			}
			else if (arg.type == ValueType::STRING) {
				return Value::Number(static_cast<double>(arg.stringValue.size()));
			}
			throw RuntimeError("len() expects array or string");
			});

		defineBuiltin("input", [](const std::vector<Value>& args) -> Value {
			if (args.size() > 1) {
				throw RuntimeError("input() expects 0 or 1 arguments");
			}
			if (!args.empty()) {
				std::cout << args[0].toString();
			}
			std::string input;
			std::getline(std::cin, input);
			return Value::String(input);
			});

		defineBuiltin("sqrt", [&](const std::vector<Value>& args) -> Value {
			if (args.size() != 1) throw RuntimeError("sqrt() expects 1 argument");
			checkNumber("sqrt()", args[0]);
			return Value::Number(std::sqrt(args[0].numberValue));
			});

		defineBuiltin("pow", [&](const std::vector<Value>& args) -> Value {
			if (args.size() != 2) throw RuntimeError("pow() expects 2 arguments");
			checkNumber("pow()", args[0]);
			checkNumber("pow()", args[1]);
			return Value::Number(std::pow(args[0].numberValue, args[1].numberValue));
			});

		defineBuiltin("abs", [&](const std::vector<Value>& args) -> Value {
			if (args.size() != 1) throw RuntimeError("abs() expects 1 argument");
			checkNumber("abs()", args[0]);
			return Value::Number(std::abs(args[0].numberValue));
			});

		defineBuiltin("sin", [&](const std::vector<Value>& args) -> Value {
			if (args.size() != 1) throw RuntimeError("sin() expects 1 argument");
			checkNumber("sin()", args[0]);
			return Value::Number(std::sin(args[0].numberValue));
			});

		defineBuiltin("cos", [&](const std::vector<Value>& args) -> Value {
			if (args.size() != 1) throw RuntimeError("cos() expects 1 argument");
			checkNumber("cos()", args[0]);
			return Value::Number(std::cos(args[0].numberValue));
			});

		defineBuiltin("tan", [&](const std::vector<Value>& args) -> Value {
			if (args.size() != 1) throw RuntimeError("tan() expects 1 argument");
			checkNumber("tan()", args[0]);
			return Value::Number(std::tan(args[0].numberValue));
			});

		defineBuiltin("floor", [&](const std::vector<Value>& args) -> Value {
			if (args.size() != 1) throw RuntimeError("floor() expects 1 argument");
			checkNumber("floor()", args[0]);
			return Value::Number(std::floor(args[0].numberValue));
			});

		defineBuiltin("ceil", [&](const std::vector<Value>& args) -> Value {
			if (args.size() != 1) throw RuntimeError("ceil() expects 1 argument");
			checkNumber("ceil()", args[0]);
			return Value::Number(std::ceil(args[0].numberValue));
			});
	}

	Value evaluateExpr(ExprPtr expr) {
		if (!expr) return Value::Nil();

		if (auto lit = std::dynamic_pointer_cast<LiteralExpr>(expr)) {
			return lit->value;
		}

		if (auto var = std::dynamic_pointer_cast<VariableExpr>(expr)) {
			try {
				return environment->get(var->name);
			}
			catch (const std::runtime_error& e) {
				throw RuntimeError(e.what());
			}
		}

		if (auto assign = std::dynamic_pointer_cast<AssignmentExpr>(expr)) {
			Value val = evaluateExpr(assign->value);
			try {
				environment->assign(assign->name, val);
			}
			catch (const std::runtime_error& e) {
				throw RuntimeError(e.what());
			}
			return val;
		}

		if (auto bin = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
			if (bin->op.type == TokenType::AND) {
				Value left = evaluateExpr(bin->left);
				if (!left.isTruthy()) return Value::Bool(false);
				return Value::Bool(evaluateExpr(bin->right).isTruthy());
			}
			if (bin->op.type == TokenType::OR) {
				Value left = evaluateExpr(bin->left);
				if (left.isTruthy()) return Value::Bool(true);
				return Value::Bool(evaluateExpr(bin->right).isTruthy());
			}

			Value left = evaluateExpr(bin->left);
			Value right = evaluateExpr(bin->right);

			try {
				return left.applyBinaryOp(bin->op.type, right);
			}
			catch (const std::runtime_error& e) {
				throw RuntimeError(e.what(), bin->op.line, bin->op.column);
			}
		}

		if (auto un = std::dynamic_pointer_cast<UnaryExpr>(expr)) {
			Value right = evaluateExpr(un->right);
			try {
				return right.applyUnaryOp(un->op.type);
			}
			catch (const std::runtime_error& e) {
				throw RuntimeError(e.what(), un->op.line, un->op.column);
			}
		}

		if (auto arr = std::dynamic_pointer_cast<ArrayExpr>(expr)) {
			std::vector<Value> elements;
			elements.reserve(arr->elements.size());
			for (const auto& e : arr->elements) {
				elements.push_back(evaluateExpr(e));
			}
			return Value::Array(elements);
		}

		if (auto index = std::dynamic_pointer_cast<IndexExpr>(expr)) {
			Value arrayVal = evaluateExpr(index->array);
			Value idxVal = evaluateExpr(index->index);

			if (arrayVal.type != ValueType::ARRAY) {
				throw RuntimeError("Cannot index non-array type");
			}
			if (idxVal.type != ValueType::NUMBER) {
				throw RuntimeError("Array index must be a number");
			}

			int i = static_cast<int>(idxVal.numberValue);
			int size = static_cast<int>(arrayVal.arrayValue.size());

			if (i < 0) {
				i = size + i;
			}

			if (i < 0 || i >= size) {
				throw RuntimeError("Array index out of bounds");
			}
			return arrayVal.arrayValue[i];
		}

		if (auto call = std::dynamic_pointer_cast<CallExpr>(expr)) {

			// 1. Evaluate arguments first
			std::vector<Value> args;
			args.reserve(call->arguments.size());
			for (const auto& arg : call->arguments) {
				args.push_back(evaluateExpr(arg));
			}

			// 2. Evaluate the callee (which might be a var or a builtin name)
			if (auto var = std::dynamic_pointer_cast<VariableExpr>(call->callee)) {
				auto it = builtins.find(var->name);
				if (it != builtins.end()) {
					try {
						return it->second(args);
					}
					catch (const std::runtime_error& e) {
						throw RuntimeError(e.what());
					}
				}
			}

			Value calleeValue = evaluateExpr(call->callee);

			// 3. Check if it's a callable function
			if (calleeValue.type != ValueType::FUNCTION) {
				throw RuntimeError("Can only call functions");
			}

			auto& func = calleeValue.funcValue;
			if (func->params.size() != args.size()) {
				throw RuntimeError("Expects " + std::to_string(func->params.size()) +
					" arguments, but got " + std::to_string(args.size()));
			}

			// 4. Execute the call
			// Create a new environment whose parent is the function's closure
			auto callEnv = std::make_shared<Environment>(func->closure);
			for (size_t i = 0; i < func->params.size(); ++i) {
				callEnv->define(func->params[i], args[i]);
			}

			Value returnValue = Value::Nil();
			try {
				executeBlock(func->body, callEnv);
			}
			catch (const ReturnException& ret) {
				returnValue = ret.value;
			}

			return returnValue;
		}

		if (auto group = std::dynamic_pointer_cast<GroupingExpr>(expr)) {
			return evaluateExpr(group->expression);
		}

		return Value::Nil();
	}

	void executeBlock(const std::vector<StmtPtr>& statements, std::shared_ptr<Environment> blockEnv) {
		std::shared_ptr<Environment> previous = this->environment;
		this->environment = blockEnv;

		try {
			for (const auto& s : statements) {
				executeStmt(s);
			}
		}
		catch (...) {
			this->environment = previous;
			throw;
		}

		this->environment = previous;
	}


	void executeStmt(StmtPtr stmt) {
		if (!stmt) return;

		if (auto exprStmt = std::dynamic_pointer_cast<ExprStmt>(stmt)) {
			evaluateExpr(exprStmt->expression);
			return;
		}

		if (auto printStmt = std::dynamic_pointer_cast<PrintStmt>(stmt)) {
			Value value = evaluateExpr(printStmt->expression);
			std::cout << value.toString() << std::endl;
			return;
		}

		if (auto varStmt = std::dynamic_pointer_cast<VarStmt>(stmt)) {
			Value value = Value::Nil();
			if (varStmt->initializer) {
				value = evaluateExpr(varStmt->initializer);
			}

			// Allow redeclaration in the same scope
			// Simply define/redefine the variable
			environment->define(varStmt->name, value);
			return;
		}

		if (auto blockStmt = std::dynamic_pointer_cast<BlockStmt>(stmt)) {
			// Create a new scope nested inside the current one
			auto blockEnv = std::make_shared<Environment>(environment);
			executeBlock(blockStmt->statements, blockEnv);
			return;
		}

		if (auto ifStmt = std::dynamic_pointer_cast<IfStmt>(stmt)) {
			Value cond = evaluateExpr(ifStmt->condition);
			if (cond.isTruthy()) {
				executeStmt(ifStmt->thenBranch);
			}
			else if (ifStmt->elseBranch) {
				executeStmt(ifStmt->elseBranch);
			}
			return;
		}

		if (auto whileStmt = std::dynamic_pointer_cast<WhileStmt>(stmt)) {
			while (evaluateExpr(whileStmt->condition).isTruthy()) {
				executeStmt(whileStmt->body);
			}
			return;
		}

		if (auto funcStmt = std::dynamic_pointer_cast<FunctionStmt>(stmt)) {
			// Create a function object, capturing the *current* environment
			auto func = std::make_shared<Function>(funcStmt->params, funcStmt->body, environment);
			// Define the function *as a variable* in the current scope
			environment->define(funcStmt->name, Value::Function(func));
			return;
		}

		if (auto retStmt = std::dynamic_pointer_cast<ReturnStmt>(stmt)) {
			Value value = Value::Nil();
			if (retStmt->value) {
				value = evaluateExpr(retStmt->value);
			}
			throw ReturnException(value);
		}
	}

public:
	Interpreter() {
		globals = std::make_shared<Environment>();
		environment = globals;
		initializeBuiltins();
	}

	void execute(const std::vector<StmtPtr>& statements) {
		for (const auto& stmt : statements) {
			if (stmt) executeStmt(stmt);
		}
	}

	void clear() {
		globals = std::make_shared<Environment>();
		environment = globals;
		initializeBuiltins();
	}
};

#endif