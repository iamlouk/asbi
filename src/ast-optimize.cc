#include <string>
#include <stdexcept>
#include <iostream>
#include <utility>
#include "include/ast.hh"

using namespace ast;

/*
 * TODO:
 * - if und for optimieren und dabei auf memory leaks achten
 *
 */

Node* InfixOperator::optimize() {
	switch (type) {
	case InfixOperator::Add:{
		auto a = dynamic_cast<Number*>(lhs = lhs->optimize());
		auto b = dynamic_cast<Number*>(rhs = rhs->optimize());
		if (a != nullptr && b != nullptr) {
			auto res = a->value + b->value;
			delete this;
			return new ast::Number(res);
		}
		return this;
	}
	case InfixOperator::Sub:{
		auto a = dynamic_cast<Number*>(lhs = lhs->optimize());
		auto b = dynamic_cast<Number*>(rhs = rhs->optimize());
		if (a != nullptr && b != nullptr) {
			auto res = a->value - b->value;
			delete this;
			return new ast::Number(res);
		}
		return this;
	}
	case InfixOperator::Mul:{
		auto a = dynamic_cast<Number*>(lhs = lhs->optimize());
		auto b = dynamic_cast<Number*>(rhs = rhs->optimize());
		if (a != nullptr && b != nullptr) {
			auto res = a->value * b->value;
			delete this;
			return new ast::Number(res);
		}
		return this;
	}
	case InfixOperator::Div:{
		auto a = dynamic_cast<Number*>(lhs = lhs->optimize());
		auto b = dynamic_cast<Number*>(rhs = rhs->optimize());
		if (a != nullptr && b != nullptr) {
			auto res = a->value / b->value;
			delete this;
			return new ast::Number(res);
		}
		return this;
	}
	case InfixOperator::Or:{
		/*auto a = dynamic_cast<Bool*>(lhs = lhs->optimize());
		auto b = dynamic_cast<Bool*>(rhs = rhs->optimize());
		if (a != nullptr && b != nullptr) {
			auto res = a->value || b->value;
			delete this;
			return new ast::Bool(res);
		}*/
		lhs = lhs->optimize();
		rhs = rhs->optimize();
		return this;
	}
	case InfixOperator::And:{
		/*auto a = dynamic_cast<Bool*>(lhs = lhs->optimize());
		auto b = dynamic_cast<Bool*>(rhs = rhs->optimize());
		if (a != nullptr && b != nullptr) {
			auto res = a->value && b->value;
			delete this;
			return new ast::Bool(res);
		}*/
		lhs = lhs->optimize();
		rhs = rhs->optimize();
		return this;
	}
	case InfixOperator::Equals:{
		lhs = lhs->optimize();
		rhs = rhs->optimize();
		return this;
	}
	case InfixOperator::EqualsNot:{
		lhs = lhs->optimize();
		rhs = rhs->optimize();
		return this;
	}
	case InfixOperator::Bigger:{
		auto a = dynamic_cast<Number*>(lhs = lhs->optimize());
		auto b = dynamic_cast<Number*>(rhs = rhs->optimize());
		if (a != nullptr && b != nullptr) {
			auto res = a->value > b->value;
			delete this;
			return new ast::Bool(res);
		}
		return this;
	}
	case InfixOperator::BiggerOrEqual:{
		auto a = dynamic_cast<Number*>(lhs = lhs->optimize());
		auto b = dynamic_cast<Number*>(rhs = rhs->optimize());
		if (a != nullptr && b != nullptr) {
			auto res = a->value >= b->value;
			delete this;
			return new ast::Bool(res);
		}
		return this;
	}
	case InfixOperator::Smaller:{
		auto a = dynamic_cast<Number*>(lhs = lhs->optimize());
		auto b = dynamic_cast<Number*>(rhs = rhs->optimize());
		if (a != nullptr && b != nullptr) {
			auto res = a->value < b->value;
			delete this;
			return new ast::Bool(res);
		}
		return this;
	}
	case InfixOperator::SmallerOrEqual:{
		auto a = dynamic_cast<Number*>(lhs = lhs->optimize());
		auto b = dynamic_cast<Number*>(rhs = rhs->optimize());
		if (a != nullptr && b != nullptr) {
			auto res = a->value <= b->value;
			delete this;
			return new ast::Bool(res);
		}
		return this;
	}
	}
}

Node* PrefxOperator::optimize() {
	switch (type) {
	case PrefxOperator::Neg:{
		auto a = dynamic_cast<Bool*>(operand = operand->optimize());
		if (a != nullptr) {
			auto res = !a->value;
			delete this;
			return new ast::Bool(res);
		}
		return this;
	}
	case PrefxOperator::Not:{
		auto a = dynamic_cast<Number*>(operand = operand->optimize());
		if (a != nullptr) {
			auto res = -a->value;
			delete this;
			return new ast::Number(res);
		}
		return this;
	}
	}
}

Node* VariableDecl::optimize() {
	for (auto &pair : decls)
		if (pair.second != nullptr)
			pair.second = pair.second->optimize();
	return this;
}
Node* DestructList::optimize() {
	/*
	val = val->optimize();
	if (auto list = dynamic_cast<List*>(val); list != nullptr) {
		if (vars.size() != list->values.size())
			throw std::runtime_error("optimizer error");

		std::vector<std::pair<Variable*, Node*>> decls;
		for (unsigned int i = 0; i < vars.size(); ++i)
			decls.push_back(std::make_pair(vars[i], list->values[i]));

		list->values.clear();
		vars.clear();
		delete this;
		return new Let(std::move(decls));
	}
	*/
	rhs = rhs->optimize();
	return this;
}
/*
Node* DestructMap::optimize() {
	val = val->optimize();
	return this;
}
*/
Node* AssignVariable::optimize() {
	val = val->optimize();
	return this;
}
Node* AssignAccess::optimize() {
	acs = dynamic_cast<Access*>(acs->optimize());
	if (acs == nullptr)
		throw std::runtime_error("optimizer error");

	val = val->optimize();
	return this;
}
Node* If::optimize() {
	cond = cond->optimize();
	ifbody = ifbody->optimize();
	if (elsebody != nullptr)
		elsebody = elsebody->optimize();

	return this;
}
Node* For::optimize() {
	if (init) init = init->optimize();
	cond = cond->optimize();
	if (inc) inc = inc->optimize();
	body = body->optimize();
	return this;
}
Node* Nil::optimize() { return this; }
Node* Number::optimize() { return this; }
Node* Bool::optimize() { return this; }
Node* String::optimize() { return this; }
Node* List::optimize() {
	for (auto it = values.begin(); it != values.end(); ++it)
		*it = (*it)->optimize();

	return this;
}
Node* Map::optimize() {
	// TODO: doppelte entfernen und funktioniert das mit den Referenzen?
	for (auto& pair : values) {
		pair.first = pair.first->optimize();
		pair.second = pair.second->optimize();
	}
	return this;
}
Node* Variable::optimize() { return this; }
Node* Symbol::optimize() { return this; }
Node* Lambda::optimize() {
	for (auto it = body->exprs.begin(); it != body->exprs.end(); ++it)
		*it = (*it)->optimize();

	return this;
}
Node* Call::optimize() {
	callable = callable->optimize();
	for (auto it = args.begin(); it != args.end(); ++it)
		*it = (*it)->optimize();

	return this;
}
Node* Access::optimize() {
	left = left->optimize();
	right = right->optimize();
	return this;
}
Node* Block::optimize() {
	for (auto it = exprs.begin(); it != exprs.end(); ++it)
		*it = (*it)->optimize();

	return this;
}
