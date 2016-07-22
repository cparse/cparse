#include "./shunting-yard.h"
#include "./shunting-yard-exceptions.h"

#include <math.h>

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <exception>
#include <string>
#include <stack>
#include <utility>  // For std::pair
#include <cstring>  // For strchr()

OppMap_t calculator::buildOpPrecedence() {
  OppMap_t opp;

  // Create the operator precedence map based on C++ default
  // precedence order as described on cppreference website:
  // http://en.cppreference.com/w/cpp/language/operator_precedence
  opp["[]"] = 2; opp["()"] = 2; opp["."] = 2;
  opp["^"]  = 3;
  opp["*"]  = 5; opp["/"]  = 5; opp["%"] = 5;
  opp["+"]  = 6; opp["-"]  = 6;
  opp["<<"] = 7; opp[">>"] = 7;
  opp["<"]  = 8; opp["<="] = 8; opp[">="] = 8; opp[">"] = 8;
  opp["=="] = 9; opp["!="] = 9;
  opp["&&"] = 13;
  opp["||"] = 14;
  opp["="] = 15;
  opp[","] = 16;
  opp["("]  = 17; opp["["] = 17;

  return opp;
}
// Builds the opPrecedence map only once:
OppMap_t calculator::_opPrecedence = calculator::buildOpPrecedence();

// Using the "Construct On First Use Idiom"
// to avoid the "static initialization order fiasco",
// for more information read:
//
// - https://isocpp.org/wiki/faq/ctors#static-init-order
//
const Scope Scope::empty = Scope();
TokenMap_t& Scope::default_global() {
  static TokenMap_t global_map;
  return global_map;
}

packToken trueToken = packToken(1);
packToken falseToken = packToken(0);

