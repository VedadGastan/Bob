#ifndef CALLABLE_H
#define CALLABLE_H

#include <vector>
#include <string>
#include <memory>
#include <map>

class Environment;
class Stmt;

class Function {
public:
	std::vector<std::string> params;
	std::vector<std::shared_ptr<Stmt>> body;
	std::shared_ptr<Environment> closure;

	Function(const std::vector<std::string>& p, const std::vector<std::shared_ptr<Stmt>>& b, const std::shared_ptr<Environment>& c)
		: params(p), body(b), closure(c) {
	}
};

#endif