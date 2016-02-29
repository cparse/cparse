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
    std::map<std::string, double>* vars = 0) {
  double actual = calculator::calculate(expr, vars);
  assert(actual, expected, expr);
}

int main(int argc, char** argv) {
  std::map<std::string, double> vars;
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

  std::cout << "\nTesting exception management\n" << std::endl;

  try {
    c3.eval();
  } catch(std::domain_error err) {
    std::cout << "  THROWS as expected" << std::endl;
  }

  try {
    vars.erase("b2");
    c3.eval(&vars);
  } catch(std::domain_error err) {
    std::cout << "  THROWS as expected" << std::endl;
  }

  try {
    vars.erase("b1");
    vars["b2"] = 0;
    c3.eval(&vars);
    std::cout << "  Do not THROW as expected" << std::endl;
  } catch(std::domain_error err) {
    std::cout << "  If it THROWS it's a problem!" << std::endl;
  }

  std::cout << "\nEnd testing" << std::endl;

  return 0;
}
