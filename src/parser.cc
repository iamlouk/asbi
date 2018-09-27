#include <vector>
#include <memory>
#include <map>
#include <utility>
#include "utils.hh"
#include "parser.hh"
#include "tokenizer.hh"
#include "ast.hh"

using namespace asbi;

Parser::Parser(tok::Tokenizer toker, Context* ctx): tokenizer(toker), ctx(ctx) {}

void Parser::expect(tok::Type type) {
	auto t = tokenizer.next();
	if (t.type != type)
		throw utils::parser_error("unexpected token", t);
}

ast::Node* Parser::parse() {
	std::vector<ast::Node*> parsed;
	while (tokenizer.peek().type != tok::EOFTok) {
		parsed.push_back(next());
	}

	return parsed.size() == 1 ? parsed[0] : new ast::Block(parsed);
}

ast::Node* Parser::next() {
	auto node = parse_expr();
	auto t = tokenizer.peek().type;
	if (t == tok::EOFTok)
		return node;

	if (t == tok::Semicolon) {
		tokenizer.next();
		return node;
	}

	throw utils::parser_error("expected ';' or EOF", tokenizer.peek());
}

ast::Node* Parser::parse_expr() {
	return parse_controllflow();
}

ast::Node* Parser::parse_controllflow() {
	auto t = tokenizer.peek().type;
	if (t == tok::If) {
		tokenizer.next();

		auto cond = parse_assign();
		expect(tok::LeftCurlyBracket);
		auto ifbody = parse_block();
		ast::Node* elsebody = nullptr;
		if (tokenizer.peek().type == tok::Else) {
			tokenizer.next();
			if (tokenizer.peek().type == tok::If) {
				elsebody = parse_controllflow();
			} else {
				expect(tok::LeftCurlyBracket);
				elsebody = parse_block();
			}
		}

		return new ast::If(cond, ifbody, elsebody);
	}
	if (t == tok::For) {
		tokenizer.next();

		auto expr1 = parse_assign();
		if (tokenizer.peek().type == tok::Semicolon) {
			tokenizer.next();
			auto expr2 = parse_assign();
			expect(tok::Semicolon);
			auto expr3 = parse_assign();
			expect(tok::LeftCurlyBracket);
			return new ast::For(expr1, expr2, expr3, parse_block());
		}

		expect(tok::LeftCurlyBracket);
		return new ast::For(nullptr, expr1, nullptr, parse_block());
	}
	return parse_assign();
}

ast::Node* Parser::parse_assign() {
	if (tokenizer.peek().type == tok::Let) {
		std::vector<std::pair<ast::Variable*, ast::Node*>> decls;
		do {
			tokenizer.next();
			auto node = parse_logic();
			auto var = dynamic_cast<ast::Variable*>(node);
			if (var == nullptr)
				throw utils::parser_error("expected Variable", node);

			ast::Node* val = nullptr;
			if (tokenizer.peek().type == tok::Assign) {
				tokenizer.next();
				val = parse_logic();
			}

			decls.push_back(std::make_pair(var, val));
		} while (tokenizer.peek().type == tok::Comma);

		return new ast::Let(std::move(decls));
	}

	auto left = parse_logic();
	if (tokenizer.peek().type == tok::Assign) {
		tokenizer.next();
		auto right = parse_expr();

		ast::Variable* var = dynamic_cast<ast::Variable*>(left);
		if (var != nullptr)
			return new ast::AssignVariable(var, right);

		ast::Access* acs = dynamic_cast<ast::Access*>(left);
		if (acs != nullptr)
			return new ast::AssignAccess(acs, right);

		ast::List* list = dynamic_cast<ast::List*>(left);
		if (list != nullptr) {
			auto node = new ast::DestructList(std::move(list->values), right);
			list->values.clear();
			delete list;
			return node;
		}

		ast::Map* map = dynamic_cast<ast::Map*>(left);
		if (map != nullptr) {
			/*std::vector<std::pair<ast::Variable*, ast::Node*>> vars;
			for (auto [key, val]: map->values) {
				auto var = dynamic_cast<ast::Variable*>(val);
				if (var == nullptr)
					throw utils::parser_error("only variables allowed in destructuring", left);

				vars.push_back(std::make_pair(var, key));
			}
			map->values.clear();
			delete map;
			return new ast::DestructMap(std::move(vars), right);*/

			throw utils::parser_error("unimplemented!", left);
		}

		throw utils::parser_error("one can only assign to variables (or destruct)", left);
	}
	return left;
}

ast::Node* Parser::parse_logic() {
	auto left = parse_cmp();
	auto t = tokenizer.peek().type;
	while (t == tok::Or || t == tok::And) {
		tokenizer.next();
		auto right = parse_cmp();
		if (t == tok::Or)
			left = new ast::InfixOperator(ast::InfixOperator::Or,  left, right);
		else
			left = new ast::InfixOperator(ast::InfixOperator::And, left, right);
		t = tokenizer.peek().type;
	}
	return left;
}

