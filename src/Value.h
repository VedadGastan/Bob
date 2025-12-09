#ifndef VALUE_H
#define VALUE_H

#include <string>
#include <vector>
#include <sstream>
#include <cmath>
#include <stdexcept>
#include <memory>
#include "Token.h"
#include "Callable.h"

enum class ValueType {
	NIL, BOOL, NUMBER, STRING, ARRAY, FUNCTION
};

class Value {
public:
	ValueType type;
	bool boolValue;
	double numberValue;
	std::string stringValue;
	std::vector<Value> arrayValue;
	std::shared_ptr<Function> funcValue;

	Value() : type(ValueType::NIL), boolValue(false), numberValue(0) {}

	static Value Nil() { return Value(); }

	static Value Bool(bool b) {
		Value v;
		v.type = ValueType::BOOL;
		v.boolValue = b;
		return v;
	}

	static Value Number(double n) {
		Value v;
		v.type = ValueType::NUMBER;
		v.numberValue = n;
		return v;
	}

	static Value String(const std::string& s) {
		Value v;
		v.type = ValueType::STRING;
		v.stringValue = s;
		return v;
	}

	static Value Array(const std::vector<Value>& arr) {
		Value v;
		v.type = ValueType::ARRAY;
		v.arrayValue = arr;
		return v;
	}

	static Value Function(std::shared_ptr<Function> func) {
		Value v;
		v.type = ValueType::FUNCTION;
		v.funcValue = func;
		return v;
	}


	bool isTruthy() const {
		switch (type) {
		case ValueType::NIL: return false;
		case ValueType::BOOL: return boolValue;
		case ValueType::NUMBER: return numberValue != 0;
		case ValueType::STRING: return !stringValue.empty();
		case ValueType::ARRAY: return !arrayValue.empty();
		case ValueType::FUNCTION: return true;
		default: return true;
		}
	}

	std::string toString() const {
		std::stringstream ss;
		switch (type) {
			case ValueType::NIL:
				return "nil";

			case ValueType::BOOL:
				return boolValue ? "true" : "false";

			case ValueType::NUMBER:
				ss << numberValue;
				return ss.str();

			case ValueType::STRING:
				return stringValue;

			case ValueType::ARRAY:
				ss << "[";
				for (size_t i = 0; i < arrayValue.size(); i++) {
					if (i > 0) ss << ", ";
					ss << arrayValue[i].toString();
				}
				ss << "]";
				return ss.str();
			case ValueType::FUNCTION:
				return "<function>";
			default:
				return "unknown";
			}
	}

	bool isEqual(const Value& other) const {
		if (type != other.type) return false;
		switch (type) {
		case ValueType::NIL: return true;
		case ValueType::BOOL: return boolValue == other.boolValue;
		case ValueType::NUMBER: return numberValue == other.numberValue;
		case ValueType::STRING: return stringValue == other.stringValue;
		case ValueType::ARRAY:
			return arrayValue.size() == other.arrayValue.size();
		case ValueType::FUNCTION:
			return funcValue == other.funcValue;
		default: return false;
		}
	}

	Value applyUnaryOp(TokenType opType) const;
	Value applyBinaryOp(TokenType opType, const Value& other) const;
};


inline Value Value::applyUnaryOp(TokenType opType) const {
	switch (opType) {
	case TokenType::MINUS:
		if (type == ValueType::NUMBER) return Value::Number(-numberValue);
		throw std::runtime_error("Unary '-' requires a number");
	case TokenType::NOT:
		return Value::Bool(!isTruthy());
	default:
		throw std::runtime_error("Unsupported unary operator");
	}
}

inline Value Value::applyBinaryOp(TokenType opType, const Value& other) const {
	if (type == ValueType::NUMBER && other.type == ValueType::NUMBER) {
		switch (opType) {
		case TokenType::PLUS: return Value::Number(numberValue + other.numberValue);
		case TokenType::MINUS: return Value::Number(numberValue - other.numberValue);
		case TokenType::STAR: return Value::Number(numberValue * other.numberValue);
		case TokenType::SLASH:
			if (other.numberValue == 0) throw std::runtime_error("Division by zero");
			return Value::Number(numberValue / other.numberValue);
		case TokenType::PERCENT:
			if (other.numberValue == 0) throw std::runtime_error("Modulo by zero");
			return Value::Number(std::fmod(numberValue, other.numberValue));
		case TokenType::STAR_STAR:
			return Value::Number(std::pow(numberValue, other.numberValue));
		case TokenType::GREATER: return Value::Bool(numberValue > other.numberValue);
		case TokenType::GREATER_EQUAL: return Value::Bool(numberValue >= other.numberValue);
		case TokenType::LESS: return Value::Bool(numberValue < other.numberValue);
		case TokenType::LESS_EQUAL: return Value::Bool(numberValue <= other.numberValue);
		case TokenType::EQUAL_EQUAL: return Value::Bool(numberValue == other.numberValue);
		case TokenType::BANG_EQUAL: return Value::Bool(numberValue != other.numberValue);
		default: throw std::runtime_error("Unsupported number operation");
		}
	}

	if (type == ValueType::STRING && other.type == ValueType::NUMBER && opType == TokenType::STAR) {
		std::stringstream ss;
		int count = static_cast<int>(other.numberValue);
		for (int i = 0; i < count; ++i) {
			ss << stringValue;
		}
		return Value::String(ss.str());
	}

	if (type == ValueType::STRING && opType == TokenType::PLUS) {
		return Value::String(stringValue + other.toString());
	}

	if (type != ValueType::STRING && other.type == ValueType::STRING && opType == TokenType::PLUS) {
		return Value::String(toString() + other.stringValue);
	}

	if (type == ValueType::STRING && other.type == ValueType::STRING) {
		if (opType == TokenType::EQUAL_EQUAL) return Value::Bool(stringValue == other.stringValue);
		if (opType == TokenType::BANG_EQUAL) return Value::Bool(stringValue != other.stringValue);
		if (opType == TokenType::IN)
			return Value::Bool(other.stringValue.find(stringValue) != std::string::npos);
		throw std::runtime_error("Unsupported string operation");
	}

	if (other.type == ValueType::ARRAY && opType == TokenType::IN) {
		for (const auto& element : other.arrayValue) {
			if (this->isEqual(element)) {
				return Value::Bool(true);
			}
		}
		return Value::Bool(false);
	}

	if (opType == TokenType::EQUAL_EQUAL) {
		if (type == ValueType::STRING && other.type != ValueType::STRING) {
			return Value::Bool(stringValue == other.toString());
		}
		if (type != ValueType::STRING && other.type == ValueType::STRING) {
			return Value::Bool(toString() == other.stringValue);
		}
		return Value::Bool(isEqual(other));
	}

	if (opType == TokenType::BANG_EQUAL) {
		if (type == ValueType::STRING && other.type != ValueType::STRING) {
			return Value::Bool(stringValue != other.toString());
		}
		if (type != ValueType::STRING && other.type == ValueType::STRING) {
			return Value::Bool(toString() != other.stringValue);
		}
		return Value::Bool(!isEqual(other));
	}

	throw std::runtime_error("Unsupported binary operation");
}

#endif