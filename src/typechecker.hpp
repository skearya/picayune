#pragma once

#include "ast.hpp"
#include "storage.hpp"
#include "type.hpp"
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

struct TypeChecker {
  Storage &storage;
  std::unordered_map<std::string_view, Type::TypeID> types;
  std::vector<std::unordered_map<std::string_view, Type::TypeID>> environments;
  Type::TypeID currentFunctionReturnType;

  Type::TypeID voidId;
  Type::TypeID stringId;
  Type::TypeID charId;
  Type::TypeID intId;
  Type::TypeID booleanId;

  TypeChecker(Storage &ts);

  void check(const std::vector<Ast::DeclId> &program);

  Type::TypeID checkExpr(Ast::ExprId ExprId);

  Type::TypeID operator()(const Ast::String &node);
  Type::TypeID operator()(const Ast::Char &node);
  Type::TypeID operator()(const Ast::Number &node);
  Type::TypeID operator()(const Ast::Boolean &node);
  Type::TypeID operator()(const Ast::StructInit &node);
  Type::TypeID operator()(const Ast::Get &node);
  Type::TypeID operator()(const Ast::Ident &node);
  Type::TypeID operator()(const Ast::Binary &node);
  Type::TypeID operator()(const Ast::Call &node);
  Type::TypeID operator()(const Ast::Assign &node);
  Type::TypeID operator()(const Ast::Grouping &node);

  void checkStmt(Ast::StmtId);

  void operator()(const Ast::Block &node);
  void operator()(const Ast::Let &node);
  void operator()(const Ast::If &node);
  void operator()(const Ast::While &node);
  void operator()(const Ast::For &node);
  void operator()(const Ast::Return &node);
  void operator()(const Ast::ExprStmt &node);

  std::optional<Type::TypeID> lookupVar(std::string_view name);
  std::optional<Type::TypeID> lookupType(std::string_view ident);

  bool doesBlockReturn(const Ast::Block &node);
  bool doesReturn(Ast::StmtId stmtId);
};
