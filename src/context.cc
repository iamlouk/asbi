#include <cassert>
#include <stdexcept>
#include <iostream>
#include <functional>
#include "context.hh"
#include "parser.hh"
#include "tokenizer.hh"
#include "types.hh"
#include "utils.hh"

using namespace asbi;

void Context::push(Value v) {
	stack.push_back(v);
}

Value Context::pop() {
	Value v = stack.back();
	assert(stack.size() > 0);
	stack.pop_back();
	return v;
}

Env::Env(Context*, std::shared_ptr<Env> outer) {
	this->outer = outer;
}
Env::~Env() {

}

void Env::gc_visit() const {
	for (auto [key, val]: vars)
		val.gc_visit();

	if (outer != nullptr)
		outer->gc_visit();

	if (caller != nullptr)
		caller->gc_visit();
}
Value Env::lookup(StringContainer* sc) {
	auto env = this;
	while (env != nullptr) {
		auto search = env->vars.find(sc);
		if (search != env->vars.end())
			return search->second;

		env = env->outer.get();
	}

	throw utils::interpreter_error("variable not found in env", sc->data);
}
Value Env::lookup(Context* ctx, const char* str) {
	std::string cppstr(str);
	return lookup(ctx->new_stringconstant(cppstr));
}
void Env::decl(StringContainer* sc, Value val) {
	vars[sc] = val;
}
void Env::decl(Context* ctx, const char* str, Value val) {
	std::string cppstr(str);
	vars[ctx->new_stringconstant(cppstr)] = val;
}
void Env::set(StringContainer* sc, Value val) {
	auto env = this;
	while (env != nullptr) {
		auto search = env->vars.find(sc);
		if (search != env->vars.end()) {
			search->second = val;
			return;
		}

		env = env->outer.get();
	}

	throw std::runtime_error("variable not found in env");
}

Context::Context() {
	global_env = std::make_shared<Env>(this, nullptr);

	std::string str = "__file";
	names.__file = new_stringconstant(str);
	str = "__main";
	names.__main = new_stringconstant(str);
	str = "__imports";
	names.__imports = new_stringconstant(str);
	str = "exports";
	names.exports = new_stringconstant(str);

	load_macros();
}

Context::~Context() {
	assert(stack.size() == 0);
	gc(nullptr);

	for (unsigned int i = 0; i < sizeof(strconsts) / sizeof(*strconsts); ++i)
		for (auto sc: strconsts[i])
			delete sc;

	for (auto lc: lambdas) {
		delete lc->ops;
		delete lc;
	}
}

Value Context::run(const std::string& str) {
	return run(str, std::make_shared<Env>(this, global_env));
}

Value Context::run(const std::string& str, std::shared_ptr<Env> env) {
	tok::Tokenizer toker(str);
	Parser parser(toker, this);
	auto ast = parser.parse()->optimize();
	std::vector<OpCode> ops;
	ast->to_vmops(this, ops);
	delete ast;

	return execute(ops, env, this);
}

StringContainer* Context::new_stringconstant(std::string &string) {
	std::size_t hash = std::hash<std::string>()(string);
	auto &consts = strconsts[hash % (sizeof(strconsts) / sizeof(*strconsts))];
	for (auto sc: consts)
		if (sc->hash == hash && sc->data == string)
			return sc;

	auto sc = new StringContainer(string, hash, this, false);
	consts.push_back(sc);
	return sc;
}

StringContainer* Context::new_string(std::string &string) {
	return new StringContainer(string, std::hash<std::string>()(string), this, true);
}
