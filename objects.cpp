#include "./shunting-yard.h"

#include "./objects.h"
#include "./functions.h"

/* * * * * Initialize TokenMap * * * * */

// Using the "Construct On First Use Idiom"
// to avoid the "static initialization order fiasco",
// for more information read:
//
// - https://isocpp.org/wiki/faq/ctors#static-init-order
//
TokenMap& TokenMap::base_map() {
  static TokenMap global_map(0);
  return global_map;
}

TokenMap& TokenMap::default_global() {
  static TokenMap global_map(base_map());
  return global_map;
}

TokenMap TokenMap::empty = TokenMap(&default_global());

/* * * * * TokenList built-in functions * * * * */

const char* push_args[] = {"item"};
packToken list_push(packMap scope) {
  packToken* list = scope->find("this");
  packToken* token = scope->find("item");
  
  // If "this" is not a list it will throw here:
  list->asList()->list.push_back(*token);

  return *list;
}

const char* pop_args[] = {"pos"};
packToken list_pop(packMap scope) {
  TokenList* list = scope->find("this")->asList();
  packToken* token = scope->find("pos");

  uint pos;

  if ((*token)->type == NUM) {
    pos = (uint)(token->asDouble());
  } else {
    pos = list->list.size()-1;
  }

  packToken result = list->list[pos];

  // Erase the item from the list:
  // Note that this operation is optimal if pos == list.size()
  list->list.erase(list->list.begin() + pos);

  return result;
}

const char* list_no_args[] = {""};
packToken list_len(packMap scope) {
  packList list = scope->find("this")->asList();
  return list->list.size();
}

/* * * * * Initialize TokenList functions * * * * */

struct TokenList::Startup {
  Startup() {
    TokenMap& base = calculator::type_attribute_map()[LIST];
    base["push"] = CppFunction(list_push, 1, push_args, "push");
    base["pop"] = CppFunction(list_pop, 1, pop_args, "pop");
    base["len"] = CppFunction(list_len, 0, list_no_args, "len");
  }
} list_startup;

/* * * * * TokenList iterator implemented functions * * * * */

packToken* TokenList::next() {
  if (i < list.size()) {
    return &list[i++];
  } else {
    i = 0;
    return NULL;
  }
}

void TokenList::reset() { i = 0; }

