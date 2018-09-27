#include <string>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include "utils.hh"
#include "tokenizer.hh"
#include "ast.hh"

namespace utils {

	const int PATH_MAX = 4096;

	tokenizer_error::tokenizer_error(const char* msg, int line, char c):
		std::runtime_error(msg), line(line), c(c), what_message("TokenizerError: ") {
		what_message += msg;
		what_message += " (line: ";
		what_message += std::to_string(line);
		what_message += ", '";
		what_message += c;
		what_message += "')";
	}

	parser_error::parser_error(const char* msg, tok::Token t):
		std::runtime_error(msg), token(t), node(nullptr), what_message("SyntaxError: ") {
		what_message += msg;
		what_message += " (near token: ";
		what_message += tok::to_string(t);
		what_message += ", line: ";
		what_message += std::to_string(t.line);
		what_message += ")";
	}

	parser_error::parser_error(const char* msg, ast::Node* node):
		std::runtime_error(msg), token(tok::Token(tok::EOFTok, 0)), node(node), what_message("SyntaxError: ") {
		what_message += msg;
		/*what_message += " (near ast node: ";
		node->to_string(what_message);
		what_message += ")";*/
	}

	interpreter_error::interpreter_error(const char* msg, std::string name):
		std::runtime_error(msg), what_message("RuntimeError: ") {
		what_message += msg;
		what_message += " (";
		what_message += name;
		what_message += ")";
	}

	type_error::type_error(const char* msg, const ast::Node* node):
		std::runtime_error(msg), node(node), what_message("TypeError: ") {
		what_message += msg;
		/*what_message += " (near ast node: ";
		node->to_string(what_message);
		what_message += ")";*/
	}

	type_error::type_error(const char* msg):
		std::runtime_error(msg), node(nullptr), what_message("TypeError: ") {
		what_message += msg;
	}

	std::string dirname(std::string &path) {
		auto i = path.find_last_of("/");
		if (i == std::string::npos)
			return ".";

		return path.substr(0, i);
	}
	std::string normalize(std::string &path) {
		char normalized[PATH_MAX + 1];
		if (realpath(path.c_str(), normalized) == nullptr) {
			throw std::runtime_error("normalize path error");
		}
		return std::string(normalized);
	}

	void readfile(std::string &path, std::string &content) {
		content.clear();
		std::ifstream ifs(path);
		if (!ifs.good())
			throw std::runtime_error("readfile error");

		content.assign((std::istreambuf_iterator<char>(ifs)),(std::istreambuf_iterator<char>()));
	}

	std::string join(std::string &a, std::string &b) {
		std::string path = a + "/" + b;
		return normalize(path);
	}

}
