#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <string>
#include <stdexcept>
#include <cerrno>
#include <iostream>
#include <cctype>  // For tolower() and toupper()

#include "./shunting-yard.h"
#include "./functions.h"
#include "./shunting-yard-exceptions.h"

/* * * * * class Function * * * * */
packToken Function::call(packToken _this, Function* func,
                         TokenList* args, TokenMap scope) {
  // Build the local namespace:
  TokenMap kwargs;
  TokenMap local = scope.getChild();

  args_t arg_names = func->args();

  TokenList_t::iterator args_it = args->list().begin();
  args_t::const_iterator names_it = arg_names.begin();

  /* * * * * Parse named arguments: * * * * */

  while (args_it != args->list().end() && names_it != arg_names.end()) {
    // If the positional argument list is over:
    if ((*args_it)->type == STUPLE) break;

    // Else add it to the local namespace:
    local[*names_it] = *args_it;

    ++args_it;
    ++names_it;
  }

  /* * * * * Parse extra positional arguments: * * * * */

  TokenList arglist;
  for (; args_it != args->list().end(); ++args_it) {
    // If there is a keyword argument:
    if ((*args_it)->type == STUPLE) break;
    // Else add it to arglist:
    arglist.list().push_back(*args_it);
  }

  /* * * * * Parse keyword arguments: * * * * */

  for (; args_it != args->list().end(); ++args_it) {
    packToken& arg = *args_it;

    if (arg->type != STUPLE) {
      throw syntax_error("Positional argument follows keyword argument");
    }

    STuple* st = static_cast<STuple*>(arg.token());

    if (st->list().size() != 2) {
      throw syntax_error("Keyword tuples must have exactly 2 items!");
    }

    if (st->list()[0]->type != STR) {
      throw syntax_error("Keyword first argument should be of type string!");
    }

    // Save it:
    std::string key = st->list()[0].asString();
    packToken& value = st->list()[1];
    kwargs[key] = value;
  }

  /* * * * * Set missing arguments to None: * * * * */

  for (; names_it != arg_names.end(); ++names_it) {
    // If not set by a keyword argument:
    if (local.map().count(*names_it) == 0) {
      local[*names_it] = packToken::None;
    }
  }

  /* * * * * Set built-in variables: * * * * */

  local["this"] = _this;
  local["args"] = arglist;
  local["kwargs"] = kwargs;

  return func->exec(local);
}

namespace builtin_functions {

/* * * * * Built-in Functions: * * * * */

packToken default_print(TokenMap scope) {
  // Get the argument list:
  TokenList list = scope["args"].asList();

  bool first = true;
  for (packToken item : list.list()) {
    if (first) {
      first = false;
    } else {
      std::cout << " ";
    }

    if (item->type == STR) {
      std::cout << item.asString();
    } else {
      std::cout << item.str();
    }
  }

  std::cout << std::endl;

  return packToken::None;
}

packToken default_sum(TokenMap scope) {
  // Get the arguments:
  TokenList list = scope["args"].asList();

  if (list.list().size() == 1 && list.list().front()->type == LIST) {
    list = list.list().front().asList();
  }

  double sum = 0;
  for (packToken num : list.list()) {
    sum += num.asDouble();
  }

  return sum;
}

const args_t value_arg = {"value"};
packToken default_eval(TokenMap scope) {
  std::string code = scope["value"].asString();
  // Evaluate it as a calculator expression:
  return calculator::calculate(code.c_str(), scope);
}

packToken default_float(TokenMap scope) {
  packToken tok = scope["value"];
  if (tok->type & NUM) return tok.asDouble();

  // Convert it to double:
  char* rest;
  const std::string& str = tok.asString();
  errno = 0;
  double ret = strtod(str.c_str(), &rest);

  if (str == rest) {
    throw std::runtime_error("Could not convert \"" + str + "\" to float!");
  } else if (errno) {
    std::range_error("Value too big or too small to fit a Double!");
  }
  return ret;
}

packToken default_int(TokenMap scope) {
  packToken tok = scope["value"];
  if (tok->type & NUM) return tok.asInt();

  // Convert it to double:
  char* rest;
  const std::string& str = tok.asString();
  errno = 0;
  int64_t ret = strtol(str.c_str(), &rest, 10);

  if (str == rest) {
    throw std::runtime_error("Could not convert \"" + str + "\" to integer!");
  } else if (errno) {
    std::range_error("Value too big or too small to fit an Integer!");
  }
  return ret;
}

packToken default_str(TokenMap scope) {
  // Return its string representation:
  packToken tok = scope["value"];
  if (tok->type == STR) return tok;
  return tok.str();
}

packToken default_type(TokenMap scope) {
  packToken tok = scope["value"];
  switch (tok->type) {
  case NONE: return "none";
  case VAR: return "variable";
  case REAL: return "float";
  case INT: return "integer";
  case STR: return "string";
  case FUNC: return "function";
  case IT: return "iterable";
  case TUPLE: return "tuple";
  case STUPLE: return "argument tuple";
  case LIST: return "list";
  case MAP: return "map";
  default: return "unknown_type";
  }
}

packToken default_extend(TokenMap scope) {
  packToken tok = scope["value"];

  if (tok->type == MAP) {
    return tok.asMap().getChild();
  } else {
    throw std::runtime_error(tok.str() + " is not extensible!");
  }
}

packToken default_instanceof(TokenMap scope) {
  TokenMap _super = scope["value"].asMap();
  TokenMap* _this = scope["this"].asMap().parent();

  TokenMap* parent = _this;
  while (parent) {
    if ((*parent) == _super) {
      return true;
    }

    parent = parent->parent();
  }

  return false;
}

const args_t num_arg = {"number"};
packToken default_sqrt(TokenMap scope) {
  // Get a single argument:
  double number = scope["number"].asDouble();

  return sqrt(number);
}
packToken default_sin(TokenMap scope) {
  // Get a single argument:
  double number = scope["number"].asDouble();

  return sin(number);
}
packToken default_cos(TokenMap scope) {
  // Get a single argument:
  double number = scope["number"].asDouble();

  return cos(number);
}
packToken default_tan(TokenMap scope) {
  // Get a single argument:
  double number = scope["number"].asDouble();

  return tan(number);
}
packToken default_abs(TokenMap scope) {
  // Get a single argument:
  double number = scope["number"].asDouble();

  return std::abs(number);
}

const args_t pow_args = {"number", "exp"};
packToken default_pow(TokenMap scope) {
  // Get two arguments:
  double number = scope["number"].asDouble();
  double exp = scope["exp"].asDouble();

  return pow(number, exp);
}

/* * * * * Type-specific default functions * * * * */

packToken string_len(TokenMap scope) {
  std::string str = scope["this"].asString();
  return static_cast<int64_t>(str.size());
}

packToken string_lower(TokenMap scope) {
  std::string str = scope["this"].asString();
  std::string out;
  for (char c : str) {
    out.push_back(tolower(c));
  }
  return out;
}

packToken string_upper(TokenMap scope) {
  std::string str = scope["this"].asString();
  std::string out;
  for (char c : str) {
    out.push_back(toupper(c));
  }
  return out;
}

/* * * * * default constructor functions * * * * */

packToken default_list(TokenMap scope) {
  // Get the arguments:
  TokenList list = scope["args"].asList();

  // If the only argument is iterable:
  if (list.list().size() == 1 && list.list()[0]->type & IT) {
    TokenList new_list;
    Iterator* it = static_cast<Iterable*>(list.list()[0].token())->getIterator();

    packToken* next = it->next();
    while (next) {
      new_list.list().push_back(*next);
      next = it->next();
    }

    delete it;
    return new_list;
  } else {
    return list;
  }
}

packToken default_map(TokenMap scope) {
  return scope["kwargs"];
}

}  // namespace builtin_functions

