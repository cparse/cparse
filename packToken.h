
// Encapsulate TokenBase* into a friendlier interface
class packToken {
  TokenBase* base;
public:
  packToken() : base(0) {}
  packToken(TokenBase* t) : base(t) {}
  packToken(const TokenBase& t) : base(t.clone()) {}
  packToken(const packToken& t) : base(t.base ? t.base->clone() : 0) {}
  packToken& operator=(const packToken& t);

public:
  template<class C>
  packToken(C c, tokType type) : base(new Token<C>(c, type)) {}
  packToken(int d) : base(new Token<double>(d, NUM)) {}
  packToken(double d) : base(new Token<double>(d, NUM)) {}
  ~packToken(){ if(base) delete base; }

  packToken& operator=(int t);
  packToken& operator=(double t);
  TokenBase* operator->() const;
  bool operator==(const packToken& t) const;

  double asDouble() const;

  std::string str() const;
};

// To allow cout to print it:
std::ostream& operator<<(std::ostream& os, const packToken& t);
