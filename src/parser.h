#pragma once

#include "ast.h"
#include "token.h"
#include "tokenizer.h"
#include <string_view>

struct Parser {
  Tokenizer tokenizer;
  Token current;

  Parser(Tokenizer t);

  Token peek();

  Token advance();

  Token expect(TokenKind kind, std::string_view error);

  Expr expression();
  Expr equality();
  Expr comparison();
  Expr term();
  Expr factor();
  Expr primary();

  Stmt statement();
  Stmt block();
  Stmt let();
  Stmt ifStatement();
  Stmt returnStatement();
  Stmt expressionStatement();

  Operator tokenToOperator(TokenKind token);
};
