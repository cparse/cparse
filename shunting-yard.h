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

class calculator {
public:
  double calculate (const std::string& expr);
  template< class T > friend class Token;
private:
  std::stack< double > operands_;
  double pop ();
  void push (double d);
  double result () const;
  void flush ();
protected:
  void consume(double value);
  void consume(std::string op);
};

struct TokenBase { 
  virtual void evaluate (calculator *) = 0; 
  virtual ~TokenBase() {}
};

template< class T > class Token : public TokenBase {
public:
  void evaluate (calculator  *c);
  Token (T t) : token_(t) {}
private:
  T token_;
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
  RPNExpression to_rpn ();
private:
  const std::string expr_;
  RPNExpression rpn_;
  std::stack< std::string > op_stack_;
  mutable std::map< std::string, int > op_precedence_;
  int precedence (std::string op) const;
  int stack_precedence () const;
  void handle_left_paren ();
  void handle_right_paren ();
  void handle_op (std::string op);
  RPNExpression convert(const std::string &infix);
};

#endif // _SHUNTING_YARD_H
