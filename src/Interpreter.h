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
#include <atomic>
#include <chrono>
#include <random>

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
				return Value::Number(static_cast<double>(arg.arrayValue->size()));
			}
			else if (arg.type == ValueType::STRING) {
				return Value::Number(static_cast<double>(arg.stringValue->size()));
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

		defineBuiltin("round", [](const std::vector<Value>& args) -> Value {
			if (args.size() != 1) throw RuntimeError("round() expects 1 argument");
			if (args[0].type != ValueType::NUMBER) throw RuntimeError("round() expects a number");
			return Value::Number(std::round(args[0].numberValue));
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

		defineBuiltin("log", [](const std::vector<Value>& args) -> Value {
			if (args.size() != 1) throw RuntimeError("log() expects 1 argument");
			if (args[0].type != ValueType::NUMBER) throw RuntimeError("log() expects a number");
			return Value::Number(std::log(args[0].numberValue));
			});

		defineBuiltin("random", [](const std::vector<Value>&) -> Value {
			static std::random_device rd;
			static std::mt19937 gen(rd());
			static std::uniform_real_distribution<> dis(0.0, 1.0);
			return Value::Number(dis(gen));
			});

		defineBuiltin("time", [](const std::vector<Value>&) -> Value {
			auto now = std::chrono::system_clock::now();
			auto duration = now.time_since_epoch();
			double millis = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
			return Value::Number(millis);
			});

		defineBuiltin("thread_id", [this](const std::vector<Value>&) -> Value {
			std::stringstream ss;
			ss << std::this_thread::get_id();
			return Value::String(ss.str());
			});

		defineBuiltin("num_threads", [this](const std::vector<Value>&) -> Value {
			return Value::Number(static_cast<double>(numThreads));
			});

		defineBuiltin("sleep", [](const std::vector<Value>& args) -> Value {
			if (args.size() != 1 || args[0].type != ValueType::NUMBER) {
				throw RuntimeError("sleep() expects 1 number argument (ms)");
			}
			int ms = static_cast<int>(args[0].numberValue);
			std::this_thread::sleep_for(std::chrono::milliseconds(ms));
			return Value::Nil();
			});

		defineBuiltin("atomic_store", [this](const std::vector<Value>& args) -> Value {
			if (args.size() != 2) throw RuntimeError("atomic_store(var_name, val)");
			std::string varName = *args[0].stringValue;
			double val = args[1].numberValue;
			std::lock_guard<std::mutex> lock(atomicMutex);
			atomicVars[varName] = val;
			globals->define(varName, Value::Number(val));
			return Value::Nil();
			});

		defineBuiltin("atomic_load", [this](const std::vector<Value>& args) -> Value {
			if (args.size() != 1) throw RuntimeError("atomic_load(var_name)");
			std::string varName = *args[0].stringValue;
			std::lock_guard<std::mutex> lock(atomicMutex);
			if (atomicVars.find(varName) == atomicVars.end()) return Value::Number(0);
			return Value::Number(atomicVars[varName]);
			});

		defineBuiltin("atomic_add", [this](const std::vector<Value>& args) -> Value {
			if (args.size() != 2) throw RuntimeError("atomic_add(var_name, val)");
			std::string varName = *args[0].stringValue;
			double val = args[1].numberValue;
			std::lock_guard<std::mutex> lock(atomicMutex);
			atomicVars[varName] += val;
			globals->assign(varName, Value::Number(atomicVars[varName]));
			return Value::Number(atomicVars[varName]);
			});

		defineBuiltin("atomic_sub", [this](const std::vector<Value>& args) -> Value {
			if (args.size() != 2) throw RuntimeError("atomic_sub(var_name, val)");
			std::string varName = *args[0].stringValue;
			double val = args[1].numberValue;
			std::lock_guard<std::mutex> lock(atomicMutex);
			atomicVars[varName] -= val;
			globals->assign(varName, Value::Number(atomicVars[varName]));
			return Value::Number(atomicVars[varName]);
			});

		defineBuiltin("atomic_inc", [this](const std::vector<Value>& args) -> Value {
			if (args.size() != 1) throw RuntimeError("atomic_inc(var_name)");
			std::string varName = *args[0].stringValue;
			std::lock_guard<std::mutex> lock(atomicMutex);
			atomicVars[varName] += 1.0;
			globals->assign(varName, Value::Number(atomicVars[varName]));
			return Value::Number(atomicVars[varName]);
			});

		defineBuiltin("atomic_dec", [this](const std::vector<Value>& args) -> Value {
			if (args.size() != 1) throw RuntimeError("atomic_dec(var_name)");
			std::string varName = *args[0].stringValue;
			std::lock_guard<std::mutex> lock(atomicMutex);
			atomicVars[varName] -= 1.0;
			globals->assign(varName, Value::Number(atomicVars[varName]));
			return Value::Number(atomicVars[varName]);
			});

		defineBuiltin("atomic_xchg", [this](const std::vector<Value>& args) -> Value {
			if (args.size() != 2) throw RuntimeError("atomic_xchg(var_name, new_val)");
			std::string varName = *args[0].stringValue;
			double newVal = args[1].numberValue;
			std::lock_guard<std::mutex> lock(atomicMutex);
			double oldVal = atomicVars[varName];
			atomicVars[varName] = newVal;
			globals->assign(varName, Value::Number(newVal));
			return Value::Number(oldVal);
			});

		defineBuiltin("atomic_cas", [this](const std::vector<Value>& args) -> Value {
			if (args.size() != 3) throw RuntimeError("atomic_cas(var_name, expected, new_val)");
			std::string varName = *args[0].stringValue;
			double expected = args[1].numberValue;
			double newVal = args[2].numberValue;
			std::lock_guard<std::mutex> lock(atomicMutex);
			if (atomicVars[varName] == expected) {
				atomicVars[varName] = newVal;
				globals->assign(varName, Value::Number(newVal));
				return Value::Bool(true);
			}
			return Value::Bool(false);
			});
		defineBuiltin("push", [](const std::vector<Value>& args) -> Value {
			if (args.size() != 2) throw RuntimeError("push() expects 2 arguments: array and value");
			if (args[0].type != ValueType::ARRAY) throw RuntimeError("First argument must be an array");

			args[0].arrayValue->push_back(args[1]);
			return Value::Nil();
			});

		defineBuiltin("pop", [](const std::vector<Value>& args) -> Value {
			if (args.size() != 1) throw RuntimeError("pop() expects 1 argument: array");
			if (args[0].type != ValueType::ARRAY) throw RuntimeError("Argument must be an array");
			if (args[0].arrayValue->empty()) throw RuntimeError("Cannot pop from empty array");

			Value val = args[0].arrayValue->back();
			args[0].arrayValue->pop_back();
			return val;
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

			if (idxVal.type != ValueType::NUMBER) {
				throw RuntimeError("Index must be a number");
			}

			int i = static_cast<int>(idxVal.numberValue);

			if (arrayVal.type == ValueType::STRING) {
				int size = static_cast<int>(arrayVal.stringValue->size());
				if (i < 0) i = size + i;
				if (i < 0 || i >= size) {
					throw RuntimeError("String index out of bounds");
				}
				return Value::String(std::string(1, arrayVal.stringValue->at(i)));
			}

			if (arrayVal.type != ValueType::ARRAY) {
				throw RuntimeError("Cannot index non-array/string value");
			}

			int size = static_cast<int>(arrayVal.arrayValue->size());

			if (i < 0) {
				i = size + i;
			}

			if (i < 0 || i >= size) {
				throw RuntimeError("Array index out of bounds");
			}
			return (*arrayVal.arrayValue)[i];
		}

		if (auto indexAssign = std::dynamic_pointer_cast<IndexAssignExpr>(expr)) {
			Value arrayVal = evaluateExpr(indexAssign->array);
			Value idxVal = evaluateExpr(indexAssign->index);
			Value newVal = evaluateExpr(indexAssign->value);

			if (idxVal.type != ValueType::NUMBER) {
				throw RuntimeError("Index must be a number");
			}

			int i = static_cast<int>(idxVal.numberValue);

			if (arrayVal.type == ValueType::STRING) {
				int size = static_cast<int>(arrayVal.stringValue->size());
				if (i < 0) i = size + i;
				if (i < 0 || i >= size) throw RuntimeError("String index out of bounds");

				(*arrayVal.stringValue)[i] = newVal.toString()[0];
				return newVal;
			}

			if (arrayVal.type != ValueType::ARRAY) {
				throw RuntimeError("Cannot index assign to non-array value");
			}

			int size = static_cast<int>(arrayVal.arrayValue->size());

			if (i < 0) i = size + i;
			if (i < 0 || i >= size) throw RuntimeError("Array index out of bounds");

			(*arrayVal.arrayValue)[i] = newVal;

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
					return it->second(args);
				}
			}

			Value calleeValue = evaluateExpr(call->callee);

			if (calleeValue.type != ValueType::FUNCTION) {
				throw RuntimeError("Cannot call non-function value");
			}

			auto& func = calleeValue.funcValue;
			if (func->params.size() != args.size()) {
				throw RuntimeError("Function argument count mismatch");
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

		if (auto parallelStmt = std::dynamic_pointer_cast<ParallelStmt>(stmt)) {
			std::string varName = "";
			double startVal = 0;
			double endVal = 0;
			double stepVal = 1;
			bool validRange = false;

			auto prevEnv = environment;
			auto loopEnv = std::make_shared<Environment>(environment);
			environment = loopEnv;

			if (parallelStmt->initializer) {
				executeStmt(parallelStmt->initializer);
				if (auto varStmt = std::dynamic_pointer_cast<VarStmt>(parallelStmt->initializer)) {
					varName = varStmt->name;
					Value v = environment->get(varName);
					if (v.type == ValueType::NUMBER) {
						startVal = v.numberValue;
					}
				}
			}

			if (varName.empty()) {
				throw RuntimeError("Parallel for requires a variable initializer");
			}

			if (auto bin = std::dynamic_pointer_cast<BinaryExpr>(parallelStmt->condition)) {
				if (bin->op.type == TokenType::LESS || bin->op.type == TokenType::LESS_EQUAL) {
					Value r = evaluateExpr(bin->right);
					if (r.type == ValueType::NUMBER) {
						endVal = r.numberValue;
						if (bin->op.type == TokenType::LESS_EQUAL) endVal += 1;
						validRange = true;
					}
				}
			}

			if (auto post = std::dynamic_pointer_cast<PostfixExpr>(parallelStmt->increment)) {
				if (post->op.type == TokenType::PLUS_PLUS) stepVal = 1;
				else if (post->op.type == TokenType::MINUS_MINUS) stepVal = -1;
			}
			else if (auto assign = std::dynamic_pointer_cast<CompoundAssignExpr>(parallelStmt->increment)) {
				Value v = evaluateExpr(assign->value);
				if (v.type == ValueType::NUMBER) {
					if (assign->op.type == TokenType::PLUS_EQUAL) stepVal = v.numberValue;
					if (assign->op.type == TokenType::MINUS_EQUAL) stepVal = -v.numberValue;
				}
			}

			environment = prevEnv;

			if (validRange && stepVal != 0) {
				int totalOps = static_cast<int>((endVal - startVal) / stepVal);

				if (totalOps <= 0 || totalOps < 20) {
					auto seqEnv = std::make_shared<Environment>(environment);
					environment = seqEnv;
					executeStmt(parallelStmt->initializer);
					while (evaluateExpr(parallelStmt->condition).isTruthy()) {
						executeStmt(parallelStmt->body);
						evaluateExpr(parallelStmt->increment);
					}
					environment = prevEnv;
					return;
				}

				int actualThreads = std::min(numThreads, totalOps);
				std::vector<std::thread> threads;
				std::atomic<bool> hasError{ false };
				std::string errorMsg;
				std::mutex errorMutex;

				auto sharedGlobals = globals;
				auto sharedEnv = environment;
				auto sharedBuiltins = builtins;
				auto bodyStmt = parallelStmt->body;
				auto condExpr = parallelStmt->condition;
				auto incExpr = parallelStmt->increment;

				std::atomic<int> sharedCounter{ 0 };

				for (int t = 0; t < actualThreads; t++) {
					threads.emplace_back([=, &sharedCounter, &hasError, &errorMsg, &errorMutex]() {
						try {
							Interpreter threadInterp;
							threadInterp.globals = sharedGlobals;
							threadInterp.environment = std::make_shared<Environment>(sharedEnv);
							threadInterp.builtins = sharedBuiltins;

							while (!hasError) {
								int idx = sharedCounter.fetch_add(1);
								double currentI = startVal + (idx * stepVal);

								bool stillRunning = false;
								if (stepVal > 0) stillRunning = (currentI < endVal);
								else stillRunning = (currentI > endVal);

								if (!stillRunning) break;

								auto iterEnv = std::make_shared<Environment>(sharedEnv);
								iterEnv->define(varName, Value::Number(currentI));
								threadInterp.environment = iterEnv;

								threadInterp.executeStmt(bodyStmt);
							}
						}
						catch (const std::exception& e) {
							std::lock_guard<std::mutex> lock(errorMutex);
							hasError = true;
							errorMsg = e.what();
						}
						});
				}

				for (auto& th : threads) if (th.joinable()) th.join();
				if (hasError) throw RuntimeError(errorMsg);
			}
			else {
				throw RuntimeError("Parallel loop too complex for automatic parallelization. Use simple numeric ranges.");
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