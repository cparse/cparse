#include "./shunting-yard.h"
#include "./shunting-yard-exceptions.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <exception>
#include <string>
#include <stack>
#include <utility>  // For std::pair
#include <cstring>  // For strchr()

/* * * * * BaseOperation class: * * * * */

// Convert a type into an unique mask for bit wise operations:
const uint32_t BaseOperation::mask(tokType_t type) {
  if (type == ANY_TYPE) {
    return 0xFFFF;
  } else {
    return ((type & 0xE0) << 24) | (1 << (type & 0x1F));
  }
}

// Build a mask for each pair of operands
const opID_t BaseOperation::build_mask(tokType_t left, tokType_t right) {
  opID_t result = mask(left);
  return (result << 32) | mask(right);
}

/* * * * * Operation Utilities: * * * * */

bool match_op_id(opID_t id, opID_t mask) {
  uint64_t result = id & mask;
  uint32_t* val = reinterpret_cast<uint32_t*>(&result);
  if (val[0] && val[1]) return true;
  return false;
}

#define EXEC_OPERATION(result, opID, opMap, OP_MASK)\
  for (BaseOperation* operation : opMap[OP_MASK]) {\
    if (match_op_id(opID, operation->getMask())) {\
      result = operation->exec(b_left, op, b_right);\
      if (result) break;\
    }\
  }\

/* * * * * Static containers: * * * * */

// Builds the opPrecedence map only once:
OppMap_t& calculator::default_opPrecedence() {
  static OppMap_t opp;
  return opp;
}

typeMap_t& calculator::type_attribute_map() {
  static typeMap_t type_map;
  return type_map;
}

opMap_t& calculator::default_opMap() {
  static opMap_t opMap;
  return opMap;
}

rWordMap_t& calculator::default_rWordMap() {
  static rWordMap_t rwMap;
  return rwMap;
}

/* * * * * Calculator Class: * * * * */

// Check for unary operators and "convert" them to binary:
bool calculator::handle_unary(const std::string& op,
                              TokenQueue_t* rpnQueue, bool lastTokenWasOp) {
  if (lastTokenWasOp) {
    // Convert unary operators to binary in the RPN.
    if (!op.compare("-") || !op.compare("+")) {
      rpnQueue->push(new Token<int64_t>(0, INT));
      return true;
    } else {
      cleanRPN(rpnQueue);
      throw std::domain_error("Unrecognized unary operator: '" + op + "'.");
    }
  }

  return false;
}

// Consume operators with precedence >= than op then add op
void calculator::handle_op(const std::string& op,
                           TokenQueue_t* rpnQueue,
                           std::stack<std::string>* operatorStack,
                           OppMap_t opPrecedence) {
  // Check if operator exists:
  if (opPrecedence.find(op) == opPrecedence.end()) {
    cleanRPN(rpnQueue);
    throw std::domain_error("Undefined operator: `" + op + "`!");
  }

  float cur_opp = opPrecedence[op];
  // To force "=" to be evaluated from the right to the left:
  if (op == "=") cur_opp -= 0.1;

  // Let p(o) denote the precedence of an operator o.
  //
  // If the token is an operator, o1, then
  //   While there is an operator token, o2, at the top
  //       and p(o1) <= p(o2), then
  //     pop o2 off the stack onto the output queue.
  //   Push o1 on the stack.
  while (!operatorStack->empty() && cur_opp >= opPrecedence[operatorStack->top()]) {
    rpnQueue->push(new Token<std::string>(operatorStack->top(), OP));
    operatorStack->pop();
  }
  operatorStack->push(op);
}

// Use this function to discard a reference to an object
// And obtain the original TokenBase*.
// Please note that it only deletes memory if the token
// is of type REF.
TokenBase* resolve_reference(TokenBase* b, TokenMap* scope = 0) {
  TokenBase* value = 0;

  if (b->type & REF) {
    // Grab the possible values:
    RefToken* ref = static_cast<RefToken*>(b);

    // If its a local variable,
    // and a local scope is available:
    if (ref->source->type == NONE && scope) {
      // Try to get the most recent value of the reference:
      packToken* r_value = scope->find(ref->key.asString());
      if (r_value) {
        value = (*r_value)->clone();
        delete ref->value;
      }
    }

    if (!value) value = ref->value;
    delete ref;

    return value;
  } else {
    return b;
  }
}

