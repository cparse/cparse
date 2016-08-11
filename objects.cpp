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

/* * * * * TokenList built-in functions * * * * */

const char* push_args[] = {"item"};
packToken list_push(TokenMap scope) {
  packToken* list = scope.find("this");
  packToken* token = scope.find("item");

  // If "this" is not a list it will throw here:
  list->asList()->list.push_back(*token);

  return *list;
}

const char* pop_args[] = {"pos"};
packToken list_pop(TokenMap scope) {
  TokenList* list = scope.find("this")->asList();
  packToken* token = scope.find("pos");

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

packToken list_len(TokenMap scope) {
  packList list = scope.find("this")->asList();
  return list->list.size();
}

/* * * * * Initialize TokenList functions * * * * */

struct TokenList::Startup {
  Startup() {
    TokenMap& base = calculator::type_attribute_map()[LIST];
    base["push"] = CppFunction(list_push, 1, push_args, "push");
    base["pop"] = CppFunction(list_pop, 1, pop_args, "pop");
    base["len"] = CppFunction(list_len, "len");
  }
} list_startup;

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
MapData_t::~MapData_t() { if(parent) delete parent; }

MapData_t& MapData_t::operator=(const MapData_t& other) {
  if (this != &other) {
    if(parent) delete parent;
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

const packToken* TokenMap::find(std::string key) const {
  TokenMap_t::const_iterator it = map().find(key);

  if (it != map().end()) {
    return &it->second;
  } else if (parent()) {
    return parent()->find(key);
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
