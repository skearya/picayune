#include "codegen.h"
#include "extra/printer.h"
#include "parser.h"
#include "tokenizer.h"
#include "typechecker.h"
#include <fstream>
#include <print>
#include <sstream>
#include <string_view>

int main(int, char **) {
  auto filename = "../example.lang";

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
