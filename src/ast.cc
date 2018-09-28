#include <string>
#include <stdexcept>
#include <cassert>
#include "ast.hh"
#include "vm.hh"

using std::string;
using namespace ast;
using namespace asbi;

static_assert(sizeof(OpCode) == sizeof(void*));
static_assert(sizeof(double) == sizeof(OpCode));

// TODO: bei sprüngen -1 als OpCode?

void Number::to_vmops(Context*, std::vector<OpCode> &ops) const {
	ops.push_back(OpCode::PUSH_NUMBER);
	ops.push_back(*reinterpret_cast<const OpCode*>(&value));
}

void Bool::to_vmops(Context*, std::vector<OpCode> &ops) const {
	if (value)
		ops.push_back(OpCode::PUSH_TRUE);
	else
		ops.push_back(OpCode::PUSH_FALSE);
}

void Nil::to_vmops(Context*, std::vector<OpCode> &ops) const {
	ops.push_back(OpCode::PUSH_NIL);
}

void Symbol::to_vmops(Context*, std::vector<OpCode> &ops) const {
	ops.push_back(OpCode::PUSH_SYMBOL);
	ops.push_back(*reinterpret_cast<const OpCode*>(&sc));
}

void String::to_vmops(Context*, std::vector<OpCode> &ops) const {
	ops.push_back(OpCode::PUSH_STRING);
	ops.push_back(*reinterpret_cast<const OpCode*>(&sc));
}

void Lambda::to_vmops(Context* ctx, std::vector<OpCode> &ops) const {
	ops.push_back(OpCode::PUSH_LAMBDA);
	auto lambdaops = new std::vector<OpCode>();
	body->to_vmops(ctx, *lambdaops);
	auto lc = new LambdaContainer(nullptr, lambdaops, argnames, ctx, false);
	ctx->lambdas.push_back(lc);
	ops.push_back(*reinterpret_cast<const OpCode*>(&lc));
}

void InfixOperator::to_vmops(Context* ctx, std::vector<OpCode> &ops) const {
	switch (type) {
	case InfixOperator::Add:
		lhs->to_vmops(ctx, ops);
		rhs->to_vmops(ctx, ops);
		ops.push_back(OpCode::ADD);
		return;
	case InfixOperator::Sub:
		lhs->to_vmops(ctx, ops);
		rhs->to_vmops(ctx, ops);
		ops.push_back(OpCode::SUB);
		return;
	case InfixOperator::Mul:
		lhs->to_vmops(ctx, ops);
		rhs->to_vmops(ctx, ops);
		ops.push_back(OpCode::MUL);
		return;
	case InfixOperator::Div:
		lhs->to_vmops(ctx, ops);
		rhs->to_vmops(ctx, ops);
		ops.push_back(OpCode::DIV);
		return;
	case InfixOperator::Or:{
		// a | b <-> if a { true } else { b }
		lhs->to_vmops(ctx, ops);
		ops.push_back(OpCode::IF_TRUE_GOTO);
		auto pos_iftrue = ops.size();
		ops.push_back(OpCode::NOOP);
		rhs->to_vmops(ctx, ops);
		ops.push_back(OpCode::GOTO);
		auto pos_end = ops.size();
		ops.push_back(OpCode::NOOP);
		*(ops.data() + pos_iftrue) = static_cast<OpCode>(ops.size());
		ops.push_back(OpCode::PUSH_TRUE);
		*(ops.data() + pos_end) = static_cast<OpCode>(ops.size());
		return;
	}
	case InfixOperator::And:{
		// a & b <-> if a { b } else { false }
		lhs->to_vmops(ctx, ops);
		ops.push_back(OpCode::IF_TRUE_GOTO);
		auto pos_iftrue = ops.size();
		ops.push_back(OpCode::NOOP);
		ops.push_back(OpCode::PUSH_FALSE);
		ops.push_back(OpCode::GOTO);
		auto pos_end = ops.size();
		ops.push_back(OpCode::NOOP);
		*(ops.data() + pos_iftrue) = static_cast<OpCode>(ops.size());
		rhs->to_vmops(ctx, ops);
		*(ops.data() + pos_end) = static_cast<OpCode>(ops.size());
		return;
	}
	case InfixOperator::Equals:
		lhs->to_vmops(ctx, ops);
		rhs->to_vmops(ctx, ops);
		ops.push_back(OpCode::EQUALS);
		return;
	case InfixOperator::EqualsNot:
		lhs->to_vmops(ctx, ops);
		rhs->to_vmops(ctx, ops);
		ops.push_back(OpCode::EQUALS_NOT);
		return;
	case InfixOperator::Bigger:
		lhs->to_vmops(ctx, ops);
		rhs->to_vmops(ctx, ops);
		ops.push_back(OpCode::BIGGER);
		return;
	case InfixOperator::BiggerOrEqual:
		throw std::runtime_error("unimplemented!");
		return;
	case InfixOperator::Smaller:
		lhs->to_vmops(ctx, ops);
		rhs->to_vmops(ctx, ops);
		ops.push_back(OpCode::SMALLER);
		return;
	case InfixOperator::SmallerOrEqual:
		throw std::runtime_error("unimplemented!");
		return;
	}
}


