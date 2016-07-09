#include "catch.hpp"

#include "./shunting-yard.h"

TokenMap_t vars, tmap, key3, emap;

void PREPARE_ENVIRONMENT() {
  vars["pi"] = 3.14;
  vars["b1"] = 0.0;
  vars["b2"] = 0.86;
  vars["str1"] = "foo";
  vars["str2"] = "bar";
  vars["str3"] = "foobar";
  vars["str4"] = "foo10";
  vars["str5"] = "10bar";

  vars["map"] = &tmap;
  tmap["key"] = "mapped value";
  tmap["key1"] = "second mapped value";
  tmap["key2"] = 10;
  tmap["key3"] = &key3;
  tmap["key3"]["map1"] = "inception1";
  tmap["key3"]["map2"] = "inception2";

  emap["a"] = 10;
  emap["b"] = 20;
}

TEST_CASE("Static calculate::calculate()") {
  REQUIRE(calculator::calculate("-pi + 1", &vars).asDouble() == Approx(-2.14));
  REQUIRE(calculator::calculate("-pi + 1 * b1", &vars).asDouble() == Approx(-3.14));
  REQUIRE(calculator::calculate("(20+10)*3/2-3", &vars).asDouble() == Approx(42.0));
  REQUIRE(calculator::calculate("1 << 4", &vars).asDouble() == Approx(16.0));
  REQUIRE(calculator::calculate("1+(-2*3)", &vars).asDouble() == Approx(-5));
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

  REQUIRE(calculator::calculate("(3 && true) == true").asBool());
  REQUIRE_FALSE(calculator::calculate("(3 && 0) == true").asBool());
  REQUIRE(calculator::calculate("(3 || 0) == true").asBool());
  REQUIRE_FALSE(calculator::calculate("(false || 0) == true").asBool());
}

TEST_CASE("String expressions") {
  REQUIRE(calculator::calculate("str1 + str2 == str3", &vars).asBool());
  REQUIRE_FALSE(calculator::calculate("str1 + str2 != str3", &vars).asBool());
  REQUIRE(calculator::calculate("str1 + 10 == str4", &vars).asBool());
  REQUIRE(calculator::calculate("10 + str2 == str5", &vars).asBool());

  REQUIRE(calculator::calculate("'foo' + \"bar\" == str3", &vars).asBool());
  REQUIRE(calculator::calculate("'foo' + \"bar\" != 'foobar\"'", &vars).asBool());
  REQUIRE(calculator::calculate("'foo' + \"bar\\\"\" == 'foobar\"'", &vars).asBool());
}

TEST_CASE("Map access expressions") {
  REQUIRE(calculator::calculate("map[\"key\"]", &vars).asString() == "mapped value");
  REQUIRE(calculator::calculate("map[\"key\"+1]", &vars).asString() ==
          "second mapped value");
  REQUIRE(calculator::calculate("map[\"key\"+2] + 3 == 13", &vars).asBool());
  REQUIRE(calculator::calculate("map.key1", &vars).asString() == "second mapped value");

  REQUIRE(calculator::calculate("map.key3.map1", &vars).asString() == "inception1");
  REQUIRE(calculator::calculate("map.key3['map2']", &vars).asString() == "inception2");
  REQUIRE_THROWS(calculator::calculate("map[\"no_key\"]", &vars));
}

TEST_CASE("Function usage expressions") {

  TokenMap_t vars;
  vars["pi"] = 3.141592653589793;
  vars["a"] = -4;

  REQUIRE(calculator::calculate("sqrt(4)", &vars).asDouble() == 2);
  REQUIRE(calculator::calculate("sin(pi)", &vars).asDouble() == Approx(0));
  REQUIRE(calculator::calculate("cos(pi/2)", &vars).asDouble() == Approx(0));
  REQUIRE(calculator::calculate("tan(pi)", &vars).asDouble() == Approx(0));
  calculator c("a + sqrt(4) * 2");
  REQUIRE(c.eval(&vars).asDouble() == 0);
  REQUIRE(calculator::calculate("sqrt(4-a*3) * 2", &vars).asDouble() == 8);

  // With more than one argument:
  REQUIRE(calculator::calculate("pow(2,2)", &vars).asDouble() == 4);
  REQUIRE(calculator::calculate("pow(2,3)", &vars).asDouble() == 8);
  REQUIRE(calculator::calculate("pow(2,a)", &vars).asDouble() == Approx(1./16));
  REQUIRE(calculator::calculate("pow(2,a+4)", &vars).asDouble() == 1);

  REQUIRE_THROWS(calculator::calculate("foo(10)"));
  REQUIRE_THROWS(calculator::calculate("foo(10),"));
  REQUIRE_THROWS(calculator::calculate("foo,(10)"));

  // The test bellow will fail, TODO fix it:
  // REQUIRE_NOTHROW(calculator::calculate("print()"));
}

TEST_CASE("Scope management") {
  calculator c("pi+b1+b2");
  Scope scope;

  // Add vars to scope:
  scope.push(&vars);
  REQUIRE(c.eval(scope).asDouble() == Approx(4));

  tmap["b2"] = 1.0;
  scope.push(&tmap);
  REQUIRE(c.eval(scope).asDouble() == Approx(4.14));

  Scope scope_bkp = scope;

  // Remove vars from scope:
  scope.pop();
  scope.pop();

  // Test what happens when you try to drop more namespaces than possible:
  REQUIRE_THROWS(scope.pop());

  // Load Saved Scope
  scope = scope_bkp;
  REQUIRE(c.eval(scope).asDouble() == Approx(4.14));

  // Testing with 3 namespaces:
  TokenMap_t vmap;
  vmap["b1"] = -1.14;
  scope.push(&vmap);
  REQUIRE(c.eval(scope).asDouble() == Approx(3.0));

  scope_bkp = scope;
  calculator c2("pi+b1+b2", scope_bkp);
  REQUIRE(c2.eval().asDouble() == Approx(3.0));
  REQUIRE(calculator::calculate("pi+b1+b2", scope_bkp).asDouble() == Approx(3.0));

  scope.clean();
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

  TokenMap_t v1, v2;
  v1["map"] = &v2;
  // Mismatched types, no supported operators.
  REQUIRE_THROWS(calculator("map == 0").eval(&v1));
}
