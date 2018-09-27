#include <iostream>
#include <ctime>
#include <cstdlib>
#include <cmath>
#include "types.hh"
#include "context.hh"
#include "utils.hh"

using namespace asbi;

static Value macro_println(int n, Context* ctx, std::shared_ptr<Env>) {
	for (int i = 0; i < n; i++) {
		std::cout << ctx->pop().to_string(false);
	}
	std::cout << '\n' << std::flush;
	return Value::nil();
}

static Value macro_assert(int n, Context* ctx, std::shared_ptr<Env>) {
	auto msg = ctx->pop();
	auto val = ctx->pop();
	if (n == 2) {
		if (!(val == Value::boolean(true))) {
			std::cerr << "==asbi==: Assertion failed: " << val.to_string(true) << " (" << msg.to_string(false) << ")\n";
			throw std::runtime_error("assertion failed");
		}
		return Value::nil();
	}

	throw std::runtime_error("assert macro usage error");
}

static Value macro_mod(int n, Context* ctx, std::shared_ptr<Env>) {
	auto a = ctx->pop();
	auto b = ctx->pop();
	if (n != 2 || a.type != type_t::Number || b.type != type_t::Number)
		throw std::runtime_error("mod macro usage error");

	return Value::number(static_cast<int>(a._number) % static_cast<int>(b._number));
}

static Value macro_random(int n, Context*, std::shared_ptr<Env>) {
	if (n != 0)
		throw std::runtime_error("random macro usage error");

	return Value::number(static_cast<double>(rand()) / RAND_MAX);
}

static Value macro_toInt(int n, Context* ctx, std::shared_ptr<Env>) {
	auto num = ctx->pop();
	if (n != 1 || num.type != type_t::Number)
		throw std::runtime_error("toInt macro usage error");

	return Value::number(floor(num._number));
}

static Value macro_len(int n, Context* ctx, std::shared_ptr<Env>) {
	auto val = ctx->pop();
	if (n == 1 && val.type == type_t::Dict)
		return Value::number(val._dict->vecdata.size());

	if (n == 1 && val.type == type_t::String)
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

static Value macro_import(int n, Context* ctx, std::shared_ptr<Env> env) {
	auto arg = ctx->pop();
	auto __file = env->lookup(ctx->names.__file);
	auto __imports = ctx->global_env->lookup(ctx->names.__imports);
	// std::cerr << "import(arg: " << arg.to_string(true) << ", __file: " << __file.to_string(true) << ", __imports: " << __imports.to_string(true) << ")\n";
	if (n != 1 || arg.type != type_t::String || __file.type != type_t::String || __imports.type != type_t::Dict)
		throw std::runtime_error("import macro usage error or environment corruption");

	auto dir = utils::dirname(__file._string->data);
	auto path = utils::join(dir, arg._string->data);
	auto filepathvalue = Value::string(ctx->new_string(path));

	if (auto data = __imports._dict->get(filepathvalue); data.type != type_t::Nil)
		return data;

	// std::cout << "loading file: " << filepath << '\n';

	std::string content;
	utils::readfile(path, content);

	auto new_env = std::make_shared<Env>(ctx, ctx->global_env);
	new_env->decl(ctx->names.__file, filepathvalue);
	new_env->decl(ctx->names.exports, Value::dict(new DictContainer(ctx)));

	ctx->run(content, new_env);

	auto data = new_env->lookup(ctx->names.exports);
	__imports._dict->set(filepathvalue, data);
	return data;
}

void Context::load_macros() {
	static bool seeded = false;
	if (!seeded) {
		seeded = true;
		srand((unsigned)time(nullptr));
	}

	global_env->decl(this, "println",  Value::macro( macro_println ));
	global_env->decl(this, "assert",   Value::macro( macro_assert  ));
	global_env->decl(this, "mod",      Value::macro( macro_mod     ));
	global_env->decl(this, "random",   Value::macro( macro_random  ));
	global_env->decl(this, "toInt",    Value::macro( macro_toInt   ));
	global_env->decl(this, "len",      Value::macro( macro_len     ));
	global_env->decl(this, "__debug",  Value::macro( macro_debug   ));
	global_env->decl(this, "import",   Value::macro( macro_import  ));

}
