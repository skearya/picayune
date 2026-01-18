#pragma once

#include "ast.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include <string_view>
#include <vector>

struct Parser {
  Tokenizer tokenizer;
  Token current;

  Parser(Tokenizer t);

  Token peek();
  Token advance();
  Token expect(TokenKind kind, std::string_view error);

  /* Expressions */
  Ast::Expr expression();
  Ast::Expr assignment();
  Ast::Expr logical();
  Ast::Expr equality();
  Ast::Expr comparison();
  Ast::Expr term();
  Ast::Expr factor();
  Ast::Expr primary();

  /* Statements */
  Ast::Stmt statement();
  Ast::Stmt let();
  Ast::Stmt ifStatement();
  Ast::Stmt whileStatement();
  Ast::Stmt forStatement();
  Ast::Stmt returnStatement();
  Ast::Stmt expressionStatement();

  /* Declarations */
  Ast::Decl declaration();
  Ast::Decl function();
  Ast::Decl structDeclaration();

  /* Program (Entry) */
  std::vector<Ast::Decl> program();

  /* Helpers */
  Ast::Block block();
  Ast::Parameter parameter();
  std::vector<Ast::Parameter> parameters();
  std::vector<Ast::Expr> arguments();

  Ast::Operator tokenToOperator(TokenKind token);
};
