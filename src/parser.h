#pragma once

#include "ast.h"
#include "token.h"
#include "tokenizer.h"
#include <string_view>
#include <vector>

struct Parser {
  Tokenizer tokenizer;
  Token current;

  Parser(Tokenizer t);

  Token peek();

  Token advance();

  Token expect(TokenKind kind, std::string_view error);

  Ast::Expr expression();
  Ast::Expr equality();
  Ast::Expr comparison();
  Ast::Expr term();
  Ast::Expr factor();
  Ast::Expr primary();

  Ast::Stmt statement();
  Ast::Stmt let();
  Ast::Stmt ifStatement();
  Ast::Stmt returnStatement();
  Ast::Stmt expressionStatement();

  Ast::Decl declaration();
  Ast::Decl function();

  std::vector<Ast::Decl> program();

  // Helpers

  Ast::Block block();

  Ast::Parameter parameter();
  std::vector<Ast::Parameter> parameters();

  std::vector<Ast::Expr> arguments();

  Ast::Operator tokenToOperator(TokenKind token);
};
