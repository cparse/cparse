// Source: http://www.daniweb.com/software-development/cpp/code/427500/calculator-using-shunting-yard-algorithm#
// Author: Jesse Brown
// Modifications: Brandon Amos, redpois0n

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <math.h>

#include "shunting-yard.h"

OppMap_t calculator::buildOpPrecedence() {
  OppMap_t opp;

  // Create the operator precedence map based on C++ default
  // precedence order as described on cppreference website:
  // http://en.cppreference.com/w/c/language/operator_precedence
  opp["^"]  = 2;
  opp["*"]  = 3; opp["/"]  = 3; opp["%"] = 3;
  opp["+"]  = 4; opp["-"]  = 4;
  opp["<<"] = 5; opp[">>"] = 5;
  opp["<"]  = 6; opp["<="] = 6; opp[">="] = 6; opp[">"] = 6;
  opp["=="] = 7; opp["!="] = 7;
  opp["&&"] = 11;
  opp["||"] = 12;
  opp["("]  = 16;

  return opp;
}
// Builds the opPrecedence map only once:
OppMap_t calculator::opPrecedence = calculator::buildOpPrecedence();

#define isvariablechar(c) (isalpha(c) || c == '_')
TokenQueue_t calculator::toRPN(const char* expr,
    TokenMap_t* vars, OppMap_t opPrecedence) {
  TokenQueue_t rpnQueue; std::stack<std::string> operatorStack;
  bool lastTokenWasOp = true;

  // In one pass, ignore whitespace and parse the expression into RPN
  // using Dijkstra's Shunting-yard algorithm.
  while (*expr && isspace(*expr )) ++expr;
  while (*expr ) {
    if (isdigit(*expr )) {
      // If the token is a number, add it to the output queue.
      char* nextChar = 0;
      double digit = strtod(expr , &nextChar);
#     ifdef DEBUG
        std::cout << digit << std::endl;
#     endif
      rpnQueue.push(new Token<double>(digit, NUM));
      expr = nextChar;
      lastTokenWasOp = false;
    } else if (isvariablechar(*expr )) {
      // If the function is a variable, resolve it and
      // add the parsed number to the output queue.
      std::stringstream ss;
      ss << *expr;
      ++expr;
      while( isvariablechar(*expr ) || isdigit(*expr) ) {
        ss << *expr;
        ++expr;
      }

      bool found = false;
      double val;

      std::string key = ss.str();

      if(key == "true") {
        found = true; val = 1;
      } else if(key == "false") {
        found = true; val = 0;
      } else if(vars) {
        TokenMap_t::iterator it = vars->find(key);
        if(it != vars->end()) { found = true; val = it->second; }
      }

      if (found) {
        // Save the number
  #     ifdef DEBUG
          std::cout << val << std::endl;
  #     endif
        rpnQueue.push(new Token<double>(val, NUM));;
      } else {
        // Save the variable name:
  #     ifdef DEBUG
          std::cout << key << std::endl;
  #     endif
        rpnQueue.push(new Token<std::string>(key, VAR));
      }

      lastTokenWasOp = false;
    } else {
      // Otherwise, the variable is an operator or paranthesis.
      switch (*expr) {
        case '(':
          operatorStack.push("(");
          ++expr;
          break;
        case ')':
          while (operatorStack.top().compare("(")) {
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
            while (*expr && !isspace(*expr ) && !isdigit(*expr )
                && !isvariablechar(*expr) && *expr != '(' && *expr != ')') {
              ss << *expr;
              ++expr;
            }
            ss.clear();
            std::string str;
            ss >> str;
#           ifdef DEBUG
              std::cout << str << std::endl;
#           endif

            if (lastTokenWasOp) {
              // Convert unary operators to binary in the RPN.
              if (!str.compare("-") || !str.compare("+")) {
                rpnQueue.push(new Token<double>(0, NUM));
              } else {
                throw std::domain_error(
                    "Unrecognized unary operator: '" + str + "'.");
              }
            }

            while (!operatorStack.empty() &&
                opPrecedence[str] >= opPrecedence[operatorStack.top()]) {
              rpnQueue.push(new Token<std::string>(operatorStack.top(), OP));
              operatorStack.pop();
            }
            operatorStack.push(str);
            lastTokenWasOp = true;
          }
      }
    }
    while (*expr && isspace(*expr )) ++expr;
  }
  while (!operatorStack.empty()) {
    rpnQueue.push(new Token<std::string>(operatorStack.top(), OP));
    operatorStack.pop();
  }
  return rpnQueue;
}

