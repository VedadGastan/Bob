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
#include <thread>
#include <mutex>
#include <future>

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
	static std::map<std::string, double> atomicVars;
	static std::mutex atomicMutex;
	int numThreads;

	// Helper for builtins
	void defineBuiltin(const std::string& name,
		std::function<Value(const std::vector<Value>&)> func) {
		builtins[name] = func;
	}

	void checkNumberOperand(const Token& op, const Value& operand) {
		if (operand.type != ValueType::NUMBER) {
			throw RuntimeError("Operand must be a number", op.line, op.column);
		}
	}

	void checkNumberOperands(const Token& op, const Value& left, const Value& right) {
		if (left.type != ValueType::NUMBER || right.type != ValueType::NUMBER) {
			throw RuntimeError("Operands must be numbers", op.line, op.column);
		}
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

		defineBuiltin("sqrt", [](const std::vector<Value>& args) -> Value {
			if (args.size() != 1) throw RuntimeError("sqrt() expects 1 argument");
			if (args[0].type != ValueType::NUMBER) throw RuntimeError("sqrt() expects a number");
			return Value::Number(std::sqrt(args[0].numberValue));
			});

		defineBuiltin("pow", [](const std::vector<Value>& args) -> Value {
			if (args.size() != 2) throw RuntimeError("pow() expects 2 arguments");
			if (args[0].type != ValueType::NUMBER) throw RuntimeError("pow() expects numbers");
			if (args[1].type != ValueType::NUMBER) throw RuntimeError("pow() expects numbers");
			return Value::Number(std::pow(args[0].numberValue, args[1].numberValue));
			});

		defineBuiltin("abs", [](const std::vector<Value>& args) -> Value {
			if (args.size() != 1) throw RuntimeError("abs() expects 1 argument");
			if (args[0].type != ValueType::NUMBER) throw RuntimeError("abs() expects a number");
			return Value::Number(std::abs(args[0].numberValue));
			});

		defineBuiltin("sin", [](const std::vector<Value>& args) -> Value {
			if (args.size() != 1) throw RuntimeError("sin() expects 1 argument");
			if (args[0].type != ValueType::NUMBER) throw RuntimeError("sin() expects a number");
			return Value::Number(std::sin(args[0].numberValue));
			});

		defineBuiltin("cos", [](const std::vector<Value>& args) -> Value {
			if (args.size() != 1) throw RuntimeError("cos() expects 1 argument");
			if (args[0].type != ValueType::NUMBER) throw RuntimeError("cos() expects a number");
			return Value::Number(std::cos(args[0].numberValue));
			});

		defineBuiltin("tan", [](const std::vector<Value>& args) -> Value {
			if (args.size() != 1) throw RuntimeError("tan() expects 1 argument");
			if (args[0].type != ValueType::NUMBER) throw RuntimeError("tan() expects a number");
			return Value::Number(std::tan(args[0].numberValue));
			});

		defineBuiltin("floor", [](const std::vector<Value>& args) -> Value {
			if (args.size() != 1) throw RuntimeError("floor() expects 1 argument");
			if (args[0].type != ValueType::NUMBER) throw RuntimeError("floor() expects a number");
			return Value::Number(std::floor(args[0].numberValue));
			});

		defineBuiltin("ceil", [](const std::vector<Value>& args) -> Value {
			if (args.size() != 1) throw RuntimeError("ceil() expects 1 argument");
			if (args[0].type != ValueType::NUMBER) throw RuntimeError("ceil() expects a number");
			return Value::Number(std::ceil(args[0].numberValue));
			});

		defineBuiltin("thread_id", [this](const std::vector<Value>& args) -> Value {
			if (args.size() != 0) {
				throw RuntimeError("thread_id() expects 0 arguments");
			}
			std::stringstream ss;
			ss << std::this_thread::get_id();
			return Value::String(ss.str());
			});

		defineBuiltin("num_threads", [this](const std::vector<Value>& args) -> Value {
			if (args.size() != 0) {
				throw RuntimeError("num_threads() expects 0 arguments");
			}
			return Value::Number(numThreads);
			});

		defineBuiltin("sleep", [](const std::vector<Value>& args) -> Value {
			if (args.size() != 1) {
				throw RuntimeError("sleep() expects 1 argument");
			}
			if (args[0].type != ValueType::NUMBER) {
				throw RuntimeError("sleep() expects a number");
			}
			int ms = static_cast<int>(args[0].numberValue);
			std::this_thread::sleep_for(std::chrono::milliseconds(ms));
			return Value::Nil();
			});

		defineBuiltin("atomic_add", [this](const std::vector<Value>& args) -> Value {
			if (args.size() != 2) {
				throw RuntimeError("atomic_add() expects 2 arguments (var_name, value)");
			}
			if (args[0].type != ValueType::STRING) {
				throw RuntimeError("atomic_add() first argument must be variable name as string");
			}
			if (args[1].type != ValueType::NUMBER) {
				throw RuntimeError("atomic_add() second argument must be a number");
			}

			std::string varName = args[0].stringValue;
			double addValue = args[1].numberValue;

			std::lock_guard<std::mutex> lock(atomicMutex);

			auto it = atomicVars.find(varName);
			if (it == atomicVars.end()) {
				try {
					Value current = globals->get(varName);
					if (current.type != ValueType::NUMBER) {
						throw RuntimeError("atomic_add() can only add to numbers");
					}
					atomicVars[varName] = current.numberValue + addValue;
				}
				catch (const std::runtime_error&) {
					atomicVars[varName] = addValue;
				}
			}
			else {
				atomicVars[varName] += addValue;
			}

			globals->assign(varName, Value::Number(atomicVars[varName]));
			return Value::Nil();
			});

		defineBuiltin("range", [](const std::vector<Value>& args) -> Value {
			if (args.size() != 2) {
				throw RuntimeError("range() expects 2 arguments");
			}
			if (args[0].type != ValueType::NUMBER || args[1].type != ValueType::NUMBER) {
				throw RuntimeError("range() expects number arguments");
			}
			std::vector<Value> result;
			int start = static_cast<int>(args[0].numberValue);
			int end = static_cast<int>(args[1].numberValue);
			for (int i = start; i < end; i++) {
				result.push_back(Value::Number(i));
			}
			return Value::Array(result);
			});

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

		defineBuiltin("sqrt", [](const std::vector<Value>& args) -> Value {
			if (args.size() != 1) throw RuntimeError("sqrt() expects 1 argument");
			if (args[0].type != ValueType::NUMBER) throw RuntimeError("sqrt() expects a number");
			return Value::Number(std::sqrt(args[0].numberValue));
			});

		defineBuiltin("pow", [](const std::vector<Value>& args) -> Value {
			if (args.size() != 2) throw RuntimeError("pow() expects 2 arguments");
			if (args[0].type != ValueType::NUMBER) throw RuntimeError("pow() expects numbers");
			if (args[1].type != ValueType::NUMBER) throw RuntimeError("pow() expects numbers");
			return Value::Number(std::pow(args[0].numberValue, args[1].numberValue));
			});

		defineBuiltin("abs", [](const std::vector<Value>& args) -> Value {
			if (args.size() != 1) throw RuntimeError("abs() expects 1 argument");
			if (args[0].type != ValueType::NUMBER) throw RuntimeError("abs() expects a number");
			return Value::Number(std::abs(args[0].numberValue));
			});

		defineBuiltin("sin", [](const std::vector<Value>& args) -> Value {
			if (args.size() != 1) throw RuntimeError("sin() expects 1 argument");
			if (args[0].type != ValueType::NUMBER) throw RuntimeError("sin() expects a number");
			return Value::Number(std::sin(args[0].numberValue));
			});

		defineBuiltin("cos", [](const std::vector<Value>& args) -> Value {
			if (args.size() != 1) throw RuntimeError("cos() expects 1 argument");
			if (args[0].type != ValueType::NUMBER) throw RuntimeError("cos() expects a number");
			return Value::Number(std::cos(args[0].numberValue));
			});

		defineBuiltin("tan", [](const std::vector<Value>& args) -> Value {
			if (args.size() != 1) throw RuntimeError("tan() expects 1 argument");
			if (args[0].type != ValueType::NUMBER) throw RuntimeError("tan() expects a number");
			return Value::Number(std::tan(args[0].numberValue));
			});

		defineBuiltin("floor", [](const std::vector<Value>& args) -> Value {
			if (args.size() != 1) throw RuntimeError("floor() expects 1 argument");
			if (args[0].type != ValueType::NUMBER) throw RuntimeError("floor() expects a number");
			return Value::Number(std::floor(args[0].numberValue));
			});

		defineBuiltin("ceil", [](const std::vector<Value>& args) -> Value {
			if (args.size() != 1) throw RuntimeError("ceil() expects 1 argument");
			if (args[0].type != ValueType::NUMBER) throw RuntimeError("ceil() expects a number");
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
				throw RuntimeError("Variable '" + var->name + "' is not defined");
			}
		}

		if (auto assign = std::dynamic_pointer_cast<AssignmentExpr>(expr)) {
			Value val = evaluateExpr(assign->value);
			try {
				environment->assign(assign->name, val);
			}
			catch (const std::runtime_error& e) {
				throw RuntimeError("Cannot assign to undefined variable '" + assign->name + "'");
			}
			return val;
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

		if (auto compound = std::dynamic_pointer_cast<CompoundAssignExpr>(expr)) {
			Value currentVal;
			try {
				currentVal = environment->get(compound->name);
			}
			catch (const std::runtime_error& e) {
				throw RuntimeError("Cannot compound assign to undefined variable '" + compound->name + "'");
			}

			Value rightVal = evaluateExpr(compound->value);
			Value newVal;

			try {
				TokenType baseOp;
				switch (compound->op.type) {
				case TokenType::PLUS_EQUAL: baseOp = TokenType::PLUS; break;
				case TokenType::MINUS_EQUAL: baseOp = TokenType::MINUS; break;
				case TokenType::STAR_EQUAL: baseOp = TokenType::STAR; break;
				case TokenType::SLASH_EQUAL: baseOp = TokenType::SLASH; break;
				case TokenType::PERCENT_EQUAL: baseOp = TokenType::PERCENT; break;
				default: throw RuntimeError("Unknown compound operator");
				}
				newVal = currentVal.applyBinaryOp(baseOp, rightVal);
			}
			catch (const std::runtime_error& e) {
				throw RuntimeError(std::string(e.what()), compound->op.line, compound->op.column);
			}

			environment->assign(compound->name, newVal);
			return newVal;
		}

		if (auto postfix = std::dynamic_pointer_cast<PostfixExpr>(expr)) {
			Value currentVal;
			try {
				currentVal = environment->get(postfix->name);
			}
			catch (const std::runtime_error& e) {
				throw RuntimeError("Cannot apply postfix to undefined variable '" + postfix->name + "'");
			}

			if (currentVal.type != ValueType::NUMBER) {
				throw RuntimeError("Postfix operators require a number");
			}

			Value oldVal = currentVal;
			Value newVal;

			if (postfix->op.type == TokenType::PLUS_PLUS) {
				newVal = Value::Number(currentVal.numberValue + 1);
			}
			else if (postfix->op.type == TokenType::MINUS_MINUS) {
				newVal = Value::Number(currentVal.numberValue - 1);
			}
			else {
				throw RuntimeError("Unknown postfix operator");
			}

			environment->assign(postfix->name, newVal);
			return oldVal;
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
				throw RuntimeError("Cannot index non-array value, got " + arrayVal.toString());
			}
			if (idxVal.type != ValueType::NUMBER) {
				throw RuntimeError("Array index must be a number, got " + idxVal.toString());
			}

			int i = static_cast<int>(idxVal.numberValue);
			int size = static_cast<int>(arrayVal.arrayValue.size());

			if (i < 0) {
				i = size + i;
			}

			if (i < 0 || i >= size) {
				throw RuntimeError("Array index " + std::to_string(i) + " out of bounds for array of size " + std::to_string(size));
			}
			return arrayVal.arrayValue[i];
		}

		if (auto indexAssign = std::dynamic_pointer_cast<IndexAssignExpr>(expr)) {
			Value arrayVal = evaluateExpr(indexAssign->array);
			Value idxVal = evaluateExpr(indexAssign->index);
			Value newVal = evaluateExpr(indexAssign->value);

			if (arrayVal.type != ValueType::ARRAY) {
				throw RuntimeError("Cannot index assign to non-array value");
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
				throw RuntimeError("Array index " + std::to_string(i) + " out of bounds for array of size " + std::to_string(size));
			}

			arrayVal.arrayValue[i] = newVal;

			if (auto var = std::dynamic_pointer_cast<VariableExpr>(indexAssign->array)) {
				try {
					environment->assign(var->name, arrayVal);
				}
				catch (const std::runtime_error& e) {
					throw RuntimeError("Cannot assign to undefined variable '" + var->name + "'");
				}
			}

			return newVal;
		}

		if (auto call = std::dynamic_pointer_cast<CallExpr>(expr)) {
			std::vector<Value> args;
			args.reserve(call->arguments.size());
			for (const auto& arg : call->arguments) {
				args.push_back(evaluateExpr(arg));
			}

			if (auto var = std::dynamic_pointer_cast<VariableExpr>(call->callee)) {
				auto it = builtins.find(var->name);
				if (it != builtins.end()) {
					try {
						return it->second(args);
					}
					catch (const RuntimeError& e) {
						throw;
					}
					catch (const std::runtime_error& e) {
						throw RuntimeError(std::string(e.what()));
					}
				}
			}

			Value calleeValue = evaluateExpr(call->callee);

			if (calleeValue.type != ValueType::FUNCTION) {
				throw RuntimeError("Cannot call non-function value");
			}

			auto& func = calleeValue.funcValue;
			if (func->params.size() != args.size()) {
				throw RuntimeError("Function expects " + std::to_string(func->params.size()) +
					" argument(s), but got " + std::to_string(args.size()));
			}

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

			environment->define(varStmt->name, value);
			return;
		}

		if (auto blockStmt = std::dynamic_pointer_cast<BlockStmt>(stmt)) {
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

		if (auto parallelStmt = std::dynamic_pointer_cast<ParallelForStmt>(stmt)) {
			Value startVal = evaluateExpr(parallelStmt->start);
			Value endVal = evaluateExpr(parallelStmt->end);

			if (startVal.type != ValueType::NUMBER || endVal.type != ValueType::NUMBER) {
				throw RuntimeError("Parallel range bounds must be numbers");
			}

			int start = static_cast<int>(startVal.numberValue);
			int end = static_cast<int>(endVal.numberValue);
			int range = end - start;

			if (range <= 0) return;

			if (range < 50) {
				for (int i = start; i < end; i++) {
					auto loopEnv = std::make_shared<Environment>(environment);
					loopEnv->define(parallelStmt->varName, Value::Number(i));
					executeBlock({ parallelStmt->body }, loopEnv);
				}
				return;
			}

			int actualThreads = std::min(numThreads, 12);
			int chunkSize = (range + actualThreads - 1) / actualThreads;

			std::vector<std::thread> threads;
			std::atomic<bool> hasError{ false };
			std::string errorMsg;
			std::mutex errorMutex;

			auto sharedGlobals = globals;
			auto sharedEnv = environment;
			auto sharedBuiltins = builtins;
			auto bodyStmt = parallelStmt->body;
			auto varName = parallelStmt->varName;

			for (int t = 0; t < actualThreads; t++) {
				int threadStart = start + t * chunkSize;
				int threadEnd = std::min(threadStart + chunkSize, end);

				if (threadStart >= threadEnd) continue;

				threads.emplace_back([this, sharedGlobals, sharedEnv, sharedBuiltins, bodyStmt, varName,
					threadStart, threadEnd, &hasError, &errorMsg, &errorMutex]() {
						try {
							Interpreter threadInterp;
							threadInterp.globals = sharedGlobals;
							threadInterp.environment = sharedEnv;
							threadInterp.builtins = sharedBuiltins;
							threadInterp.numThreads = this->numThreads;

							for (int i = threadStart; i < threadEnd; i++) {
								auto loopEnv = std::make_shared<Environment>(sharedEnv);
								loopEnv->define(varName, Value::Number(i));

								if (auto block = std::dynamic_pointer_cast<BlockStmt>(bodyStmt)) {
									threadInterp.executeBlock(block->statements, loopEnv);
								}
								else {
									auto saved = threadInterp.environment;
									threadInterp.environment = loopEnv;
									threadInterp.executeStmt(bodyStmt);
									threadInterp.environment = saved;
								}
							}
						}
						catch (const std::exception& e) {
							std::lock_guard<std::mutex> lock(errorMutex);
							if (!hasError) {
								hasError = true;
								errorMsg = e.what();
							}
						}
					});
			}

			for (auto& thread : threads) {
				if (thread.joinable()) {
					thread.join();
				}
			}

			if (hasError) {
				throw RuntimeError(errorMsg);
			}

			return;
		}

		if (auto funcStmt = std::dynamic_pointer_cast<FunctionStmt>(stmt)) {
			auto func = std::make_shared<Function>(funcStmt->params, funcStmt->body, environment);
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
	Interpreter() : numThreads(std::thread::hardware_concurrency()) {
		if (numThreads == 0) numThreads = 4;
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

std::map<std::string, double> Interpreter::atomicVars;
std::mutex Interpreter::atomicMutex;
#endif