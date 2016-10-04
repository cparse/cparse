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
struct OppMap_t : public std::map<std::string, int> {
  OppMap_t() {
    // These operations are hard-coded on the system,
    // thus they should always be defined.
    (*this)["[]"] = -1; (*this)["()"] = -1;
    (*this)["["] = 0xFFFFFF; (*this)["("] = 0xFFFFFF;
  }
};

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

struct BaseOperation {
  static inline const uint32_t mask(tokType_t type);
  static const opID_t build_mask(tokType_t left, tokType_t right);
  virtual const opID_t getMask() = 0;

  // This exec is designed for use of advanced users:
  virtual TokenBase* exec(TokenBase* left, const std::string& op,
                          TokenBase* right) = 0;
};

struct Operation : public BaseOperation {
  virtual TokenBase* exec(TokenBase* left, const std::string& op,
                          TokenBase* right) {
    packToken result = exec(packToken(left->clone()),
                             op, packToken(right->clone()));

    if (result) {
      delete left;
      delete right;
      return result->clone();
    } else {
      return 0;
    }
  }

  // This exec is designed for use of non advanced users:
  virtual packToken exec(packToken left, std::string op, packToken right) = 0;
};

typedef std::map<tokType_t, TokenMap> typeMap_t;
typedef std::vector<BaseOperation*> opList_t;
typedef std::map<std::string, opList_t> opMap_t;

class calculator {
 public:
  static OppMap_t& default_opPrecedence();
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
                            OppMap_t opPrecedence = default_opPrecedence());

  static bool handle_unary(const std::string& op,
                           TokenQueue_t* rpnQueue, bool lastTokenWasOp);
  static void handle_op(const std::string& op,
                        TokenQueue_t* rpnQueue,
                        std::stack<std::string>* operatorStack,
                        OppMap_t opPrecedence);

  // Used to dealloc a TokenQueue_t safely.
  struct RAII_TokenQueue_t;

 protected:
  virtual const opMap_t opMap() const { return default_opMap(); }
  virtual const OppMap_t opPrecedence() const { return default_opPrecedence(); }

 private:
  TokenQueue_t RPN;

 public:
  virtual ~calculator();
  calculator() { this->RPN.push(new TokenNone()); }
  calculator(const calculator& calc);
  calculator(const char* expr, TokenMap vars = &TokenMap::empty,
             const char* delim = 0, const char** rest = 0,
             const OppMap_t& opPrecedence = default_opPrecedence());
  void compile(const char* expr, TokenMap vars = &TokenMap::empty,
               const char* delim = 0, const char** rest = 0);
  packToken eval(TokenMap vars = &TokenMap::empty, bool keep_refs = false) const;

  // Serialization:
  std::string str() const;
  static std::string str(TokenQueue_t rpn);

  // Operators:
  calculator& operator=(const calculator& calc);
};

#endif  // SHUNTING_YARD_H_
