#include <iostream>
#include <limits>
#include <map>
#include <string>

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

  std::cout << "\nTests with static calculate::calculate()\n" << std::endl;

  assert("-pi+1", -2.14, &vars);

  assert("(20+10)*3/2-3", 42.0);
  assert("1 << 4", 16.0);
  assert("1+(-2*3)", -5);

  std::cout << "\nTest with calculate::compile() & calculate::eval()\n" << std::endl;

  calculator c1;
  c1.compile("-pi+1", &vars);
  assert(c1.eval(), -2.14);

  calculator c2("pi+4", &vars);
  assert(c2.eval(), 7.14);
  assert(c2.eval(), 7.14);
  
  std::cout << "\nEnd testing" << std::endl;

  return 0;
}









