// #include "codegen.hpp"
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

  auto tokenizer = Tokenizer{std::string_view{source}};
  auto parser = Parser{tokenizer};
  auto root = parser.program();

  Printer{filename}.printProgram(root);
  std::println();

  auto typechecker = TypeChecker{};
  auto troot = typechecker.check(root);

  Printer{filename, typechecker.typeArena}.printProgram(troot);
  std::println();

  // LLVMCodegen{}.codegen(troot);
}
