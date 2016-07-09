
#ifndef SHUNTING_YARD_FUNC_H_
#define SHUNTING_YARD_FUNC_H_

class Function : public TokenBase {
  static TokenMap_t initialize_functions();

 public:
  static TokenMap_t default_functions;

 public:
  packToken (*func)(const Scope*);
  unsigned nargs;
  std::string* args;

  Function(packToken (*func)(const Scope*), unsigned nargs, std::string* args)
           : func(func), nargs(nargs), args(args) { this->type = FUNC; }

  virtual TokenBase* clone() const {
    return new Function(static_cast<const Function&>(*this));
  }
};

#endif
