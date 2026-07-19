#pragma once

#include "ast.hpp"
#include "span.hpp"
#include "storage.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include <string_view>
#include <vector>

struct Parser {
  Storage &storage;
  Tokenizer tokenizer;
  Token current;

  Parser(Storage &storage, Tokenizer t);

  Token peek();
  Token advance();
  Token expect(TokenKind kind, std::string_view error);

  /* Expressions */
  Ast::ExprId expression();
  Ast::ExprId assignment();
  Ast::ExprId logical();
  Ast::ExprId equality();
  Ast::ExprId comparison();
  Ast::ExprId term();
  Ast::ExprId factor();
  Ast::ExprId call();
  Ast::ExprId primary();

  /* Statements */
  Ast::StmtId statement();
  Ast::StmtId block();
  Ast::StmtId let();
  Ast::StmtId ifStatement();
  Ast::StmtId whileStatement();
  Ast::StmtId forStatement();
  Ast::StmtId returnStatement();
  Ast::StmtId expressionStatement();

  /* Declarations */
  Ast::DeclId declaration();
  Ast::DeclId function();
  Ast::DeclId structDeclaration();

  /* Program (Entry) */
  std::vector<Ast::DeclId> program();

  /* Helpers */
  Ast::Field field();
  std::vector<Ast::Field> fields();
  Ast::FieldInit fieldInit();
  std::vector<Ast::FieldInit> fieldInits();
  Ast::Parameter parameter();
  std::vector<Ast::Parameter> parameters();
  std::vector<Ast::ExprId> arguments();

  Ast::Operator tokenToOperator(TokenKind token);

  Span getSpan(Ast::ExprId id);
  Span getSpan(Ast::StmtId id);
  Span getSpan(Ast::DeclId id);
};
