#include <iostream>
#include <ctime>
#include <cstdlib>
#include <cmath>
#include <cassert>
#include "include/types.hh"
#include "include/context.hh"
#include "include/utils.hh"
#include "events/utils.hh"

extern "C" {
	#include <stdio.h>
	#include <readline/readline.h>
	#include <readline/history.h>
}

using namespace asbi;

static Value macro_assert(int n, Context* ctx, std::shared_ptr<Env>) {
	if (n == 2) {
		auto msg = ctx->pop();
		auto val = ctx->pop();
		if (!(val == Value::boolean(true))) {
			std::cerr << "==asbi==: Assertion failed: " << val.to_string(true) << " (" << msg.to_string(false) << ")\n";
			throw std::runtime_error("assertion failed");
		}
		return Value::nil();
	}

	throw std::runtime_error("assert macro usage error");
}

static Value macro_mod(int n, Context* ctx, std::shared_ptr<Env>) {
	if (n != 2)
		throw std::runtime_error("mod macro usage error");

	auto a = ctx->pop();
	auto b = ctx->pop();
	if (a.type != type_t::Number || b.type != type_t::Number)
		throw std::runtime_error("mod macro usage error");

	return Value::number(static_cast<int>(a._number) % static_cast<int>(b._number));
}

static Value macro_random(int n, Context*, std::shared_ptr<Env>) {
	if (n != 0)
		throw std::runtime_error("random macro usage error");

	return Value::number(static_cast<double>(rand()) / RAND_MAX);
}

static Value macro_toInt(int n, Context* ctx, std::shared_ptr<Env>) {
	if (n != 1)
		throw std::runtime_error("toInt macro usage error");

	auto num = ctx->pop();
	if (num.type != type_t::Number)
		throw std::runtime_error("toInt macro usage error");

	return Value::number(floor(num._number));
}

static Value macro_len(int n, Context* ctx, std::shared_ptr<Env>) {
	if (n != 1)
		throw std::runtime_error("len macro usage error");

	auto val = ctx->pop();
	if (val.type == type_t::Map)
		return Value::number(val._map->vecdata.size());

	if (val.type == type_t::String)
		return Value::number(val._string->data.size());

	throw std::runtime_error("len macro usage error");
}

static Value macro_debug(int n, Context* ctx, std::shared_ptr<Env>) {
	if (n != 1)
		throw std::runtime_error("__debug macro usage error");

	std::cerr << "==asbi==: Stack start" << '\n';
	for (unsigned int i = 0; i < ctx->stack.size(); ++i) {
		std::cerr << i << ": " << ctx->stack[i].to_string(true) << '\n';
	}
	std::cerr << "==asbi==: Stack end" << '\n';

	return ctx->pop();
}

static Value macro_scope(int n, Context* ctx, std::shared_ptr<Env> env) {
	if (n != 0)
		throw std::runtime_error("__scope macro usage error");

	return env->to_map(ctx);
}

static Value macro_import(int n, Context* ctx, std::shared_ptr<Env> env) {
	if (n != 1)
		throw std::runtime_error("import macro usage error or environment corruption");

	auto arg = ctx->pop();
	auto __file = env->lookup(ctx->names.__file);
	auto __imports = ctx->global_env->lookup(ctx->names.__imports);
	// std::cerr << "import(arg: " << arg.to_string(true) << ", __file: " << __file.to_string(true) << ", __imports: " << __imports.to_string(true) << ")\n";
	if (arg.type != type_t::String || __file.type != type_t::String || __imports.type != type_t::Map)
		throw std::runtime_error("import macro usage error or environment corruption");

	auto dir = utils::dirname(__file._string->data);
	auto path = utils::join(dir, arg._string->data);
	auto filepathvalue = Value::string(ctx->new_string(path));

	if (auto data = __imports._map->get(filepathvalue); data.type != type_t::Nil)
		return data;

	// std::cout << "loading file: " << filepath << '\n';

	std::string content;
	utils::readfile(path, content);

	auto new_env = std::make_shared<Env>(ctx, ctx->global_env);
	new_env->decl(ctx->names.__file, filepathvalue);
	new_env->decl(ctx->names.exports, Value::map(new MapContainer(ctx)));

	ctx->run(content, new_env);

	auto data = new_env->lookup(ctx->names.exports);
	__imports._map->set(filepathvalue, data);
	return data;
}

static Value macro_typeof(int n, Context* ctx, std::shared_ptr<Env>) {
	if (n != 1)
		throw std::runtime_error("typeof macro usage error");

	auto arg = ctx->pop();
	switch (arg.type) {
	case type_t::Bool:
		return Value::symbol("bool", ctx);
	case type_t::Number:
		return Value::symbol("number", ctx);
	case type_t::Nil:
		return Value::symbol("nil", ctx);
	case type_t::Symbol:
		return Value::symbol("symbol", ctx);
	case type_t::String:
		return Value::symbol("string", ctx);
	case type_t::Lambda:
		return Value::symbol("function", ctx);
	case type_t::Map:
		return Value::symbol("map", ctx);
	case type_t::Macro:
		return Value::symbol("function", ctx);
	case type_t::StackPlaceholder:
		assert(false);
		return Value::symbol("internal_stack_placeholder", ctx);
	}
}

