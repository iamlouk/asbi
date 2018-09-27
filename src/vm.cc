#include <cassert>
#include <stdexcept>
#include "vm.hh"
#include "context.hh"

using namespace asbi;

Value asbi::execute(std::vector<OpCode> opcodes, std::shared_ptr<Env> env, Context* ctx) {
#ifndef NDEBUG
	auto oldstacksize = ctx->stack.size();
#endif
	unsigned int pc = 0;
	while (pc < opcodes.size()) {
		switch (opcodes[pc++]) {
		case PUSH_NUMBER:{
			auto flt = opcodes[pc++];
			auto fltptr = reinterpret_cast<double*>(&flt);
			ctx->push(Value::number(*fltptr));
			break;
		}
		case PUSH_BOOLEAN:{
			auto val = opcodes[pc++];
			ctx->push(Value::boolean(val != 0));
			break;
		}
		case PUSH_TRUE:
			ctx->push(Value::boolean(true));
			break;
		case PUSH_FALSE:
			ctx->push(Value::boolean(false));
			break;
		case PUSH_NIL:
			ctx->push(Value::nil());
			break;
		case PUSH_SYMBOL:{
			auto raw = opcodes[pc++];
			auto sc = reinterpret_cast<StringContainer*>(raw);
			ctx->push(Value::symbol(sc));
			break;
		}
		case PUSH_STRING:{
			auto raw = opcodes[pc++];
			auto sc = reinterpret_cast<StringContainer*>(raw);
			ctx->push(Value::string(sc));
			break;
		}
		case PUSH_LAMBDA:{
			auto raw = opcodes[pc++];
			auto lc = reinterpret_cast<LambdaContainer*>(raw);
			ctx->push(Value::lambda(new LambdaContainer(env, lc->ops, lc->argnames, ctx, true)));
			break;
		}
		case PUSH_STACK_PLACEHOLDER:{
			auto raw = opcodes[pc++];
			auto sc = reinterpret_cast<StringContainer*>(raw);
			ctx->push(Value::stackplaceholder(sc));
			break;
		}
		case POP:
			ctx->pop();
			break;
		case ENTER_SCOPE:{
			env = std::make_shared<Env>(ctx, env);
			break;
		}
		case LEAVE_SCOPE:{
			env = env->outer;
			assert(env != nullptr);
			break;
		}
		case CALL:{
			auto n = static_cast<unsigned int>(opcodes[pc++]);
			auto callable = ctx->pop();
			if (callable.type == type_t::Lambda) {
				auto lc = callable._lambda;
				if (lc->argnames.size() != n)
					throw std::runtime_error("callable argnum does not match call");

				auto lbdenv = std::make_shared<Env>(ctx, lc->env);
				for (unsigned int i = 0; i < n; ++i) {
					lbdenv->decl(lc->argnames[i], ctx->pop());
				}
				lbdenv->caller = env;
				ctx->push(execute(*lc->ops, lbdenv, ctx));
				break;
			}
			if (callable.type == type_t::Macro) {
				ctx->push(callable._macro(n, ctx, env));
				break;
			}
			throw std::runtime_error("expected callable");
		}
		case LOOKUP:{
			auto raw = opcodes[pc++];
			auto sc = reinterpret_cast<StringContainer*>(raw);
			ctx->push(env->lookup(sc));
			break;
		}
		case DECL:{
			auto raw = opcodes[pc++];
			auto sc = reinterpret_cast<StringContainer*>(raw);
			env->decl(sc, ctx->pop());
			ctx->push(Value::nil());
			break;
		}
		case SET:{
			auto raw = opcodes[pc++];
			auto sc = reinterpret_cast<StringContainer*>(raw);
			auto val = ctx->pop();
			env->set(sc, val);
			ctx->push(val);
			break;
		}
		case ADD:{
			auto b = ctx->pop();
			auto a = ctx->pop();
			if (a.type == type_t::Number && b.type == type_t::Number) {
				ctx->push(Value::number(a._number + b._number));
				break;
			}

			if (a.type == type_t::String && b.type == type_t::String) {
				auto sc = ctx->new_string(a._string->data);
				sc->data += b._string->data;
				ctx->push(Value::string(sc));
				ctx->heap_size += sc->gc_size();
				ctx->check_gc(env);
				break;
			}

			throw std::runtime_error("expected number or string");
		}
		case SUB:{
			auto b = ctx->pop();
			auto a = ctx->pop();
			if (a.type != type_t::Number || b.type != type_t::Number)
				throw std::runtime_error("expected number");
			ctx->push(Value::number(a._number - b._number));
			break;
		}
		case MUL:{
			auto b = ctx->pop();
			auto a = ctx->pop();
			if (a.type != type_t::Number || b.type != type_t::Number)
				throw std::runtime_error("expected number");
			ctx->push(Value::number(a._number * b._number));
			break;
		}
		case DIV:{
			auto b = ctx->pop();
			auto a = ctx->pop();
			if (a.type != type_t::Number || b.type != type_t::Number)
				throw std::runtime_error("expected number");
			ctx->push(Value::number(a._number / b._number));
			break;
		}
		case EQUALS:{
			auto b = ctx->pop();
			auto a = ctx->pop();
			ctx->push(Value::boolean(a == b));
			break;
		}
		case EQUALS_NOT:{
			auto b = ctx->pop();
			auto a = ctx->pop();
			ctx->push(Value::boolean(!(a == b)));
			break;
		}
		case SMALLER:{
			auto b = ctx->pop();
			auto a = ctx->pop();
			if (a.type != type_t::Number || b.type != type_t::Number)
				throw std::runtime_error("expected number");
			ctx->push(Value::boolean(a._number < b._number));
			break;
		}
		case BIGGER:{
			auto b = ctx->pop();
			auto a = ctx->pop();
			if (a.type != type_t::Number || b.type != type_t::Number)
				throw std::runtime_error("expected number");
			ctx->push(Value::boolean(a._number > b._number));
			break;
		}
		case NOT:{
			auto a = ctx->pop();
			if (a.type != type_t::Bool)
				throw std::runtime_error("expected boolean");
			ctx->push(Value::boolean(!a._boolean));
			break;
		}
		case MAKE_DICT_MAPLIKE:{
			auto n = static_cast<unsigned int>(opcodes[pc++]);
			auto dc = new DictContainer(ctx);
			for (unsigned int i = 0; i < n; ++i) {
				dc->set(ctx->pop(), ctx->pop());
			}

			ctx->push(Value::dict(dc));
			ctx->heap_size += dc->gc_size();
			ctx->check_gc(env);
			break;
		}
		case MAKE_DICT_ARRLIKE:{
			auto n = static_cast<unsigned int>(opcodes[pc++]);
			auto dc = new DictContainer(ctx);
			for (unsigned int i = 0; i < n; ++i) {
				dc->set(Value::number(n - 1- i), ctx->pop());
			}

			ctx->push(Value::dict(dc));
			ctx->heap_size += dc->gc_size();
			ctx->check_gc(env);
			break;
		}
		case GET_DICT_VAL:{
			auto dict = ctx->pop();
			if (dict.type != type_t::Dict)
				throw std::runtime_error("expected dict");

			ctx->push(dict._dict->get(ctx->pop()));
			break;
		}
		case SET_DICT_VAL:{
			auto dict = ctx->pop();
			if (dict.type != type_t::Dict)
				throw std::runtime_error("expected dict");

			auto val = ctx->pop();
			auto key = ctx->pop();
			auto prevsize = dict._dict->gc_size();
			dict._dict->set(key, val);
			if (prevsize < dict._dict->gc_size()) {
				ctx->heap_size -= prevsize;
				ctx->heap_size += dict._dict->gc_size();
				ctx->push(val);
				ctx->check_gc(env);
				break;
			}
			ctx->push(val);
			break;
		}
		case DESTRUCT_ARRLIKE:{
			auto n = static_cast<unsigned int>(opcodes[pc++]);
			auto dict = ctx->pop();
			if (dict.type != type_t::Dict)
				throw std::runtime_error("expected dict");

			for (unsigned int i = 0; i < n; ++i) {
				auto tomatch = ctx->pop();
				auto value = dict._dict->get(Value::number(i));
				if (tomatch.type == type_t::StackPlaceholder) {
					env->decl(tomatch._string, value);
				} else {
					if (!(tomatch == value)) {
						throw std::runtime_error("match error in destruction");
					}
				}
			}
			ctx->push(dict);
			break;
		}
		case GOTO:{
			pc = static_cast<unsigned int>(opcodes[pc++]);
			assert(pc <= opcodes.size());
			break;
		}
		case IF_TRUE_GOTO:{
			auto a = ctx->pop();
			auto new_pc = static_cast<unsigned int>(opcodes[pc++]);
			if (a.type != type_t::Bool)
				throw std::runtime_error("expected boolean");

			if (a._boolean)
				pc = new_pc;
			break;
		}
		case IF_FALSE_GOTO:{
			auto a = ctx->pop();
			auto new_pc = static_cast<unsigned int>(opcodes[pc++]);
			if (a.type != type_t::Bool)
				throw std::runtime_error("expected boolean");

			if (!a._boolean)
				pc = new_pc;
			break;
		}
		case NOOP:
			assert("NOOPs should not happen");
			break;
		default:
			throw std::runtime_error("invalid opcode");
		}
	}

	assert(pc <= opcodes.size());
	assert(ctx->stack.size() == oldstacksize + 1);
	return ctx->pop();
}
