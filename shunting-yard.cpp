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
Scope calculator::empty_scope = Scope();

packToken trueToken = packToken(1);
packToken falseToken = packToken(0);

// Check for unary operators and "convert" them to binary:
bool calculator::handle_unary(const std::string& str,
                              TokenQueue_t* rpnQueue, bool lastTokenWasOp,
                              OppMap_t opPrecedence) {
  if (lastTokenWasOp) {
    // Convert unary operators to binary in the RPN.
    if (!str.compare("-") || !str.compare("+")) {
      rpnQueue->push(new Token<double>(0, NUM));
      return true;
    } else {
      cleanRPN(rpnQueue);
      throw std::domain_error(
                              "Unrecognized unary operator: '" + str + "'.");
    }
  }

  return false;
}

// Consume operators with precedence >= than op then add op
void calculator::handle_op(const std::string& str,
                           TokenQueue_t* rpnQueue,
                           std::stack<std::string>* operatorStack,
                           OppMap_t opPrecedence) {
  // Check if operator exists:
  if (opPrecedence.find(str) == opPrecedence.end()) {
    cleanRPN(rpnQueue);
    throw std::domain_error("Unknown operator: `" + str + "`!");
  }

  float cur_opp = opPrecedence[str];
  // To force "=" to be evaluated from the right to the left:
  if (str == "=") cur_opp -= 0.1;
  while (!operatorStack->empty() && cur_opp >= opPrecedence[operatorStack->top()]) {
    rpnQueue->push(new Token<std::string>(operatorStack->top(), OP));
    operatorStack->pop();
  }
  operatorStack->push(str);
}

#define isvariablechar(c) (isalpha(c) || c == '_')
TokenQueue_t calculator::toRPN(const char* expr,
                               const Scope* vars, OppMap_t opPrecedence) {
  TokenQueue_t rpnQueue; std::stack<std::string> operatorStack;
  bool lastTokenWasOp = true;
  bool lastTokenWasUnary = false;

  // In one pass, ignore whitespace and parse the expression into RPN
  // using Dijkstra's Shunting-yard algorithm.
  while (*expr && isspace(*expr)) ++expr;
  while (*expr) {
    if (isdigit(*expr)) {
      // If the token is a number, add it to the output queue.
      char* nextChar = 0;
      double digit = strtod(expr , &nextChar);
      rpnQueue.push(new Token<double>(digit, NUM));
      expr = nextChar;
      lastTokenWasOp = false;
      lastTokenWasUnary = false;
    } else if (isvariablechar(*expr) && lastTokenWasOp
               && operatorStack.size() > 0 && !operatorStack.top().compare(".")) {
      // If it is a member access key (e.g. `map.name`)
      // parse it and add to the output queue.
      std::stringstream ss;
      while (isvariablechar(*expr) || isdigit(*expr)) {
        ss << *expr;
        ++expr;
      }

      rpnQueue.push(new Token<std::string>(ss.str(), STR));
      lastTokenWasOp = false;
    } else if (isvariablechar(*expr)) {
      // If the function is a variable, resolve it and
      // add the parsed number to the output queue.
      std::stringstream ss;
      ss << *expr;
      ++expr;
      while (isvariablechar(*expr) || isdigit(*expr)) {
        ss << *expr;
        ++expr;
      }

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
        rpnQueue.push(new Token<RefValue_t>({key, copy}, copy->type | REF));
      } else {
        // Save the variable name:
        rpnQueue.push(new Token<std::string>(key, VAR));
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
        if (*expr == '\\') ++expr;
        ss << *expr;
        ++expr;
      }

      if (*expr != quote) {
        std::string squote = (quote == '"' ? "\"": "'");
        cleanRPN(&rpnQueue);
        throw syntax_error(
                           "Expected quote (" + squote +
                           ") at end of string declaration: " + squote + ss.str() + ".");
      }
      ++expr;
      rpnQueue.push(new Token<std::string>(ss.str(), STR));
      lastTokenWasOp = false;
    } else {
      // Otherwise, the variable is an operator or paranthesis.
      uint8_t lastType;

      // Check for syntax errors (excess of operators i.e. 10 + + -1):
      if (lastTokenWasUnary) {
        std::string op;
        op.push_back(*expr);
        cleanRPN(&rpnQueue);
        throw syntax_error(
                           "Expected operand after unary operator `" + operatorStack.top() +
                           "` but found: `" + op + "` instead.");
      }

      switch (*expr) {
      case '(':
        // If it is a function call:
        lastType = rpnQueue.size() ? rpnQueue.front()->type : NONE;
        if (lastType == VAR || lastType == (FUNC | REF)) {
          // This counts as a bracket and as an operator:
          lastTokenWasUnary = handle_unary("()", &rpnQueue,
                                           lastTokenWasOp, opPrecedence);
          handle_op("()", &rpnQueue, &operatorStack, opPrecedence);
          // Add it as a bracket to the op stack:
        }
        operatorStack.push("(");
        ++expr;
        lastTokenWasOp = true;
        break;
      case '[':
        // This counts as a bracket and as an operator:
        lastTokenWasUnary = handle_unary("[]", &rpnQueue,
                                         lastTokenWasOp, opPrecedence);
        handle_op("[]", &rpnQueue, &operatorStack, opPrecedence);
        // Add it as a bracket to the op stack:
        operatorStack.push("[");
        ++expr;
        lastTokenWasOp = true;
        break;
      case ')':
        while (operatorStack.top().compare("(")) {
          rpnQueue.push(new Token<std::string>(operatorStack.top(), OP));
          operatorStack.pop();
        }
        operatorStack.pop();
        ++expr;
        break;
      case ']':
        while (operatorStack.top().compare("[")) {
          rpnQueue.push(new Token<std::string>(operatorStack.top(), OP));
          operatorStack.pop();
        }
        operatorStack.pop();
        ++expr;
        break;
      default:
        {
          // The token is an operator.
          //
          // Let p(o) denote the precedence of an operator o.
          //
          // If the token is an operator, o1, then
          //   While there is an operator token, o2, at the top
          //       and p(o1) <= p(o2), then
          //     pop o2 off the stack onto the output queue.
          //   Push o1 on the stack.
          std::stringstream ss;
          ss << *expr;
          ++expr;
          while (*expr && !isspace(*expr) && !isdigit(*expr)
                 && !isvariablechar(*expr) && *expr != '(' && *expr != ')') {
            ss << *expr;
            ++expr;
          }
          ss.clear();
          std::string str;

          ss >> str;

          lastTokenWasUnary = handle_unary(str, &rpnQueue,
                                           lastTokenWasOp, opPrecedence);

          handle_op(str, &rpnQueue, &operatorStack, opPrecedence);

          lastTokenWasOp = true;
        }
      }
    }
    while (*expr && isspace(*expr)) ++expr;
  }

  // Check for syntax errors (excess of operators i.e. 10 + + -1):
  if (lastTokenWasUnary) {
    std::string op;
    cleanRPN(&rpnQueue);
    throw syntax_error(
                       "Expected operand after unary operator `" + operatorStack.top() + "`");
  }

  while (!operatorStack.empty()) {
    rpnQueue.push(new Token<std::string>(operatorStack.top(), OP));
    operatorStack.pop();
  }
  return rpnQueue;
}

