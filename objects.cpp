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
  static TokenMap _base_map(0);
  return _base_map;
}

TokenMap& TokenMap::default_global() {
  static TokenMap global_map(base_map());
  return global_map;
}

TokenMap TokenMap::empty = TokenMap(&default_global());

/* * * * * TokenMap built-in functions * * * * */

const args_t map_pop_args = {"key", "default"};
packToken map_pop(TokenMap scope) {
  TokenMap map = scope["this"].asMap();
  std::string key = scope["key"].asString();

  // Check if the item is available and remove it:
  if (map.map().count(key)) {
    packToken value = map[key];
    map.erase(key);
    return value;
  }

  // If not available return the default value or None
  packToken* def = scope.find("default");
  if (def) {
    return *def;
  } else {
    return packToken::None;
  }
}

packToken map_len(TokenMap scope) {
  TokenMap map = scope.find("this")->asMap();
  return map.map().size();
}

/* * * * * TokenList built-in functions * * * * */

const args_t push_args = {"item"};
packToken list_push(TokenMap scope) {
  packToken* list = scope.find("this");
  packToken* token = scope.find("item");

  // If "this" is not a list it will throw here:
  list->asList().list().push_back(*token);

  return *list;
}

const args_t list_pop_args = {"pos"};
packToken list_pop(TokenMap scope) {
  TokenList list = scope.find("this")->asList();
  packToken* token = scope.find("pos");

  int64_t pos;

  if ((*token)->type & NUM) {
    pos = token->asInt();

    // So that pop(-1) is the same as pop(last_idx):
    if (pos < 0) pos = list.list().size()-pos;
  } else {
    pos = list.list().size()-1;
  }

  packToken result = list.list()[pos];

  // Erase the item from the list:
  // Note that this operation is optimal if pos == list.size()-1
  list.list().erase(list.list().begin() + pos);

  return result;
}

packToken list_len(TokenMap scope) {
  TokenList list = scope.find("this")->asList();
  return list.list().size();
}

/* * * * * Initialize TokenList functions * * * * */

struct TokenList::Startup {
  Startup() {
    TokenMap& base_list = calculator::type_attribute_map()[LIST];
    base_list["push"] = CppFunction(list_push, push_args, "push");
    base_list["pop"] = CppFunction(list_pop, list_pop_args, "pop");
    base_list["len"] = CppFunction(list_len, "len");

    TokenMap& base_map = TokenMap::base_map();
    base_map["pop"] = CppFunction(map_pop, map_pop_args, "pop");
    base_map["len"] = CppFunction(map_len, "len");
  }
} list_startup;

/* * * * * Iterator functions * * * * */

Iterator*  Iterator::getIterator() {
  return static_cast<Iterator*>(this->clone());
}

/* * * * * TokenMap iterator implemented functions * * * * */

packToken* TokenMap::MapIterator::next() {
  if (it != map.end()) {
    last = packToken(it->first);
    ++it;
    return &last;
  } else {
    it = map.begin();
    return NULL;
  }
}

void TokenMap::MapIterator::reset() { it = map.begin(); }

/* * * * * TokenList iterator implemented functions * * * * */

packToken* TokenList::ListIterator::next() {
  if (i < list->size()) {
    return &list->at(i++);
  } else {
    i = 0;
    return NULL;
  }
}

void TokenList::ListIterator::reset() { i = 0; }

/* * * * * MapData_t struct: * * * * */

MapData_t::MapData_t(TokenMap* p) : parent(p ? new TokenMap(*p) : 0) {}
MapData_t::MapData_t(const MapData_t& other) {
  map = other.map;
  if (other.parent) {
    parent = new TokenMap(*(other.parent));
  } else {
    parent = 0;
  }
}
MapData_t::~MapData_t() { if (parent) delete parent; }

MapData_t& MapData_t::operator=(const MapData_t& other) {
  if (this != &other) {
    if (parent) delete parent;
    map = other.map;
    parent = other.parent;
  }
  return *this;
}

/* * * * * TokenMap Class: * * * * */

packToken* TokenMap::find(std::string key) {
  TokenMap_t::iterator it = map().find(key);

  if (it != map().end()) {
    return &it->second;
  } else if (parent()) {
    return parent()->find(key);
  } else {
    return 0;
  }
}

TokenMap* TokenMap::findMap(std::string key) {
  TokenMap_t::iterator it = map().find(key);

  if (it != map().end()) {
    return this;
  } else if (parent()) {
    return parent()->findMap(key);
  } else {
    return 0;
  }
}

void TokenMap::assign(std::string key, TokenBase* value) {
  if (value) {
    value = value->clone();
  } else {
    throw std::invalid_argument("TokenMap assignment expected a non NULL argument as value!");
  }

  packToken* variable = find(key);

  if (variable) {
    (*variable) = packToken(value);
  } else {
    map()[key] = packToken(value);
  }
}

void TokenMap::insert(std::string key, TokenBase* value) {
  (*this)[key] = packToken(value->clone());
}

packToken& TokenMap::operator[](const std::string& key) {
  return map()[key];
}

TokenMap TokenMap::getChild() {
  return TokenMap(this);
}

void TokenMap::erase(std::string key) {
  map().erase(key);
}
