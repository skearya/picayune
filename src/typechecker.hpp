#pragma once

#include "ast.hpp"
#include "tast.hpp"
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

struct TypeChecker {
  std::vector<TAst::Type> typeArena;
  std::unordered_map<std::string_view, TAst::TypeID> types;
  std::unordered_map<std::string_view, TAst::TypeID> functions;
  std::vector<std::unordered_map<std::string_view, TAst::TypeID>> environments;

  const TAst::TFunction *currentFunction;

  TAst::TypeID voidTypeID;
  TAst::TypeID stringTypeID;
  TAst::TypeID charTypeID;
  TAst::TypeID intTypeID;
  TAst::TypeID booleanTypeID;

  TypeChecker();

  std::vector<TAst::Decl> check(const std::vector<Ast::Decl> &program);

  TAst::Expr checkExpr(const Ast::Expr &node);

  TAst::Expr operator()(const Ast::String &node);
  TAst::Expr operator()(const Ast::Char &node);
  TAst::Expr operator()(const Ast::Number &node);
  TAst::Expr operator()(const Ast::Boolean &node);
  TAst::Expr operator()(const Ast::StructInit &node);
  TAst::Expr operator()(const Ast::Get &node);
  TAst::Expr operator()(const Ast::Ident &node);
  TAst::Expr operator()(const Ast::Binary &node);
  TAst::Expr operator()(const Ast::Call &node);
  TAst::Expr operator()(const Ast::Assign &node);
  TAst::Expr operator()(const Ast::Grouping &node);

  TAst::Stmt checkStmt(const Ast::Stmt &node);

  TAst::Stmt operator()(const Ast::Block &node);
  TAst::Stmt operator()(const Ast::Let &node);
  TAst::Stmt operator()(const Ast::If &node);
  TAst::Stmt operator()(const Ast::While &node);
  TAst::Stmt operator()(const Ast::For &node);
  TAst::Stmt operator()(const Ast::Return &node);
  TAst::Stmt operator()(const Ast::ExprStmt &node);

  TAst::Block checkBlock(const Ast::Block &node);

  std::optional<TAst::TypeID> lookupVar(std::string_view name);
  std::optional<TAst::TypeID> lookupType(std::string_view ident);
  TAst::TypeID internType(TAst::Type type);
};

bool doesBlockReturn(const TAst::Block &node);
bool doesReturn(const TAst::Stmt &node);