packToken calculator::calculate(const char* expr, const Scope& vars) {
  // Convert to RPN with Dijkstra's Shunting-yard algorithm.
  TokenQueue_t rpn = calculator::toRPN(expr, &vars);
  packToken ret;

  try {
    ret = calculator::calculate(rpn, &vars);
  } catch (undefined_operation e) {
    cleanRPN(&rpn);
    throw e;
  } catch (std::domain_error e) {
    cleanRPN(&rpn);
    throw e;
  }

  cleanRPN(&rpn);
  return ret;
}

void cleanStack(std::stack<TokenBase*> st) {
  while (st.size() > 0) {
    delete st.top();
    st.pop();
  }
}

packToken calculator::calculate(TokenQueue_t _rpn,
                                const Scope* vars) {
  TokenQueue_t rpn;

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
      std::string str = static_cast<Token<std::string>*>(base)->val;
      delete base;

      if (evaluation.size() < 2) {
        cleanRPN(&rpn);
        cleanStack(evaluation);
        throw std::domain_error("Invalid equation.");
      }
      TokenBase* b_right = evaluation.top(); evaluation.pop();
      TokenBase* b_left  = evaluation.top(); evaluation.pop();

      if (b_right->type & REF) {
        RefValue_t rvalue = static_cast<Token<RefValue_t>*>(b_right)->val;
        delete b_right;
        b_right = rvalue.second->clone();
      } else if (b_right->type == VAR) {
        std::string var_name = static_cast<Token<std::string>*>(b_right)->val;
        delete b_right;
        delete b_left;
        cleanRPN(&rpn);
        cleanStack(evaluation);
        throw std::domain_error("Unable to find the variable '" + var_name + "'.");
      }

      std::string r_left;
      if (b_left->type & REF) {
        RefValue_t rvalue = static_cast<Token<RefValue_t>*>(b_left)->val;
        delete b_left;
        r_left = rvalue.first;
        b_left = rvalue.second->clone();
      } else if (b_left->type == VAR) {
        r_left = static_cast<Token<std::string>*>(b_left)->val;
      }

      // If it is a tuple operator:
      if (!str.compare(",")) {
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
      } else if (!str.compare("=")) {
        delete b_left;

        if (r_left.size() > 0) {
          if (vars) {
            vars->asign(r_left, b_right);
            evaluation.push(b_right);
          } else {
            delete b_right;
            cleanRPN(&rpn);
            cleanStack(evaluation);
            throw std::domain_error("No Scope available for asignment of variable `" + r_left + "`.");
          }
        } else {
          packToken p_right(b_right->clone());
          delete b_right;

          cleanRPN(&rpn);
          cleanStack(evaluation);
          throw undefined_operation(str, r_left, p_right);
        }
      } else if (b_left->type == NUM && b_right->type == NUM) {
        double left = static_cast<Token<double>*>(b_left)->val;
        double right = static_cast<Token<double>*>(b_right)->val;
        delete b_left;
        delete b_right;

        int left_i = static_cast<int>(left);
        int right_i = static_cast<int>(right);

        if (!str.compare("+")) {
          evaluation.push(new Token<double>(left + right, NUM));
        } else if (!str.compare("*")) {
          evaluation.push(new Token<double>(left * right, NUM));
        } else if (!str.compare("-")) {
          evaluation.push(new Token<double>(left - right, NUM));
        } else if (!str.compare("/")) {
          evaluation.push(new Token<double>(left / right, NUM));
        } else if (!str.compare("<<")) {
          evaluation.push(new Token<double>(left_i << right_i, NUM));
        } else if (!str.compare("^")) {
          evaluation.push(new Token<double>(pow(left, right), NUM));
        } else if (!str.compare(">>")) {
          evaluation.push(new Token<double>(left_i >> right_i, NUM));
        } else if (!str.compare("%")) {
          evaluation.push(new Token<double>(left_i % right_i, NUM));
        } else if (!str.compare("<")) {
          evaluation.push(new Token<double>(left < right, NUM));
        } else if (!str.compare(">")) {
          evaluation.push(new Token<double>(left > right, NUM));
        } else if (!str.compare("<=")) {
          evaluation.push(new Token<double>(left <= right, NUM));
        } else if (!str.compare(">=")) {
          evaluation.push(new Token<double>(left >= right, NUM));
        } else if (!str.compare("==")) {
          evaluation.push(new Token<double>(left == right, NUM));
        } else if (!str.compare("!=")) {
          evaluation.push(new Token<double>(left != right, NUM));
        } else if (!str.compare("&&")) {
          evaluation.push(new Token<double>(left_i && right_i, NUM));
        } else if (!str.compare("||")) {
          evaluation.push(new Token<double>(left_i || right_i, NUM));
        } else {
          cleanRPN(&rpn);
          cleanStack(evaluation);
          throw undefined_operation(str, left, right);
        }
      } else if (b_left->type == STR && b_right->type == STR) {
        std::string left = static_cast<Token<std::string>*>(b_left)->val;
        std::string right = static_cast<Token<std::string>*>(b_right)->val;
        delete b_left;
        delete b_right;

        if (!str.compare("+")) {
          evaluation.push(new Token<std::string>(left + right, STR));
        } else if (!str.compare("==")) {
          evaluation.push(new Token<double>(left.compare(right) == 0, NUM));
        } else if (!str.compare("!=")) {
          evaluation.push(new Token<double>(left.compare(right) != 0, NUM));
        } else {
          cleanRPN(&rpn);
          cleanStack(evaluation);
          throw undefined_operation(str, left, right);
        }
      } else if (b_left->type == STR && b_right->type == NUM) {
        std::string left = static_cast<Token<std::string>*>(b_left)->val;
        double right = static_cast<Token<double>*>(b_right)->val;
        delete b_left;
        delete b_right;

        std::stringstream ss;
        if (!str.compare("+")) {
          ss << left << right;
          evaluation.push(new Token<std::string>(ss.str(), STR));
        } else {
          cleanRPN(&rpn);
          cleanStack(evaluation);
          throw undefined_operation(str, left, right);
        }
      } else if (b_left->type == NUM && b_right->type == STR) {
        double left = static_cast<Token<double>*>(b_left)->val;
        std::string right = static_cast<Token<std::string>*>(b_right)->val;
        delete b_left;
        delete b_right;

        std::stringstream ss;
        if (!str.compare("+")) {
          ss << left << right;
          evaluation.push(new Token<std::string>(ss.str(), STR));
        } else {
          cleanRPN(&rpn);
          cleanStack(evaluation);
          throw undefined_operation(str, left, right);
        }
      } else if (b_left->type == MAP && b_right->type == STR) {
        TokenMap_t* left = static_cast<Token<TokenMap_t*>*>(b_left)->val;
        std::string right = static_cast<Token<std::string>*>(b_right)->val;
        delete b_left;
        delete b_right;

        if (!str.compare("[]") || !str.compare(".")) {
          TokenMap_t::iterator it = left->find(right);

          if (it == left->end()) {
            cleanRPN(&rpn);
            cleanStack(evaluation);
            throw std::domain_error(
                                    "Unable to find the variable '" + right + "'.");
          }

          evaluation.push(it->second->clone());
        } else {
          cleanRPN(&rpn);
          cleanStack(evaluation);
          throw undefined_operation(str, left, right);
        }
      } else if (b_left->type == FUNC) {
        Function left = *static_cast<Function*>(b_left);
        delete b_left;

        if (!str.compare("()")) {
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
          for (unsigned i = 0; i < left.nargs; ++i) {
            packToken value;
            if (right.size()) {
              value = packToken(right.pop_front());
            } else {
              value = packToken::None;
            }

            local.insert(std::pair<std::string, packToken>(left.arg_names[i], value));
          }

          // Add args to scope:
          vars->push(&local);
          // Execute the function:
          packToken ret = left.func(vars);
          // Drop the local scope:
          vars->pop();

          evaluation.push(ret->clone());
        } else {
          packToken p_right(b_right->clone());
          delete b_right;

          cleanRPN(&rpn);
          cleanStack(evaluation);
          throw undefined_operation(str, left, p_right);
        }
      } else {
        packToken p_left(b_left->clone());
        packToken p_right(b_right->clone());
        delete b_left;
        delete b_right;

        cleanRPN(&rpn);
        cleanStack(evaluation);
        throw undefined_operation(str, p_left, p_right);
      }
    } else if (base->type == VAR) {  // Variable
      packToken* value = NULL;
      std::string key = static_cast<Token<std::string>*>(base)->val;

      if (vars) { value = vars->find(key); }

      if (value) {
        TokenBase* copy = (*value)->clone();
        evaluation.push(new Token<RefValue_t>({key, copy}, copy->type | REF));
        delete base;
      } else {
        evaluation.push(base);
      }
    } else {
      evaluation.push(base);
    }
  }

  TokenBase* result = evaluation.top();
  if (result->type & REF) {
    RefValue_t rvalue = static_cast<Token<RefValue_t>*>(result)->val;
    delete result;
    result = rvalue.second;
  }
  return packToken(result);
}