/* * * * * RAII_TokenQueue_t struct  * * * * */

// Used to make sure an rpn is dealloc'd correctly
// even when an exception is thrown.
//
// Note: This is needed because C++ does not
// allow a try-finally block.
struct calculator::RAII_TokenQueue_t : TokenQueue_t {
  RAII_TokenQueue_t() {}
  RAII_TokenQueue_t(const TokenQueue_t& rpn) : TokenQueue_t(rpn) {}
  ~RAII_TokenQueue_t() { cleanRPN(this); }

  RAII_TokenQueue_t(const RAII_TokenQueue_t& rpn) {
    throw std::runtime_error("You should not copy this class!");
  }
  RAII_TokenQueue_t& operator=(const RAII_TokenQueue_t& rpn) {
    throw std::runtime_error("You should not copy this class!");
  }
};

/* * * * * calculator class * * * * */

#define isvariablechar(c) (isalpha(c) || c == '_')
TokenQueue_t calculator::toRPN(const char* expr,
                               TokenMap vars, const char* delim,
                               const char** rest, OppMap_t opPrecedence,
                               rWordMap_t rWordMap) {
  rpnBuilder data;
  char* nextChar;

  static char c = '\0';
  if (!delim) delim = &c;

  while (*expr && isspace(*expr) && !strchr(delim, *expr)) ++expr;

  if (*expr == '\0' || strchr(delim, *expr)) {
    throw std::invalid_argument("Cannot build a calculator from an empty expression!");
  }

  // In one pass, ignore whitespace and parse the expression into RPN
  // using Dijkstra's Shunting-yard algorithm.
  while (*expr && (data.bracketLevel || !strchr(delim, *expr))) {
    if (isdigit(*expr)) {
      // If the token is a number, add it to the output queue.
      int64_t _int = strtol(expr, &nextChar, 10);

      // If the number was not a float:
      if (!strchr(".eE", *nextChar)) {
        data.rpn.push(new Token<int64_t>(_int, INT));
      } else {
        double digit = strtod(expr, &nextChar);
        data.rpn.push(new Token<double>(digit, REAL));
      }

      expr = nextChar;
      data.lastTokenWasOp = false;
      data.lastTokenWasUnary = false;
    } else if (isvariablechar(*expr)) {
      rWordMap_t::iterator it;

      // If the token is a variable, resolve it and
      // add the parsed number to the output queue.
      std::stringstream ss;
      ss << *expr;
      ++expr;
      while (isvariablechar(*expr) || isdigit(*expr)) {
        ss << *expr;
        ++expr;
      }
      std::string key = ss.str();

      if (data.lastTokenWasOp == '.') {
        data.rpn.push(new Token<std::string>(key, STR));
      } else if ((it=rWordMap.find(key)) != rWordMap.end()) {
        // Parse reserved words:
        data.rpn.push(it->second(expr, &expr, &data));
      } else {
        packToken* value = vars.find(key);

        if (value) {
          // Save a reference token:
          TokenBase* copy = (*value)->clone();
          data.rpn.push(new RefToken(key, copy));
        } else {
          // Save the variable name:
          data.rpn.push(new Token<std::string>(key, VAR));
        }
      }

      data.lastTokenWasOp = false;
      data.lastTokenWasUnary = false;
    } else if (*expr == '\'' || *expr == '"') {
      // If it is a string literal, parse it and
      // add to the output queue.
      char quote = *expr;

      ++expr;
      std::stringstream ss;
      while (*expr && *expr != quote && *expr != '\n') {
        if (*expr == '\\') {
          switch (expr[1]) {
          case 'n':
            expr+=2;
            ss << '\n';
            break;
          case 't':
            expr+=2;
            ss << '\t';
            break;
          default:
            if (strchr("\"'\n", expr[1])) ++expr;
            ss << *expr;
            ++expr;
          }
        } else {
          ss << *expr;
          ++expr;
        }
      }

      if (*expr != quote) {
        std::string squote = (quote == '"' ? "\"": "'");
        cleanRPN(&data.rpn);
        throw syntax_error("Expected quote (" + squote +
                           ") at end of string declaration: " + squote + ss.str() + ".");
      }
      ++expr;
      data.rpn.push(new Token<std::string>(ss.str(), STR));
      data.lastTokenWasOp = false;
    } else {
      // Otherwise, the variable is an operator or paranthesis.
      tokType_t lastType;
      char lastOp;

      // Check for syntax errors (excess of operators i.e. 10 + + -1):
      if (data.lastTokenWasUnary) {
        std::string op;
        op.push_back(*expr);
        cleanRPN(&data.rpn);
        throw syntax_error("Expected operand after unary operator `" + data.opStack.top() +
                           "` but found: `" + op + "` instead.");
      }

      switch (*expr) {
      case '(':
        // If it is a function call:
        lastType = data.rpn.size() ? data.rpn.back()->type : NONE;
        lastOp = data.opStack.size() ? data.opStack.top()[0] : '\0';
        if (lastType == VAR || lastType == (FUNC | REF) || lastOp == '.') {
          // This counts as a bracket and as an operator:
          data.lastTokenWasUnary = handle_unary("()", &data.rpn,
                                                data.lastTokenWasOp);
          handle_op("()", &data.rpn, &data.opStack, opPrecedence);
          // Add it as a bracket to the op stack:
        }
        data.opStack.push("(");
        data.lastTokenWasOp = '(';
        ++data.bracketLevel;
        ++expr;
        break;
      case '[':
        // This counts as a bracket and as an operator:
        data.lastTokenWasUnary = handle_unary("[]", &data.rpn,
                                              data.lastTokenWasOp);
        handle_op("[]", &data.rpn, &data.opStack, opPrecedence);
        // Add it as a bracket to the op stack:
        data.opStack.push("[");
        data.lastTokenWasOp = true;
        ++data.bracketLevel;
        ++expr;
        break;
      case ')':
        if (data.lastTokenWasOp == '(') {
          data.rpn.push(new Tuple());
          data.lastTokenWasOp = false;
        }
        while (data.opStack.size() && data.opStack.top().compare("(")) {
          data.rpn.push(new Token<std::string>(data.opStack.top(), OP));
          data.opStack.pop();
        }

        if (data.opStack.size() == 0) {
          cleanRPN(&data.rpn);
          throw syntax_error("Extra ')' on the expression!");
        }

        data.opStack.pop();
        --data.bracketLevel;
        ++expr;
        break;
      case ']':
        while (data.opStack.size() && data.opStack.top().compare("[")) {
          data.rpn.push(new Token<std::string>(data.opStack.top(), OP));
          data.opStack.pop();
        }

        if (data.opStack.size() == 0) {
          cleanRPN(&data.rpn);
          throw syntax_error("Extra ']' on the expression!");
        }

        data.opStack.pop();
        --data.bracketLevel;
        ++expr;
        break;
      default:
        {
          // Then the token is an operator
          std::stringstream ss;
          ss << *expr;
          ++expr;
          while (*expr && ispunct(*expr) && !strchr("+-'\"()_", *expr)) {
            ss << *expr;
            ++expr;
          }
          ss.clear();
          std::string op;

          ss >> op;

          data.lastTokenWasUnary = handle_unary(op, &data.rpn,
                                                data.lastTokenWasOp);

          handle_op(op, &data.rpn, &data.opStack, opPrecedence);

          data.lastTokenWasOp = op[0];
        }
      }
    }
    // Ignore spaces but stop on delimiter if not inside brackets.
    while (*expr && isspace(*expr)
           && (data.bracketLevel || !strchr(delim, *expr))) ++expr;
  }

  // Check for syntax errors (excess of operators i.e. 10 + + -1):
  if (data.lastTokenWasUnary) {
    cleanRPN(&data.rpn);
    throw syntax_error("Expected operand after unary operator `" + data.opStack.top() + "`");
  }

  while (!data.opStack.empty()) {
    data.rpn.push(new Token<std::string>(data.opStack.top(), OP));
    data.opStack.pop();
  }

  if (rest) *rest = expr;
  return data.rpn;
}

