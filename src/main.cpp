#include "ast.h"
#include "interpreter.h"
#include "parser.h"
#include "tokenizer.h"
#include <print>

int main(int, char **) {
  // auto filename = "example.lang";

  // std::ifstream file{filename};
  // std::stringstream buffer;
  // buffer << file.rdbuf();
  // file.close();

  // auto content = buffer.str();

  Parser parser{Tokenizer{std::string_view{"1 + 2 * 3 + 4 * 3 * 3 * 1"}}};
  Expr root = parser.expression();

  printAst(root, std::string_view{"main.cpp"});

  std::println("Result: {}", std::visit(Interpreter{}, root));
}