// Check for unary operators and "convert" them to binary:
bool calculator::handle_unary(const std::string& op,
                              TokenQueue_t* rpnQueue, bool lastTokenWasOp) {
  if (lastTokenWasOp) {
    // Convert unary operators to binary in the RPN.
    if (!op.compare("-") || !op.compare("+")) {
      rpnQueue->push(new Token<double>(0, NUM));
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
    throw std::domain_error("Unknown operator: `" + op + "`!");
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
TokenBase* pop_reference(TokenBase* b) {
  if (b->type & REF) {
    TokenBase* ref = static_cast<RefToken*>(b)->value;
    delete b;
    return ref;
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
                               const Scope* vars, const char* delim,
                               const char** rest, OppMap_t opPrecedence) {
  TokenQueue_t rpnQueue; std::stack<std::string> operatorStack;
  uint8_t lastTokenWasOp = true;
  bool lastTokenWasUnary = false;
  char* nextChar;

  // Used to make sure the expression won't
  // end inside a bracket evaluation just because
  // found a delimiter like '\n' or ')'
  int bracketLevel = 0;

  static char c = '\0';
  if (!delim) delim = &c;

  while (*expr && isspace(*expr) && !strchr(delim, *expr)) ++expr;

  if (*expr == '\0' || strchr(delim, *expr)) {
    throw std::invalid_argument("Cannot build a calculator from an empty expression!");
  }

  // In one pass, ignore whitespace and parse the expression into RPN
  // using Dijkstra's Shunting-yard algorithm.
  while (*expr && (bracketLevel || !strchr(delim, *expr))) {
    if (isdigit(*expr)) {
      // If the token is a number, add it to the output queue.
      double digit = strtod(expr, &nextChar);
      rpnQueue.push(new Token<double>(digit, NUM));
      expr = nextChar;
      lastTokenWasOp = false;
      lastTokenWasUnary = false;
    } else if (isvariablechar(*expr)) {
      // If the token is a variable, resolve it and
      // add the parsed number to the output queue.
      std::stringstream ss;
      ss << *expr;
      ++expr;
      while (isvariablechar(*expr) || isdigit(*expr)) {
        ss << *expr;
        ++expr;
      }

      if (lastTokenWasOp == '.') {
        rpnQueue.push(new Token<std::string>(ss.str(), STR));
      } else {
        packToken* value = NULL;
        std::string key = ss.str();

        if (key == "true") {
          value = &trueToken;
        } else if (key == "false") {
          value = &falseToken;
        } else {
          if (vars) value = vars->find(key);
        }

        if (value) {
          // Save a reference token:
          TokenBase* copy = (*value)->clone();
          rpnQueue.push(new RefToken(key, copy, copy->type | REF));
        } else {
          // Save the variable name:
          rpnQueue.push(new Token<std::string>(key, VAR));
        }
      }

      lastTokenWasOp = false;
      lastTokenWasUnary = false;
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
        cleanRPN(&rpnQueue);
        throw syntax_error("Expected quote (" + squote +
                           ") at end of string declaration: " + squote + ss.str() + ".");
      }
      ++expr;
      rpnQueue.push(new Token<std::string>(ss.str(), STR));
      lastTokenWasOp = false;
    } else {
      // Otherwise, the variable is an operator or paranthesis.
      uint8_t lastType;
      char lastOp;

      // Check for syntax errors (excess of operators i.e. 10 + + -1):
      if (lastTokenWasUnary) {
        std::string op;
        op.push_back(*expr);
        cleanRPN(&rpnQueue);
        throw syntax_error("Expected operand after unary operator `" + operatorStack.top() +
                           "` but found: `" + op + "` instead.");
      }

      switch (*expr) {
      case '(':
        // If it is a function call:
        lastType = rpnQueue.size() ? rpnQueue.back()->type : NONE;
        lastOp = operatorStack.size() ? operatorStack.top()[0] : '\0';
        if (lastType == VAR || lastType == (FUNC | REF) || lastOp == '.') {
          // This counts as a bracket and as an operator:
          lastTokenWasUnary = handle_unary("()", &rpnQueue, lastTokenWasOp);
          handle_op("()", &rpnQueue, &operatorStack, opPrecedence);
          // Add it as a bracket to the op stack:
        }
        operatorStack.push("(");
        lastTokenWasOp = '(';
        ++bracketLevel;
        ++expr;
        break;
      case '[':
        // This counts as a bracket and as an operator:
        lastTokenWasUnary = handle_unary("[]", &rpnQueue, lastTokenWasOp);
        handle_op("[]", &rpnQueue, &operatorStack, opPrecedence);
        // Add it as a bracket to the op stack:
        operatorStack.push("[");
        lastTokenWasOp = true;
        ++bracketLevel;
        ++expr;
        break;
      case ')':
        if (lastTokenWasOp == '(') {
          rpnQueue.push(new TokenNone());
        }
        while (operatorStack.top().compare("(")) {
          rpnQueue.push(new Token<std::string>(operatorStack.top(), OP));
          operatorStack.pop();
        }
        operatorStack.pop();
        --bracketLevel;
        ++expr;
        break;
      case ']':
        while (operatorStack.top().compare("[")) {
          rpnQueue.push(new Token<std::string>(operatorStack.top(), OP));
          operatorStack.pop();
        }
        operatorStack.pop();
        --bracketLevel;
        ++expr;
        break;
      default:
        {
          // Then the token is an operator
          std::stringstream ss;
          ss << *expr;
          ++expr;
          while (*expr && ispunct(*expr) && !strchr("'\"()_", *expr)) {
            ss << *expr;
            ++expr;
          }
          ss.clear();
          std::string op;

          ss >> op;

          lastTokenWasUnary = handle_unary(op, &rpnQueue, lastTokenWasOp);

          handle_op(op, &rpnQueue, &operatorStack, opPrecedence);

          lastTokenWasOp = op[0];
        }
      }
    }
    // Ignore spaces but stop on delimiter if not inside brackets.
    while (*expr && isspace(*expr)
           && (bracketLevel || !strchr(delim, *expr))) ++expr;
  }

  // Check for syntax errors (excess of operators i.e. 10 + + -1):
  if (lastTokenWasUnary) {
    cleanRPN(&rpnQueue);
    throw syntax_error("Expected operand after unary operator `" + operatorStack.top() + "`");
  }

  while (!operatorStack.empty()) {
    rpnQueue.push(new Token<std::string>(operatorStack.top(), OP));
    operatorStack.pop();
  }

  if (rest) *rest = expr;
  return rpnQueue;
}

packToken calculator::calculate(const char* expr, const Scope& vars,
                                const char* delim, const char** rest) {
  // Convert to RPN with Dijkstra's Shunting-yard algorithm.
  RAII_TokenQueue_t rpn = calculator::toRPN(expr, &vars, delim, rest);
  packToken ret;

  ret = calculator::calculate(rpn, &vars);

  return ret;
}

void cleanStack(std::stack<TokenBase*> st) {
  while (st.size() > 0) {
    delete pop_reference(st.top());
    st.pop();
  }
}

packToken calculator::calculate(TokenQueue_t _rpn,
                                const Scope* vars) {
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

      if (evaluation.size() < 2) {
        cleanStack(evaluation);
        throw std::domain_error("Invalid equation.");
      }
      TokenBase* b_right = evaluation.top(); evaluation.pop();
      TokenBase* b_left  = evaluation.top(); evaluation.pop();

      if (b_right->type == VAR) {
        std::string var_name = static_cast<Token<std::string>*>(b_right)->val;
        delete b_right;
        delete pop_reference(b_left);
        cleanStack(evaluation);
        throw std::domain_error("Unable to find the variable '" + var_name + "'.");
      } else {
        b_right = pop_reference(b_right);
      }

      std::string r_left;
      TokenMap_t* m_left = NULL;
      if (b_left->type & REF) {
        RefToken* left = static_cast<RefToken*>(b_left);
        r_left = left->name;
        m_left = left->source_map;
        b_left = left->value;
        delete left;
      } else if (b_left->type == VAR) {
        r_left = static_cast<Token<std::string>*>(b_left)->val;
      }

      // If it is a tuple operator:
      if (!op.compare(",")) {
        if (b_left->type == TUPLE) {
          Tuple* tuple = static_cast<Tuple*>(b_left);
          tuple->push_back(b_right);
          delete b_right;
          evaluation.push(tuple);
        } else {
          evaluation.push(new Tuple(b_left, b_right));
          delete b_left;
          delete b_right;
        }
      // If it is an assignment operation:
      } else if (!op.compare("=")) {
        delete b_left;

        // If the left operand has a variable name:
        if (r_left.size() > 0) {
          if (vars) {
            if (m_left && b_right->type != NONE) {
              (*m_left)[r_left] = packToken(b_right->clone());
            } else {
              vars->assign(r_left, b_right);
            }
            evaluation.push(b_right);
          } else {
            delete b_right;
            cleanStack(evaluation);
            throw std::domain_error("No Scope available for assignment of variable `" + r_left + "`.");
          }
        } else {
          packToken p_right(b_right->clone());
          delete b_right;

          cleanStack(evaluation);
          throw undefined_operation(op, r_left, p_right);
        }
      } else if (b_left->type == NUM && b_right->type == NUM) {
        double left = static_cast<Token<double>*>(b_left)->val;
        double right = static_cast<Token<double>*>(b_right)->val;
        delete b_left;
        delete b_right;

        int left_i = static_cast<int>(left);
        int right_i = static_cast<int>(right);

        if (!op.compare("+")) {
          evaluation.push(new Token<double>(left + right, NUM));
        } else if (!op.compare("*")) {
          evaluation.push(new Token<double>(left * right, NUM));
        } else if (!op.compare("-")) {
          evaluation.push(new Token<double>(left - right, NUM));
        } else if (!op.compare("/")) {
          evaluation.push(new Token<double>(left / right, NUM));
        } else if (!op.compare("<<")) {
          evaluation.push(new Token<double>(left_i << right_i, NUM));
        } else if (!op.compare("^")) {
          evaluation.push(new Token<double>(pow(left, right), NUM));
        } else if (!op.compare(">>")) {
          evaluation.push(new Token<double>(left_i >> right_i, NUM));
        } else if (!op.compare("%")) {
          evaluation.push(new Token<double>(left_i % right_i, NUM));
        } else if (!op.compare("<")) {
          evaluation.push(new Token<double>(left < right, NUM));
        } else if (!op.compare(">")) {
          evaluation.push(new Token<double>(left > right, NUM));
        } else if (!op.compare("<=")) {
          evaluation.push(new Token<double>(left <= right, NUM));
        } else if (!op.compare(">=")) {
          evaluation.push(new Token<double>(left >= right, NUM));
        } else if (!op.compare("==")) {
          evaluation.push(new Token<double>(left == right, NUM));
        } else if (!op.compare("!=")) {
          evaluation.push(new Token<double>(left != right, NUM));
        } else if (!op.compare("&&")) {
          evaluation.push(new Token<double>(left_i && right_i, NUM));
        } else if (!op.compare("||")) {
          evaluation.push(new Token<double>(left_i || right_i, NUM));
        } else {
          cleanStack(evaluation);
          throw undefined_operation(op, left, right);
        }
      } else if (b_left->type == STR && b_right->type == STR) {
        std::string left = static_cast<Token<std::string>*>(b_left)->val;
        std::string right = static_cast<Token<std::string>*>(b_right)->val;
        delete b_left;
        delete b_right;

        if (!op.compare("+")) {
          evaluation.push(new Token<std::string>(left + right, STR));
        } else if (!op.compare("==")) {
          evaluation.push(new Token<double>(left.compare(right) == 0, NUM));
        } else if (!op.compare("!=")) {
          evaluation.push(new Token<double>(left.compare(right) != 0, NUM));
        } else {
          cleanStack(evaluation);
          throw undefined_operation(op, left, right);
        }
      } else if (b_left->type == STR && b_right->type == NUM) {
        std::string left = static_cast<Token<std::string>*>(b_left)->val;
        double right = static_cast<Token<double>*>(b_right)->val;
        delete b_left;
        delete b_right;

        std::stringstream ss;
        if (!op.compare("+")) {
          ss << left << right;
          evaluation.push(new Token<std::string>(ss.str(), STR));
        } else {
          cleanStack(evaluation);
          throw undefined_operation(op, left, right);
        }
      } else if (b_left->type == NUM && b_right->type == STR) {
        double left = static_cast<Token<double>*>(b_left)->val;
        std::string right = static_cast<Token<std::string>*>(b_right)->val;
        delete b_left;
        delete b_right;

        std::stringstream ss;
        if (!op.compare("+")) {
          ss << left << right;
          evaluation.push(new Token<std::string>(ss.str(), STR));
        } else {
          cleanStack(evaluation);
          throw undefined_operation(op, left, right);
        }
      } else if (b_left->type == MAP && b_right->type == STR) {
        TokenMap_t* left = static_cast<Token<TokenMap_t*>*>(b_left)->val;
        std::string right = static_cast<Token<std::string>*>(b_right)->val;
        delete b_left;
        delete b_right;

        if (!op.compare("[]") || !op.compare(".")) {
          TokenMap_t::iterator it = left->find(right);

          TokenBase* value;
          uint8_t type;
          if (it == left->end()) {
            value = new TokenNone();
            type = NONE | REF;
          } else {
            value = it->second->clone();
            type = value->type | REF;
          }

          evaluation.push(new RefToken(right, value, left, type));
        } else {
          cleanStack(evaluation);
          throw undefined_operation(op, left, right);
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

          // Build the local namespace:
          TokenMap_t local;
          for (const std::string& name : f_left->args()) {
            packToken value;
            if (right.size()) {
              value = packToken(right.pop_front());
            } else {
              value = packToken::None;
            }

            local.insert(std::pair<std::string, packToken>(name, value));
          }

          // Add args to scope:
          vars->push(&local);
          packToken ret;
          try {
            // Execute the function:
            ret = f_left->exec(vars);
          } catch (...) {
            cleanStack(evaluation);
            vars->pop();
            delete f_left;
            throw;
          }
          // Drop the local scope:
          vars->pop();
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
        packToken p_left(b_left->clone());
        packToken p_right(b_right->clone());
        delete b_left;
        delete b_right;

        cleanStack(evaluation);
        throw undefined_operation(op, p_left, p_right);
      }
    } else if (base->type == VAR) {  // Variable
      packToken* value = NULL;
      std::string key = static_cast<Token<std::string>*>(base)->val;

      if (vars) { value = vars->find(key); }

      if (value) {
        TokenBase* copy = (*value)->clone();
        evaluation.push(new RefToken(key, copy, copy->type | REF));
        delete base;
      } else {
        evaluation.push(base);
      }
    } else {
      evaluation.push(base);
    }
  }

  return packToken(pop_reference(evaluation.top()));
}

void calculator::cleanRPN(TokenQueue_t* rpn) {
  while (rpn->size()) {
    delete pop_reference(rpn->front());
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
calculator::calculator(const char* expr, const Scope& vars,
                            const char* delim, const char** rest, OppMap_t opPrecedence) {
  compile(expr, vars, delim, rest, opPrecedence);
}

void calculator::compile(const char* expr,
                         const Scope& vars, const char* delim,
                         const char** rest, OppMap_t opPrecedence) {
  // Make sure it is empty:
  cleanRPN(&this->RPN);

  this->RPN = calculator::toRPN(expr, &vars, delim, rest, opPrecedence);
}

packToken calculator::eval(const Scope& vars) const {
  return calculate(this->RPN, &vars);
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
    ss << packToken(pop_reference(rpn.front()->clone())).str();
    rpn.pop();

    ss << (rpn.size() ? ", ":"");
  }
  ss << " ] }";
  return ss.str();
}

/* * * * * Scope Class: * * * * */

Scope::Scope(TokenMap_t* vars) {
  // Add default functions to the global namespace:
  scope.push_front(&default_global());

  if (vars) scope.push_front(vars);
}

packToken* Scope::find(std::string key) const {
  packToken* value = NULL;

  Scope_t::iterator s_it = scope.begin();
  for (; s_it != scope.end(); s_it++) {
    TokenMap_t::iterator it = (*s_it)->find(key);
    if (it != (*s_it)->end()) {
      value = &(it->second);
      break;
    }
  }

  return value;
}

void Scope::assign(std::string key, TokenBase* value) const {
  if (value) {
    value = value->clone();
  } else {
    throw std::invalid_argument("Scope assignment expected a non NULL argument as value!");
  }

  packToken* variable = find(key);

  if (variable) {
    (*variable) = packToken(value);
  } else {
    // Insert it on the most local context
    if (scope.size() == 0) {
      throw std::range_error("Cannot insert a variable into an empty scope!");
    }
    scope.front()->insert(std::pair<std::string, packToken>(key, packToken(value)));
  }
}

void Scope::push(TokenMap_t* vars) const {
  if (vars) scope.push_front(vars);
}

void Scope::push(Scope other) const {
  Scope_t::reverse_iterator s_it = other.scope.rbegin();
  for (; s_it != other.scope.rend(); s_it++) {
    push(*s_it);
  }
}

void Scope::pop() const {
  if (scope.size() == 0)
    throw std::range_error("Calculator::drop_namespace(): No namespace left to drop!");
  scope.pop_front();
}

// Pop the N top elements:
void Scope::pop(unsigned N) const {
  for (unsigned i=0; i < N; i++) {
    pop();
  }
}

void Scope::clean() const {
  scope = Scope_t();
}

unsigned Scope::size() const {
  return scope.size();
}
