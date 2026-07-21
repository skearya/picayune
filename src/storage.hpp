#pragma once

#include "ast.hpp"
#include "type.hpp"

struct Storage {
  std::vector<Ast::Expr> exprs;
  std::vector<Ast::Stmt> stmts;
  std::vector<Ast::Decl> decls;
  std::vector<Type::Type> types;

  std::vector<Type::TypeID> exprTypeIds;
  std::vector<Type::TypeID> declTypeIds;

  Storage();

  Ast::ExprId add(Ast::Expr expr);
  Ast::StmtId add(Ast::Stmt stmt);
  Ast::DeclId add(Ast::Decl Decl);
  Type::TypeID add(Type::Type type);

  const Ast::Expr &get(Ast::ExprId exprId);
  const Ast::Stmt &get(Ast::StmtId stmtId);
  const Ast::Decl &get(Ast::DeclId DeclId);
  const Type::Type &get(Type::TypeID typeId);

  Type::Type &getMut(Type::TypeID typeId);
};