void PrefxOperator::to_vmops(Context* ctx, std::vector<OpCode> &ops) const {
	switch (type) {
	case PrefxOperator::Neg:{
		ops.push_back(OpCode::PUSH_NUMBER);
		assert(sizeof(double) == sizeof(OpCode));
		double flt = 0.0;
		ops.push_back(*reinterpret_cast<const OpCode*>(&flt));
		operand->to_vmops(ctx, ops);
		ops.push_back(OpCode::SUB);
		return;
	}
	case PrefxOperator::Not:{
		operand->to_vmops(ctx, ops);
		ops.push_back(OpCode::NOT);
		return;
	}
	}
}


VariableDecl::~VariableDecl() {
	for (auto [var, val] : decls) {
		delete var;
		if (val != nullptr)
			delete val;
	}
}

void VariableDecl::to_vmops(asbi::Context* ctx, std::vector<asbi::OpCode> &ops) const {
	for (auto [var, val]: decls) {
		if (val == nullptr)
			ops.push_back(OpCode::PUSH_NIL);
		else
			val->to_vmops(ctx, ops);

		ops.push_back(OpCode::DECL);
		ops.push_back(*reinterpret_cast<const OpCode*>(&var->sc));
		ops.push_back(OpCode::POP);
	}
	ops.push_back(OpCode::PUSH_NIL);
}

void AssignVariable::to_vmops(asbi::Context* ctx, std::vector<asbi::OpCode> &ops) const {
	val->to_vmops(ctx, ops);
	ops.push_back(OpCode::SET);
	ops.push_back(*reinterpret_cast<const OpCode*>(&var->sc));
}

void Variable::to_vmops(asbi::Context*, std::vector<asbi::OpCode> &ops) const {
	ops.push_back(OpCode::LOOKUP);
	ops.push_back(*reinterpret_cast<const OpCode*>(&sc));
}

void DestructList::to_vmops(asbi::Context *ctx, std::vector<asbi::OpCode> &ops) const {
	for (auto rit = lhss.rbegin(); rit != lhss.rend(); ++rit) {
		if (auto var = dynamic_cast<ast::Variable*>(*rit); var != nullptr) {
			ops.push_back(OpCode::PUSH_STACK_PLACEHOLDER);
			ops.push_back(*reinterpret_cast<const OpCode*>(&var->sc));
		} else {
			(*rit)->to_vmops(ctx, ops);
		}
	}
	rhs->to_vmops(ctx, ops);
	ops.push_back(OpCode::DESTRUCT_ARRLIKE);
	ops.push_back(static_cast<OpCode>(lhss.size()));
}

DestructList::~DestructList() {
	for (auto lhs: lhss)
		delete lhs;
	delete rhs;
}

/*
DestructMap::~DestructMap() {
	for (auto [a, b]: vars) {
		delete a;
		delete b;
	}
	delete val;
}
*/

