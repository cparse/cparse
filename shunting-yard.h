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
class TokenMap;

struct RefToken : public TokenBase {
  std::string name;
  TokenBase* value;
  TokenMap* source_map;
  RefToken(std::string n, TokenBase* v, TokenMap* m, uint8_t type) :
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

struct TokenMap {
  typedef std::map<std::string, packToken> TokenMap_t;

  // Static factories:
  static TokenMap empty;
  static TokenMap& base_map();
  static TokenMap& default_global();

 public:
  TokenMap_t map;
  TokenMap* parent;

 public:
  TokenMap(TokenMap* parent = &TokenMap::default_global()) : parent(parent) {}

 public:
  packToken* find(std::string key);
  const packToken* find(std::string key) const;
  void assign(std::string key, TokenBase* value);
  void insert(std::string key, TokenBase* value);

  TokenMap getChild();

  packToken& operator[](const std::string& str);

  void erase(std::string key);

  void clean();
  unsigned size() const;
};

// Build a TokenMap which is not a child of default_global()
struct Object : public TokenMap {
  Object() : TokenMap(TokenMap::base_map()) {}
};

class calculator {
 private:
  static OppMap_t _opPrecedence;
  static OppMap_t buildOpPrecedence();

 public:
  static packToken calculate(const char* expr, TokenMap* vars = &TokenMap::empty,
                             const char* delim = 0, const char** rest = 0);

 private:
  static TokenBase* calculate(TokenQueue_t RPN, TokenMap* vars);
  static void cleanRPN(TokenQueue_t* rpn);
  static TokenQueue_t toRPN(const char* expr, TokenMap* vars,
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
  calculator(const char* expr, TokenMap* vars = &TokenMap::empty,
             const char* delim = 0, const char** rest = 0,
             OppMap_t opPrecedence = _opPrecedence);
  void compile(const char* expr, TokenMap* vars = &TokenMap::empty,
               const char* delim = 0, const char** rest = 0,
               OppMap_t opPrecedence = _opPrecedence);
  packToken eval(TokenMap* vars = &TokenMap::empty, bool keep_refs = false) const;

  // Serialization:
  std::string str() const;
  static std::string str(TokenQueue_t rpn);

  // Operators:
  calculator& operator = (const calculator& calc);
};

#endif  // SHUNTING_YARD_H_