ast::Node* Parser::parse_cmp() {
	auto left = parse_addsub();
	auto t = tokenizer.peek().type;
	while (t == tok::Equals || t == tok::EqualsNot || t == tok::Smaller || t == tok::Bigger) {
		tokenizer.next();
		auto right = parse_addsub();
		if (t == tok::Equals)
			left = new ast::InfixOperator(ast::InfixOperator::Equals,    left, right);
		else if (t == tok::EqualsNot)
			left = new ast::InfixOperator(ast::InfixOperator::EqualsNot, left, right);
		else if (t == tok::Smaller)
			left = new ast::InfixOperator(ast::InfixOperator::Smaller,   left, right);
		else
			left = new ast::InfixOperator(ast::InfixOperator::Bigger,    left, right);
		t = tokenizer.peek().type;
	}
	return left;
}

ast::Node* Parser::parse_addsub() {
	auto left = parse_muldiv();
	auto t = tokenizer.peek().type;
	while (t == tok::Plus || t == tok::Minus) {
		tokenizer.next();
		auto right = parse_muldiv();
		if (t == tok::Plus)
			left = new ast::InfixOperator(ast::InfixOperator::Add, left, right);
		else
			left = new ast::InfixOperator(ast::InfixOperator::Sub, left, right);
		t = tokenizer.peek().type;
	}
	return left;
}

ast::Node* Parser::parse_muldiv() {
	auto left = parse_unary_ops();
	auto t = tokenizer.peek().type;
	while (t == tok::Mul || t == tok::Div) {
		tokenizer.next();
		auto right = parse_unary_ops();
		if (t == tok::Mul)
			left = new ast::InfixOperator(ast::InfixOperator::Mul, left, right);
		else
			left = new ast::InfixOperator(ast::InfixOperator::Div, left, right);
		t = tokenizer.peek().type;
	}
	return left;
}

ast::Node* Parser::parse_unary_ops() {
	auto t = tokenizer.peek().type;
	if (t == tok::Minus) {
		tokenizer.next();
		return new ast::PrefxOperator(ast::PrefxOperator::Neg, parse_factor());
	}
	if (t == tok::Not) {
		tokenizer.next();
		return new ast::PrefxOperator(ast::PrefxOperator::Not, parse_factor());
	}
	return parse_factor();
}

ast::Node* Parser::parse_factor() {
	auto node = parse_final();
	auto t = tokenizer.peek().type;
	while (t == tok::LeftBracket || t == tok::Dot || t == tok::Colon || t == tok::DoubleColon) {
		tokenizer.next();
		if (t == tok::LeftBracket) {
			std::vector<ast::Node*> args;
			parse_arglist(args);
			node = new ast::Call(node, args);
		} else if (t == tok::Colon) {
			auto token = tokenizer.next();
			if (token.type != tok::Identifier)
				throw utils::parser_error("expected identifier/symbol after ':'", token);

			node = new ast::Access(node, new ast::Symbol(ctx->new_stringconstant(*token.str)));
			delete token.str;
		} else if (t == tok::DoubleColon) {
			/*auto token = tokenizer.next();
			if (token.type != tok::Identifier)
				throw utils::parser_error("expected identifier/symbol after '::'", token);

			auto acs = new ast::Access(node, new ast::Symbol(token.str));
			expect(tok::LeftBracket);
			std::vector<ast::Node*> args;
			parse_arglist(args);
			node = new ast::OOCall(acs, args);*/
			throw std::runtime_error("unimplemented");
		} else {
			node = new ast::Access(node, parse_final());
		}
		t = tokenizer.peek().type;
	}
	return node;
}

