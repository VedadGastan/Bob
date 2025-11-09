#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "Value.h"
#include <map>
#include <string>
#include <memory>
#include <stdexcept>

class Environment : public std::enable_shared_from_this<Environment> {
public:
	std::shared_ptr<Environment> enclosing;
	std::map<std::string, Value> values;

	Environment() : enclosing(nullptr) {}
	Environment(std::shared_ptr<Environment> enc) : enclosing(enc) {}

	void define(const std::string& name, const Value& value) {
		// This defines a new variable in the *current* scope.
		// It allows shadowing.
		values[name] = value;
	}

	Value get(const std::string& name) {
		auto it = values.find(name);
		if (it != values.end()) {
			return it->second;
		}

		if (enclosing != nullptr) {
			return enclosing->get(name);
		}

		throw std::runtime_error("Undefined variable '" + name + "'");
	}

	void assign(const std::string& name, const Value& value) {
		auto it = values.find(name);
		if (it != values.end()) {
			values[name] = value;
			return;
		}

		if (enclosing != nullptr) {
			enclosing->assign(name, value);
			return;
		}

		throw std::runtime_error("Undefined variable '" + name + "'");
	}
};

#endif