# C++ expression parsing.

[![Build Status](https://travis-ci.org/bamos/cpp-expression-parser.svg?branch=master)](https://travis-ci.org/bamos/cpp-expression-parser)

A simple form of mathematical expression parsing can take a string such
as `-pi+1` on input and output `-2.14`.
This post presents a C++ library to parse a character sequence
as an expression using Dijkstra's
[Shunting-yard algorithm](http://en.wikipedia.org/wiki/Shunting-yard_algorithm),
which modifies
[Jesse Brown's code](http://www.daniweb.com/software-development/cpp/code/427500/calculator-using-shunting-yard-algorithm).

# Minimal example.

```C
#include <iostream>
#include "shunting-yard.h"

int main() {
  std::map<std::string, double> vars;
  vars["pi"] = 3.14;
  std::cout << calculator::calculate("-pi+1", &vars) << std::endl;

  // Or if you want to evaluate an expression
  // several times efficiently:
  calculator c1("pi-b");
  vars["b"] = 0.14;
  std::cout << c1.eval(&vars) << std::endl; // 3
  vars["b"] = 2.14;
  std::cout << c1.eval(&vars) << std::endl; // 1

  return 0;
}
```

# More examples.
 + See `test-shunting-yard.cpp`.

# Features.
 + Unary operators. +, -
 + Binary operators. +, -, /, *, %, <<, >>, ^
 + Boolean operators. <, >, <=, >=, ==, !=, &&, ||
 + Map of variable names.

# Adding a binary operator.
To add a binary operator,

 1. Update the operator precedence map in `calculator::buildOpPrecedence`.
 2. Add the computation to `calculator::calculate`.

# Implementation Details
The main steps of the calculation process are:

 1. Create the operator precedence map.
 2. Convert to [RPN](http://en.wikipedia.org/wiki/Reverse_Polish_notation)
    with Dijkstra's Shunting-yard algorithm.
 3. Evaluate the expression in RPN form.

## Converting to RPN.
Most of the Shunting-yard algorithm resides here.
The idea is to do everything in one pass for elegance.
Please see the
[source code](https://github.com/bamos/cpp-expression-parser/blob/master/shunting-yard.cpp)
for implementation-specific details,
and refer to the pruned code below for a summary.

```C++
TokenQueue_t calculator::toRPN(const char* expr,
    std::map<std::string, double>* vars,
    std::map<std::string, int> opPrecedence) {
  TokenQueue_t rpnQueue; std::stack<std::string> operatorStack;

  while (*expr ) {
    if (isdigit(*expr )) {
      // If the token is a number, add it to the output queue.
    } else if (isvariablechar(*expr )) {
      // If the function is a variable, resolve it and
      // add the parsed number to the output queue.
    } else {
      // Otherwise, the variable is an operator or parenthesis.
      switch (*expr) {
        case '(':
          operatorStack.push("(");
          ++expr;
          break;
        case ')':
          while (operatorStack.top().compare("(")) {
            rpnQueue.push(new Token<std::string>(operatorStack.top()));
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
          }
      }
    }
  }
  while (!operatorStack.empty()) {
    rpnQueue.push(new Token<std::string>(operatorStack.top()));
    operatorStack.pop();
  }
  return rpnQueue;
}
```


## Evaluating RPN form.
The RPN is represented as tokens in a stack.
To evaluate this, pop all of the elements off and handle
operations when encountered.


```C++
std::stack<double> evaluation;
while (!rpn.empty()) {
  TokenBase* base = rpn.front();
  rpn.pop();

  if (base->type == OP) {
    Token<std::string>* strTok = static_cast<Token<std::string>*>(base);
    std::string str = strTok->val;
    if (evaluation.size() < 2) {
      throw std::domain_error("Invalid equation.");
    }
    double right = evaluation.top(); evaluation.pop();
    double left  = evaluation.top(); evaluation.pop();
    if (!str.compare("+")) {
      evaluation.push(left + right);
    } else if (!str.compare("*")) {
      evaluation.push(left * right);
    } else if (!str.compare("-")) {
      evaluation.push(left - right);
    } else if (!str.compare("/")) {
      evaluation.push(left / right);
    } else if (!str.compare("<<")) {
      evaluation.push((int) left << (int) right);
    } else if (!str.compare(">>")) {
      evaluation.push((int) left >> (int) right);
    } else {
      throw std::domain_error("Unknown operator: '" + str + "'.");
    }
  } else if (base->type == NUM) {
    Token<double>* doubleTok = static_cast<Token<double>*>(base);
    evaluation.push(doubleTok->val);
  } else {
    throw std::domain_error("Invalid token.");
  }
  delete base;
}
```

The evaluated value resides in `evaluation.top` of type double.

# Future work.
See the [issues](https://github.com/bamos/cpp-expression-parser/issues)
on GitHub for work to be done.
I'm currently not actively working on additional features,
so please contribute if you add additional functionality or bug fixes.