static Value macro_reduce(int n, Context* ctx, std::shared_ptr<Env> env) {
	if (n != 3)
		throw std::runtime_error("reduce macro usage error");

	auto map = ctx->pop();
	auto acc = ctx->pop();
	auto fn  = ctx->pop();
	if (map.type != type_t::Map || fn.type != type_t::Lambda)
		throw std::runtime_error("reduce macro usage error");

	auto &vecdata = map._map->vecdata;
	for (unsigned int i = 0; i < vecdata.size(); ++i) {
		ctx->push(vecdata[i]);
		ctx->push(Value::number(i));
		ctx->push(acc);
		acc = fn.call(ctx, 3, env);
	}

	for (auto [key, value]: map._map->data) {
		ctx->push(value);
		ctx->push(key);
		ctx->push(acc);
		acc = fn.call(ctx, 3, env);
	}

	return acc;
}

static Value macro_map(int n, Context* ctx, std::shared_ptr<Env> env) {
	if (n != 2)
		throw std::runtime_error("map macro usage error");

	auto map = ctx->pop();
	auto fn  = ctx->pop();
	if (map.type != type_t::Map || fn.type != type_t::Lambda)
		throw std::runtime_error("map macro usage error");

	auto res = new MapContainer(ctx);
	ctx->push(Value::map(res));

	auto &vecdata = map._map->vecdata;
	for (unsigned int i = 0; i < vecdata.size(); ++i) {
		auto key = Value::number(i);
		ctx->push(vecdata[i]);
		ctx->push(key);
		res->set(key, fn.call(ctx, 2, env));
	}

	for (auto [key, value]: map._map->data) {
		ctx->push(value);
		ctx->push(key);
		res->set(key, fn.call(ctx, 2, env));
	}

	return ctx->pop(); // res muss auf stack liegen weil GC
}

static Value macro_io_readline(int n, Context* ctx, std::shared_ptr<Env>) {
	if (n != 1)
		throw std::runtime_error("io:readline macro usage error");

	auto lambda = ctx->pop();
	if (lambda.type != type_t::Lambda)
		throw std::runtime_error("io:readline macro usage error");

	ctx->evtloop.addTask(lambda._lambda, [ctx](evts::callback_t cb) -> void {
		char* line = readline("");
		std::vector<Value> args;
		if (line == NULL)
			args.push_back(Value::nil());
		else
			args.push_back(Value::string(line, ctx));
		cb(&args, true);
		free(line);
	});

	return Value::nil();
}

static Value macro_io_println(int n, Context* ctx, std::shared_ptr<Env>) {
	for (int i = 0; i < n; i++) {
		std::cout << ctx->pop().to_string(false);
	}
	std::cout << '\n' << std::flush;
	return Value::nil();
}

static Value macro_io_print(int n, Context* ctx, std::shared_ptr<Env>) {
	for (int i = 0; i < n; i++) {
		std::cout << ctx->pop().to_string(false);
	}
	std::cout << std::flush;
	return Value::nil();
}

static Value macro_time_now(int n, Context*, std::shared_ptr<Env>) {
	if (n != 0)
		throw std::runtime_error("time:now macro usage error");

	return Value::number(evts::to_double_seconds(evts::timestamp()));
}

static Value macro_time_runAt(int n, Context* ctx, std::shared_ptr<Env>) {
	if (n != 2)
		throw std::runtime_error("time:runat macro usage error");

	auto timestamp = ctx->pop();
	auto callback = ctx->pop();
	if (timestamp.type != type_t::Number || callback.type != type_t::Lambda)
		throw std::runtime_error("time:runat macro usage error");

	ctx->evtloop.addTimer(callback._lambda, timestamp._number);
	return Value::nil();
}

static Value macro_eval(int n, Context* ctx, std::shared_ptr<Env> callerenv) {
	if (n != 2)
		throw std::runtime_error("eval macro usage error");

	auto string = ctx->pop();
	auto envdepth = ctx->pop();
	if (string.type != type_t::String || envdepth.type != type_t::Number)
		throw std::runtime_error("eval macro usage error");

	auto env = callerenv;
	for (int i = 0; i < envdepth._number; i++) {
		if (callerenv->getOuter() == nullptr)
			break;

		env = callerenv->getOuter();
	}

	return ctx->run(string._string->data, env);
}

void Context::load_macros() {
	static bool seeded = false;
	if (!seeded) {
		seeded = true;
		srand((unsigned)time(nullptr));
	}

	global_env->decl(this, "assert",   Value::macro( macro_assert  ));
	global_env->decl(this, "mod",      Value::macro( macro_mod     ));
	global_env->decl(this, "random",   Value::macro( macro_random  ));
	global_env->decl(this, "toInt",    Value::macro( macro_toInt   ));
	global_env->decl(this, "len",      Value::macro( macro_len     ));
	global_env->decl(this, "__debug",  Value::macro( macro_debug   ));
	global_env->decl(this, "__scope",  Value::macro( macro_scope   ));
	global_env->decl(this, "import",   Value::macro( macro_import  ));
	global_env->decl(this, "typeof",   Value::macro( macro_typeof  ));
	global_env->decl(this, "reduce",   Value::macro( macro_reduce  ));
	global_env->decl(this, "map",      Value::macro( macro_map     ));
	global_env->decl(this, "eval",     Value::macro( macro_eval    ));

	auto io = new MapContainer(this);
	io->set(Value::symbol("print", this), Value::macro(macro_io_print));
	io->set(Value::symbol("println", this), Value::macro(macro_io_println));
	io->set(Value::symbol("readline", this), Value::macro(macro_io_readline));
	global_env->decl(this, "io", Value::map(io));

	auto time = new MapContainer(this);
	time->set(Value::symbol("now", this), Value::macro(macro_time_now));
	time->set(Value::symbol("runat", this), Value::macro(macro_time_runAt));
	global_env->decl(this, "time", Value::map(time));

}
