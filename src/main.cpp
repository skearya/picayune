#include "extra/printer.h"
#include "parser.h"
#include "tokenizer.h"
#include "typechecker.h"
#include <print>
#include <string_view>

int main(int, char **) {
  // auto filename = "example.lang";

  // std::ifstream file{filename};
  // std::stringstream buffer;
  // buffer << file.rdbuf();
  // file.close();

  // auto content = buffer.str();

  auto source = R"(
    function main(): void {
      let one = 1 + 1;
      let two = one + 1;

      if (two == 3) {
        return true;
      } else {
        return false;
      }
    }
  )";

  std::vector<Ast::Decl> root =
      Parser{Tokenizer{std::string_view{source}}}.program();

  Printer::printProgram(root, "main.cpp");

  std::println();

  std::vector<TAst::Decl> troot = TypeChecker{}.check(root);

  Printer::printProgram(troot, "main.cpp");
}
