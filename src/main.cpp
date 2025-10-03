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

  auto source = R"(
    if (9 + 10 == 21) {
      return a + b;
    } else {
      let something = x * y;
      return something;
    }
  )";

  Stmt root = Parser{Tokenizer{std::string_view{source}}}.statement();

  printStmt(root, std::string_view{"main.cpp"});
}
