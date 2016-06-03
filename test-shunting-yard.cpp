#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <stdexcept>

#include "shunting-yard.h"

void assert(packToken actual, packToken expected, const char* expr = 0) {
  bool match = false;
  if(actual->type == expected->type && actual->type == NUM) {
    double diff = actual.asDouble() - expected.asDouble();
    if (diff < 0) diff *= -1;
    if (diff < 1e-15) match = true;
  } else if(actual == expected) {
    match = true;
  }

  if(match) {
    if(expr) {
      std::cout << "  '" << expr << "' indeed evaluated to " <<
        expected << "." << std::endl;
    } else {
      std::cout << "  actual value '" << actual <<
        "' indeed matches the expected value '" << expected << "'" << std::endl;
    }
  } else {
    if(expr) {
      std::cout << "  FAILURE '" << expr << "' evaluated to " <<
        actual << " and NOT " << expected << "!" << std::endl;
    } else {
      std::cout << "  FAILURE, actual value '" << actual <<
        "' does not match the expected value '" << expected <<
        "'" << std::endl;
    }
  }
}
void assert(const char* expr, packToken expected,
    TokenMap_t* vars = 0) {
  packToken actual = calculator::calculate(expr, vars);
  assert(actual, expected, expr);
}

#define assert_throws(a) {try {\
  a; \
  std::cout << "  FAILURE, it did not THROW as expected" << std::endl; \
} catch(...) { \
  std::cout << "  THROWS as expected" << std::endl; \
}}

#define assert_not_throw(a) {try {\
  a; \
  std::cout << "  Do not THROW as expected" << std::endl; \
} catch(...) { \
  std::cout << "  FAILURE, it did THROW which was unexpected" << std::endl; \
}}

int main(int argc, char** argv) {
  TokenMap_t vars;
  vars["pi"] = 3.14;
  vars["b1"] = 0;

  std::cout << "\nTests with static calculate::calculate()\n" << std::endl;

  assert("-pi + 1", -2.14, &vars);
  assert("-pi + 1 * b1", -3.14, &vars);

  assert("(20+10)*3/2-3", 42.0);
  assert("1 << 4", 16.0);
  assert("1+(-2*3)", -5);

  std::cout << "\nTests with calculate::compile() & calculate::eval()\n" << std::endl;

  calculator c1;
  c1.compile("-pi+1", &vars);
  assert(c1.eval(), -2.14);

  calculator c2("pi+4", &vars);
  assert(c2.eval(), 7.14);
  assert(c2.eval(), 7.14);

  calculator c3("pi+b1+b2", &vars);

  vars["b2"] = 1;
  assert(c3.eval(&vars), 4.14);

  vars["b2"] = .86;
  assert(c3.eval(&vars), 4);

  std::cout << "\nTesting boolean expressions\n" << std::endl;

  assert("3 < 3", false);
  assert("3 <= 3", true);
  assert("3 > 3", false);
  assert("3 >= 3", true);
  assert("3 == 3", true);
  assert("3 != 3", false);

  assert("(3 && true) == true", true);
  assert("(3 && 0) == true", false);
  assert("(3 || 0) == true", true);
  assert("(false || 0) == true", false);

  std::cout << "\nTesting string expressions\n" << std::endl;

  vars["str1"] = new Token<std::string>("foo", STR);
  vars["str2"] = new Token<std::string>("bar", STR);
  vars["str3"] = new Token<std::string>("foobar", STR);
  vars["str4"] = new Token<std::string>("foo10", STR);
  vars["str5"] = new Token<std::string>("10bar", STR);

  assert("str1 + str2 == str3", true, &vars);
  assert("str1 + str2 != str3", false, &vars);
  assert("str1 + 10 == str4", true, &vars);
  assert("10 + str2 == str5", true, &vars);

  assert("'foo' + \"bar\" == str3", true, &vars);
  assert("'foo' + \"bar\" != 'foobar\"'", true, &vars);
  assert("'foo' + \"bar\\\"\" == 'foobar\"'", true, &vars);

  assert(vars["str1"].asString(), "foo");

  std::cout << "\nTesting map access expressions\n" << std::endl;


  TokenMap_t tmap;
  vars["map"] = new Token<TokenMap_t*>(&tmap, MAP);
  tmap["key"] = "mapped value";
  tmap["key1"] = "second mapped value";
  tmap["key2"] = 10;
  tmap["key3"] = new TokenMap_t;

  assert("map[\"key\"]", "mapped value", &vars);
  assert("map[\"key\"+1]", "second mapped value", &vars);
  assert("map[\"key\"+2] + 3 == 13", true, &vars);
  assert("map.key1", "second mapped value", &vars);
  tmap["key3"]["map1"] = "inception1";
  tmap["key3"]["map2"] = "inception2";
  assert("map.key3.map1", "inception1", &vars);
  assert("map.key3['map2']", "inception2", &vars);
  assert_throws(calculator::calculate("map[\"no_key\"]", &vars));

  std::cout << "\nTesting scope management\n" << std::endl;

  // Add vars to scope:
  c3.scope.push(&vars);
  assert(c3.eval(), 4);

  tmap["b2"] = 1;
  c3.scope.push(&tmap);
  assert(c3.eval(), 4.14);

  Scope scope = c3.scope;

  // Remove vars from scope:
  c3.scope.pop();
  c3.scope.pop();

  // Test what happens when you try to drop more namespaces than possible:
  assert_throws(c3.scope.pop());

  // Load Saved Scope
  c3.scope = scope;
  assert(c3.eval(), 4.14);

  // Testing with 3 namespaces:
  TokenMap_t vmap;
  vmap["b1"] = -1.14;
  c3.scope.push(&vmap);
  assert(c3.eval(), 4.14);

  scope = c3.scope;
  calculator c4("pi+b1+b2", scope);
  assert(c4.eval(), 3.);
  assert(calculator::calculate("pi+b1+b2", scope), 3.);

  c3.scope.clean();

  std::cout << "\nTesting resource management\n" << std::endl;

  calculator C1, C2("1 + 1");

  // These are likely to cause seg fault if
  // RPN copy is not handled:

  // Copy:
  assert_not_throw(calculator C3(C2));
  // Assignment:
  assert_not_throw(C1 = C2);

  std::cout << "\nTesting exception management\n" << std::endl;

  assert_throws(c3.eval());

  assert_throws({
    vars.erase("b2");
    c3.eval(&vars);
  });

  assert_not_throw({
    vars["b2"] = 0;
    vars.erase("b1");
    c3.eval(&vars);
  });

  assert_throws({
    calculator c5("10 + - - 10");
  });

  assert_throws({
    calculator c5("10 + +");
  });

  assert_not_throw({
    calculator c5("10 + -10");
  });

  std::cout << "\nEnd testing" << std::endl;

  return 0;
}
