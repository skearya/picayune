#pragma once

#include "span.hpp"
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
  And,
  AndAnd,
  Or,
  OrOr,
  Colon,
  Comma,
  LParen,
  RParen,
  LBrace,
  RBrace,
  Semi,

  True,
  False,
  Function,
  Let,
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