double calculator::calculate(const char* expr,
    TokenMap_t* vars) {

  // Convert to RPN with Dijkstra's Shunting-yard algorithm.
  TokenQueue_t rpn = toRPN(expr, vars);

  double ret = calculate(rpn);

  cleanRPN(rpn);

  return ret;
}

double calculator::calculate(TokenQueue_t _rpn,
    TokenMap_t* vars) {

  TokenQueue_t rpn;

  // Deep copy the token list, so everything can be
  // safely deallocated:
  while(!_rpn.empty()) {
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
      Token<std::string>* strTok = static_cast<Token<std::string>*>(base);
      std::string str = strTok->val;
      if (evaluation.size() < 2) {
        throw std::domain_error("Invalid equation.");
      }
      TokenBase* b_right = evaluation.top(); evaluation.pop();
      TokenBase* b_left  = evaluation.top(); evaluation.pop();
      if(b_right->type == NUM && b_left->type == NUM) {
        double right = static_cast<Token<double>*>(b_right)->val;
        double left = static_cast<Token<double>*>(b_left)->val;
        delete b_right;
        delete b_left;

        if (!str.compare("+")) {
          evaluation.push(new Token<double>(left + right, NUM));
        } else if (!str.compare("*")) {
          evaluation.push(new Token<double>(left * right, NUM));
        } else if (!str.compare("-")) {
          evaluation.push(new Token<double>(left - right, NUM));
        } else if (!str.compare("/")) {
          evaluation.push(new Token<double>(left / right, NUM));
        } else if (!str.compare("<<")) {
          evaluation.push(new Token<double>((int) left << (int) right, NUM));
        } else if (!str.compare("^")) {
          evaluation.push(new Token<double>(pow(left, right), NUM));
        } else if (!str.compare(">>")) {
          evaluation.push(new Token<double>((int) left >> (int) right, NUM));
        } else if (!str.compare("%")) {
          evaluation.push(new Token<double>((int) left % (int) right, NUM));
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
          evaluation.push(new Token<double>((int) left && (int) right, NUM));
        } else if (!str.compare("||")) {
          evaluation.push(new Token<double>((int) left || (int) right, NUM));
        } else {
          throw std::domain_error("Unknown operator: '" + str + "'.");
        }
      }
    } else if (base->type == VAR) { // Variable
      if (!vars) {
        throw std::domain_error(
            "Detected variable, but the variable map is null.");
      }

      std::string key = static_cast<Token<std::string>*>(base)->val;
      delete base;

      TokenMap_t::iterator it = vars->find(key);

      if (it == vars->end()) {
        throw std::domain_error(
            "Unable to find the variable '" + key + "'.");
      }
      evaluation.push(new Token<double>(it->second, NUM));
    } else {
      evaluation.push(base);
    }
  }
  TokenBase* top = evaluation.top();
  evaluation.pop();

  double result = static_cast<Token<double>*>(top)->val;
  delete top;

  return result;
}

void calculator::cleanRPN(TokenQueue_t& rpn) {
  while( rpn.size() ) {
    delete rpn.front();
    rpn.pop();
  }
}

/* * * * * Non Static Functions * * * * */

calculator::~calculator() {
  cleanRPN(this->RPN);
}

calculator::calculator(const char* expr,
    TokenMap_t* vars, OppMap_t opPrecedence) {
  compile(expr, vars, opPrecedence);
}

void calculator::compile(const char* expr,
    TokenMap_t* vars, OppMap_t opPrecedence) {

  // Make sure it is empty:
  cleanRPN(this->RPN);

  this->RPN = calculator::toRPN(expr, vars, opPrecedence);
}

double calculator::eval(TokenMap_t* vars) {
  return calculate(this->RPN, vars);
}

/* * * * * For Debug Only * * * * */

std::string calculator::str() {
  std::stringstream ss;
  TokenQueue_t rpn = this->RPN;

  ss << "calculator { RPN: [ ";
  while( rpn.size() ) {
    TokenBase* base = rpn.front();

    Token<double>* doubleTok = dynamic_cast<Token<double>*>(base);
    if(doubleTok)
      ss << doubleTok->val;

    Token<std::string>* strTok = dynamic_cast<Token<std::string>*>(base);
    if(strTok)
      ss << "'" << strTok->val << "'";

    rpn.pop();

    ss << (rpn.size() ? ", ":"");
  }
  ss << " ] }";
  return ss.str();
}
