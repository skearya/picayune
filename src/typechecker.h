#pragma once

#include "ast.h"
#include "tast.h"
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

struct PartialFunction;

using PartialDecl = std::variant<PartialFunction>;

struct PartialFunction {
  Span span;
  TAst::Type type;
  std::string_view name;
  std::vector<TAst::Parameter> params;
  const Ast::Block &body;
};

struct TypeChecker {
  std::vector<PartialDecl> partialDeclarations;
  std::vector<std::unordered_map<std::string_view, TAst::Type>> environments;
  const PartialFunction *currentFunction;

  std::vector<TAst::Decl> check(const std::vector<Ast::Decl> &program);

  TAst::Expr checkExpr(const Ast::Expr &node);

  TAst::Expr operator()(const Ast::Number &node);
  TAst::Expr operator()(const Ast::Boolean &node);
  TAst::Expr operator()(const Ast::Ident &node);
  TAst::Expr operator()(const Ast::Binary &node);
  TAst::Expr operator()(const Ast::Call &node);
  TAst::Expr operator()(const Ast::Assign &node);
  TAst::Expr operator()(const Ast::Grouping &node);

  TAst::Stmt checkStmt(const Ast::Stmt &node);

  TAst::Stmt operator()(const Ast::Block &node);
  TAst::Stmt operator()(const Ast::Let &node);
  TAst::Stmt operator()(const Ast::If &node);
  TAst::Stmt operator()(const Ast::Return &node);
  TAst::Stmt operator()(const Ast::ExprStmt &node);

  TAst::Block checkBlock(const Ast::Block &node);

  std::optional<TAst::Type> lookup(std::string_view name);
  std::optional<const PartialFunction *> lookupFunction(std::string_view name);
};

TAst::Type identToType(std::string_view ident);