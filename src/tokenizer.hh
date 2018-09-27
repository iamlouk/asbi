#ifndef TOKENIZER_HH
#define TOKENIZER_HH

#include <string>
#include <memory>

namespace tok {

	enum Type {
		Identifier, Int, Real, String, Bool, Nil,

		Assign, Plus, Minus, Mul, Div, Arrow, KeyValue,
		Equals, EqualsNot, Smaller, Bigger,
		Not, Or, And,

		LeftBracket, RightBracket,
		LeftCurlyBracket, RightCurlyBracket,
		LeftSquareBracket, RightSquareBracket,
		Comma, Semicolon, Colon, Dot, DoubleColon,

		Let, If, Else, For,

		EOFTok
	};

	struct Token {
		inline Token(Type type, unsigned int line):
			type(type), line(line), integer(0) {}
		inline Token(Type type, std::string* _str, unsigned int line):
			type(type), line(line), str(_str) {}
		inline Token(Type type, int _integer, unsigned int line):
			type(type), line(line), integer(_integer) {}
		inline Token(Type type, double _real, unsigned int line):
			type(type), line(line), real(_real) {}

		Type type;
		unsigned int line;
		union {
			std::string* str;
			int integer;
			double real;
		};
	};

	std::string to_string(Token);

	class Tokenizer {
	public:
		explicit Tokenizer(const std::string &source):
			pos(0), line(1), source(source), tokbuf(Token(EOFTok, 0)), tokbuffed(false) {}

		Token next(void);
		Token peek(void);
	private:
		Token next_string(void);
		Token next_number(void);

		unsigned long pos;
		unsigned int line;
		const std::string &source;
		Token tokbuf;
		bool tokbuffed;
	};
}

#endif
