#ifndef AST_HH
#define AST_HH

#include <string>
#include <memory>
#include <vector>
#include <utility>
#include "utils.hh"
#include "vm.hh"
#include "context.hh"
#include "types.hh"

namespace ast {
	class Node {
	public:
		virtual ~Node() = default;
		virtual Node* optimize(void) = 0;
		virtual void to_vmops(asbi::Context*, std::vector<asbi::OpCode>&) const { throw std::runtime_error("unimplemented!"); };
	};

	class Variable: public Node {
	public:
		explicit Variable(asbi::StringContainer* sc): sc(sc) {}
		asbi::StringContainer* sc;
		Node* optimize(void) override;
		void to_vmops(asbi::Context*, std::vector<asbi::OpCode>&) const override;
	};

	class Access: public Node {
	public:
		Access(Node* left, Node* right): left(left), right(right) {}
		~Access() { delete left; delete right; }
		Node *left, *right;
		Node* optimize(void) override;
		void to_vmops(asbi::Context*, std::vector<asbi::OpCode>&) const override;
	};

	class InfixOperator: public Node {
	public:
		enum optype_t {
			Add, Sub, Mul, Div, And, Or, Equals, EqualsNot, Bigger, BiggerOrEqual, Smaller, SmallerOrEqual
		} type;
		Node *lhs, *rhs;
		InfixOperator(optype_t type, Node* lhs, Node* rhs): type(type), lhs(lhs), rhs(rhs) {}
		~InfixOperator() { delete lhs; delete rhs; }
		Node* optimize(void) override;
		void to_vmops(asbi::Context*, std::vector<asbi::OpCode>&) const override;
	};

	class PrefxOperator: public Node {
	public:
		enum optype_t {
			Not, Neg
		} type;
		Node* operand;
		PrefxOperator(optype_t type, Node* operand): type(type), operand(operand) {}
		~PrefxOperator() { delete operand; }
		Node* optimize(void) override;
		void to_vmops(asbi::Context*, std::vector<asbi::OpCode>&) const override;
	};

	class VariableDecl: public Node {
	public:
		VariableDecl(std::vector<std::pair<Variable*, Node*>> decls): decls(decls) {}
		~VariableDecl();
		std::vector<std::pair<Variable*, Node*>> decls;
		Node* optimize(void) override;
		void to_vmops(asbi::Context*, std::vector<asbi::OpCode>&) const override;
	};

	class DestructList: public Node {
	public:
		DestructList(std::vector<Node*> lhss, Node* rhs): lhss(lhss), rhs(rhs) {}
		~DestructList();
		std::vector<Node*> lhss;
		Node* rhs;
		Node* optimize(void) override;
		void to_vmops(asbi::Context*, std::vector<asbi::OpCode>&) const override;
	};

	/*
	class DestructMap: public Node {
	public:
		DestructMap(std::vector<std::pair<Variable*, Node*>> vars, Node* val): vars(vars), val(val) {}
		~DestructMap();
		std::vector<std::pair<Variable*, Node*>> vars;
		Node* val;
		Node* optimize(void) override;
		// void to_vmops(asbi::Context*, std::vector<asbi::OpCode>&) const override;
	};
	*/

	class AssignVariable: public Node {
	public:
		AssignVariable(Variable* var, Node* val): var(var), val(val) {}
		~AssignVariable() { delete var; delete val; }
		Variable* var;
		Node* val;
		Node* optimize(void) override;
		void to_vmops(asbi::Context*, std::vector<asbi::OpCode>&) const override;
	};

	class AssignAccess: public Node {
	public:
		AssignAccess(Access* acs, Node* val): acs(acs), val(val) {}
		~AssignAccess() { delete acs; delete val; }
		Access* acs;
		Node* val;
		Node* optimize(void) override;
		void to_vmops(asbi::Context*, std::vector<asbi::OpCode>&) const override;
	};

	class If: public Node {
	public:
		If(Node* cond, Node* ifbody, Node* elsebody): cond(cond), ifbody(ifbody), elsebody(elsebody) {}
		~If();
		Node* cond;
		Node* ifbody;
		Node* elsebody;
		Node* optimize(void) override;
		void to_vmops(asbi::Context*, std::vector<asbi::OpCode>&) const override;
	};

	class For: public Node {
	public:
		For(Node* init, Node* cond, Node* inc, Node* body):
		 	init(init), cond(cond), inc(inc), body(body) {}
		~For();
		Node *init, *cond, *inc, *body;
		Node* optimize(void) override;
		void to_vmops(asbi::Context*, std::vector<asbi::OpCode>&) const override;
	};

	class Block: public Node {
	public:
		Block(std::vector<Node*> exprs): exprs(exprs) {}
		~Block();
		std::vector<Node*> exprs;
		Node* optimize(void) override;
		void to_vmops(asbi::Context*, std::vector<asbi::OpCode>&) const override;
	};


	class Nil: public Node {
	public:
		Nil() {}
		Node* optimize(void) override;
		void to_vmops(asbi::Context*, std::vector<asbi::OpCode>&) const override;
	};

	class Number: public Node {
	public:
		explicit Number(double val): value(val) {}
		double value;
		Node* optimize(void) override;
		void to_vmops(asbi::Context*, std::vector<asbi::OpCode>&) const override;
	};

	class Bool: public Node {
	public:
		explicit Bool(bool val): value(val) {}
		bool value;
		Node* optimize(void) override;
		void to_vmops(asbi::Context*, std::vector<asbi::OpCode>&) const override;
	};

	class String: public Node {
	public:
		explicit String(asbi::StringContainer* sc): sc(sc) {}
		mutable asbi::StringContainer* sc;
		Node* optimize(void) override;
		void to_vmops(asbi::Context*, std::vector<asbi::OpCode>&) const override;
	};

	class Symbol: public Node {
	public:
		explicit Symbol(asbi::StringContainer* sc): sc(sc) {}
		mutable asbi::StringContainer* sc;
		Node* optimize(void) override;
		void to_vmops(asbi::Context*, std::vector<asbi::OpCode>&) const override;
	};

	class List: public Node {
	public:
		List(std::vector<Node*> vals): values(vals) {}
		~List();
		std::vector<Node*> values;
		Node* optimize(void) override;
		void to_vmops(asbi::Context*, std::vector<asbi::OpCode>&) const override;
	};

	class Map: public Node {
	public:
		Map(std::vector<std::pair<Node*,Node*>> vals): values(vals) {}
		~Map();
		std::vector<std::pair<Node*,Node*>> values;
		Node* optimize(void) override;
		void to_vmops(asbi::Context*, std::vector<asbi::OpCode>&) const override;
	};

	class Lambda: public Node {
	public:
		Lambda(std::vector<asbi::StringContainer*> argnames, Block* body):
			argnames(argnames), body(body) {}
		~Lambda() { delete body; }
		std::vector<asbi::StringContainer*> argnames;
		Block* body;
		Node* optimize(void) override;
		void to_vmops(asbi::Context*, std::vector<asbi::OpCode>&) const override;
	};

	class Call: public Node {
	public:
		Call(Node* callable, std::vector<Node*> args): callable(callable), args(args) {}
		~Call();
		Node* callable;
		std::vector<Node*> args;
		Node* optimize(void) override;
		void to_vmops(asbi::Context*, std::vector<asbi::OpCode>&) const override;
	};

}

#endif
