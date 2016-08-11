#ifndef SHUNTING_YARD_H_
#define SHUNTING_YARD_H_

#include <map>
#include <stack>
#include <string>
#include <queue>
#include <list>

enum tokType {
  // Base types:
  NONE, OP, VAR, NUM, STR, FUNC,

  // Complex types:
  IT = 0x20,     // Everything with the bit 0x20 set is an iterator.
  TUPLE = 0x21,  // == 0x20 + 0x01 => Tuples are iterators.
  LIST = 0x22,   // == 0x20 + 0x01 => Lists are iterators.
  MAP = 0x60,    // == 0x20 + 0x40 => Maps are Iterators
                 // Everything with the bit 0x40 set is a MAP.
  REF = 0x80
};

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
typedef std::map<std::string, int> OppMap_t;
typedef std::list<TokenBase*> Tuple_t;

// The pack template manages
// reference counting.
#include "./pack.h"

class TokenMap;

class TokenList;
typedef pack<TokenList> packList;

class Function;
#include "./packToken.h"

// Define the Tuple, TokenMap and TokenList classes:
#include "./objects.h"

// Define the `Function` class
// as well as some built-in functions:
#include "./functions.h"

struct RefToken : public TokenBase {
  packToken key;
  TokenBase* value;
  packToken source;
  RefToken(packToken k, TokenBase* v, packToken m) :
    key(k), value(v), source(m) { this->type = v->type | REF; }
  RefToken(packToken k, TokenBase* v) :
    key(k), value(v), source(packToken::None) { this->type = v->type | REF; }

  virtual TokenBase* clone() const {
    RefToken* copy = new RefToken(static_cast<const RefToken&>(*this));
    copy->value = value->clone();
    return copy;
  }
};

typedef std::map<uint8_t, TokenMap> typeMap_t;

class calculator {
 private:
  static OppMap_t _opPrecedence;
  static OppMap_t buildOpPrecedence();

 public:
  static typeMap_t& type_attribute_map();

 public:
  static packToken calculate(const char* expr, TokenMap vars = &TokenMap::empty,
                             const char* delim = 0, const char** rest = 0);

 private:
  static TokenBase* calculate(TokenQueue_t RPN, TokenMap vars);
  static void cleanRPN(TokenQueue_t* rpn);
  static TokenQueue_t toRPN(const char* expr, TokenMap vars,
                            const char* delim = 0, const char** rest = 0,
                            OppMap_t opPrecedence = _opPrecedence);

  static bool handle_unary(const std::string& op,
                           TokenQueue_t* rpnQueue, bool lastTokenWasOp);
  static void handle_op(const std::string& op,
                        TokenQueue_t* rpnQueue,
                        std::stack<std::string>* operatorStack,
                        OppMap_t opPrecedence);

  // Used to dealloc a TokenQueue_t safely.
  struct RAII_TokenQueue_t;

 private:
  TokenQueue_t RPN;

 public:
  ~calculator();
  calculator() {}
  calculator(const calculator& calc);
  calculator(const char* expr, TokenMap vars = &TokenMap::empty,
             const char* delim = 0, const char** rest = 0,
             OppMap_t opPrecedence = _opPrecedence);
  void compile(const char* expr, TokenMap vars = &TokenMap::empty,
               const char* delim = 0, const char** rest = 0,
               OppMap_t opPrecedence = _opPrecedence);
  packToken eval(TokenMap vars = &TokenMap::empty, bool keep_refs = false) const;

  // Serialization:
  std::string str() const;
  static std::string str(TokenQueue_t rpn);

  // Operators:
  calculator& operator = (const calculator& calc);
};

#endif  // SHUNTING_YARD_H_
