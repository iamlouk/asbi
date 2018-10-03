#include <iostream>
#include <cassert>
#include <memory>
#include <fstream>
#include <cstring>
#include <stdexcept>
#include "include/types.hh"
#include "include/utils.hh"
#include "include/procenv.hh"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

using namespace asbi;

#ifndef NDEBUG
namespace tests { void run(); }
#endif

static const char* get_prompt(char* buf, size_t bufsize) {
	char* wdbuf = getcwd(NULL, 0);
	snprintf(buf, bufsize - 1, "\033[0;34m%s[asbi]:\033[0m\033[0;36m%s\033[0m \033[0;37m>_\033[0m ", getenv("USER"), wdbuf);
	free(wdbuf);
	return buf;
}

static void repl(int argc, const char* argv[]) {
	char* line = NULL;

	Context ctx;
	auto file = Value::string(".", &ctx);
	ctx.global_env->decl(ctx.names.__file, file);
	ctx.global_env->decl(ctx.names.__main, file);
	load_procenv(&ctx, argc, argv);
	auto env = std::make_shared<Env>(&ctx, ctx.global_env);

	char prompt[256];
	while ((line = readline(get_prompt(prompt, sizeof(prompt))))) {
		if (line[0] == '\0') {
			free(line);
			continue;
		}

		std::string cppline = line;
		auto res = ctx.run(line, env);
		std::cout << "\033[0;37m->\033[0m " << res.to_string(true) << std::endl;
		add_history(line);
		free(line);
	}

	free(line);
}

static void usage(const char *name) {
	std::cout << "usage: " << name << " [--repl] [--file <file>] [--eval <code...>] [--help]" << '\n';
}

int main(int argc, const char *argv[]) {
	if (argc <= 1) {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	for (int i = 1; i < argc; i++) {
		auto arg = argv[i];
		if (strcmp(arg, "--repl") == 0) {
			repl(argc, argv);
		} else if (strcmp(arg, "--file") == 0 && i + 1 < argc) {
			auto cfilepath = argv[++i];
			auto filepath = std::string(cfilepath);
			filepath = utils::normalize(filepath);
			// std::cout << "loading file: " << filepath << '\n';

			Context ctx;
			auto filevalue = Value::string(utils::normalize(filepath).c_str(), &ctx);
			ctx.global_env->decl(ctx.names.__file, filevalue);
			ctx.global_env->decl(ctx.names.__main, filevalue);
			ctx.global_env->decl(ctx.names.__imports, Value::map(new MapContainer(&ctx)));

			load_procenv(&ctx, argc, argv);

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
