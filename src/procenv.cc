#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <memory>
#include <stdexcept>
#include "procenv.hh"
#include "context.hh"
#include "types.hh"

extern char** environ;

using namespace asbi;

static Value all_env_vars(int n, Context* ctx, std::shared_ptr<Env>) {
	if (n != 0)
		throw std::runtime_error("env:all usage error");

	auto vars = new MapContainer(ctx);
	for (int i = 0; environ[i] != NULL; ++i) {
		const char* str = environ[i];
		const char* pos = std::strchr(str, '=');
		if (pos == NULL)
			throw std::runtime_error("unexpected environment variable format");

		std::size_t len = std::abs(str - pos);
		std::string name(str, len);
		std::string value(str + len + 1);
		vars->set(Value::string(name, ctx), Value::string(value, ctx));
	}
	return Value::map(vars);
}

static Value set_env_var(int n, Context* ctx, std::shared_ptr<Env>) {
	auto name = ctx->pop();
	auto value = ctx->pop();
	if (n != 2 || name.type != type_t::String || value.type != type_t::String)
		throw std::runtime_error("env:set usage error");

	if(setenv(name._string->data.c_str(), value._string->data.c_str(), 1) != 0)
		throw std::runtime_error(std::strerror(errno));

	return Value::nil();
}

static Value get_env_var(int n, Context* ctx, std::shared_ptr<Env>) {
	auto name = ctx->pop();
	if (n != 1 || name.type != type_t::String)
		throw std::runtime_error("env:get usage error");

	const char* value = std::getenv(name._string->data.c_str());
	return value == NULL ? Value::nil() : Value::string(value, ctx);
}

void asbi::load_procenv(Context* ctx, int argc, const char* argv[]) {
	auto env = new MapContainer(ctx);
	auto args = new MapContainer(ctx);
	for (int i = 0; i < argc; i++) {
		args->set(Value::number(i), Value::string(ctx->new_string(argv[i])));
	}

	env->set(Value::symbol("all",  ctx), Value::macro(all_env_vars));
	env->set(Value::symbol("set",  ctx), Value::macro(set_env_var));
	env->set(Value::symbol("get",  ctx), Value::macro(get_env_var));
	env->set(Value::symbol("argv", ctx), Value::map(args));
	ctx->global_env->decl(ctx, "env", Value::map(env));
}
