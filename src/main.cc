#include <iostream>
#include <cassert>
#include <memory>
#include <fstream>
#include <cstring>
#include <stdexcept>
#include "include/types.hh"
#include "include/utils.hh"
#include "include/procenv.hh"

extern "C" {
	#include <stdlib.h>
	#include <stdio.h>
	#include <unistd.h>
	#include <readline/readline.h>
	#include <readline/history.h>
}

using namespace asbi;

#ifndef NDEBUG
namespace tests { void run(); }
#endif

static const char* get_prompt(char* buf, size_t bufsize) {
	char* wdbuf = getcwd(NULL, 0);
	snprintf(buf, bufsize - 1, "\033[0;34m[%s]:\033[0m\033[0;36m%s\033[0m \033[0;37m>_\033[0m ", "asbi"/*getenv("USER")*/, wdbuf);
	free(wdbuf);
	return buf;
}

static void repl(Context &ctx) {
	char* line = NULL;
	auto file = Value::string(".", &ctx);
	ctx.global_env->decl(ctx.names.__file, file);
	ctx.global_env->decl(ctx.names.__main, file);
	auto env = std::make_shared<Env>(&ctx, ctx.global_env);

	char prompt[256];
	while ((line = readline(get_prompt(prompt, sizeof(prompt))))) {
		if (line[0] == '\0') {
			free(line);
			continue;
		}

		try {
			auto res = ctx.run(line, env);
			std::cout << "\033[0;37m->\033[0m " << res.to_string(true) << std::endl;
			add_history(line);
		} catch (std::runtime_error e) {
			std::cerr << "\033[0;31mError:\033[0m " << e.what() << ' ';
			std::cerr << "\033[0;37m(C++ Exceptions cause Memory Leaks!)\033[0m" << '\n';
		}

		free(line);
	}

	free(line);
}

static void usage(const char *name) {
	std::cout << "usage: " << name << " [--eval <code...>] [--help] [<file> | --repl] [script-args...]" << '\n';
	std::cout << "\tASBI: A Stack Based Interpreter (version " << ASBI_VERSION << ", clang " << __clang_version__ << ")\n";
	std::cout << "\tGo look at README.md and examples/ for help.\n";
}

int main(int argc, const char *argv[]) {
	if (argc <= 1) {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	Context ctx;
	ctx.global_env->decl(ctx.names.__imports, Value::map(new MapContainer(&ctx)));

	for (int i = 1; i < argc; i++) {
		auto arg = argv[i];
		if (strcmp(arg, "--repl") == 0) {
			load_procenv(&ctx, argc, argv, i + 1);
			repl(ctx);
			break;
		} else if (strcmp(arg, "--eval") == 0 && i + 1 < argc) {
			std::string str = argv[++i];
			std::cout << ctx.run(str).to_string(true) << '\n';
		} else if (strcmp(arg, "--help") == 0) {
			usage(argv[0]);
#ifndef NDEBUG
		} else if (strcmp(arg, "--test") == 0) {
			tests::run();
#endif
		} else if (arg[0] != '-') {
			auto filepath = std::string(arg);
			filepath = utils::normalize(filepath);

			load_procenv(&ctx, argc, argv, i + 1);
			ctx.global_env->decl(ctx.names.__file, Value::string(filepath.c_str(), &ctx));
			ctx.global_env->decl(ctx.names.__main, Value::string(filepath.c_str(), &ctx));

			std::string content;
			utils::readfile(filepath, content);
			ctx.run(content);
			ctx.evtloop.start();
			break;
		} else {
			usage(argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	return EXIT_SUCCESS;
}
