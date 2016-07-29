#include <sstream>
#include <string>
#include <iostream>

#include "./shunting-yard.h"
#include "./packToken.h"
#include "./shunting-yard-exceptions.h"

const packToken packToken::None = packToken(TokenNone());

packToken& packToken::operator=(int t) {
  delete base;
  base = new Token<double>(t, NUM);
  return *this;
}

packToken& packToken::operator=(double t) {
  delete base;
  base = new Token<double>(t, NUM);
  return *this;
}

packToken& packToken::operator=(const char* t) {
  delete base;
  base = new Token<std::string>(t, STR);
  return *this;
}

packToken& packToken::operator=(const std::string& t) {
  delete base;
  base = new Token<std::string>(t, STR);
  return *this;
}

packToken& packToken::operator=(const packToken& t) {
  delete base;
  base = t.base->clone();
  return *this;
}

// Used mainly for testing
bool packToken::operator==(const packToken& token) const {
  if (token.base->type != base->type) {
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

packToken& packToken::operator[](const std::string& key) {
  if (base->type != MAP) {
    throw bad_cast(
      "The Token is not a map!");
  }
  return (*static_cast<Token<packMap>*>(base)->val)[key];
}
const packToken& packToken::operator[](const std::string& key) const {
  if (base->type != MAP) {
    throw bad_cast(
      "The Token is not a map!");
  }
  return (*static_cast<Token<packMap>*>(base)->val)[key];
}
packToken& packToken::operator[](const char* key) {
  if (base->type != MAP) {
    throw bad_cast(
      "The Token is not a map!");
  }
  return (*static_cast<Token<packMap>*>(base)->val)[key];
}
const packToken& packToken::operator[](const char* key) const {
  if (base->type != MAP) {
    throw bad_cast(
      "The Token is not a map!");
  }
  return (*static_cast<Token<packMap>*>(base)->val)[key];
}

bool packToken::asBool() const {
  switch (base->type) {
    case NUM:
      return static_cast<Token<double>*>(base)->val != 0;
    case STR:
      return static_cast<Token<std::string>*>(base)->val != std::string();
    case MAP:
    case FUNC:
      return true;
    case NONE:
      return false;
    case TUPLE:
      return static_cast<Tuple*>(base)->tuple.size() != 0;
    default:
      throw bad_cast("Token type can not be cast to boolean!");
  }
}

double packToken::asDouble() const {
  if (base->type != NUM) {
    throw bad_cast(
      "The Token is not a number!");
  }
  return static_cast<Token<double>*>(base)->val;
}

std::string& packToken::asString() const {
  if (base->type != STR && base->type != VAR && base->type != OP) {
    throw bad_cast(
      "The Token is not a string!");
  }
  return static_cast<Token<std::string>*>(base)->val;
}

packMap& packToken::asMap() const {
  if (base->type != MAP) {
    throw bad_cast(
      "The Token is not a map!");
  }
  return static_cast<Token<packMap>*>(base)->val;
}

std::string packToken::str() const {
  return packToken::str(base);
}
std::string packToken::str(const TokenBase* base) {
  std::stringstream ss;
  TokenMap::TokenMap_t* tmap;
  TokenMap::TokenMap_t::iterator it;
  const Function* func;
  bool first;
  std::string name;

  if (!base) return "undefined";

  if (base->type & REF) {
    base = static_cast<const RefToken*>(base)->value;
    name = static_cast<const RefToken*>(base)->key.str();
  }

  switch (base->type) {
    case NONE:
      return "None";
    case OP:
      return static_cast<const Token<std::string>*>(base)->val;
    case VAR:
      return static_cast<const Token<std::string>*>(base)->val;
    case NUM:
      ss << static_cast<const Token<double>*>(base)->val;
      return ss.str();
    case STR:
      return "\"" + static_cast<const Token<std::string>*>(base)->val + "\"";
    case FUNC:
      func = static_cast<const Function*>(base);
      if (func->name.size()) return "[Function: " + func->name + "]";
      if (name.size()) return "[Function: " + name + "]";
      return "[Function]";
    case TUPLE:
      ss << "(";
      first = true;
      for (const TokenBase* token : static_cast<const Tuple*>(base)->tuple) {
        if (!first) {
          ss << ", ";
        } else {
          first = false;
        }
        ss << str(token);
      }
      ss << ")";
      return ss.str();
    case MAP:
      tmap = &(static_cast<const Token<packMap>*>(base)->val->map);
      if (tmap->size() == 0) return "{}";
      ss << "{";
      for (it = tmap->begin(); it != tmap->end(); ++it) {
        ss << (it == tmap->begin() ? "" : ",");
        ss << " \"" << it->first << "\": " << it->second.str();
      }
      ss << " }";
      return ss.str();
    default:
      return "unknown_type";
  }
}
