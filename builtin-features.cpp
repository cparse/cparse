#include <cstdio>
#include <cstdlib>
#include <string>
#include <stdexcept>
#include <cerrno>
#include <sstream>
#include <iostream>
#include <cctype>  // For tolower() and toupper()

#include "./shunting-yard.h"
#include "./shunting-yard-exceptions.h"

/* * * * * Built-in Features: * * * * */

#include "./builtin-features/operations.inc"

#include "./builtin-features/reservedWords.inc"

#include "./builtin-features/functions.inc"

#include "./builtin-features/typeSpecificFunctions.inc"
