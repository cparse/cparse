
// Encapsulate TokenBase* into a friendlier interface
class packToken {
  TokenBase* base;
public:
  packToken() : base(new Token<void*>(0, NONE)) {}

public:
  template<class C>
  packToken(C c, tokType type) : base(new Token<C>(c, type)) {}
  packToken(int d) : base(new Token<double>(d, NUM)) {}
  packToken(double d) : base(new Token<double>(d, NUM)) {}
  packToken(TokenBase* t) : base(t->clone()) {}
  ~packToken(){ delete base; }

  packToken& operator=(int t);
  packToken& operator=(double t);
  TokenBase* operator->() const;
  bool operator==(const packToken& t) const;

  double asDouble() const;

  std::string str() const;
};
