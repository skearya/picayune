#pragma once

#include "../ast.hpp"
#include "../storage.hpp"
#include "../type.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

const std::array<uint8_t, 9> colors = {39,  219, 45,  220, 154,
                                       225, 121, 213, 105};

struct Printer {
  Storage &storage;
  const std::string_view filename;

  Printer(Storage &storage, std::string_view filename);

  void printExpr(Ast::ExprId exprId);
  void printExpr(Ast::ExprId exprId, std::string prefix, std::string_view label,
                 bool isLeft);

  void printStmt(Ast::StmtId stmtId);
  void printStmt(Ast::StmtId stmtId, std::string prefix, std::string_view label,
                 bool isLeft);

  void printDecl(Ast::DeclId declId);
  void printDecl(Ast::DeclId declId, std::string prefix, std::string_view label,
                 bool isLeft);

  void printProgram(const std::vector<Ast::DeclId> &prog);

  void startPrint(std::string_view prefix, std::string_view label, bool isLeft);

  void printHeader(uint8_t color, std::string_view label,
                   std::optional<Type::TypeID> typeId, const Span &span);

  std::string nextPrefix(std::string prefix, bool isLeft);

  void printStructField(std::string_view label, std::string_view typeId);

  std::string_view operatorName(Ast::Operator op);
};
