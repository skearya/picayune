#pragma once

#include "../tast.h"
#include <string_view>

namespace TPrinter {

void printExpr(const TAst::Expr &expr, std::string_view filename);
void printExpr(const TAst::Expr &expr, std::string_view filename,
               std::string prefix, std::string_view label, bool isLeft);

void printStmt(const TAst::Stmt &stmt, std::string_view filename);
void printStmt(const TAst::Stmt &stmt, std::string_view filename,
               std::string prefix, std::string_view label, bool isLeft);

void printDecl(const TAst::Decl &decl, std::string_view filename);
void printDecl(const TAst::Decl &decl, std::string_view filename,
               std::string prefix, std::string_view label, bool isLeft);

void printBlock(const TAst::Block &block, std::string_view filename);
void printBlock(const TAst::Block &block, std::string_view filename,
                std::string prefix, std::string_view label, bool isLeft);

const char *typeName(const TAst::Type &type);

} // namespace TPrinter