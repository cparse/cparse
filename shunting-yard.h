// Source: http://www.daniweb.com/software-development/cpp/code/427500/calculator-using-shunting-yard-algorithm#
// Author: Jesse Brown
// Modifications: Brandon Amos

#ifndef _SHUNTING_YARD_H
#define _SHUNTING_YARD_H

#include <map>
#include <stack>
#include <string>
#include <queue>

enum tokType { NONE, OP, VAR, NUM };

struct TokenBase {
  tokType type;
  virtual ~TokenBase() {}
};

template<class T> class Token : public TokenBase {
public:
  Token (T t, tokType type) : val(t) { this->type=type; }
  T val;
};

typedef std::queue<TokenBase*> TokenQueue_t;

class calculator {
private:
  static std::map<std::string, int> opPrecedence;
  static std::map<std::string, int> buildOpPrecedence();

public:
  static double calculate(const char* expr,
      std::map<std::string, double>* vars = 0);

private:
  static double calculate(TokenQueue_t RPN,
      std::map<std::string, double>* vars = 0);
  static void cleanRPN(TokenQueue_t& rpn);
  static TokenQueue_t toRPN(const char* expr,
      std::map<std::string, double>* vars,
      std::map<std::string, int> opPrecedence=opPrecedence);

private:
  TokenQueue_t RPN;
public:
  ~calculator();
  calculator(){}
  calculator(const char* expr,
      std::map<std::string, double>* vars = 0,
      std::map<std::string, int> opPrecedence=opPrecedence);
  void compile(const char* expr,
      std::map<std::string, double>* vars = 0,
      std::map<std::string, int> opPrecedence=opPrecedence);
  double eval(std::map<std::string, double>* vars = 0);
  std::string str();
};

#endif // _SHUNTING_YARD_H
