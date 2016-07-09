#include <cstdio>
#include <cmath>
#include "./shunting-yard.h"

TokenMap_t Function::default_functions = Function::initialize_functions();

std::string text_arg[] = {"text"};
packToken default_print(const Scope* args) {
  // Get a single argument:
  std::string text = args->find("text")->asString();
  printf(text.c_str());
  printf("\n");

  return packToken::None;
}

std::string num_arg[] = {"number"};
packToken default_sqrt(const Scope* args) {
  // Get a single argument:
  double number = args->find("number")->asDouble();

  return sqrt(number);
}
packToken default_sin(const Scope* args) {
  // Get a single argument:
  double number = args->find("number")->asDouble();

  return sin(number);
}
packToken default_cos(const Scope* args) {
  // Get a single argument:
  double number = args->find("number")->asDouble();

  return cos(number);
}
packToken default_tan(const Scope* args) {
  // Get a single argument:
  double number = args->find("number")->asDouble();

  return tan(number);
}

TokenMap_t Function::initialize_functions() {
  TokenMap_t funcs;

  funcs["print"] = Function(&default_print, 1, text_arg);
  funcs["sqrt"] = Function(&default_sqrt, 1, num_arg);
  funcs["sin"] = Function(&default_sin, 1, num_arg);
  funcs["cos"] = Function(&default_cos, 1, num_arg);
  funcs["tan"] = Function(&default_tan, 1, num_arg);
  return funcs;
}
