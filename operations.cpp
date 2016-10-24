#include <math.h>
#include <sstream>

#include "./shunting-yard.h"
#include "./shunting-yard-exceptions.h"

namespace builtin_operations {

packToken Comma(const packToken& left, const std::string& op, const packToken& right) {
  if (left->type == TUPLE) {
    left.asTuple().list().push_back(right);
    return left;
  } else {
    return Tuple(left, right);
  }
}

packToken Colon(const packToken& left, const std::string& op, const packToken& right) {
  if (left->type == STUPLE) {
    left.asSTuple().list().push_back(right);
    return left;
  } else {
    return STuple(left, right);
  }
}

packToken Equal(const packToken& left, const std::string& op, const packToken& right) {
  if (left->type == VAR || right->type == VAR) {
    throw undefined_operation(op, left, right);
  }

  return left == right;
}

packToken Different(const packToken& left, const std::string& op, const packToken& right) {
  if (left->type == VAR || right->type == VAR) {
    throw undefined_operation(op, left, right);
  }

  return left != right;
}

packToken MapIndex(const packToken& p_left, const std::string& op, const packToken& p_right) {
  TokenMap& left = p_left.asMap();
  std::string& right = p_right.asString();

  if (!op.compare("[]") || !op.compare(".")) {
    packToken* p_value = left.find(right);
    TokenBase* value;

    if (p_value) {
      value = (*p_value)->clone();
    } else {
      value = new TokenNone();
    }

    return RefToken(right, value, left);
  } else {
    throw undefined_operation(op, left, right);
  }
}

// Resolve build-in operations for non-map types, e.g.: 'str'.len()
packToken TypeSpecificFunction(const packToken& p_left, const std::string& op, const packToken& p_right) {
  if (p_left->type == MAP) throw Operation::Reject();

  TokenMap& attr_map = calculator::type_attribute_map()[p_left->type];
  std::string& key = p_right.asString();

  packToken* attr = attr_map.find(key);
  if (attr) {
    TokenBase* value = (*attr)->clone();
    const packToken& source = p_left;

    // Note: If attr is a function, it will receive have
    // scope["this"] == source, so it can make changes on this object.
    // Or just read some information for example: its length.
    return RefToken(key, value, source);
  } else {
    throw undefined_operation(op, p_left, p_right);
  }
}

packToken NumeralOperation(const packToken& left, const std::string& op, const packToken& right) {
  double left_d, right_d;
  int64_t left_i, right_i;

  // Extract integer and real values of the operators:
  left_d = left.asDouble();
  left_i = left.asInt();

  right_d = right.asDouble();
  right_i = right.asInt();

  if (!op.compare("+")) {
    return left_d + right_d;
  } else if (!op.compare("*")) {
    return left_d * right_d;
  } else if (!op.compare("-")) {
    return left_d - right_d;
  } else if (!op.compare("/")) {
    return left_d / right_d;
  } else if (!op.compare("<<")) {
    return left_i << right_i;
  } else if (!op.compare("**")) {
    return pow(left_d, right_d);
  } else if (!op.compare(">>")) {
    return left_i >> right_i;
  } else if (!op.compare("%")) {
    return left_i % right_i;
  } else if (!op.compare("<")) {
    return left_d < right_d;
  } else if (!op.compare(">")) {
    return left_d > right_d;
  } else if (!op.compare("<=")) {
    return left_d <= right_d;
  } else if (!op.compare(">=")) {
    return left_d >= right_d;
  } else if (!op.compare("&&")) {
    return left_i && right_i;
  } else if (!op.compare("||")) {
    return left_i || right_i;
  } else {
    throw undefined_operation(op, left, right);
  }
}

packToken FormatOperation(const packToken& p_left, const std::string& op, const packToken& p_right) {
  std::string& s_left = p_left.asString();
  const char* left = s_left.c_str();

  Tuple right;

  if (p_right->type == TUPLE) {
    right = p_right.asTuple();
  } else {
    right = Tuple(p_right);
  }

  std::string result;
  for (const packToken& token : right.list()) {
    // Find the next occurrence of "%s"
    while (*left && (*left != '%' || left[1] != 's')) {
      if (*left == '\\' && left[1] == '%') ++left;
      result.push_back(*left);
      ++left;
    }

    if (*left == '\0') {
      throw type_error("Not all arguments converted during string formatting");
    } else {
      left += 2;
    }

    // Replace it by the token string representation:
    if (token->type == STR) {
      // Avoid using packToken::str for strings
      // or it will enclose it quotes `"str"`
      result += token.asString();
    } else {
      result += token.str();
    }
  }

  // Find the next occurrence of "%s" if exists:
  while (*left && (*left != '%' || left[1] != 's')) {
    if (*left == '\\' && left[1] == '%') ++left;
    result.push_back(*left);
    ++left;
  }

  if (*left != '\0') {
    throw type_error("Not enough arguments for format string");
  } else {
    return result;
  }
}

packToken StringOnStringOperation(const packToken& p_left, const std::string& op, const packToken& p_right) {
  std::string& left = p_left.asString();
  std::string& right = p_right.asString();

  if (!op.compare("+")) {
    return left + right;
  } else if (!op.compare("==")) {
    return (left.compare(right) == 0);
  } else if (!op.compare("!=")) {
    return (left.compare(right) != 0);
  } else {
    throw undefined_operation(op, p_left, p_right);
  }
}

packToken StringOnNumberOperation(const packToken& p_left, const std::string& op, const packToken& p_right) {
  std::string& left = p_left.asString();

  std::stringstream ss;
  if (!op.compare("+")) {
    ss << left << p_right.asDouble();
    return ss.str();
  } else if (!op.compare("[]")) {
    int64_t index = p_right.asInt();

    if (index < 0) {
      // Reverse index, i.e. list[-1] = list[list.size()-1]
      index += left.size();
    }
    if (index < 0 || static_cast<size_t>(index) >= left.size()) {
      throw std::domain_error("String index out of range!");
    }

    ss << left[index];
    return ss.str();
  } else {
    throw undefined_operation(op, p_left, p_right);
  }
}

packToken NumberOnStringOperation(const packToken& p_left, const std::string& op, const packToken& p_right) {
  double left = p_left.asDouble();
  std::string& right = p_right.asString();

  std::stringstream ss;
  if (!op.compare("+")) {
    ss << left << right;
    return ss.str();
  } else {
    throw undefined_operation(op, p_left, p_right);
  }
}

packToken ListOnNumberOperation(const packToken& p_left, const std::string& op, const packToken& p_right) {
  TokenList left = p_left.asList();

  if (!op.compare("[]")) {
    int64_t index = p_right.asInt();

    if (index < 0) {
      // Reverse index, i.e. list[-1] = list[list.size()-1]
      index += left.list().size();
    }

    if (index < 0 || static_cast<size_t>(index) >= left.list().size()) {
      throw std::domain_error("List index out of range!");
    }

    TokenBase* value = left.list()[index]->clone();

    return RefToken(index, value, packToken(p_left));
  } else {
    throw undefined_operation(op, p_left, p_right);
  }
}

packToken ListOnListOperation(const packToken& p_left, const std::string& op, const packToken& p_right) {
  TokenList& left = p_left.asList();
  TokenList& right = p_right.asList();

  if (!op.compare("+")) {
    // Deep copy the first list:
    TokenList result;
    result.list() = left.list();

    // Insert items from the right list into the left:
    for (packToken& p : right.list()) {
      result.list().push_back(p);
    }

    return result;
  } else {
    throw undefined_operation(op, left, right);
  }
}

struct Startup {
  Startup() {
    // Create the operator precedence map based on C++ default
    // precedence order as described on cppreference website:
    // http://en.cppreference.com/w/cpp/language/operator_precedence
    OppMap_t& opp = calculator::default_opPrecedence();
    opp["[]"] = 2; opp["()"] = 2; opp["."] = 2;
    opp["**"] = 3;
    opp["*"]  = 5; opp["/"]  = 5; opp["%"] = 5;
    opp["+"]  = 6; opp["-"]  = 6;
    opp["<<"] = 7; opp[">>"] = 7;
    opp["<"]  = 8; opp["<="] = 8; opp[">="] = 8; opp[">"] = 8;
    opp["=="] = 9; opp["!="] = 9;
    opp["&&"] = 13;
    opp["||"] = 14;
    opp["="]  = 15; opp[":"] = 15;
    opp[","]  = 16;

    // Link operations to respective operators:
    opMap_t& opMap = calculator::default_opMap();
    opMap.add({ANY_TYPE, ",", ANY_TYPE}, &Comma);
    opMap.add({ANY_TYPE, ":", ANY_TYPE}, &Colon);
    opMap.add({ANY_TYPE, "==", ANY_TYPE}, &Equal);
    opMap.add({ANY_TYPE, "!=", ANY_TYPE}, &Different);
    opMap.add({MAP, "[]", STR}, &MapIndex);
    opMap.add({ANY_TYPE, ".", STR}, &TypeSpecificFunction);
    opMap.add({MAP, ".", STR}, &MapIndex);
    opMap.add({STR, "%", ANY_TYPE}, &FormatOperation);

    // Note: The order is important:
    opMap.add({NUM, ANY_OP, NUM}, &NumeralOperation);
    opMap.add({STR, ANY_OP, STR}, &StringOnStringOperation);
    opMap.add({STR, ANY_OP, NUM}, &StringOnNumberOperation);
    opMap.add({NUM, ANY_OP, STR}, &NumberOnStringOperation);
    opMap.add({LIST, ANY_OP, NUM}, &ListOnNumberOperation);
    opMap.add({LIST, ANY_OP, LIST}, &ListOnListOperation);
  }
} StartUp;

}  // namespace builtin_operations

namespace builtin_reservedWords {

// Literal Tokens: True, False and None:
packToken trueToken = packToken(1);
packToken falseToken = packToken(0);
packToken noneToken = TokenNone();

TokenBase* True(const char* expr, const char** rest, rpnBuilder* data) {
  return trueToken->clone();
}

TokenBase* False(const char* expr, const char** rest, rpnBuilder* data) {
  return falseToken->clone();
}

TokenBase* None(const char* expr, const char** rest, rpnBuilder* data) {
  return noneToken->clone();
}

struct Startup {
  Startup() {
    rWordMap_t& rwMap = calculator::default_rWordMap();
    rwMap["True"] = &True;
    rwMap["False"] = &False;
    rwMap["None"] = &None;
  }
} Startup;

}  // namespace builtin_reservedWords
