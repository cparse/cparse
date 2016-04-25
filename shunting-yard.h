// Source: http://www.daniweb.com/software-development/cpp/code/427500/calculator-using-shunting-yard-algorithm#
// Author: Jesse Brown
// Modifications: Brandon Amos, Vinícius Garcia

#ifndef _SHUNTING_YARD_H
#define _SHUNTING_YARD_H

#include <map>
#include <stack>
#include <string>
#include <queue>

enum tokType { NONE, OP, VAR, NUM, STR, MAP };

struct TokenBase {
  tokType type;
  virtual ~TokenBase() {}
  virtual TokenBase* clone() const = 0;
};

template<class T> class Token : public TokenBase {
public:
  T val;
  Token (T t, tokType type) : val(t) { this->type=type; }
  virtual TokenBase* clone() const {
    return new Token(static_cast<const Token&>(*this));
  }
};

#include "packToken.h"

typedef std::queue<TokenBase*> TokenQueue_t;
typedef std::map<std::string, packToken> TokenMap_t;
typedef std::map<std::string, int> OppMap_t;

class calculator {
private:
  static OppMap_t _opPrecedence;
  static OppMap_t buildOpPrecedence();

public:
  static TokenBase* calculate(const char* expr, TokenMap_t* vars = 0);

private:
  static TokenBase* calculate(TokenQueue_t RPN, TokenMap_t* vars = 0);
  static void cleanRPN(TokenQueue_t& rpn);
  static TokenQueue_t toRPN(const char* expr,
      TokenMap_t* vars,
      OppMap_t opPrecedence=_opPrecedence);

  static void handle_unary(const std::string& str,
    TokenQueue_t& rpnQueue, bool& lastTokenWasOp,
    OppMap_t& opPrecedence);
  static void handle_op(const std::string& str,
    TokenQueue_t& rpnQueue,
    std::stack<std::string>& operatorStack,
    OppMap_t& opPrecedence);

private:
  TokenQueue_t RPN;
public:
  ~calculator();
  calculator(){}
  calculator(const char* expr,
      TokenMap_t* vars = 0,
      OppMap_t opPrecedence=_opPrecedence);
  void compile(const char* expr,
      TokenMap_t* vars = 0,
      OppMap_t opPrecedence=_opPrecedence);
  TokenBase* eval(TokenMap_t* vars = 0);
  std::string str();
};

#endif // _SHUNTING_YARD_H
