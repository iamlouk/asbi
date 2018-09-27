#include <iostream>
#include <cassert>
#include <memory>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include "ast.hh"
#include "mem.hh"
#include "parser.hh"
#include "tokenizer.hh"
#include "types.hh"
#include "vm.hh"
#include "utils.hh"

using namespace asbi;

#ifndef NDEBUG
namespace tests { void run(); }
#endif

static void repl() {
	auto prompt = "\033[0m[asbi]\033[0m \033[0;36m>_\033[0m ";
	std::cout << prompt << std::flush;
	std::string line;

	Context ctx;
	auto file = Value::string(".", &ctx);
	ctx.global_env->decl(ctx.names.__file, file);
	ctx.global_env->decl(ctx.names.__main, file);
	auto env = std::make_shared<Env>(&ctx, ctx.global_env);

	while (std::getline(std::cin, line)) {
		auto res = ctx.run(line, env);
		std::cout << "\033[0;37m->\033[0m " << res.to_string(true) << '\n';
		std::cout << prompt << std::flush;
	}
}

static void usage(const char *name) {
	std::cout << "usage: " << name << " [--repl] [--file <file>] [--eval <code...>] [--help]" << '\n';
}

int main(int argc, char const *argv[]) {
	if (argc <= 1) {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	for (int i = 1; i < argc; i++) {
		auto arg = argv[i];
		if (strcmp(arg, "--repl") == 0) {
			repl();
		} else if (strcmp(arg, "--file") == 0 && i + 1 < argc) {
			auto cfilepath = argv[++i];
			auto filepath = std::string(cfilepath);
			filepath = utils::normalize(filepath);
			// std::cout << "loading file: " << filepath << '\n';

			Context ctx;
			auto filevalue = Value::string(utils::normalize(filepath).c_str(), &ctx);
			ctx.global_env->decl(ctx.names.__file, filevalue);
			ctx.global_env->decl(ctx.names.__main, filevalue);
			ctx.global_env->decl(ctx.names.__imports, Value::dict(new DictContainer(&ctx)));

			std::string content;
			utils::readfile(filepath, content);
			ctx.run(content);
		} else if (strcmp(arg, "--eval") == 0 && i + 1 < argc) {
			Context ctx;
			std::string str = argv[++i];
			std::cout << ctx.run(str).to_string(true) << '\n';
		} else if (strcmp(arg, "--help") == 0) {
			usage(argv[0]);
#ifndef NDEBUG
		} else if (strcmp(arg, "--test") == 0) {
			tests::run();
#endif
		} else {
			usage(argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	return EXIT_SUCCESS;
}
