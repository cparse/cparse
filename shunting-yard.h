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

template<class T> class Token : public TokenBase {
public:
  Token (T t) : val(t) {}
  T val;
};

typedef std::queue<TokenBase*> TokenQueue_t;

class calculator {
public:
  static double calculate(const char* expr,
      std::map<std::string, double>* vars = 0);
private:
  static TokenQueue_t toRPN(const char* expr,
      std::map<std::string, double>* vars,
      std::map<std::string, int> opPrecedence);
};

#endif // _SHUNTING_YARD_H
