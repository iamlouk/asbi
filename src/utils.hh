#ifndef UTILS_HH
#define UTILS_HH

#include <string>
#include <stdexcept>
#include "tokenizer.hh"

namespace ast { class Node;   }

namespace utils {

	class tokenizer_error: public std::runtime_error {
	public:
		tokenizer_error(const char* msg, int line, char c);

		virtual const char* what() const noexcept { return what_message.c_str(); }
		int line;
		char c;
		std::string what_message;
	};

	class parser_error: public std::runtime_error {
	public:
		parser_error(const char* msg, tok::Token t);
		parser_error(const char* msg, ast::Node* node);

		virtual const char* what() const noexcept { return what_message.c_str(); }
		tok::Token token;
		ast::Node* node;
		std::string what_message;
	};

	class interpreter_error: public std::runtime_error {
	public:
		interpreter_error(const char* msg, std::string name);

		virtual const char* what() const noexcept { return what_message.c_str(); }
		std::string what_message;
	};

	class type_error: public std::runtime_error {
	public:
		type_error(const char* msg, const ast::Node* node);
		type_error(const char* msg);

		virtual const char* what() const noexcept { return what_message.c_str(); }
		const ast::Node* node;
		std::string what_message;
	};

	std::string dirname(std::string &path);
	std::string normalize(std::string &path);
	void readfile(std::string &path, std::string &content);
	std::string join(std::string &a, std::string &b);

}

#endif
