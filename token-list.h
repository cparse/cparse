#include <list>

#ifndef TOKEN_LIST_H_
#define TOKEN_LIST_H_

class TokenList {
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

#endif  // TOKEN_LIST_H_
