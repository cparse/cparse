#include <iostream>
#include <limits>
#include <map>
#include <string>

#include "shunting-yard.h"

void assert(const char* expr, double expected, 
    std::map<std::string, double>* vars = 0) {
  double actual = calculator::calculate(expr, vars);
  double diff = actual - expected;
  if (diff < 0) diff *= -1;
  if (diff < 1e-15) {
    std::cout << "'" << expr << "' indeed evaluated to " <<
      expected << "." << std::endl;
  } else {
    std::cout << "FAILURE '" << expr << "' evaluated to " <<
      actual << " and NOT " << expected << "!" << std::endl;
  }
}

int main(int argc, char** argv) {
  std::map<std::string, double> vars;
  vars["pi"] = 3.14;
  assert("-pi+1", -2.14, &vars);

  assert("(20+10)*3/2-3", 42.0);
  assert("1 << 4", 16.0);
  assert("1+(-2*3)", -5);

  return 0;
}