/* * * * * class CppFunction * * * * */

CppFunction::CppFunction(packToken (*func)(TokenMap), const args_t args,
                         std::string name)
                         : func(func), _args(args) {
  this->_name = name;
}

CppFunction::CppFunction(packToken (*func)(TokenMap), unsigned int nargs,
                         const char** args, std::string name)
                         : func(func) {
  this->_name = name;
  // Add all strings to args list:
  for (uint32_t i = 0; i < nargs; ++i) {
    this->_args.push_back(args[i]);
  }
}

// Build a function with no named args:
CppFunction::CppFunction(packToken (*func)(TokenMap), std::string name)
                         : func(func) {
  this->_name = name;
}

/* * * * * CppFunction Initializer Constructor: * * * * */

namespace builtin_functions {

struct Startup {
  Startup() {
    TokenMap& global = TokenMap::default_global();

    global["print"] = CppFunction(&default_print, "print");
    global["sum"] = CppFunction(&default_sum, "sum");
    global["sqrt"] = CppFunction(&default_sqrt, num_arg, "sqrt");
    global["sin"] = CppFunction(&default_sin, num_arg, "sin");
    global["cos"] = CppFunction(&default_cos, num_arg, "cos");
    global["tan"] = CppFunction(&default_tan, num_arg, "tan");
    global["abs"] = CppFunction(&default_abs, num_arg, "abs");
    global["pow"] = CppFunction(&default_pow, pow_args, "pow");
    global["float"] = CppFunction(&default_float, value_arg, "float");
    global["str"] = CppFunction(&default_str, value_arg, "str");
    global["eval"] = CppFunction(&default_eval, value_arg, "eval");
    global["type"] = CppFunction(&default_type, value_arg, "type");
    global["extend"] = CppFunction(&default_extend, value_arg, "extend");

    // Default constructors:
    global["list"] = CppFunction(&default_list, "list");
    global["map"] = CppFunction(&default_map, "map");

    TokenMap& base_map = TokenMap::base_map();
    base_map["instanceof"] = CppFunction(&default_instanceof,
                                         value_arg, "instanceof");

    typeMap_t& type_map = calculator::type_attribute_map();
    type_map[STR]["len"] = CppFunction(&string_len, "len");
    type_map[STR]["lower"] = CppFunction(&string_lower, "lower");
    type_map[STR]["upper"] = CppFunction(&string_upper, "upper");
  }
} base_functions_startup;

}  // namespace builtin_functions
