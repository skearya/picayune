#pragma once

#include "span.h"

enum struct TokenKind {
  Num,
  Ident,
  Plus,
  Minus,
  Star,
  Slash,
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
  int integer;
};

struct Token {
  TokenKind kind;
  TokenValue value;
  Span span;
};
