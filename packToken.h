#ifndef PACKTOKEN_H_
#define PACKTOKEN_H_

#include <string>

class calculator;
class Scope;

// Encapsulate TokenBase* into a friendlier interface
class packToken {
  TokenBase* base;
 public:
  static const packToken None;

 public:
  packToken() : base(0) {}
  packToken(const TokenBase& t) : base(t.clone()) {}
  packToken(const packToken& t) : base(t.base ? t.base->clone() : 0) {}
  packToken& operator=(const packToken& t);

  template<class C>
  packToken(C c, tokType type) : base(new Token<C>(c, type)) {}
  packToken(int d) : base(new Token<double>(d, NUM)) {}
  packToken(double d) : base(new Token<double>(d, NUM)) {}
  packToken(const char* s) : base(new Token<std::string>(s, STR)) {}
  packToken(const std::string& s) : base(new Token<std::string>(s, STR)) {}
  packToken(TokenMap_t* tmap) : base(new Token<TokenMap_t*>(tmap, MAP)) {}
  ~packToken() {if (base) delete base;}

  packToken& operator=(int t);
  packToken& operator=(double t);
  packToken& operator=(const char* t);
  packToken& operator=(const std::string& t);
  TokenBase* operator->() const;
  bool operator==(const packToken& t) const;
  packToken& operator[](const std::string& key);
  packToken& operator[](const char* key);
  const packToken& operator[](const std::string& key) const;
  const packToken& operator[](const char* key) const;
  operator TokenBase*() { return base; }

  bool asBool() const;
  double asDouble() const;
  std::string asString() const;
  TokenMap_t* asMap() const;

  std::string str() const;
  static std::string str(TokenBase* t);

 private:
  // Note:
  // This constructor makes sure the TokenBase*
  // will be deleted when the packToken destructor is called.
  //
  // Do not delete the TokenBase* by yourself after
  // building a packToken!
  //
  // If you want to copy the TokenBase do instead:
  // packToken(token->clone())
  explicit packToken(TokenBase* t) : base(t) {}

  // This constructor should only be called
  // from inside the calculator os Scope classes:
  friend class calculator;
  friend class Scope;
};

// To allow cout to print it:
std::ostream& operator<<(std::ostream& os, const packToken& t);

#endif  // PACKTOKEN_H_
