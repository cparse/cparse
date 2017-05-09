
namespace builtin_reservedWords {

// Literal Tokens: True, False and None:
packToken trueToken = packToken(true);
packToken falseToken = packToken(false);
packToken noneToken = packToken::None();

void True(const char* expr, const char** rest, rpnBuilder* data) {
  data->handle_token(trueToken->clone());
}

void False(const char* expr, const char** rest, rpnBuilder* data) {
  data->handle_token(falseToken->clone());
}

void None(const char* expr, const char** rest, rpnBuilder* data) {
  data->handle_token(noneToken->clone());
}

void LineComment(const char* expr, const char** rest, rpnBuilder* data) {
  while (*expr && *expr != '\n') ++expr;
  *rest = expr;
}

void SlashStarComment(const char* expr, const char** rest, rpnBuilder* data) {
  while (*expr && !(expr[0] == '*' && expr[1] == '/')) ++expr;
  if (*expr == '\0') {
    throw syntax_error("Unexpected end of file after '/*' comment!");
  }
  // Drop the characters `*/`:
  expr += 2;
  *rest = expr;
}

struct Startup {
  Startup() {
    parserMap_t& parser = calculator::Default().parserMap;
    parser.add("True", &True);
    parser.add("False", &False);
    parser.add("None", &None);
    parser.add("#", &LineComment);
    parser.add("//", &LineComment);
    parser.add("/*", &SlashStarComment);
  }
} Startup;

}  // namespace builtin_reservedWords
