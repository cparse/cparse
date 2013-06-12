#include <iostream>
#include "shunting-yard.h"

int main() {
  std::map<std::string, double> vars;
  vars["pi"] = 3.14;
  std::cout << calculator::calculate("pi+1", &vars) << std::endl;
  return 0;
}
