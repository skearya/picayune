#include "codegen.hpp"
#include "extra/printer.hpp"
#include "parser.hpp"
#include "tokenizer.hpp"
#include "typechecker.hpp"
#include <fstream>
#include <print>
#include <sstream>
#include <string_view>

int main(int argc, char **argv) {
  if (argc != 2) {
    std::println("Invalid argument count");
    return 1;
  }

  auto filename = argv[1];

  std::ifstream file{filename};
  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();

  auto source = buffer.str();

  std::vector<Ast::Decl> root =
      Parser{Tokenizer{std::string_view{source}}}.program();

  Printer::printProgram(root, "example.lang");

  std::println();

  std::vector<TAst::Decl> troot = TypeChecker{}.check(root);

  Printer::printProgram(troot, "example.lang");

  LLVMCodegen{}.codegen(troot);
}
