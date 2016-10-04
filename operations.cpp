
#ifndef OPERATIONS_H_
#define OPERATIONS_H_

#include <math.h>
#include <sstream>

#include "./shunting-yard.h"
#include "./shunting-yard-exceptions.h"

namespace builtin_operations {

struct Comma : public BaseOperation {
  const opID_t getMask() { return Operation::build_mask(ANY_TYPE, ANY_TYPE); }

  TokenBase* exec(TokenBase* b_left, const std::string& op, TokenBase* b_right) {
    TokenBase* result;
    if (b_left->type == TUPLE) {
      Tuple* tuple = static_cast<Tuple*>(b_left);
      tuple->list().push_back(packToken(b_right));
      result = tuple;
    } else {
      result = new Tuple(b_left, b_right);
      delete b_left;
      delete b_right;
    }
    return result;
  }
} Comma;

struct Colon : public BaseOperation {
  const opID_t getMask() { return Operation::build_mask(ANY_TYPE, ANY_TYPE); }

  TokenBase* exec(TokenBase* b_left, const std::string& op, TokenBase* b_right) {
    TokenBase* result;
    if (b_left->type == STUPLE) {
      STuple* tuple = static_cast<STuple*>(b_left);
      tuple->list().push_back(packToken(b_right));
      result = tuple;
    } else {
      result = new STuple(b_left, b_right);
      delete b_left;
      delete b_right;
    }
    return result;
  }
} Colon;

  // If it is an assignment operation:
struct Equal : public BaseOperation {
  const opID_t getMask() { return Operation::build_mask(ANY_TYPE, ANY_TYPE); }

  TokenBase* exec(TokenBase* b_left, const std::string& op, TokenBase* b_right) {
    if (b_left->type == VAR || b_right->type == VAR) {
      throw undefined_operation(op, b_left, b_right);
    }

    packToken left(b_left);
    packToken right(b_right);

    return new Token<int64_t>(left == right, INT);
  }
} Equal;

struct Different : public BaseOperation {
  const opID_t getMask() { return Operation::build_mask(ANY_TYPE, ANY_TYPE); }

  TokenBase* exec(TokenBase* b_left, const std::string& op, TokenBase* b_right) {
    if (b_left->type == VAR || b_right->type == VAR) {
      throw undefined_operation(op, b_left, b_right);
    }

    packToken left(b_left);
    packToken right(b_right);

    return new Token<int64_t>(left != right, INT);
  }
} Different;

struct MapIndex : public BaseOperation {
  const opID_t getMask() { return Operation::build_mask(MAP, STR); }

  TokenBase* exec(TokenBase* b_left, const std::string& op, TokenBase* b_right) {
    TokenMap left = *static_cast<TokenMap*>(b_left);
    std::string right = static_cast<Token<std::string>*>(b_right)->val;

    if (!op.compare("[]") || !op.compare(".")) {
      packToken* p_value = left.find(right);
      TokenBase* value;

      if (p_value) {
        value = (*p_value)->clone();
      } else {
        value = new TokenNone();
      }

      delete b_left;
      delete b_right;
      return new RefToken(right, value, left);
    } else {
      throw undefined_operation(op, b_left, b_right);
    }
  }
} MapIndex;

// Resolve build-in operations for non-map types, e.g.: 'str'.len()
struct TypeSpecificFunction : public BaseOperation {
  const opID_t getMask() { return Operation::build_mask(ANY_TYPE, STR); }

  TokenBase* exec(TokenBase* b_left, const std::string& op, TokenBase* b_right) {
    if (b_left->type == MAP) return 0;

    TokenMap& left = calculator::type_attribute_map()[b_left->type];
    std::string right = static_cast<Token<std::string>*>(b_right)->val;

    packToken* attr = left.find(right);
    if (attr) {
      TokenBase* value = (*attr)->clone();
      packToken source = packToken(b_left);
      delete b_right;

      // Note: If attr is a function, it will receive have
      // scope["this"] == source, so it can make changes on this object.
      // Or just read some information for example: its length.
      return new RefToken(right, value, source);
    } else {
      throw undefined_operation(op, b_left, b_right);
    }
  }
} TypeSpecificFunction;

struct NumeralOperation : public BaseOperation {
  const opID_t getMask() { return Operation::build_mask(NUM, NUM); }

