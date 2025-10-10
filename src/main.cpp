#include "parser.h"
#include "tokenizer.h"
#include "typechecker.h"
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
    }
  )";

  std::vector<Ast::Decl> root =
      Parser{Tokenizer{std::string_view{source}}}.program();

  std::vector<TAst::Decl> troot = TypeChecker{}.check(root);
}
