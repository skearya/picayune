#include "interpreter.h"
#include "parser.h"
#include "tokenizer.h"
#include <print>

int main(int, char **) {
  Parser parser{Tokenizer{std::string_view{"301 * 2"}}};
  Expr root = parser.expression();

  std::println("{}", std::visit(Interpreter{}, root));
}
