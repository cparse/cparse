// Source: http://www.daniweb.com/software-development/cpp/code/427500/calculator-using-shunting-yard-algorithm#
// Author: Jesse Brown
// Modifications: Brandon Amos

#include <sstream>
#include "shunting-yard.h"

void RPNExpression::push(TokenBase *t) {
	stack_.push_back (t);
}

TokenBase* RPNExpression::pop() {
	TokenBase *t = stack_.front();
	stack_.erase (stack_.begin());
	return t;
}

bool RPNExpression::empty() const {
	return stack_.empty();
}

int ShuntingYard::precedence(std::string op) const {
	return op_precedence_[op];
}

int ShuntingYard::stack_precedence() const { 
	if (op_stack_.empty()) {
		return -1;
	}
	return precedence(op_stack_.top());
}

void ShuntingYard::handle_left_paren() {
	op_stack_.push("(");
}

void ShuntingYard::handle_right_paren() {
	while (op_stack_.top().compare("(")) {
		rpn_.push (new Token< std::string >(op_stack_.top()));
		op_stack_.pop();
	}
	op_stack_.pop();
}
void ShuntingYard::handle_op(std::string op) {
	while (! op_stack_.empty() &&
			precedence (op) <= stack_precedence()) {
		rpn_.push (new Token< std::string >(op_stack_.top()));
		op_stack_.pop();
	}
	op_stack_.push(op);
}

RPNExpression ShuntingYard::convert(const std::string &infix) {
	const char * token = infix.c_str();
	while (token && *token) {
		while (*token && isspace(*token)) { ++token; }
		if (!*token) { break; }
		if (isdigit(*token)) {
			char* next_token = 0;
			rpn_.push (new Token< double >( strtod(token, &next_token)));
			token = next_token;
		} else {
			char op = *token;
			switch (op) {
				case '(':
					handle_left_paren ();
					++token;
					break;
				case ')':
					handle_right_paren ();
					++token;
					break;
				default:
					{
						std::stringstream ss;
						ss << op;
						++token;
						while (!isspace(*token) && !isdigit(*token)) {
							ss << *token;
							++token;
						}
						handle_op(ss.str());
					}
			}
		}
	}
	while (!op_stack_.empty()) {
		rpn_.push (new Token< std::string >(op_stack_.top()));
		op_stack_.pop();
	}
	return rpn_;
}

ShuntingYard::ShuntingYard (const std::string& infix) : expr_(infix) {
	op_precedence_["("] = -1;
	op_precedence_["*"] = 5; op_precedence_["/"] = 5;
	op_precedence_["+"] = 6; op_precedence_["-"] = 6;
	op_precedence_["<<"] = 7; op_precedence_[">>"] = 7;
}

RPNExpression ShuntingYard::to_rpn() {
	return convert (expr_);
}

double calculator::pop() { 
	double d = operands_.top();
	operands_.pop();
	return d; 
}

void calculator::push (double d) {
	operands_.push (d);
}

double calculator::result() const {
	return operands_.top();
}

void calculator::flush() { 
	while (! operands_.empty()) { operands_.pop(); }
}

void calculator::consume(double value) {
	push(value);
}

void calculator::consume(std::string op) { 
	if (!op.compare("+")) {
		push (pop() + pop());
	} else if (!op.compare("*")) {
		push (pop() * pop());
	} else if (!op.compare("-")) {
		double right = pop();
		push (pop() - right);
	} else if (!op.compare("/")) {
		double right = pop();
		push (pop() / right);
	} else if (!op.compare("<<")) {
		int right = (int) pop();
		push ((int) pop() << right);
	} else if (!op.compare(">>")) {
		int right = (int) pop();
		push ((int) pop() >> right);
	} else {
		throw std::domain_error("Unknown Operator");
	}
} 

double calculator::calculate(const std::string& expr) { 
	ShuntingYard shunting(expr);
	RPNExpression rpn = shunting.to_rpn();
	flush();
	while (!rpn.empty()) { 
		TokenBase * token = rpn.pop();
		token->evaluate(this);
		delete token;
	}
	return result();
}

template< class T > void Token< T >::evaluate(calculator * c) {
	c->consume(token_);
}

// TODO: Delete.
// int main() {
// 	calculator c;
// 	std::cout << c.calculate ("(20+10)*3/2-3") << std::endl;
// 	std::cout << c.calculate ("1 << 4") << std::endl;
// 	return 0;
// }
