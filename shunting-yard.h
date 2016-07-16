#ifndef SHUNTING_YARD_H_
#define SHUNTING_YARD_H_

#include <map>
#include <stack>
#include <string>
#include <queue>
#include <list>

enum tokType { NONE, OP, VAR, NUM, STR, MAP, FUNC, TUPLE, REF = 0x10 };
typedef unsigned char uint8_t;

struct TokenBase {
  uint8_t type;
  virtual ~TokenBase() {}
  virtual TokenBase* clone() const = 0;
};

template<class T> class Token : public TokenBase {
 public:
  T val;
  Token(T t, uint8_t type) : val(t) { this->type = type; }
  virtual TokenBase* clone() const {
    return new Token(static_cast<const Token&>(*this));
  }
};

struct TokenNone : public TokenBase {
  TokenNone() { this->type = NONE; }
  virtual TokenBase* clone() const {
    return new TokenNone(static_cast<const TokenNone&>(*this));
  }
};

class packToken;
typedef std::queue<TokenBase*> TokenQueue_t;
typedef std::map<std::string, packToken> TokenMap_t;
typedef std::map<std::string, int> OppMap_t;
typedef std::list<TokenBase*> Tuple_t;

struct RefToken : public TokenBase {
  std::string name;
  TokenBase* value;
  TokenMap_t* source_map;
  RefToken(std::string n, TokenBase* v, TokenMap_t* m, uint8_t type) :
    name(n), value(v), source_map(m) { this->type = type; }
  RefToken(std::string n, TokenBase* v, uint8_t type) :
    name(n), value(v), source_map(0) { this->type = type; }

  virtual TokenBase* clone() const {
    RefToken* copy = new RefToken(static_cast<const RefToken&>(*this));
    copy->value = value->clone();
    return copy;
  }
};

#include "./packToken.h"

// Define the `Function` class
// as well as some built-in functions:
#include "./functions.h"

class Scope {
 public:
  static const Scope empty;
  static TokenMap_t default_global;

 public:
  typedef std::list<TokenMap_t*> Scope_t;
  mutable Scope_t scope;

  Scope(TokenMap_t* vars);
  Scope() : Scope(NULL) {}

  packToken* find(std::string key) const;
  void assign(std::string key, TokenBase* value) const;

  void push(TokenMap_t* vars) const;
  void push(Scope vars) const;
  void pop() const;
  void pop(unsigned N) const;

  void clean() const;
  unsigned size() const;
};

class calculator {
 private:
  static OppMap_t _opPrecedence;
  static OppMap_t buildOpPrecedence();

 public:
  static packToken calculate(const char* expr, const Scope& vars = Scope::empty,
                             const char* delim = 0, const char** rest = 0);

 private:
  static packToken calculate(TokenQueue_t RPN,
                             const Scope* vars);
  static void cleanRPN(TokenQueue_t* rpn);
  static TokenQueue_t toRPN(const char* expr,
                            const Scope* vars, const char* delim = 0, const char** rest = 0,
                            OppMap_t opPrecedence = _opPrecedence);

  static bool handle_unary(const std::string& op,
                           TokenQueue_t* rpnQueue, bool lastTokenWasOp,
                           OppMap_t opPrecedence);
  static void handle_op(const std::string& op,
                        TokenQueue_t* rpnQueue,
                        std::stack<std::string>* operatorStack,
                        OppMap_t opPrecedence);

 private:
  TokenQueue_t RPN;

 public:
  ~calculator();
  calculator() {}
  calculator(const calculator& calc);
  calculator(const char* expr, const Scope& vars = Scope::empty,
             const char* delim = 0, const char** rest = 0,
             OppMap_t opPrecedence = _opPrecedence);
  void compile(const char* expr,
               const Scope& vars = Scope::empty, const char* delim = 0,
               const char** rest = 0, OppMap_t opPrecedence = _opPrecedence);
  packToken eval(const Scope& vars = Scope::empty);

  // Serialization:
  std::string str() const;
  static std::string str(TokenQueue_t rpn);

  // Operators:
  calculator& operator = (const calculator& calc);
};

#endif  // SHUNTING_YARD_H_
