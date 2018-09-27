#ifndef PARSER_HH
#define PARSER_HH

#include <memory>
#include <vector>
#include "ast.hh"
#include "tokenizer.hh"
#include "context.hh"

namespace asbi {
	class Parser {
	public:
		explicit Parser(tok::Tokenizer, Context*);
		ast::Node* next();
		ast::Node* parse();
	private:
		tok::Tokenizer tokenizer;
		Context *ctx;

	    void expect(tok::Type);

	    ast::Node* parse_expr();
		ast::Node* parse_controllflow();
		ast::Node* parse_assign();
		ast::Node* parse_logic();
		ast::Node* parse_cmp();
		ast::Node* parse_addsub();
		ast::Node* parse_muldiv();
		ast::Node* parse_unary_ops();
		ast::Node* parse_factor();
		ast::Node* parse_final();

	    ast::Lambda* parse_lambda(std::vector<ast::Node*>);
		ast::Map* parse_map(ast::Node*);
		ast::Block* parse_block();
		void parse_arglist(std::vector<ast::Node*>&);
	};
}

#endif
