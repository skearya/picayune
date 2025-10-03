#pragma once

#include "ast.h"
#include "token.h"
#include "tokenizer.h"

struct Parser {
  Tokenizer tokenizer;
  Token current;

  Parser(Tokenizer t);

  Token peek();

  Token advance();

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
