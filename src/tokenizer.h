#pragma once

#include "token.h"
#include <string_view>

struct Tokenizer {
  std::string_view src;
  uint32_t line, start, current;

  Tokenizer(std::string_view src);

  char peek();

  char next();

  bool match(char c);

  void skipWhitespace();

  Token token();
};
