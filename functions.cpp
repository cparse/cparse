#include <cstdio>
#include <cmath>
#include <string>
#include <stdexcept>
#include "./shunting-yard.h"

TokenMap_t Function::default_functions = Function::initialize_functions();

/* * * * * Built-in Functions: * * * * */

std::string text_arg[] = {"text"};
packToken default_print(const Scope* scope) {
  // Get a single argument:
  std::string text = scope->find("text")->asString();
  printf("%s\n", text.c_str());

  return packToken::None;
}

std::string num_arg[] = {"number"};
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

std::string pow_args[] = {"number", "exp"};
packToken default_pow(const Scope* scope) {
  // Get two arguments:
  double number = scope->find("number")->asDouble();
  double exp = scope->find("exp")->asDouble();

  return pow(number, exp);
}

/* * * * * Initializer Function: * * * * */

TokenMap_t Function::initialize_functions() {
  TokenMap_t funcs;

  funcs["print"] = Function(&default_print, 1, text_arg);
  funcs["sqrt"] = Function(&default_sqrt, 1, num_arg);
  funcs["sin"] = Function(&default_sin, 1, num_arg);
  funcs["cos"] = Function(&default_cos, 1, num_arg);
  funcs["tan"] = Function(&default_tan, 1, num_arg);
  funcs["pow"] = Function(&default_pow, 2, pow_args);
  return funcs;
}

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
