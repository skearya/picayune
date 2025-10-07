#include "ast.h"
#include "parser.h"
#include "printer.h"
#include "tokenizer.h"
#include <string_view>

int main(int, char **) {
  // auto filename = "example.lang";

  // std::ifstream file{filename};
  // std::stringstream buffer;
  // buffer << file.rdbuf();
  // file.close();

  // auto content = buffer.str();

  auto source = R"(
    function factorial(n: u32): void {
      if (n == 0) {
        return 1;
      } else {
        return n * factorial(n - 1);
      }
    }

    function factorial(n: u32): void {
      if (n == 0) {
        return 1;
      } else {
        return n * factorial(n - 1);
      }
    }
  )";

  auto root = Parser{Tokenizer{std::string_view{source}}}.program();

  for (auto &decl : root) {
    printDecl(decl, std::string_view{"main.cpp"});
  }
}
