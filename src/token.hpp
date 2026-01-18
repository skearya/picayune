#pragma once

#include "span.hpp"
#include <cstdint>
#include <string_view>

enum struct TokenKind {
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
  Struct,
  Function,
  Let,
  If,
  Else,
  While,
  For,
  Return,

  Char,
  Str,
  Num,
  Ident,

  Eof
};

union TokenValue {
  char character;
  std::string_view string;
  int32_t integer;
};

struct Token {
  TokenKind kind;
  TokenValue value;
  Span span;
};
