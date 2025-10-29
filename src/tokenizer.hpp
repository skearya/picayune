#pragma once

#include "token.hpp"
#include <string_view>

struct Tokenizer {
  std::string_view src;
  uint32_t line;
  uint32_t start;
  uint32_t current;

  Tokenizer(std::string_view src);

  char peek();

  char next();

  bool match(char c);

  void skipWhitespace();

  Token token();
};
