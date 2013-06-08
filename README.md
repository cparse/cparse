# Dijkstra's Shunting-yard algorithm.
Object-oriented `string` expression parsing in C++
with Dijkstra's
[Shunting-yard algorithm](http://en.wikipedia.org/wiki/Shunting-yard_algorithm).

Original version by
[Jessee Brown](http://www.daniweb.com/software-development/cpp/code/427500/calculator-using-shunting-yard-algorithm).

Improvements and additions.
 + Split into header and cpp files.
 + Support for bitshifts.

## Minimal example.
```C
#include <stdio>
#include "shunting-yard.h"

int main() {
  calculator c;
  std::cout << c.calculate ("(20+10)*3/2-3") << std::endl;
  return 0;
}
```

## TODO
 + Unary operators.
 + Suggestions from post.
