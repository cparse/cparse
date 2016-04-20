
#include <sstream>
#include <string>
#include <iostream>
#include "shunting-yard.h"
#include "shunting-yard-exceptions.h"

packToken& packToken::operator=(int t) {
  if(base) delete base;
  base = new Token<double>(t, NUM);
  return *this;
}

packToken& packToken::operator=(double t) {
  if(base) delete base;
  base = new Token<double>(t, NUM);
  return *this;
}

packToken& packToken::operator=(const char* t) {
  if(base) delete base;
  base = new Token<std::string>(t, STR);
  return *this;
}

packToken& packToken::operator=(const std::string& t) {
  if(base) delete base;
  base = new Token<std::string>(t, STR);
  return *this;
}

packToken& packToken::operator=(const packToken& t) {
  if(base) delete base;
  if(t.base) {
    base = t.base->clone();
  } else {
    base = 0;
  }
  return *this;
}

// Used mainly for testing
bool packToken::operator==(const packToken& token) const {
  if(!base) {
    return token.base == 0;
  } else if(!token.base) {
    return base == 0;
  } else if(token.base->type != base->type) {
    return false;
  } else {
    // Compare strings to simplify code
    return token.str().compare(str()) == 0;
  }
}

TokenBase* packToken::operator->() const {
  return base;
}

std::ostream& operator<<(std::ostream &os, const packToken& t) {
  return os << t.str();
}

double packToken::asDouble() const {
  if(!base || base->type != NUM) {
    throw bad_cast(
      "The Token is not a number!");
  }
  return static_cast<Token<double>*>(base)->val;
}

std::string packToken::asString() const {
  if(!base ||
    (base->type != STR && base->type != VAR && base->type != OP)) {
    throw bad_cast(
      "The Token is not a string!");
  }
  return static_cast<Token<std::string>*>(base)->val;
}
std::string packToken::str() const {
  std::stringstream ss;
  if(!base) return "undefined";
  switch(base->type) {
    case NONE:
      return "None";
    case OP:
      return static_cast<Token<std::string>*>(base)->val;
    case VAR:
      return static_cast<Token<std::string>*>(base)->val;
    case NUM:
      ss << asDouble();
      return ss.str();
    case STR:
      return "\"" + asString() + "\"";
    default:
      return "unknown_type";
  }
}