  TokenBase* exec(TokenBase* b_left, const std::string& op, TokenBase* b_right) {
    double left, right;
    int64_t left_i, right_i;
    TokenBase* result;

    // Extract integer and real values of the operators:
    if (b_left->type == REAL) {
      left = static_cast<Token<double>*>(b_left)->val;
      left_i = static_cast<int64_t>(left);
    } else {
      left_i = static_cast<Token<int64_t>*>(b_left)->val;
      left = static_cast<double>(left_i);
    }

    if (b_right->type == REAL) {
      right = static_cast<Token<double>*>(b_right)->val;
      right_i = static_cast<int64_t>(right);
    } else {
      right_i = static_cast<Token<int64_t>*>(b_right)->val;
      right = static_cast<double>(right_i);
    }

    if (!op.compare("+")) {
      result = new Token<double>(left + right, REAL);
    } else if (!op.compare("*")) {
      result = new Token<double>(left * right, REAL);
    } else if (!op.compare("-")) {
      result = new Token<double>(left - right, REAL);
    } else if (!op.compare("/")) {
      result = new Token<double>(left / right, REAL);
    } else if (!op.compare("<<")) {
      result = new Token<double>(left_i << right_i, REAL);
    } else if (!op.compare("**")) {
      result = new Token<double>(pow(left, right), REAL);
    } else if (!op.compare(">>")) {
      result = new Token<double>(left_i >> right_i, REAL);
    } else if (!op.compare("%")) {
      result = new Token<double>(left_i % right_i, REAL);
    } else if (!op.compare("<")) {
      result = new Token<double>(left < right, REAL);
    } else if (!op.compare(">")) {
      result = new Token<double>(left > right, REAL);
    } else if (!op.compare("<=")) {
      result = new Token<double>(left <= right, REAL);
    } else if (!op.compare(">=")) {
      result = new Token<double>(left >= right, REAL);
    } else if (!op.compare("&&")) {
      result = new Token<double>(left_i && right_i, REAL);
    } else if (!op.compare("||")) {
      result = new Token<double>(left_i || right_i, REAL);
    } else {
      throw undefined_operation(op, b_left, b_right);
    }

    delete b_left;
    delete b_right;
    return result;
  }
} NumeralOperation;

struct FormatOperation : public BaseOperation {
  const opID_t getMask() { return Operation::build_mask(STR, ANY_TYPE); }

  TokenBase* exec(TokenBase* b_left, const std::string& op, TokenBase* b_right) {
    std::string s_left = static_cast<Token<std::string>*>(b_left)->val;
    const char* left = s_left.c_str();

    Tuple right;

    if (b_right->type == TUPLE) {
      right = *static_cast<Tuple*>(b_right);
    } else {
      right = Tuple(b_right);
    }

    std::string output;
    for (const TokenBase* token : right.list()) {
      // Find the next occurrence of "%s"
      while (*left && (*left != '%' || left[1] != 's')) {
        if (*left == '\\' && left[1] == '%') ++left;
        output.push_back(*left);
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
        output += static_cast<const Token<std::string>*>(token)->val;
      } else {
        output += packToken::str(token);
      }
    }

    // Find the next occurrence of "%s"
    while (*left && (*left != '%' || left[1] != 's')) {
      if (*left == '\\' && left[1] == '%') ++left;
      output.push_back(*left);
      ++left;
    }

    if (*left != '\0') {
      throw type_error("Not enough arguments for format string");
    } else {
      delete b_left;
      delete b_right;
      return new Token<std::string>(output, STR);
    }
  }
} FormatOperation;

struct StringOnStringOperation : public BaseOperation {
  const opID_t getMask() { return Operation::build_mask(STR, STR); }

  TokenBase* exec(TokenBase* b_left, const std::string& op, TokenBase* b_right) {
    std::string left = static_cast<Token<std::string>*>(b_left)->val;
    std::string right = static_cast<Token<std::string>*>(b_right)->val;
    TokenBase* result;

    if (!op.compare("+")) {
      result = new Token<std::string>(left + right, STR);
    } else if (!op.compare("==")) {
      result = new Token<int64_t>(left.compare(right) == 0, INT);
    } else if (!op.compare("!=")) {
      result = new Token<int64_t>(left.compare(right) != 0, INT);
    } else {
      throw undefined_operation(op, b_left, b_right);
    }

    delete b_left;
    delete b_right;
    return result;
  }
} StringOnStringOperation;

struct StringOnNumberOperation : public BaseOperation {
  const opID_t getMask() { return Operation::build_mask(STR, NUM); }

