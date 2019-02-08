#include <cassert>
#include <stdexcept>
#include "include/vm.hh"
#include "include/context.hh"

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
			ctx->push(callable.call(ctx, n, env));
			break;
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

			if (a.type == type_t::String/* && b.type == type_t::String*/) {
				auto sc = ctx->new_string(a._string->data);
				sc->data += b/*._string->data*/.to_string(false);
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
		case SMALLER_OR_EQUAL:{
			auto b = ctx->pop();
			auto a = ctx->pop();
			if (a.type != type_t::Number || b.type != type_t::Number)
				throw std::runtime_error("expected number");
			ctx->push(Value::boolean(a._number <= b._number));
			break;
		}
		case BIGGER_OR_EQUAL:{
			auto b = ctx->pop();
			auto a = ctx->pop();
			if (a.type != type_t::Number || b.type != type_t::Number)
				throw std::runtime_error("expected number");
			ctx->push(Value::boolean(a._number >= b._number));
			break;
		}
		case NOT:{
			auto a = ctx->pop();
			if (a.type != type_t::Bool)
				throw std::runtime_error("expected boolean");
			ctx->push(Value::boolean(!a._boolean));
			break;
		}
		case MAKE_MAP:{
			auto n = static_cast<unsigned int>(opcodes[pc++]);
			auto mc = new MapContainer(ctx);
			for (unsigned int i = 0; i < n; ++i) {
				mc->set(ctx->pop(), ctx->pop());
			}

			ctx->push(Value::map(mc));
			ctx->heap_size += mc->gc_size();
			ctx->check_gc(env);
			break;
		}
		case MAKE_MAP_ARRLIKE:{
			auto n = static_cast<unsigned int>(opcodes[pc++]);
			auto mc = new MapContainer(ctx);
			for (unsigned int i = 0; i < n; ++i) {
				mc->set(Value::number(n - 1- i), ctx->pop());
			}

			ctx->push(Value::map(mc));
			ctx->heap_size += mc->gc_size();
			ctx->check_gc(env);
			break;
		}
		case GET_MAP_VAL:{
			auto map = ctx->pop();
			if (map.type != type_t::Map)
				throw std::runtime_error("expected map");

			ctx->push(map._map->get(ctx->pop()));
			break;
		}
		case SET_MAP_VAL:{
			auto map = ctx->pop();
			if (map.type != type_t::Map)
				throw std::runtime_error("expected map");

			auto val = ctx->pop();
			auto key = ctx->pop();
			auto prevsize = map._map->gc_size();
			map._map->set(key, val);
			if (prevsize < map._map->gc_size()) {
				ctx->heap_size -= prevsize;
				ctx->heap_size += map._map->gc_size();
				ctx->push(val);
				ctx->check_gc(env);
				break;
			}
			ctx->push(val);
			break;
		}
		case DESTRUCT_ARRLIKE:{
			auto n = static_cast<unsigned int>(opcodes[pc++]);
			auto map = ctx->pop();
			if (map.type != type_t::Map)
				throw std::runtime_error("expected map");

			for (unsigned int i = 0; i < n; ++i) {
				auto tomatch = ctx->pop();
				auto value = map._map->get(Value::number(i));
				if (tomatch.type == type_t::StackPlaceholder) {
					env->decl(tomatch._string, value);
				} else {
					if (!(tomatch == value)) {
						throw std::runtime_error("match error in destruction");
					}
				}
			}
			ctx->push(map);
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
