#include "parser.h"

Parser::Parser(Tokenizer t) : tokenizer(t), current(tokenizer.token()) {}

Token Parser::peek() { return current; }

Token Parser::advance() {
  Token prev = current;
  current = tokenizer.token();
  return prev;
}

Expr Parser::expression() { return term(); };

Expr Parser::term() {
  Expr lhs = factor();

  while (peek().kind == TokenKind::Plus || peek().kind == TokenKind::Minus) {
    Token op = advance();

    lhs = Binary{
        std::make_unique<Expr>(std::move(lhs)),
        tokenToOperator(op.kind),
        std::make_unique<Expr>(factor()),
    };
  }

  return lhs;
}

Expr Parser::factor() {
  Expr lhs = primary();

  while (peek().kind == TokenKind::Star || peek().kind == TokenKind::Slash) {
    Token op = advance();

    lhs = Binary{
        std::make_unique<Expr>(std::move(lhs)),
        tokenToOperator(op.kind),
        std::make_unique<Expr>(primary()),
    };
  }

  return lhs;
}

Expr Parser::primary() {
  if (peek().kind == TokenKind::Num) {
    Token num = advance();

    return Number{num.value.integer};
  } else {
    throw std::runtime_error("Expected expression");
  }
}

Operator Parser::tokenToOperator(TokenKind token) {
  switch (token) {
  case TokenKind::Plus:
    return Operator::Add;
  case TokenKind::Minus:
    return Operator::Sub;
  case TokenKind::Star:
    return Operator::Mul;
  case TokenKind::Slash:
    return Operator::Div;
  default:
    throw std::runtime_error("Token cannot be converted to operator");
  }
};