packToken calculator::calculate(const char* expr, TokenMap vars,
                                const char* delim, const char** rest) {
  // Convert to RPN with Dijkstra's Shunting-yard algorithm.
  RAII_TokenQueue_t rpn = calculator::toRPN(expr, vars, delim, rest);

  TokenBase* ret = calculator::calculate(rpn, vars);

  return packToken(resolve_reference(ret));
}

void cleanStack(std::stack<TokenBase*> st) {
  while (st.size() > 0) {
    delete resolve_reference(st.top());
    st.pop();
  }
}

TokenBase* calculator::calculate(TokenQueue_t _rpn, TokenMap vars,
                                 opMap_t opMap) {
  RAII_TokenQueue_t rpn;

  // Deep copy the token list, so everything can be
  // safely deallocated:
  while (!_rpn.empty()) {
    TokenBase* base = _rpn.front();
    _rpn.pop();
    rpn.push(base->clone());
  }

  // Evaluate the expression in RPN form.
  std::stack<TokenBase*> evaluation;
  while (!rpn.empty()) {
    TokenBase* base = rpn.front();
    rpn.pop();

    // Operator:
    if (base->type == OP) {
      std::string op = static_cast<Token<std::string>*>(base)->val;
      delete base;

      /* * * * * Resolve operands Values and References: * * * * */

      if (evaluation.size() < 2) {
        cleanStack(evaluation);
        throw std::domain_error("Invalid equation.");
      }
      TokenBase* b_right = evaluation.top(); evaluation.pop();
      TokenBase* b_left  = evaluation.top(); evaluation.pop();

      if (b_right->type == VAR) {
        std::string var_name = static_cast<Token<std::string>*>(b_right)->val;
        delete b_right;
        delete resolve_reference(b_left);
        cleanStack(evaluation);
        throw std::domain_error("Unable to find the variable '" + var_name + "'.");
      } else {
        b_right = resolve_reference(b_right, &vars);
      }

      packToken r_left;
      packToken m_left;
      if (b_left->type & REF) {
        RefToken* left = static_cast<RefToken*>(b_left);
        r_left = left->key;
        m_left = left->source;
        b_left = resolve_reference(left, &vars);
      } else if (b_left->type == VAR) {
        r_left = static_cast<Token<std::string>*>(b_left)->val;
      }

      /* * * * * Resolve Asign Operation * * * * */

      if (!op.compare("=")) {
        delete b_left;

        // If the left operand has a variable name:
        if (r_left->type == STR) {
          if (m_left->type == MAP) {
            TokenMap& map = m_left.asMap();
            std::string& key = r_left.asString();
            map[key] = packToken(b_right->clone());
          } else if (vars) {
            TokenMap* map = vars.findMap(r_left.asString());
            if (!map || *map == TokenMap::default_global()) {
              // Assign on the local scope.
              // The user should not be able to implicitly overwrite
              // variables he did not declare, since it's error prone.
              vars[r_left.asString()] = packToken(b_right->clone());
            } else {
              (*map)[r_left.asString()] = packToken(b_right->clone());
            }
          } else {
            delete b_right;
            cleanStack(evaluation);
            throw std::domain_error("Could not assign variable `" + r_left.asString() + "`!");
          }

          evaluation.push(b_right);
        // If the left operand has an index number:
        } else if (r_left->type & NUM) {
          if (m_left->type == LIST) {
            TokenList& list = m_left.asList();
            size_t index = r_left.asInt();
            list[index] = packToken(b_right->clone());
          } else {
            delete b_right;
            cleanStack(evaluation);
            throw std::domain_error("Left operand of assignment is not a list!");
          }

          evaluation.push(b_right);
        } else {
          packToken p_right(b_right->clone());
          delete b_right;

          cleanStack(evaluation);
          throw undefined_operation(op, r_left, p_right);
        }
      } else if (b_left->type == FUNC) {
        Function* f_left = static_cast<Function*>(b_left);

        if (!op.compare("()")) {
          // Collect the parameter tuple:
          Tuple right;
          if (b_right->type == TUPLE) {
            right = *static_cast<Tuple*>(b_right);
          } else {
            right = Tuple(b_right);
          }
          delete b_right;

          packToken _this;
          if (m_left->type != NONE) {
            _this = m_left;
          } else {
            _this = vars;
          }

          // Execute the function:
          packToken ret;
          try {
            ret = Function::call(_this, f_left, &right, vars);
          } catch (...) {
            cleanStack(evaluation);
            delete f_left;
            throw;
          }

          delete f_left;

          evaluation.push(ret->clone());
        } else {
          packToken p_right(b_right->clone());
          packToken p_left(b_left->clone());
          delete b_right;

          cleanStack(evaluation);
          throw undefined_operation(op, p_left, p_right);
        }
      } else {
        opID_t opID = BaseOperation::build_mask(b_left->type, b_right->type);
        TokenBase* result = 0;

        try {
          // Resolve the operation:
          EXEC_OPERATION(result, opID, opMap, op);
          if (!result) {
            EXEC_OPERATION(result, opID, opMap, ANY_OP);
          }
        } catch (...) {
          delete b_left;
          delete b_right;
          cleanStack(evaluation);
          throw;
        }

        if (result) {
          evaluation.push(result);
        } else {
          packToken p_left(b_left->clone());
          packToken p_right(b_right->clone());
          delete b_left;
          delete b_right;

          cleanStack(evaluation);
          throw undefined_operation(op, p_left, p_right);
        }
      }
    } else if (base->type == VAR) {  // Variable
      packToken* value = NULL;
      std::string key = static_cast<Token<std::string>*>(base)->val;

      if (vars) { value = vars.find(key); }

      if (value) {
        TokenBase* copy = (*value)->clone();
        evaluation.push(new RefToken(key, copy));
        delete base;
      } else {
        evaluation.push(base);
      }
    } else {
      evaluation.push(base);
    }
  }

  return evaluation.top();
}

