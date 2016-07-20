
#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

#include <list>
#include <string>

class Function : public TokenBase {
  static TokenMap_t initialize_functions();

 private:
  // Used only to initialize
  // default functions on program startup.
  struct Startup;

 public:
  packToken (*func)(const Scope*);
  uint nargs;
  const char** arg_names;
  std::string name;

  Function(packToken (*func)(const Scope*), unsigned nargs,
           const char** arg_names, std::string name = "")
           : func(func), nargs(nargs), arg_names(arg_names), name(name) { this->type = FUNC; }
  virtual ~Function() {}

  virtual packToken exec(const Scope* scope) { return func(scope); };

  virtual TokenBase* clone() const {
    return new Function(static_cast<const Function&>(*this));
  }
};

class Tuple : public TokenBase {
 public:
  typedef std::list<TokenBase*> Tuple_t;

 public:
  Tuple_t tuple;

 public:
  Tuple() {}
  Tuple(const TokenBase* a);
  Tuple(const TokenBase* a, const TokenBase* b);
  Tuple(const Tuple& t) : tuple(copyTuple(t.tuple)) { this->type = TUPLE; }
  ~Tuple() { cleanTuple(&tuple); }

 public:
  void push_back(const TokenBase* tb);
  TokenBase* pop_front();
  unsigned size();

 private:
  Tuple_t copyTuple(const Tuple_t& t);
  void cleanTuple(Tuple_t* t);

 public:
  Tuple& operator=(const Tuple& t);

  virtual TokenBase* clone() const {
    return new Tuple(static_cast<const Tuple&>(*this));
  }
};

#endif  // FUNCTIONS_H_
