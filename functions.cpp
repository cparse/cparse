#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <string>
#include <stdexcept>

#include "./shunting-yard.h"
#include "./functions.h"

/* * * * * Built-in Functions: * * * * */

const char* text_arg[] = {"text"};
packToken default_print(const Scope* scope) {
  // Get a single argument:
  packToken* p = scope->find("text");
  if ((*p)->type == STR) {
    std::string text = p->asString();
    printf("%s\n", text.c_str());
  } else {
    printf("\n");
  }

  return packToken::None;
}

const char* value_arg[] = {"value"};
packToken default_eval(const Scope* scope) {
  std::string code = scope->find("value")->asString();
  // Evaluate it as a calculator expression:
  return calculator::calculate(code.c_str(), *scope);
}
packToken default_float(const Scope* scope) {
  packToken* tok = scope->find("value");
  // Return it as double:
  return atof(tok->asString().c_str());
}
packToken default_str(const Scope* scope) {
  // Return its string representation:
  packToken* tok = scope->find("value");
  if ((*tok)->type == STR) return *tok;
  return tok->str();
}

const char* num_arg[] = {"number"};
packToken default_sqrt(const Scope* scope) {
  // Get a single argument:
  double number = scope->find("number")->asDouble();

  return sqrt(number);
}
packToken default_sin(const Scope* scope) {
  // Get a single argument:
  double number = scope->find("number")->asDouble();

  return sin(number);
}
packToken default_cos(const Scope* scope) {
  // Get a single argument:
  double number = scope->find("number")->asDouble();

  return cos(number);
}
packToken default_tan(const Scope* scope) {
  // Get a single argument:
  double number = scope->find("number")->asDouble();

  return tan(number);
}
packToken default_abs(const Scope* scope) {
  // Get a single argument:
  double number = scope->find("number")->asDouble();

  return std::abs(number);
}


const char* pow_args[] = {"number", "exp"};
packToken default_pow(const Scope* scope) {
  // Get two arguments:
  double number = scope->find("number")->asDouble();
  double exp = scope->find("exp")->asDouble();

  return pow(number, exp);
}

/* * * * * Function Initializer Constructor: * * * * */

struct Function::Startup {
  Startup() {
    TokenMap_t& global = Scope::default_global();

    global["print"] = Function(&default_print, 1, text_arg, "print");
    global["sqrt"] = Function(&default_sqrt, 1, num_arg, "sqrt");
    global["sin"] = Function(&default_sin, 1, num_arg, "sin");
    global["cos"] = Function(&default_cos, 1, num_arg, "cos");
    global["tan"] = Function(&default_tan, 1, num_arg, "tan");
    global["abs"] = Function(&default_abs, 1, num_arg, "abs");
    global["pow"] = Function(&default_pow, 2, pow_args, "pow");
    global["float"] = Function(&default_float, 1, value_arg, "float");
    global["str"] = Function(&default_str, 1, value_arg, "str");
    global["eval"] = Function(&default_eval, 1, value_arg, "eval");
  }
} startup;

/* * * * * Tuple Functions: * * * * */

Tuple::Tuple(const TokenBase* a) {
  tuple.push_back(a->clone());
  this->type = TUPLE;
}
Tuple::Tuple(const TokenBase* a, const TokenBase* b) {
  tuple.push_back(a->clone());
  tuple.push_back(b->clone());
  this->type = TUPLE;
}

void Tuple::push_back(const TokenBase* tb) {
  tuple.push_back(tb->clone());
}

TokenBase* Tuple::pop_front() {
  if (tuple.size() == 0) {
    throw std::range_error("Can't pop front of an empty Tuple!");
  }

  TokenBase* value = tuple.front();
  tuple.pop_front();
  return value;
}

unsigned Tuple::size() {
  return tuple.size();
}

Tuple::Tuple_t Tuple::copyTuple(const Tuple_t& t) {
  Tuple_t copy;
  Tuple_t::const_iterator it;
  for (it = t.begin(); it != t.end(); ++it) {
    copy.push_back((*it)->clone());
  }
  return copy;
}

void Tuple::cleanTuple(Tuple_t* t) {
  while (t->size()) {
    delete t->back();
    t->pop_back();
  }
}

Tuple& Tuple::operator=(const Tuple& t) {
  cleanTuple(&tuple);
  tuple = copyTuple(t.tuple);
  return *this;
}
