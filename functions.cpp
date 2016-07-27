#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <string>
#include <stdexcept>
#include <cerrno>

#include "./shunting-yard.h"
#include "./functions.h"

/* * * * * Built-in Functions: * * * * */

const char* text_arg[] = {"text"};
packToken default_print(TokenMap* scope) {
  // Get a single argument:
  packToken* p = scope->find("text");
  if ((*p)->type == STR) {
    printf("%s\n", p->asString().c_str());
  } else {
    printf("%s\n", p->str().c_str());
  }

  return packToken::None;
}

const char* value_arg[] = {"value"};
packToken default_eval(TokenMap* scope) {
  std::string code = scope->find("value")->asString();
  // Evaluate it as a calculator expression:
  return calculator::calculate(code.c_str(), scope);
}
packToken default_float(TokenMap* scope) {
  packToken* tok = scope->find("value");
  if ((*tok)->type == NUM) return *tok;

  // Convert it to double:
  char* rest;
  const std::string& str = tok->asString();
  errno = 0;
  double ret = strtod(str.c_str(), &rest);

  if (str == rest) {
    throw std::runtime_error("Could not convert \"" + str + "\" to float!");
  } else if (errno) {
    std::range_error("Value too big or too small to fit a Double!");
  }
  return ret;
}
packToken default_str(TokenMap* scope) {
  // Return its string representation:
  packToken* tok = scope->find("value");
  if ((*tok)->type == STR) return *tok;
  return tok->str();
}

const char* num_arg[] = {"number"};
packToken default_sqrt(TokenMap* scope) {
  // Get a single argument:
  double number = scope->find("number")->asDouble();

  return sqrt(number);
}
packToken default_sin(TokenMap* scope) {
  // Get a single argument:
  double number = scope->find("number")->asDouble();

  return sin(number);
}
packToken default_cos(TokenMap* scope) {
  // Get a single argument:
  double number = scope->find("number")->asDouble();

  return cos(number);
}
packToken default_tan(TokenMap* scope) {
  // Get a single argument:
  double number = scope->find("number")->asDouble();

  return tan(number);
}
packToken default_abs(TokenMap* scope) {
  // Get a single argument:
  double number = scope->find("number")->asDouble();

  return std::abs(number);
}

const char* pow_args[] = {"number", "exp"};
packToken default_pow(TokenMap* scope) {
  // Get two arguments:
  double number = scope->find("number")->asDouble();
  double exp = scope->find("exp")->asDouble();

  return pow(number, exp);
}

/* * * * * Type-specific default functions * * * */

const char* no_arg[] = {""};
packToken string_len(TokenMap* scope) {
  std::string str = scope->find("this")->asString();
  return static_cast<int>(str.size());
}

/* * * * * class CppFunction * * * * */

CppFunction::CppFunction(packToken (*func)(TokenMap*), unsigned int nargs,
            const char** args, std::string name)
            : func(func) {
  this->name = name;
  // Add all strings to args list:
  for (unsigned int i = 0; i < nargs; ++i) {
    this->_args.push_back(args[i]);
  }
}

/* * * * * CppFunction Initializer Constructor: * * * * */

struct CppFunction::Startup {
  Startup() {
    TokenMap& global = TokenMap::default_global();

    global["print"] = CppFunction(&default_print, 1, text_arg, "print");
    global["sqrt"] = CppFunction(&default_sqrt, 1, num_arg, "sqrt");
    global["sin"] = CppFunction(&default_sin, 1, num_arg, "sin");
    global["cos"] = CppFunction(&default_cos, 1, num_arg, "cos");
    global["tan"] = CppFunction(&default_tan, 1, num_arg, "tan");
    global["abs"] = CppFunction(&default_abs, 1, num_arg, "abs");
    global["pow"] = CppFunction(&default_pow, 2, pow_args, "pow");
    global["float"] = CppFunction(&default_float, 1, value_arg, "float");
    global["str"] = CppFunction(&default_str, 1, value_arg, "str");
    global["eval"] = CppFunction(&default_eval, 1, value_arg, "eval");

    typeMap_t& type_map = calculator::type_attribute_map();
    type_map[STR]["len"] = CppFunction(&string_len, 0, no_arg, "len");
  }
} CppFunction_startup;

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

unsigned int Tuple::size() {
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