void calculator::cleanRPN(TokenQueue_t* rpn) {
  while (rpn->size()) {
    delete rpn->front();
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

calculator::calculator(const char* expr,
                       const Scope& scope, OppMap_t opPrecedence) {
  compile(expr, scope, opPrecedence);
}

void calculator::compile(const char* expr, OppMap_t opPrecedence) {
  // Make sure it is empty:
  cleanRPN(&this->RPN);

  this->RPN = calculator::toRPN(expr, NULL, opPrecedence);
}

void calculator::compile(const char* expr,
                         const Scope& vars, OppMap_t opPrecedence) {
  // Make sure it is empty:
  cleanRPN(&this->RPN);

  this->RPN = calculator::toRPN(expr, &vars, opPrecedence);
}

packToken calculator::eval(const Scope& vars) {
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

std::string calculator::str() {
  std::stringstream ss;
  TokenQueue_t rpn = this->RPN;

  ss << "calculator { RPN: [ ";
  while (rpn.size()) {
    ss << packToken(rpn.front()->clone()).str();
    rpn.pop();

    ss << (rpn.size() ? ", ":"");
  }
  ss << " ] }";
  return ss.str();
}

/* * * * * Scope Class: * * * * */

Scope::Scope(TokenMap_t* vars) {
  // Add default functions to the global namespace:
  scope.push_front(&Function::default_functions);

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

void Scope::asign(std::string key, TokenBase* value) const {
  if (value) {
    value = value->clone();
  } else {
    throw std::invalid_argument("Scope asignment expected a non NULL argument as value!");
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
