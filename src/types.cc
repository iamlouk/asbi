#include <cstdlib>
#include <iostream>
#include <string>
#include <cassert>
#include <sstream>
#include <cmath>
#include "mem.hh"
#include "types.hh"
#include "context.hh"

using namespace asbi;

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

DictContainer::DictContainer(Context* ctx): GCObj(ctx, true) {}

void DictContainer::gc_visit() const {
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

std::size_t DictContainer::gc_size() const {
	return sizeof(*this) + (data.size() * sizeof(Value) * 2) + (vecdata.size() * sizeof(Value));
}

void DictContainer::set(Value key, Value val) {
	unsigned int idx;
	if (key.asUint(&idx)) {
		while (vecdata.size() <= idx)
			vecdata.push_back(Value::nil());

		vecdata[idx] = val;
	} else {
		data[key] = val;
	}
}

Value DictContainer::get(Value key) {
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
	case type_t::Dict:
		return _dict->data == rhs._dict->data;
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
	case type_t::Dict:
		return reinterpret_cast<std::size_t>(_dict);
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
	case type_t::Dict:{
		std::stringstream ss;
		ss << "[";
		for (auto val: _dict->vecdata)
			ss << val.to_string(true) << ", ";
		for (auto [key, val]: _dict->data)
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
	case type_t::Dict:
		_dict->gc_visit();
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
