#pragma once

#include "span.h"
#include <cstdint>

enum struct TokenKind {
  Num,
  Ident,
  Plus,
  Minus,
  Star,
  Slash,
  Eq,
  EqEq,
  BangEq,
  Lt,
  LtEq,
  Gt,
  GtEq,
  Colon,
  Comma,
  LParen,
  RParen,
  Semi,

  True,
  False,
  Fn,
  If,
  Else,
  Return,

  Eof
};

union TokenValue {
  int32_t integer;
};

struct Token {
  TokenKind kind;
  TokenValue value;
  Span span;
};
