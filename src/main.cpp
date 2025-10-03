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

  Parser parser{Tokenizer{std::string_view{
      "if (9 + 10 == 21) { return a + b; } else { let x = 0; return x; }"}}};
  Stmt root = parser.statement();

  printStmt(root, std::string_view{"main.cpp"});
}
