#pragma once

#include <cstdint>
#include <string_view>

enum struct TokenKind { Plus, Minus, Star, Slash, Num, Ident, Eof };

union TokenValue {
  int integer;
};

struct Token {
  TokenKind kind;
  TokenValue value;
  uint32_t line, start, end;

  std::string_view src(std::string_view file);
};