  TokenBase* exec(TokenBase* b_left, const std::string& op, TokenBase* b_right) {
    std::string left = static_cast<Token<std::string>*>(b_left)->val;
    double right;
    TokenBase* result;

    if (b_right->type == REAL) {
      right = static_cast<Token<double>*>(b_right)->val;
    } else {
      right = static_cast<Token<int64_t>*>(b_right)->val;
    }

    std::stringstream ss;
    if (!op.compare("+")) {
      ss << left << right;
      result = new Token<std::string>(ss.str(), STR);
    } else if (!op.compare("[]")) {
      int64_t index = right;

      if (index < 0) {
        // Reverse index, i.e. list[-1] = list[list.size()-1]
        index += left.size();
      }

      if (index < 0 || static_cast<size_t>(index) >= left.size()) {
        delete b_left;
        throw std::domain_error("String index out of range!");
      }

      std::string value;
      value.push_back(left[index]);

      result = new Token<std::string>(value, STR);
    } else {
      throw undefined_operation(op, left, right);
    }

    delete b_left;
    delete b_right;
    return result;
  }
} StringOnNumberOperation;

struct NumberOnStringOperation : public BaseOperation {
  const opID_t getMask() { return Operation::build_mask(NUM, STR); }

  TokenBase* exec(TokenBase* b_left, const std::string& op, TokenBase* b_right) {
    double left;
    std::string right = static_cast<Token<std::string>*>(b_right)->val;

    if (b_left->type == REAL) {
      left = static_cast<Token<double>*>(b_left)->val;
    } else {
      left = static_cast<Token<int64_t>*>(b_left)->val;
    }

    std::stringstream ss;
    if (!op.compare("+")) {
      ss << left << right;

      delete b_left;
      delete b_right;
      return new Token<std::string>(ss.str(), STR);
    } else {
      throw undefined_operation(op, left, right);
    }
  }
} NumberOnStringOperation;

struct ListOnNumberOperation : public BaseOperation {
  const opID_t getMask() { return Operation::build_mask(LIST, NUM); }

  TokenBase* exec(TokenBase* b_left, const std::string& op, TokenBase* b_right) {
    TokenList left = *static_cast<TokenList*>(b_left);
    int64_t right = static_cast<Token<double>*>(b_right)->val;

    if (b_right->type == REAL) {
      right = static_cast<Token<double>*>(b_right)->val;
    } else {
      right = static_cast<Token<int64_t>*>(b_right)->val;
    }

    if (!op.compare("[]")) {
      int64_t index = right;

      if (index < 0) {
        // Reverse index, i.e. list[-1] = list[list.size()-1]
        index += left.list().size();
      }

      if (index < 0 || static_cast<size_t>(index) >= left.list().size()) {
        throw std::domain_error("List index out of range!");
      }

      TokenBase* value = left.list()[index]->clone();

      delete b_right;
      return new RefToken(index, value, packToken(b_left));
    } else {
      throw undefined_operation(op, b_left, b_right);
    }
  }
} ListOnNumberOperation;

struct ListOnListOperation : public BaseOperation {
  const opID_t getMask() { return Operation::build_mask(LIST, LIST); }

  TokenBase* exec(TokenBase* b_left, const std::string& op, TokenBase* b_right) {
    TokenList left = *static_cast<TokenList*>(b_left);
    TokenList right = *static_cast<TokenList*>(b_right);

    if (!op.compare("+")) {
      // Deep copy the first list:
      TokenList result;
      result.list() = left.list();

      // Insert items from the right list into the left:
      for (packToken& p : right.list()) {
        result.list().push_back(p);
      }

      delete b_left;
      delete b_right;
      return new TokenList(result);
    } else {
      throw undefined_operation(op, left, right);
    }
  }
} ListOnListOperation;

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
    opMap[","].push_back(&Comma);
    opMap[":"].push_back(&Colon);
    opMap["=="].push_back(&Equal);
    opMap["!="].push_back(&Different);
    opMap["[]"].push_back(&MapIndex);
    opMap["."].push_back(&TypeSpecificFunction);
    opMap["."].push_back(&MapIndex);
    opMap["%"].push_back(&FormatOperation);

    // Note: The order is important:
    opMap[ANY_OP].push_back(&NumeralOperation);
    opMap[ANY_OP].push_back(&StringOnStringOperation);
    opMap[ANY_OP].push_back(&StringOnNumberOperation);
    opMap[ANY_OP].push_back(&NumberOnStringOperation);
    opMap[ANY_OP].push_back(&ListOnNumberOperation);
    opMap[ANY_OP].push_back(&ListOnListOperation);
  }
} StartUp;

}  // namespace builtin_operations

#endif  // OPERATIONS_H_
