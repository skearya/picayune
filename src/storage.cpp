#include "storage.hpp"
#include "ast.hpp"
#include "type.hpp"
#include <cstdint>
#include <limits>

Storage::Storage()
    : exprs{}, stmts{}, decls{}, types{}, exprTypeIds{}, declTypeIds{} {}

Ast::ExprId Storage::add(Ast::Expr expr) {
  exprs.push_back(std::move(expr));
  exprTypeIds.push_back(Type::TypeID{std::numeric_limits<uint32_t>::max()});

  return Ast::ExprId{static_cast<uint32_t>(exprs.size() - 1)};
}

Ast::StmtId Storage::add(Ast::Stmt stmt) {
  stmts.push_back(std::move(stmt));

  return Ast::StmtId{static_cast<uint32_t>(stmts.size() - 1)};
}

Ast::DeclId Storage::add(Ast::Decl decl) {
  decls.push_back(std::move(decl));
  declTypeIds.push_back(Type::TypeID{std::numeric_limits<uint32_t>::max()});

  return Ast::DeclId{static_cast<uint32_t>(decls.size() - 1)};
}

Type::TypeID Storage::add(Type::Type type) {
  for (size_t i = 0; i < types.size(); i++) {
    if (types[i] == type) {
      return Type::TypeID{static_cast<uint32_t>(i)};
    }
  }

  types.push_back(std::move(type));

  return Type::TypeID{static_cast<uint32_t>(types.size() - 1)};
}

const Ast::Expr &Storage::get(Ast::ExprId exprId) {
  return exprs.at(exprId.id);
}

const Ast::Stmt &Storage::get(Ast::StmtId stmtId) {
  return stmts.at(stmtId.id);
}

const Ast::Decl &Storage::get(Ast::DeclId declId) {
  return decls.at(declId.id);
}

const Type::Type &Storage::get(Type::TypeID typeId) {
  return types.at(typeId.id);
}

Type::Type &Storage::getMut(Type::TypeID typeId) { return types.at(typeId.id); }
