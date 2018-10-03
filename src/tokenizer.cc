#include <string>
#include <memory>
#include <iostream>
#include <map>

#include "include/utils.hh"
#include "include/tokenizer.hh"

using namespace tok;

static std::map<std::string, Token> keywords = {
	{ "if",     Token(If, 0)      },
	{ "else",   Token(Else, 0)    },
	{ "for",    Token(For, 0)     },
	{ "true",   Token(Bool, 1, 0) },
	{ "false",  Token(Bool, 0, 0) },
	{ "nil",    Token(Nil, 0)     },
};

static bool is_identifier(char c) {
	// 0-9 nicht als erstes zeichen eines identifiers
	return (c == '_' || c == '$' || c == '@' || ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9'));
}

Token Tokenizer::next_string() {
	std::string *str = new std::string("");
	pos++;
	while (pos < source.length()) {
		char c = source[pos++];
		if (c == '\\') {
			char c = source[pos++];
			switch (c) {
			case 'n': c = '\n'; break;
			case 't': c = '\t'; break;
			case 'r': c = '\r'; break;
			default:
				throw utils::tokenizer_error("not escapeable", line, c);
			}
		} else if (c == '"')
			break;
		*str += c;
	}
	return Token(String, str, line);
}

Token Tokenizer::next_number() {
	std::string str = "";
	bool contains_dot = false;
	char c = source[pos];
	while (('0' <= c && c <= '9') || c == '.') {
		str += c;
		if (c == '.')
			contains_dot = true;
		if (++pos >= source.length())
			break;
		c = source[pos];
	}

	if (contains_dot)
		return Token(Real, std::stod(str, nullptr), line);
	else
		return Token(Int, std::stoi(str, nullptr, 10), line);
}

Token Tokenizer::peek() {
	if (tokbuffed)
		return tokbuf;

	tokbuf = next();
	tokbuffed = true;
	return tokbuf;
}

Token Tokenizer::next() {
	if (tokbuffed) {
		tokbuffed = false;
		return tokbuf;
	}

	if (pos >= source.length())
		return Token(EOFTok, line);

	char c = source[pos];
	switch (c) {
	case '\n':
		line += 1;
	case ' ': case '\t': case '\v':
		pos++;
		return next();
	case '#':
		while (pos < source.length() && source[pos] != '\n') pos++;
		line++;
		pos++;
		return next();
	case '"':
		return next_string();
	case '(':
		pos++;
		return Token(LeftBracket, line);
	case ')':
		pos++;
		return Token(RightBracket, line);
	case '{':
		pos++;
		return Token(LeftCurlyBracket, line);
	case '}':
		pos++;
		return Token(RightCurlyBracket, line);
	case '[':
		pos++;
		return Token(LeftSquareBracket, line);
	case ']':
		pos++;
		return Token(RightSquareBracket, line);
	case '-':
		pos++;
		if (pos < source.length() && source[pos] == '>') {
			pos++;
			return Token(Arrow, line);
		}
		return Token(Minus, line);
	case '+':
		pos++;
		return Token(Plus, line);
	case '*':
		pos++;
		return Token(Mul, line);
	case '/':
		pos++;
		if (pos < source.length() && source[pos] == '/') {
			while (pos < source.length() && source[pos] != '\n') pos++;
			line++;
			pos++;
			return next();
		}
		if (pos < source.length() && source[pos] == '*') {
			pos++;
			while (pos + 1 < source.length() && !(source[pos] == '*' && source[pos + 1] == '/')) {
				if (source[pos++] == '\n') {
					line++;
				}
			}
			pos += 2;
			return next();
		}
		return Token(Div, line);
	case '!':
		pos++;
		if (pos < source.length() && source[pos] == '=') {
			pos++;
			return Token(EqualsNot, line);
		}
		return Token(Not, line);
	case '=':
		pos++;
		if (pos < source.length() && source[pos] == '=') {
			pos++;
			return Token(Equals, line);
		}
		return Token(Assign, line);
	case '|':
		pos++;
		return Token(Or, line);
	case '&':
		pos++;
		return Token(And, line);
	case ',':
		pos++;
		return Token(Comma, line);
	case ':':
		pos++;
		if (pos < source.length() && source[pos] == '=') {
			pos++;
			return Token(Declare, line);
		}
		if (pos < source.length() && source[pos] == ':') {
			pos++;
			return Token(DoubleColon, line);
		}
		return Token(Colon, line);
	case '<':
		pos++;
		if (pos < source.length() && source[pos] == '=') {
			pos++;
			return Token(SmallerOrEqual, line);
		}
		return Token(Smaller, line);
	case '>':
		pos++;
		if (pos < source.length() && source[pos] == '=') {
			pos++;
			return Token(BiggerOrEqual, line);
		}
		return Token(Bigger, line);
	case ';':
		pos++;
		return Token(Semicolon, line);
	case '.':
		pos++;
		return Token(Dot, line);
	case '~':
		pos++;
		return Token(KeyValue, line);
	default:
		if ('0' <= c && c <= '9')
			return next_number();

		if (!is_identifier(c))
			throw utils::tokenizer_error("unkown char", line, source[pos]);

		std::string* str = new std::string("");
		while (pos < source.length()) {
			char c = source[pos];
			if (!is_identifier(c))
				break;

			*str += c;
			pos++;
		}

		auto mappos = keywords.find(*str);
		if (mappos == keywords.end()) {
			return Token(Identifier, str, line);
		} else {
			delete str;
			mappos->second.line = line;
			return mappos->second;
		}
	}
}


std::string tok::to_string(Token token) {
	switch (token.type) {
	case Identifier:
		return std::string("Identifier(") + *(token.str) + ")";
	case Int:
	 	return std::string("Int(") + std::to_string(token.integer) + ")";
	case Real:
		return std::string("Real(") + std::to_string(token.real) + ")";
	case String:
		return std::string("String(\"") + *(token.str) + "\")";
	case Bool:
		return std::string("Bool(") + (token.integer ? "true" : "false") + ")";
	case Nil:       return std::string("Nil");
	case Assign:    return std::string("Assign");
	case Declare:   return std::string("Declare");
	case Plus:      return std::string("Plus");
	case Minus:     return std::string("Minus");
	case Mul:       return std::string("Mul");
	case Div:       return std::string("Div");
	case Arrow:     return std::string("Arrow");
	case KeyValue:  return std::string("KeyValue");
	case Equals:    return std::string("Equals");
	case EqualsNot: return std::string("EqualsNot");
	case Smaller:   return std::string("Smaller");
	case Bigger:    return std::string("Bigger");
	case Not:       return std::string("Not");
	case Or:        return std::string("Or");
	case And:       return std::string("And");
	case SmallerOrEqual:
		return std::string("SmallerOrEqual");
	case BiggerOrEqual:
		return std::string("BiggerOrEqual");
	case LeftBracket:
		return std::string("LeftBracket");
	case RightBracket:
		return std::string("RightBracket");
	case LeftCurlyBracket:
		return std::string("LeftCurlyBracket");
	case RightCurlyBracket:
		return std::string("RightCurlyBracket");
	case LeftSquareBracket:
		return std::string("LeftSquareBracket");
	case RightSquareBracket:
		return std::string("RightSquareBracket");
	case DoubleColon:
		return std::string("DoubleColon");
	case Comma:     return std::string("Comma");
	case Semicolon: return std::string("Semicolon");
	case Colon:     return std::string("Colon");
	case Dot:       return std::string("Dot");
	case If:        return std::string("If");
	case Else:      return std::string("Else");
	case For:       return std::string("For");
	case EOFTok:    return std::string("EOF");
	}
}
