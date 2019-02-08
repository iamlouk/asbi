#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <memory>
#include <stdexcept>
#include "include/procenv.hh"
#include "include/context.hh"
#include "events/loop.hh"
#include "include/types.hh"

extern "C" {
	#include <sys/types.h>
	#include <sys/wait.h>
	#include <unistd.h>
}

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
	if (n != 2)
		throw std::runtime_error("env:set usage error");

	auto name = ctx->pop();
	auto value = ctx->pop();
	if (name.type != type_t::String || value.type != type_t::String)
		throw std::runtime_error("env:set usage error");

	if(setenv(name._string->data.c_str(), value._string->data.c_str(), 1) != 0)
		throw std::runtime_error(std::strerror(errno));

	return Value::nil();
}

static Value get_env_var(int n, Context* ctx, std::shared_ptr<Env>) {
	if (n != 1)
		throw std::runtime_error("env:get usage error");

	auto name = ctx->pop();
	if (name.type != type_t::String)
		throw std::runtime_error("env:get usage error");

	const char* value = std::getenv(name._string->data.c_str());
	return value == NULL ? Value::nil() : Value::string(value, ctx);
}

static Value macro_system(int n, Context* ctx, std::shared_ptr<Env>) {
	if (n != 2)
		throw std::runtime_error("env:$ usage error");

	auto arg = ctx->pop();
	auto callback = ctx->pop();
	if (arg.type != type_t::String || callback.type != type_t::Lambda)
		throw std::runtime_error("env:$ usage error");

	std::string cmd = arg._string->data;
	auto task = [ctx, cmd](evts::callback_t cb) -> void {
		pid_t pid = fork();
		if (pid == -1) {
			std::vector<Value> args;
			args.push_back(Value::symbol(ctx->names.err));
			args.push_back(Value::number(errno));
			cb(&args, true);
			return;
		}

		if (pid == 0) {
			const char* shell = getenv("SHELL");
			shell = shell != NULL ? shell : "/bin/sh";
			execl(shell, shell, "-c", cmd.c_str(), NULL);
			return;
		}

		int wstatus;
		if (waitpid(pid, &wstatus, 0) == -1) {
			// TODO...
			throw std::runtime_error("waitpid failed");
		}

		std::vector<Value> args;
		if (WIFEXITED(wstatus)) {
			args.push_back(Value::symbol(ctx->names.ok));
			args.push_back(Value::number(WEXITSTATUS(wstatus)));
		} else {
			args.push_back(Value::symbol(ctx->names.err));
			args.push_back(Value::number(WTERMSIG(wstatus)));
		}
		cb(&args, true);
	};

	ctx->evtloop.addTask(callback._lambda, task);

	return Value::nil();
}

static Value macro_exit(int n, Context* ctx, std::shared_ptr<Env>) {
	if (n != 1)
		throw std::runtime_error("exit macro usage error");

	auto exitcode = ctx->pop();
	if (exitcode.type != type_t::Number)
		throw std::runtime_error("exit macro usage error");

	std::exit(static_cast<unsigned int>(exitcode._number));
}

void asbi::load_procenv(Context* ctx, int argc, const char* argv[], int scriptArgs) {
	auto env = new MapContainer(ctx);
	auto args = new MapContainer(ctx);
	for (int i = scriptArgs, j = 0; i < argc; ++i, ++j)
		args->set(Value::number(j), Value::string(ctx->new_string(argv[i])));

	env->set(Value::symbol("all",  ctx), Value::macro(all_env_vars));
	env->set(Value::symbol("set",  ctx), Value::macro(set_env_var));
	env->set(Value::symbol("get",  ctx), Value::macro(get_env_var));
	env->set(Value::symbol("argv", ctx), Value::map(args));
	env->set(Value::symbol("$",    ctx), Value::macro(macro_system));
	env->set(Value::symbol("exit", ctx), Value::macro(macro_exit));
	ctx->global_env->decl(ctx, "env", Value::map(env));
}