void calculator::cleanRPN(TokenQueue_t* rpn) {
  while (rpn->size()) {
    delete resolve_reference(rpn->front());
    rpn->pop();
  }
}

/* * * * * Non Static Functions * * * * */

calculator::~calculator() {
  cleanRPN(&this->RPN);
}

calculator::calculator(const calculator& calc) {
  TokenQueue_t _rpn = calc.RPN;

  // Deep copy the token list, so everything can be
  // safely deallocated:
  while (!_rpn.empty()) {
    TokenBase* base = _rpn.front();
    _rpn.pop();
    this->RPN.push(base->clone());
  }
}

// Work as a sub-parser:
// - Stops at delim or '\0'
// - Returns the rest of the string as char* rest
calculator::calculator(const char* expr, TokenMap vars, const char* delim,
                       const char** rest, const OppMap_t& opp,
                       const rWordMap_t& rwMap) {
  this->RPN = calculator::toRPN(expr, vars, delim, rest, opp, rwMap);
}

void calculator::compile(const char* expr, TokenMap vars, const char* delim,
                         const char** rest) {
  // Make sure it is empty:
  cleanRPN(&this->RPN);

  this->RPN = calculator::toRPN(expr, vars, delim, rest,
                                opPrecedence(), rWordMap());
}

packToken calculator::eval(TokenMap vars, bool keep_refs) const {
  TokenBase* value = calculate(this->RPN, vars, opMap());
  packToken p = packToken(value->clone());
  if (keep_refs) {
    return packToken(value);
  } else {
    return packToken(resolve_reference(value));
  }
}

calculator& calculator::operator=(const calculator& calc) {
  // Make sure the RPN is empty:
  cleanRPN(&this->RPN);

  // Deep copy the token list, so everything can be
  // safely deallocated:
  TokenQueue_t _rpn = calc.RPN;
  while (!_rpn.empty()) {
    TokenBase* base = _rpn.front();
    _rpn.pop();
    this->RPN.push(base->clone());
  }
  return *this;
}

/* * * * * For Debug Only * * * * */

std::string calculator::str() const {
  return str(this->RPN);
}

std::string calculator::str(TokenQueue_t rpn) {
  std::stringstream ss;

  ss << "calculator { RPN: [ ";
  while (rpn.size()) {
    ss << packToken(resolve_reference(rpn.front()->clone())).str();
    rpn.pop();

    ss << (rpn.size() ? ", ":"");
  }
  ss << " ] }";
  return ss.str();
}
