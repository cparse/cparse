#ifndef SHUNTING_YARD_H_
#define SHUNTING_YARD_H_

#include <map>
#include <stack>
#include <string>
#include <queue>
#include <list>
#include <vector>

/*
 * About tokType enum:
 *
 * The 3 left most bits (0x80, 0x40 and 0x20) of the Token Type
 * are reserved for denoting Numerals, Iterators and References.
 * If you want to define your own type please mind this bits.
 */
typedef uint8_t tokType_t;
typedef uint64_t opID_t;
enum tokType {
  // Base types:
  // Note: The mask system accepts at most 29 (32-3) different base types.
  NONE, OP, VAR, STR, FUNC,

  // Numerals:
  NUM = 0x20,   // Everything with the bit 0x20 set is a number.
  REAL = 0x21,  // == 0x20 => Real numbers.
  INT = 0x22,   // == 0x20 + 0x1 => Integers are numbers.

  // Complex types:
  IT = 0x40,      // Everything with the bit 0x20 set is an iterator.
  LIST = 0x41,    // == 0x20 + 0x01 => Lists are iterators.
  TUPLE = 0x42,   // == 0x20 + 0x02 => Tuples are iterators.
  STUPLE = 0x43,  // == 0x20 + 0x03 => ArgTuples are iterators.
  MAP = 0x44,     // == 0x20 + 0x04 => Maps are Iterators

  REF = 0x80,

  ANY_TYPE = 0xFF
};

#define ANY_OP ""

struct TokenBase {
  tokType_t type;
  virtual ~TokenBase() {}
  virtual TokenBase* clone() const = 0;
};

template<class T> class Token : public TokenBase {
 public:
  T val;
  Token(T t, tokType_t type) : val(t) { this->type = type; }
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

class TokenMap;
class TokenList;
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

struct Operation {
  static inline const uint32_t mask(tokType_t type);
  static const opID_t build_mask(tokType_t left, tokType_t right);
  virtual const opID_t getMask() = 0;
  virtual TokenBase* exec(TokenBase* left, const std::string& op,
                          TokenBase* right) = 0;
};

typedef std::map<tokType_t, TokenMap> typeMap_t;
typedef std::vector<Operation*> opList_t;
typedef std::map<std::string, opList_t> opMap_t;

class calculator {
 public:
  static OppMap_t _opPrecedence;
  static OppMap_t buildOpPrecedence();
  static opMap_t& default_opMap();

 public:
  static typeMap_t& type_attribute_map();

 public:
  static packToken calculate(const char* expr, TokenMap vars = &TokenMap::empty,
                             const char* delim = 0, const char** rest = 0);

 private:
  static TokenBase* calculate(TokenQueue_t RPN, TokenMap vars,
                              opMap_t opMap = default_opMap());
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

 protected:
  virtual opMap_t opMap() const { return default_opMap(); }

 private:
  TokenQueue_t RPN;

 public:
  virtual ~calculator();
  calculator() { this->RPN.push(new TokenNone()); }
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
  calculator& operator=(const calculator& calc);
};

#endif  // SHUNTING_YARD_H_
