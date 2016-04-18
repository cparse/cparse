#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <stdexcept>

#include "shunting-yard.h"

void assert(double actual, double expected, const char* expr = 0) {
  double diff = actual - expected;
  if (diff < 0) diff *= -1;
  if (diff < 1e-15) {
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
void assert(const char* expr, double expected,
    TokenMap_t* vars = 0) {
  double actual = calculator::calculate(expr, vars);
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
  vars["pi"] = new Token<double>(3.14, NUM);
  vars["b1"] = new Token<double>(0, NUM);

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

  vars["b2"] = new Token<double>(1, NUM);
  assert(c3.eval(&vars), 4.14);

  vars["b2"] = new Token<double>(.86, NUM);
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

  std::cout << "\nTesting exception management\n" << std::endl;

  assert_throws(c3.eval());

  assert_throws({
    vars.erase("b2");
    c3.eval(&vars);
  });

  assert_not_throw({
    vars["b2"] = new Token<double>(0, NUM);
    vars.erase("b1");
    c3.eval(&vars);
  });

  std::cout << "\nEnd testing" << std::endl;

  return 0;
}
