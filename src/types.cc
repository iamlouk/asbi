#include <cstdlib>
#include <iostream>
#include <string>
#include <cassert>
#include <sstream>
#include <cmath>
#include "include/mem.hh"
#include "include/types.hh"
#include "include/context.hh"
#include "include/vm.hh"

using namespace asbi;

/*
static std::size_t hash_cstr(const char* str) {
	std::size_t hash = 4321;
	int c;
	while ((c = *str++))
		hash = ((hash << 5) + hash) + c;

	return hash;
}
*/

StringContainer::StringContainer(std::string &data, std::size_t hash, Context* ctx, bool gc):
	GCObj(ctx, gc), hash(hash), data(data) {}

StringContainer::~StringContainer(){
	// std::cout << "~StringContainer" << '\n';
}

void StringContainer::gc_visit() const {
	gc_inuse = true;
}

std::size_t StringContainer::gc_size() const {
	return sizeof(*this) + data.size();
}

LambdaContainer::LambdaContainer(std::shared_ptr<Env> env, std::vector<OpCode>* ops, std::vector<StringContainer*> argnames, Context* ctx, bool gc):
	GCObj(ctx, gc), env(env), ops(ops), argnames(argnames) {}

LambdaContainer::~LambdaContainer(){
	// std::cout << "~LambdaContainer" << '\n';
}

void LambdaContainer::gc_visit() const {
	if (gc_inuse)
		return;

	gc_inuse = true;
	assert(env != nullptr);
	env->gc_visit();
}

std::size_t LambdaContainer::gc_size() const {
	return sizeof(*this);
}

MapContainer::MapContainer(Context* ctx): GCObj(ctx, true) {}

void MapContainer::gc_visit() const {
	if (gc_inuse)
		return;

	gc_inuse = true;

	for (auto [key, val]: data) {
		key.gc_visit();
		val.gc_visit();
	}

	for (auto val: vecdata) {
		val.gc_visit();
	}
}

std::size_t MapContainer::gc_size() const {
	return sizeof(*this) + (data.size() * sizeof(Value) * 2) + (vecdata.size() * sizeof(Value));
}

void MapContainer::set(Value key, Value val) {
	unsigned int idx;
	if (key.asUint(&idx)) {
		while (vecdata.size() <= idx)
			vecdata.push_back(Value::nil());

		vecdata[idx] = val;
	} else {
		data[key] = val;
	}
}

Value MapContainer::get(Value key) {
	unsigned int idx;
	if (key.asUint(&idx)) {
		if (idx < vecdata.size())
			return vecdata[idx];
		else
			return Value::nil();
	} else {
		auto search = data.find(key);
		if (search == data.end())
			return Value::nil();
		else
			return search->second;
	}
}


Value Value::symbol(const char* str, Context* ctx) {
	Value val;
	val.type = type_t::Symbol;
	std::string cppstr(str);
	val._string = ctx->new_stringconstant(cppstr);
	return val;
}


Value Value::string(const char* str, Context* ctx) {
	Value val;
	val.type = type_t::String;
	std::string cppstr(str);
	val._string = ctx->new_stringconstant(cppstr);
	return val;
}

Value Value::string(std::string &str, Context *ctx) {
	Value val;
	val.type = type_t::String;
	val._string = ctx->new_string(str);
	return val;
}

Value Value::call(Context *ctx, unsigned int n, std::shared_ptr<Env> callerenv) const {
	switch (type) {
	case type_t::Lambda:{
		if (_lambda->argnames.size() != n)
			throw std::runtime_error("callable argnum does not match call");

		auto lbdenv = std::make_shared<Env>(ctx, _lambda->env);
		for (unsigned int i = 0; i < n; ++i)
			lbdenv->decl(_lambda->argnames[i], ctx->pop());

		lbdenv->caller = callerenv;
		return execute(*_lambda->ops, lbdenv, ctx);
	}
	case type_t::Macro:
		return _macro(n, ctx, callerenv);
	default:
		throw std::runtime_error("not callable");
	}
}

bool Value::operator==(const Value &rhs) const {
	assert(type != type_t::StackPlaceholder);
	assert(rhs.type != type_t::StackPlaceholder);
	if (type != rhs.type) return false;
	switch (type) {
	case type_t::Bool:
		return _boolean == rhs._boolean;
	case type_t::Number:
		return _number == rhs._number;
	case type_t::Nil:
		return true;
	case type_t::Symbol:
	case type_t::String:
		return _string->data == rhs._string->data;
	case type_t::Lambda:
		return _lambda == rhs._lambda;
	case type_t::Map:
		return _map->data == rhs._map->data;
	case type_t::Macro:
		return _macro == rhs._macro;
	case type_t::StackPlaceholder:
		return false; // should not happen
	}
}

std::size_t Value::hash() const {
	assert(type != type_t::StackPlaceholder);
	switch (type) {
	case type_t::Bool:
		return _boolean ? 0xABC1 : 0xABC0;
	case type_t::Number:
		return static_cast<std::size_t>(_number);
	case type_t::Nil:
		return 0xAFFE;
	case type_t::Symbol:
	case type_t::String:
		return _string->hash;
	case type_t::Lambda:
		return reinterpret_cast<std::size_t>(_lambda->ops);
	case type_t::Map:
		return reinterpret_cast<std::size_t>(_map);
	case type_t::Macro:
		return reinterpret_cast<std::size_t>(_macro);
	case type_t::StackPlaceholder:
		return 0; // should not happen
	}
}

std::string Value::to_string(bool debug) const {
	assert(type != type_t::StackPlaceholder);
	switch (type) {
	case type_t::Bool:
		return _boolean ? "true" : "false";
	case type_t::Number:{
		auto integer = static_cast<int>(_number);
		return integer == _number ? std::to_string(integer) : std::to_string(_number);
	}
	case type_t::Nil:
		return "nil";
	case type_t::Symbol:
		return std::string(":") + _string->data;
	case type_t::String:
		return debug ? std::string("\"") + _string->data + "\"" : _string->data;
	case type_t::Lambda:{
		std::stringstream ss;
		ss << "<Lambda#" << (void*)_lambda->ops << ">";
		return ss.str();
	}
	case type_t::Map:{
		std::stringstream ss;
		ss << "[";
		for (auto val: _map->vecdata)
			ss << val.to_string(true) << ", ";
		for (auto [key, val]: _map->data)
			ss << key.to_string(true) << " ~ " << val.to_string(true) << ", ";
		ss << "]";
		return ss.str();
	}
	case type_t::Macro:{
		std::stringstream ss;
		ss << "<Macro#" << (void*)_lambda->ops << ">";
		return ss.str();
	}
	case type_t::StackPlaceholder:
		return std::string("<StackPlaceholder>");
	}
}

void Value::gc_visit() const {
	switch (type) {
	case type_t::Symbol:
	case type_t::String:
		_string->gc_visit();
		return;
	case type_t::Lambda:
		_lambda->gc_visit();
		return;
	case type_t::Map:
		_map->gc_visit();
		return;
	default:
		return;
	}
}

bool Value::asUint(unsigned int *intpart) const {
	if (type != type_t::Number)
		return false;

	double ipart;
	double fpart = std::modf(_number, &ipart);
	if (std::fabs(fpart) < 0.00001 && ipart >= 0.0) {
		*intpart = static_cast<unsigned int>(ipart);
		return true;
	}
	return false;
}
