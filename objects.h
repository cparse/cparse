
#ifndef OBJECTS_H_
#define OBJECTS_H_

#include <map>
#include <list>
#include <vector>
#include <string>

// Iterator super class.
struct Iterator {
  // Return the next position of the iterator.
  // When it reaches the end it should return NULL
  // and reset the iterator automatically.
  virtual packToken* next() = 0;
  virtual void reset() = 0;
};

struct TokenMap : public Iterator {
  typedef std::map<std::string, packToken> TokenMap_t;

  // Static factories:
  static TokenMap empty;
  static TokenMap& base_map();
  static TokenMap& default_global();

 public:
  TokenMap_t map;
  TokenMap* parent;

 public:
  TokenMap_t::iterator it;
  packToken* last = 0;
  packToken* next();
  void reset();

 public:
  TokenMap(TokenMap* parent = &TokenMap::base_map()) : parent(parent) {}
  virtual ~TokenMap() {}

 public:
  packToken* find(std::string key);
  const packToken* find(std::string key) const;
  void assign(std::string key, TokenBase* value);
  void insert(std::string key, TokenBase* value);

  TokenMap getChild();

  packToken& operator[](const std::string& str);

  void erase(std::string key);
};

// Build a TokenMap which is a child of default_global()
struct GlobalScope : public TokenMap {
  GlobalScope() : TokenMap(TokenMap::default_global()) {}
};


class TokenList : public Iterator {
  uint i = 0;

  // Used to initialize the default list functions.
  struct Startup;

 public:
  typedef std::vector<packToken> TokenList_t;
  TokenList_t list;

 public:
  TokenList() {}
  TokenList(TokenBase* token) {
    if (token->type != TUPLE) {
      throw std::invalid_argument("Invalid argument to build a list!");
    }

    Tuple* tuple = static_cast<Tuple*>(token);
    for (TokenBase* tb : tuple->tuple) {
      list.push_back(packToken(tb->clone()));
    }
  }
  virtual ~TokenList() {}

  packToken* next();
  void reset();

  packToken& operator[](size_t idx) {
    return list[idx];
  }

  packToken& operator[](double idx) {
    return list[(size_t)idx];
  }
};

#endif  // OBJECTS_H_

