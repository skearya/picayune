#include "extra/printer.hpp"
#include "parser.hpp"
#include "tokenizer.hpp"
#include "typechecker.hpp"
#include <fstream>
#include <print>
#include <sstream>
#include <string_view>

std::stringstream readFile(const char *filename) {
  std::ifstream file{filename};
  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();

  return buffer;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    std::println("Invalid argument count");
    return 1;
  }

  auto filename = argv[1];
  auto buffer = readFile(filename);
  auto source = buffer.str();

  Storage storage{};

  Tokenizer tokenizer{std::string_view{source}};
  Parser parser{storage, tokenizer};

  auto program = parser.program();

  TypeChecker typechecker{storage};
  typechecker.check(program);

  Printer{storage, filename}.printProgram(program);

  std::println();
}
