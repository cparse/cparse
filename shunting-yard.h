// Source: http://www.daniweb.com/software-development/cpp/code/427500/calculator-using-shunting-yard-algorithm#
// Author: Jesse Brown
// Modifications: Brandon Amos

#ifndef _SHUNTING_YARD_H
#define _SHUNTING_YARD_H

#include <map>
#include <stack>
#include <string>
#include <queue>

struct TokenBase { 
  virtual ~TokenBase() {}
};

template< class T > class Token : public TokenBase {
public:
  Token (T t) : val(t) {}
  T val;
};

typedef std::queue<TokenBase*> tokenQueue_t;

class ShuntingYard {
public:
  ShuntingYard (const std::string& infix,
      std::map<std::string, double>* vars = 0);
  tokenQueue_t to_rpn();
private:
  int precedence (std::string op) const;
  int stack_precedence() const;
  tokenQueue_t convert(const std::string &infix);

  const std::string expr_;
  std::map<std::string, double>* vars_;
  tokenQueue_t rpn_;
  std::stack< std::string > op_stack_;
  mutable std::map< std::string, int > op_precedence_;
};

class calculator {
public:
  static double calculate(const std::string& expr,
      std::map<std::string, double>* vars = 0);
private:
  static void consume(std::string op, std::stack<double>* operands);
};

#endif // _SHUNTING_YARD_H
