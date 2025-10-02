#include "ast.h"
#include "parser.h"
#include "tokenizer.h"

int main(int, char **) {
  // auto filename = "example.lang";

  // std::ifstream file{filename};
  // std::stringstream buffer;
  // buffer << file.rdbuf();
  // file.close();

  // auto content = buffer.str();

  Parser parser{Tokenizer{std::string_view{"1 + 2 * 3 > (true + 21)"}}};
  Expr root = parser.expression();

  printAst(root, std::string_view{"main.cpp"});
}
