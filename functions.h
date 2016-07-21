
#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

#include <list>
#include <string>

class Function : public TokenBase {
 public:
  typedef std::list<std::string> argsList;

 public:
  std::string name;

  Function() { this->type = FUNC; }
  virtual ~Function() {}

  virtual const argsList args() const = 0;
  virtual packToken exec(const Scope* scope) const = 0;
  virtual TokenBase* clone() const = 0;
};

class CppFunction : public Function {
 private:
  // Used only to initialize
  // builtin functions at startup.
  struct Startup;

 public:
  packToken (*func)(const Scope*);
  argsList _args;

  CppFunction(packToken (*func)(const Scope*), uint nargs,
              const char** args, std::string name = "");

  virtual const argsList args() const { return _args; }
  virtual packToken exec(const Scope* scope) const { return func(scope); }

  virtual TokenBase* clone() const {
    return new CppFunction(static_cast<const CppFunction&>(*this));
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
  uint size();

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
