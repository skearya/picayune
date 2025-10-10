#pragma once

#include "../ast.h"
#include <string_view>

namespace Printer {

void printExpr(const Ast::Expr &expr, std::string_view filename);
void printExpr(const Ast::Expr &expr, std::string_view filename,
               std::string prefix, std::string_view label, bool isLeft);

void printStmt(const Ast::Stmt &stmt, std::string_view filename);
void printStmt(const Ast::Stmt &stmt, std::string_view filename,
               std::string prefix, std::string_view label, bool isLeft);

void printDecl(const Ast::Decl &decl, std::string_view filename);
void printDecl(const Ast::Decl &decl, std::string_view filename,
               std::string prefix, std::string_view label, bool isLeft);

void printBlock(const Ast::Block &block, std::string_view filename);
void printBlock(const Ast::Block &block, std::string_view filename,
                std::string prefix, std::string_view label, bool isLeft);

} // namespace Printer