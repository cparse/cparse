// Source: http://www.daniweb.com/software-development/cpp/code/427500/calculator-using-shunting-yard-algorithm#
// Author: Jesse Brown
// Modifications: Brandon Amos

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "shunting-yard.h"

int ShuntingYard::precedence(std::string op) const {
  return op_precedence_[op];
}

int ShuntingYard::stack_precedence() const { 
  if (op_stack_.empty()) {
    return -1;
  }
  return precedence(op_stack_.top());
}


#define isvariablechar(c) (isalpha(c) || c == '_')

TokenQueue_t ShuntingYard::convert(const std::string &infix) {
  const char* token = infix.c_str();
  while (*token && isspace(*token)) ++token;
  while (*token) {
    if (isdigit(*token)) {
      // If the token is a number, add it to the output queue.
      char* nextToken = 0;
      double digit = strtod(token, &nextToken);
#     ifdef DEBUG
        std::cout << digit << std::endl;
#     endif
      rpn_.push(new Token<double>(digit));
      token = nextToken;
    } else if (isvariablechar(*token)) {
      // If the function is a variable, resolve it and
      // add the parsed number to the output queue.
      if (!vars_) {
        throw std::domain_error(
            "Detected variable, but the variable map is null.");
      }

      std::stringstream ss;
      ss << *token;
      ++token;
      while (isvariablechar(*token)) {
        ss << *token;
        ++token;
      }
      std::string key = ss.str();
      std::map<std::string, double>::iterator it = vars_->find(key);
      if (it == vars_->end()) {
        throw std::domain_error(
            "Unable to find the variable '" + key + "'.");
      }
      double val = vars_->find(key)->second;
#     ifdef DEBUG
        std::cout << val << std::endl;
#     endif
      rpn_.push(new Token<double>(val));;
    } else {
      // Otherwise, the variable is an operator or paranthesis.
      switch (*token) {
        case '(':
          op_stack_.push("(");
          ++token;
          break;
        case ')':
          while (op_stack_.top().compare("(")) {
            rpn_.push(new Token<std::string>(op_stack_.top()));
            op_stack_.pop();
          }
          op_stack_.pop();
          ++token;
          break;
        default:
          {
            // Let p(o) denote the precedence of an operator o.
            //
            // If the token is an operator, o1, then
            //   While there is an operator token, o2, at the top
            //       and p(o1) <= p(o2), then
            //     pop o2 off the stack onto the output queue.
            //   Push o1 on the stack.
            std::stringstream ss;
            ss << *token;
            ++token;
            while (*token && !isspace(*token) && !isdigit(*token)
                && *token != '(' && *token != ')') {
              ss << *token;
              ++token;
            }
            ss.clear();
            std::string str;
            ss >> str;
#           ifdef DEBUG
              std::cout << str << std::endl;
#           endif

            while (!op_stack_.empty() &&
                precedence(str) <= stack_precedence()) {
              rpn_.push(new Token<std::string>(op_stack_.top()));
              op_stack_.pop();
            }
            op_stack_.push(str);
          }
      }
    }
    while (*token && isspace(*token)) ++token;
  }
  while (!op_stack_.empty()) {
    rpn_.push(new Token<std::string>(op_stack_.top()));
    op_stack_.pop();
  }
  return rpn_;
}

TokenQueue_t ShuntingYard::to_rpn() {
  return convert(expr_);
}

ShuntingYard::ShuntingYard (const std::string& infix,
    std::map<std::string, double>* vars) : expr_(infix), vars_(vars) {
  op_precedence_["("] = -1;
  op_precedence_["<<"] = 1; op_precedence_[">>"] = 1;
  op_precedence_["+"]  = 2; op_precedence_["-"]  = 2;
  op_precedence_["*"]  = 3; op_precedence_["/"]  = 3;
}

void calculator::consume(std::string op, std::stack<double>* operands) { 
  if (operands->size() < 2) {
    throw std::domain_error("Invalid equation.");
  }
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
  ShuntingYard shunting(expr, vars);
  TokenQueue_t rpn = shunting.to_rpn();

  std::stack<double> operands;
  while (!rpn.empty()) {
    TokenBase* base = rpn.front();
    rpn.pop();

    Token<std::string>* strTok = dynamic_cast<Token<std::string>*>(base);
    if (strTok) {
      consume(strTok->val, &operands);
      delete base;
      continue;
    }

    Token<double>* doubleTok = dynamic_cast<Token<double>*>(base);
    if (doubleTok) {
      operands.push(doubleTok->val);
      delete base;
      continue;
    }

    throw std::domain_error("Invalid token.");
  }
  return operands.top();
}