ast::Node* Parser::parse_final() {
	tok::Token t = tokenizer.next();
	switch (t.type) {
	case tok::Int:
		return new ast::Number(static_cast<double>(t.integer));
	case tok::Real:
		return new ast::Number(t.real);
	case tok::Bool:
		return new ast::Bool(t.integer != 0 ? true : false);
	case tok::String:{
		auto str = new ast::String(ctx->new_stringconstant(*t.str));
		delete t.str;
		return str;
	}
	case tok::Identifier:{
		auto var = new ast::Variable(ctx->new_stringconstant(*t.str));
		delete t.str;
		return var;
	}
	case tok::Nil:
		return new ast::Nil();
	case tok::Colon:{
		t = tokenizer.next();
		if (t.type != tok::Identifier)
			throw utils::parser_error("expected identifier (symbol)", t);
		auto sym = new ast::Symbol(ctx->new_stringconstant(*t.str));
		delete t.str;
		return sym;
	}
	case tok::LeftBracket:{
		if (tokenizer.peek().type == tok::RightBracket) {
			tokenizer.next();
			return parse_lambda(std::vector<ast::Node*>());
		}

		auto node = parse_expr();
		t = tokenizer.next();
		if (t.type == tok::RightBracket) {
			if (tokenizer.peek().type == tok::Arrow) {
				std::vector<ast::Node*> vec = { node };
				return parse_lambda(vec);
			}
			return node;
		}

		std::vector<ast::Node*> vec = { node };
		while (t.type == tok::Comma) {
			if (tokenizer.peek().type == tok::RightBracket) {
				t = tokenizer.next();
				break;
			}
			vec.push_back(parse_expr());
			t = tokenizer.next();
		}

		if (t.type != tok::RightBracket)
			throw utils::parser_error("expected ',' or ')' while parsing tuple", t);

		if (tokenizer.peek().type == tok::Arrow)
			return parse_lambda(std::move(vec));

		throw utils::parser_error("tuples unimplemented", t);
	}
	case tok::LeftSquareBracket:{
		if (tokenizer.peek().type == tok::Comma) {
			tokenizer.next();
			expect(tok::RightSquareBracket);
			return new ast::List(std::vector<ast::Node*>());
		}

		if (tokenizer.peek().type == tok::KeyValue) {
			tokenizer.next();
			expect(tok::RightSquareBracket);
			return new ast::Map(std::vector<std::pair<ast::Node*,ast::Node*>>());
		}

		auto node = parse_expr();
		auto token = tokenizer.next();
		if (token.type == tok::KeyValue)
			return parse_map(node);

		std::vector<ast::Node*> vec = { node };
		while (token.type == tok::Comma) {
			vec.push_back(parse_expr());
			token = tokenizer.next();
		}

		if (token.type != tok::RightSquareBracket)
			throw utils::parser_error("expected ',' or ')' while parsing list", t);

		return new ast::List(std::move(vec));
	}
	case tok::LeftCurlyBracket:
		return parse_block();
	default:
		throw utils::parser_error("unexpected token", t);
	}
}

ast::Lambda* Parser::parse_lambda(std::vector<ast::Node*> argnamenodes) {
	expect(tok::Arrow);

	std::vector<asbi::StringContainer*> argnames;
	for (auto const& node: argnamenodes) {
		ast::Variable* var = dynamic_cast<ast::Variable*>(node);
		if (var == nullptr)
			throw utils::parser_error("lambda-litteral args should only be identifiers", node);

		argnames.push_back(var->sc);
		delete var;
	}

	ast::Block* body;
	auto _body = parse_expr();
	if (body = dynamic_cast<ast::Block*>(_body); body == nullptr) {
		std::vector<ast::Node*> vec = { _body };
		body = new ast::Block(vec);
	}

	return new ast::Lambda(std::move(argnames), body);
}

ast::Map* Parser::parse_map(ast::Node* firstkey) {
	std::vector<std::pair<ast::Node*,ast::Node*>> values;
	values.push_back(std::make_pair(firstkey, parse_expr()));
	auto t = tokenizer.next();
	while (t.type == tok::Comma) {
		auto key = parse_expr();
		expect(tok::KeyValue);
		auto value = parse_expr();
		values.push_back(std::make_pair(key, value));
		t = tokenizer.next();
	}
	if (t.type != tok::RightSquareBracket)
		throw utils::parser_error("expected ',' or ')' while parsing map", t);

	return new ast::Map(std::move(values));
}

ast::Block* Parser::parse_block() {
	if (tokenizer.peek().type == tok::RightCurlyBracket) {
		tokenizer.next();
		return new ast::Block(std::vector<ast::Node*>());
	}

	std::vector<ast::Node*> exprs;
	exprs.push_back(parse_expr());
	auto t = tokenizer.next();
	while (t.type == tok::Semicolon) {
		if (tokenizer.peek().type == tok::RightCurlyBracket) {
			t = tokenizer.next();
			break;
		}

		exprs.push_back(parse_expr());
		t = tokenizer.next();
	}

	if (t.type != tok::RightCurlyBracket)
		throw utils::parser_error("expected ';' or '}'", t);

	return new ast::Block(std::move(exprs));
}

void Parser::parse_arglist(std::vector<ast::Node*> &args) {
	if (tokenizer.peek().type == tok::RightBracket) {
		tokenizer.next();
		return;
	}

	args.push_back(parse_expr());
	auto token = tokenizer.next();
	while (token.type == tok::Comma) {
		args.push_back(parse_expr());
		token = tokenizer.next();
	}

	if (token.type != tok::RightBracket)
		throw utils::parser_error("expected ',' or ')' in arglist", token);
}
