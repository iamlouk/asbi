#include <stdexcept>

#include "io.hh"
#include "utils.hh"
#include "loop.hh"
#include "../include/context.hh"
#include "../include/types.hh"

#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

using namespace asbi;

static Value macro_readline(int n, Context* ctx, std::shared_ptr<Env>) {
	auto lambda = ctx->pop();
	if (n != 1 || lambda.type != type_t::Lambda)
		throw std::runtime_error("readline macro usage error");

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

void asbi::load_evtio(Context* ctx) {
	ctx->global_env->decl(ctx, "readline", Value::macro( macro_readline ));
}
