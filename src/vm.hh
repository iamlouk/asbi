#ifndef OPCODES_HH
#define OPCODES_HH

#include <stack>
#include <vector>
#include <cstdint>
#include <memory>
#include "mem.hh"
#include "types.hh"
#include "utils.hh"
#include "context.hh"

static_assert(sizeof(void*) == sizeof(uint64_t));
static_assert(sizeof(uint64_t) == sizeof(double));

namespace asbi {

	enum OpCode: uint64_t {
		PUSH_NUMBER,
		PUSH_BOOLEAN, PUSH_TRUE, PUSH_FALSE,
		PUSH_NIL,
		PUSH_SYMBOL, PUSH_STRING,
		PUSH_LAMBDA,
		PUSH_STACK_PLACEHOLDER,
		POP,
		ENTER_SCOPE,
		LEAVE_SCOPE,
		CALL,
		LOOKUP, DECL, SET,
		ADD, SUB, MUL, DIV,
		EQUALS, EQUALS_NOT,
		SMALLER, BIGGER, SMALLER_OR_EQUAL, BIGGER_OR_EQUAL,
		NOT,
		MAKE_DICT_MAPLIKE,
		MAKE_DICT_ARRLIKE,
		GET_DICT_VAL,
		SET_DICT_VAL,
		DESTRUCT_ARRLIKE,
		GOTO, IF_TRUE_GOTO, IF_FALSE_GOTO,
		NOOP,
	};

	Value execute(std::vector<OpCode>, std::shared_ptr<Env>, Context*);

}

#endif
