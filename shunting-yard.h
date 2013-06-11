// Source: http://www.daniweb.com/software-development/cpp/code/427500/calculator-using-shunting-yard-algorithm#
// Author: Jesse Brown
// Modifications: Brandon Amos

#ifndef _SHUNTING_YARD_H
#define _SHUNTING_YARD_H

#include <iostream>
#include <stack>
#include <vector>
#include <string>
#include <cstdlib>
#include <map>
#include <stdexcept>

struct TokenBase { 
  virtual ~TokenBase() {}
};

template< class T > class Token : public TokenBase {
public:
  Token (T t) : val(t) {}
  T val;
private:
};

class RPNExpression {
public:
  void push(TokenBase *t);
  TokenBase* pop();
  bool empty() const;
private:
  std::vector< TokenBase* > stack_;
};


class ShuntingYard {
public:
  ShuntingYard (const std::string& infix);
  RPNExpression to_rpn();
private:
  const std::string expr_;
  RPNExpression rpn_;
  std::stack< std::string > op_stack_;
  mutable std::map< std::string, int > op_precedence_;
  int precedence (std::string op) const;
  int stack_precedence() const;
  void handle_left_paren();
  void handle_right_paren();
  void handle_op (std::string op);
  RPNExpression convert(const std::string &infix);
};

class calculator {
public:
  static double calculate (const std::string& expr);
private:
  static void consume(double value, std::stack<double>* operands);
  static void consume(std::string op, std::stack<double>* operands);
};

#endif // _SHUNTING_YARD_H
