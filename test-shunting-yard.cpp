#include <iostream>
#include "catch.hpp"

#include "./shunting-yard.h"

TokenMap vars, emap;
packMap tmap, key3;

void PREPARE_ENVIRONMENT() {
  vars["pi"] = 3.14;
  vars["b1"] = 0.0;
  vars["b2"] = 0.86;
  vars["_b"] = 0;
  vars["str1"] = "foo";
  vars["str2"] = "bar";
  vars["str3"] = "foobar";
  vars["str4"] = "foo10";
  vars["str5"] = "10bar";

  vars["map"] = tmap;
  (*tmap)["key"] = "mapped value";
  (*tmap)["key1"] = "second mapped value";
  (*tmap)["key2"] = 10;
  (*tmap)["key3"] = key3;
  (*tmap)["key3"]["map1"] = "inception1";
  (*tmap)["key3"]["map2"] = "inception2";

  emap["a"] = 10;
  emap["b"] = 20;
}

TEST_CASE("Static calculate::calculate()") {
  REQUIRE(calculator::calculate("-pi + 1", &vars).asDouble() == Approx(-2.14));
  REQUIRE(calculator::calculate("-pi + 1 * b1", &vars).asDouble() == Approx(-3.14));
  REQUIRE(calculator::calculate("(20+10)*3/2-3", &vars).asDouble() == Approx(42.0));
  REQUIRE(calculator::calculate("1 << 4", &vars).asDouble() == Approx(16.0));
  REQUIRE(calculator::calculate("1+(-2*3)", &vars).asDouble() == Approx(-5));
  REQUIRE(calculator::calculate("1+_b+(-2*3)", &vars).asDouble() == Approx(-5));
}

TEST_CASE("calculate::compile() and calculate::eval()") {
  calculator c1;
  c1.compile("-pi+1", &vars);
  REQUIRE(c1.eval().asDouble() == Approx(-2.14));

  calculator c2("pi+4", &vars);
  REQUIRE(c2.eval().asDouble() == Approx(7.14));
  REQUIRE(c2.eval().asDouble() == Approx(7.14));

  calculator c3("pi+b1+b2", &vars);
  REQUIRE(c3.eval(&vars).asDouble() == Approx(4.0));
}

TEST_CASE("Boolean expressions") {
  REQUIRE_FALSE(calculator::calculate("3 < 3").asBool());
  REQUIRE(calculator::calculate("3 <= 3").asBool());
  REQUIRE_FALSE(calculator::calculate("3 > 3").asBool());
  REQUIRE(calculator::calculate("3 >= 3").asBool());
  REQUIRE(calculator::calculate("3 == 3").asBool());
  REQUIRE_FALSE(calculator::calculate("3 != 3").asBool());

  REQUIRE(calculator::calculate("(3 && True) == True").asBool());
  REQUIRE_FALSE(calculator::calculate("(3 && 0) == True").asBool());
  REQUIRE(calculator::calculate("(3 || 0) == True").asBool());
  REQUIRE_FALSE(calculator::calculate("(False || 0) == True").asBool());
}

TEST_CASE("String expressions") {
  REQUIRE(calculator::calculate("str1 + str2 == str3", &vars).asBool());
  REQUIRE_FALSE(calculator::calculate("str1 + str2 != str3", &vars).asBool());
  REQUIRE(calculator::calculate("str1 + 10 == str4", &vars).asBool());
  REQUIRE(calculator::calculate("10 + str2 == str5", &vars).asBool());

  REQUIRE(calculator::calculate("'foo' + \"bar\" == str3", &vars).asBool());
  REQUIRE(calculator::calculate("'foo' + \"bar\" != 'foobar\"'", &vars).asBool());

  // Test escaping characters:
  REQUIRE(calculator::calculate("'foo\\'bar'").asString() == "foo'bar");
  REQUIRE(calculator::calculate("\"foo\\\"bar\"").asString() == "foo\"bar");

  // Special meaning escaped characters:
  REQUIRE(calculator::calculate("'foo\\bar'").asString() == "foo\\bar");
  REQUIRE(calculator::calculate("'foo\\nar'").asString() == "foo\nar");
  REQUIRE(calculator::calculate("'foo\\tar'").asString() == "foo\tar");
  REQUIRE_NOTHROW(calculator::calculate("'foo\\t'"));
  REQUIRE(calculator::calculate("'foo\\t'").asString() == "foo\t");

  // Scaping linefeed:
  REQUIRE_THROWS(calculator::calculate("'foo\nar'"));
  REQUIRE(calculator::calculate("'foo\\\nar'").asString() == "foo\nar");
}

