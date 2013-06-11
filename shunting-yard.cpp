// Source: http://www.daniweb.com/software-development/cpp/code/427500/calculator-using-shunting-yard-algorithm#
// Author: Jesse Brown
// Modifications: Brandon Amos

#include <sstream>
#include "shunting-yard.h"

void RPNExpression::push(TokenBase *t) {
  stack_.push_back(t);
}

TokenBase* RPNExpression::pop() {
  TokenBase *t = stack_.front();
  stack_.erase(stack_.begin());
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
    rpn_.push(new Token< std::string >(op_stack_.top()));
    op_stack_.pop();
  }
  op_stack_.push(op);
}

RPNExpression ShuntingYard::convert(const std::string &infix) {
  const char* token = infix.c_str();
  while (token && *token) {
    while (*token && isspace(*token)) { ++token; }
    if (!*token) { break; }
    if (isdigit(*token)) {
      char* next_token = 0;
      rpn_.push(new Token<double>(strtod(token, &next_token)));
      token = next_token;
    } if (isalpha(*token)) {
      if (!vars_) {
        std::cerr << "Error: Detected variable, " <<
          "but the variable map is null." << std::endl;
        exit(42);
      }

      std::stringstream ss;
      ss << *token;
      ++token;
      while (isalpha(*token) || *token == '_') {
        ss << *token;
        ++token;
      }
      std::string key = ss.str();
      std::map<std::string, double>::iterator it = vars_->find(key);
      if (it == vars_->end()) {
        std::cerr << "Error: Unable to find the variable '" <<
          key << "'." << std::endl;
        exit(42);
      }
      double val = vars_->find(key)->second;
      rpn_.push(new Token<double>(val));;
    } else {
      char op = *token;
      if (op == '\0') break;
      switch (op) {
        case '(':
          handle_left_paren();
          ++token;
          break;
        case ')':
          handle_right_paren();
          ++token;
          break;
        default:
          {
            std::stringstream ss;
            ss << op;
            ++token;
            while (*token && !isspace(*token) && !isdigit(*token)) {
              ss << *token;
              ++token;
            }
            ss.clear();
            std::string str;
            ss >> str;
            if (str[0] != '\0') {
              handle_op(str);
            }
          }
      }
    }
  }
  while (!op_stack_.empty()) {
    rpn_.push(new Token< std::string >(op_stack_.top()));
    op_stack_.pop();
  }
  return rpn_;
}

RPNExpression ShuntingYard::to_rpn() {
  return convert(expr_);
}

ShuntingYard::ShuntingYard (const std::string& infix,
    std::map<std::string, double>* vars) : expr_(infix), vars_(vars) {
  op_precedence_["("] = -1;
  op_precedence_["<<"] = 1; op_precedence_[">>"] = 1;
  op_precedence_["+"]  = 2; op_precedence_["-"]  = 2;
  op_precedence_["*"]  = 3; op_precedence_["/"]  = 3;
}

void calculator::consume(double value, std::stack<double>* operands) {
  operands->push(value);
}

void calculator::consume(std::string op, std::stack<double>* operands) { 
  double right = operands->top(); operands->pop();
  double left  = operands->top(); operands->pop();
  if (!op.compare("+")) {
    operands->push(left + right);
  } else if (!op.compare("*")) {
    operands->push(left * right);
  } else if (!op.compare("-")) {
    operands->push(left - right);
  } else if (!op.compare("/")) {
    operands->push(left / right);
  } else if (!op.compare("<<")) {
    operands->push((int) left << (int) right);
  } else if (!op.compare(">>")) {
    operands->push((int) left >> (int) right);
  } else {
    throw std::domain_error("Unknown operator: '" + op + "'.");
  }
} 

double calculator::calculate(const std::string& expr,
    std::map<std::string, double>* vars) { 
  std::stack<double> operands;
  ShuntingYard shunting(expr, vars);
  RPNExpression rpn = shunting.to_rpn();
  while (!operands.empty()) { operands.pop(); }
  while (!rpn.empty()) {
    TokenBase* base = rpn.pop();
    Token<std::string>* strTok = dynamic_cast<Token<std::string>*>(base);
    if (strTok) {
      consume(strTok->val, &operands);
      delete base;
      continue;
    }

    Token<double>* doubleTok = dynamic_cast<Token<double>*>(base);
    if (doubleTok) {
      consume(doubleTok->val, &operands);
      delete base;
      continue;
    }

    std::cerr << "Invalid token." << std::endl;
    exit(42);
  }
  return operands.top();
}