void If::to_vmops(Context* ctx, std::vector<OpCode> &ops) const {
	cond->to_vmops(ctx, ops);
	ops.push_back(OpCode::IF_TRUE_GOTO);
	ops.push_back(OpCode::NOOP);
	auto pos1 = ops.size() - 1;
	ops.push_back(OpCode::ENTER_SCOPE);
	if (elsebody)
		elsebody->to_vmops(ctx, ops);
	else
		ops.push_back(OpCode::PUSH_NIL);
	ops.push_back(OpCode::LEAVE_SCOPE);
	ops.push_back(OpCode::GOTO);
	ops.push_back(OpCode::NOOP);
	auto pos2 = ops.size() - 1;
	*(ops.data() + pos1) = static_cast<OpCode>(ops.size());
	ops.push_back(OpCode::ENTER_SCOPE);
	ifbody->to_vmops(ctx, ops);
	ops.push_back(OpCode::LEAVE_SCOPE);
	*(ops.data() + pos2) = static_cast<OpCode>(ops.size());
}

If::~If() {
	delete cond;
	delete ifbody;
	if (elsebody != nullptr)
		delete elsebody;
}


void For::to_vmops(Context* ctx, std::vector<OpCode> &ops) const {
	if (init) {
		init->to_vmops(ctx, ops);
		ops.push_back(OpCode::POP);
	}

	auto pos1 = ops.size();
	cond->to_vmops(ctx, ops);
	ops.push_back(OpCode::IF_FALSE_GOTO);
	ops.push_back(OpCode::NOOP);
	auto pos2 = ops.size() - 1;

	ops.push_back(OpCode::ENTER_SCOPE);
	body->to_vmops(ctx, ops);
	ops.push_back(OpCode::LEAVE_SCOPE);
	ops.push_back(OpCode::POP);

	if (inc) {
		inc->to_vmops(ctx, ops);
		ops.push_back(OpCode::POP);
	}

	ops.push_back(OpCode::GOTO);
	ops.push_back(static_cast<OpCode>(pos1));
	*(ops.data() + pos2) = static_cast<OpCode>(ops.size());
	ops.push_back(OpCode::PUSH_NIL);
}

For::~For() {
	if (init)
		delete init;
	delete cond;
	if (inc)
		delete inc;
	delete body;
}


void Block::to_vmops(Context* ctx, std::vector<OpCode> &ops) const {
	int n = exprs.size();
	if (n == 0) {
		ops.push_back(OpCode::PUSH_NIL);
		return;
	}

	for (int i = 0; i < n - 1; i++) {
		exprs[i]->to_vmops(ctx, ops);
		ops.push_back(OpCode::POP);
	}
	exprs[n - 1]->to_vmops(ctx, ops);
}


void List::to_vmops(Context* ctx, std::vector<OpCode> &ops) const {
	for (auto node: values) {
		node->to_vmops(ctx, ops);
	}

	ops.push_back(OpCode::MAKE_DICT_ARRLIKE);
	ops.push_back(static_cast<OpCode>(values.size()));
}


List::~List() {
	for (auto node : values)
		delete node;
}


void Map::to_vmops(Context* ctx, std::vector<OpCode> &ops) const {
	for (auto [key, val]: values) {
		val->to_vmops(ctx, ops);
		key->to_vmops(ctx, ops);
	}

	ops.push_back(OpCode::MAKE_DICT_MAPLIKE);
	ops.push_back(static_cast<OpCode>(values.size()));
}


Map::~Map() {
	for (auto pair : values) {
		delete pair.first;
		delete pair.second;
	}
}


void Access::to_vmops(Context* ctx, std::vector<OpCode> &ops) const {
	right->to_vmops(ctx, ops);
	left->to_vmops(ctx, ops);
	ops.push_back(OpCode::GET_DICT_VAL);
}


void AssignAccess::to_vmops(asbi::Context* ctx, std::vector<asbi::OpCode> &ops) const {
	acs->right->to_vmops(ctx, ops); // key
	val->to_vmops(ctx, ops);        // value
	acs->left->to_vmops(ctx, ops);  // dict
	ops.push_back(OpCode::SET_DICT_VAL);
}


void Call::to_vmops(asbi::Context* ctx, std::vector<asbi::OpCode> &ops) const {
	for (auto rit = args.rbegin(); rit != args.rend(); ++rit)
		(*rit)->to_vmops(ctx, ops);

	callable->to_vmops(ctx, ops);
	ops.push_back(OpCode::CALL);
	ops.push_back(static_cast<OpCode>(args.size()));
}


Call::~Call() {
	delete callable;
	for (auto arg: args)
		delete arg;
}


Block::~Block() {
	for (auto node : exprs)
		delete node;
}