TEST_CASE("Map access expressions") {
  REQUIRE(calculator::calculate("map[\"key\"]", &vars).asString() == "mapped value");
  REQUIRE(calculator::calculate("map[\"key\"+1]", &vars).asString() ==
          "second mapped value");
  REQUIRE(calculator::calculate("map[\"key\"+2] + 3 == 13", &vars).asBool() == true);
  REQUIRE(calculator::calculate("map.key1", &vars).asString() == "second mapped value");

  REQUIRE(calculator::calculate("map.key3.map1", &vars).asString() == "inception1");
  REQUIRE(calculator::calculate("map.key3['map2']", &vars).asString() == "inception2");
  REQUIRE(calculator::calculate("map[\"no_key\"]", &vars) == packToken::None);
}

TEST_CASE("Function usage expressions") {
  GlobalScope vars;
  vars["pi"] = 3.141592653589793;
  vars["a"] = -4;

  REQUIRE(calculator::calculate("sqrt(4)", &vars).asDouble() == 2);
  REQUIRE(calculator::calculate("sin(pi)", &vars).asDouble() == Approx(0));
  REQUIRE(calculator::calculate("cos(pi/2)", &vars).asDouble() == Approx(0));
  REQUIRE(calculator::calculate("tan(pi)", &vars).asDouble() == Approx(0));
  calculator c("a + sqrt(4) * 2");
  REQUIRE(c.eval(&vars).asDouble() == 0);
  REQUIRE(calculator::calculate("sqrt(4-a*3) * 2", &vars).asDouble() == 8);
  REQUIRE(calculator::calculate("abs(42)", &vars).asDouble() == 42);
  REQUIRE(calculator::calculate("abs(-42)", &vars).asDouble() == 42);

  // With more than one argument:
  REQUIRE(calculator::calculate("pow(2,2)", &vars).asDouble() == 4);
  REQUIRE(calculator::calculate("pow(2,3)", &vars).asDouble() == 8);
  REQUIRE(calculator::calculate("pow(2,a)", &vars).asDouble() == Approx(1./16));
  REQUIRE(calculator::calculate("pow(2,a+4)", &vars).asDouble() == 1);

  REQUIRE_THROWS(calculator::calculate("foo(10)"));
  REQUIRE_THROWS(calculator::calculate("foo(10),"));
  REQUIRE_THROWS(calculator::calculate("foo,(10)"));

  REQUIRE(TokenMap::default_global()["abs"].str() == "[Function: abs]");
  REQUIRE(calculator::calculate("1,2,3,4,5").str() == "(1, 2, 3, 4, 5)");

  REQUIRE(calculator::calculate(" float('0.1') ").asDouble() == 0.1);
  REQUIRE(calculator::calculate("float(10)").asDouble() == 10);
  REQUIRE(calculator::calculate(" str(10) ").asString() == "10");
  REQUIRE(calculator::calculate(" str('texto') ").asString() == "texto");

  vars["a"] = 0;
  REQUIRE(calculator::calculate(" eval('a = 3') ", &vars).asDouble() == 3);
  REQUIRE(vars["a"] == 3);

  vars["m"] = packMap();
  REQUIRE_THROWS(calculator::calculate("1 + float(m) * 3", &vars));
  REQUIRE_THROWS(calculator::calculate("float('not a number')"));

  REQUIRE_NOTHROW(calculator::calculate("pow(1,-10)"));
  REQUIRE_NOTHROW(calculator::calculate("pow(1,+10)"));

  vars["base"] = 2;
  c.compile("pow(base,2)", &vars);
  vars["base"] = 3;
  REQUIRE(c.eval().asDouble() == 4);
  REQUIRE(c.eval(&vars).asDouble() == 9);
}

TEST_CASE("Type specific functions") {
  TokenMap vars;
  vars["s"] = "string";

  REQUIRE(calculator::calculate("s.len()", &vars).asDouble() == 6);
}

TEST_CASE("Assignment expressions") {
  calculator::calculate("assignment = 10", &vars);

  // Assigning to an unexistent variable works.
  REQUIRE(calculator::calculate("assignment", &vars).asDouble() == 10);

  // Assigning to existent variables should work as well.
  REQUIRE_NOTHROW(calculator::calculate("assignment = 20", &vars));
  REQUIRE(calculator::calculate("assignment", &vars).asDouble() == 20);

  // Chain assigning should work with a right-to-left order:
  REQUIRE_NOTHROW(calculator::calculate("a = b = 20", &vars));
  REQUIRE_NOTHROW(calculator::calculate("a = b = c = d = 30", &vars));
  REQUIRE(calculator::calculate("a == b && b == c && b == d && d == 30", &vars) == true);

  REQUIRE_NOTHROW(calculator::calculate("teste='b'"));
}

