#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

#include <list>
#include <string>

class Function : public TokenBase {
 public:
  typedef std::list<std::string> argsList;

  // Used only to initialize
  // default functions on program startup.
  std::string name;

  Function() { this->type = FUNC; }
  virtual ~Function() {}

  virtual const argsList args() const = 0;
  virtual packToken exec(packMap scope) const = 0;
  virtual TokenBase* clone() const = 0;
};

class CppFunction : public Function {
 private:
  // Used only to initialize
  // builtin functions at startup.
  struct Startup;

 public:
  packToken (*func)(packMap);
  argsList _args;

  CppFunction(packToken (*func)(packMap), unsigned int nargs,
              const char** args, std::string name = "");

  virtual const argsList args() const { return _args; }
  virtual packToken exec(packMap scope) const { return func(scope); }

  virtual TokenBase* clone() const {
    return new CppFunction(static_cast<const CppFunction&>(*this));
  }
};

#endif  // FUNCTIONS_H_