TEST_CASE("Assignment expressions on maps") {
  vars["m"] = packMap();
  calculator::calculate("m['asn'] = 10", &vars);

  // Assigning to an unexistent variable works.
  REQUIRE(calculator::calculate("m['asn']", &vars).asDouble() == 10);

  // Assigning to existent variables should work as well.
  REQUIRE_NOTHROW(calculator::calculate("m['asn'] = 20", &vars));
  REQUIRE(calculator::calculate("m['asn']", &vars).asDouble() == 20);

  // Chain assigning should work with a right-to-left order:
  REQUIRE_NOTHROW(calculator::calculate("m.a = m.b = 20", &vars));
  REQUIRE_NOTHROW(calculator::calculate("m.a = m.b = m.c = m.d = 30", &vars));
  REQUIRE(calculator::calculate("m.a == m.b && m.b == m.c && m.b == m.d && m.d == 30", &vars) == true);

  REQUIRE_NOTHROW(calculator::calculate("m.m = m", &vars));
  REQUIRE(calculator::calculate("10 + (a = m.a = m.m.b)", &vars) == 40);
}

TEST_CASE("Scope management") {
  calculator c("pi+b1+b2");
  TokenMap parent;
  parent["pi"] = 3.14;
  parent["b1"] = 0;
  parent["b2"] = 0.86;

  TokenMap child = parent.getChild();

  // Check scope extension:
  REQUIRE(c.eval(&child).asDouble() == Approx(4));

  child["b2"] = 1.0;
  REQUIRE(c.eval(&child).asDouble() == Approx(4.14));

  // Testing with 3 namespaces:
  TokenMap vmap = child.getChild();
  vmap["b1"] = -1.14;
  REQUIRE(c.eval(&vmap).asDouble() == Approx(3.0));

  TokenMap copy = vmap;
  calculator c2("pi+b1+b2", &copy);
  REQUIRE(c2.eval().asDouble() == Approx(3.0));
  REQUIRE(calculator::calculate("pi+b1+b2", &copy).asDouble() == Approx(3.0));
}

// Working as a slave parser implies it will return
// a pointer to the place it has stopped parsing
// and accept a list of delimiters that should make it stop.
TEST_CASE("Parsing as slave parser") {
  const char* original_code = "a=1; b=2\n c=a+b }";
  const char* code = original_code;
  TokenMap vars;
  calculator c1, c2, c3;

  // With static function:
  REQUIRE_NOTHROW(calculator::calculate(code, &vars, ";}\n", &code));
  REQUIRE(code == &(original_code[3]));
  REQUIRE(vars["a"].asDouble() == 1);

  // With constructor:
  REQUIRE_NOTHROW((c2 = calculator(++code, &vars, ";}\n", &code)));
  REQUIRE(code == &(original_code[8]));

  // With compile method:
  REQUIRE_NOTHROW(c3.compile(++code, &vars, ";}\n", &code));
  REQUIRE(code == &(original_code[16]));

  REQUIRE_NOTHROW(c2.eval(&vars));
  REQUIRE(vars["b"] == 2);

  REQUIRE_NOTHROW(c3.eval(&vars));
  REQUIRE(vars["c"] == 3);

  // Testing with delimiter between brackets of the expression:
  const char* if_code = "if ( a+(b*c) == 3 ) { ... }";
  const char* multiline = "a = (\n  1,\n  2,\n  3\n)\n print(a);";

  code = if_code;
  REQUIRE_NOTHROW(calculator::calculate(if_code+4, &vars, ")", &code));
  REQUIRE(code == &(if_code[18]));

  code = multiline;
  REQUIRE_NOTHROW(calculator::calculate(multiline, &vars, "\n;", &code));
  REQUIRE(code == &(multiline[21]));

  const char* error_test = "a = (;  1,;  2,; 3;)\n print(a);";
  REQUIRE_THROWS(calculator::calculate(error_test, &vars, "\n;", &code));
}

TEST_CASE("Resource management") {
  calculator C1, C2("1 + 1");

  // These are likely to cause seg fault if
  // RPN copy is not handled:

  // Copy:
  REQUIRE_NOTHROW(calculator C3(C2));
  // Assignment:
  REQUIRE_NOTHROW(C1 = C2);
}

TEST_CASE("Exception management") {
  calculator ecalc;
  ecalc.compile("a+b+del", &emap);
  emap["del"] = 30;

  REQUIRE_THROWS(calculator(""));
  REQUIRE_THROWS(calculator("      "));

  REQUIRE_THROWS(ecalc.eval());
  REQUIRE_NOTHROW(ecalc.eval(&emap));

  emap.erase("del");
  REQUIRE_THROWS(ecalc.eval(&emap));

  emap["del"] = 0;
  emap.erase("a");
  REQUIRE_NOTHROW(ecalc.eval(&emap));

  REQUIRE_THROWS(calculator c5("10 + - - 10"));
  REQUIRE_THROWS(calculator c5("10 + +"));
  REQUIRE_NOTHROW(calculator c5("10 + -10"));
  REQUIRE_THROWS(calculator c5("c.[10]"));

  TokenMap v1;
  v1["map"] = packMap();
  // Mismatched types, no supported operators.
  REQUIRE_THROWS(calculator("map == 0").eval(&v1));

  // This test attempts to cause a memory leak:
  // To see if it still works run with `make check`
  REQUIRE_THROWS(calculator::calculate("a+2*no_such_variable", &vars));
}
