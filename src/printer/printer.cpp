#include "printer.h"
#include "../ast.h"
#include "../utils.h"
#include "shared.h"
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <string>
#include <string_view>

namespace Printer {

void printExpr(const Ast::Expr &expr, std::string_view filename) {
  printExpr(expr, filename, "", "", false);
}

void printExpr(const Ast::Expr &expr, std::string_view filename,
               std::string prefix, std::string_view label, bool isLeft) {
  startPrint(prefix, label, isLeft);
  std::string next = nextPrefix(prefix, isLeft);

  const auto visitor = overloads{
      [&](const Ast::Number &e) {
        printHeader(219, "Number " + std::to_string(e.value), filename, e.span);
      },
      [&](const Ast::Boolean &e) {
        printHeader(39, std::format("Boolean {}", e.value ? "true" : "false"),
                    filename, e.span);
      },
      [&](const Ast::Ident &e) {
        printHeader(181, std::format("Ident {}", e.name), filename, e.span);
      },
      [&](const Ast::Binary &e) {
        printHeader(111, std::format("Binary {}", operatorName(e.op)), filename,
                    e.span);

        printExpr(*e.left, filename, next, "", true);
        printExpr(*e.right, filename, next, "", false);
      },
      [&](const Ast::Call &e) {
        printHeader(111, std::format("Call {}", e.function), filename, e.span);

        for (size_t i = 0; i < e.arguments.size(); i++) {
          printExpr(e.arguments.at(i), filename, next, std::format("arg {}", i),
                    e.arguments.size() == 1 ? false : i == 0);
        }
      },
      [&](const Ast::Grouping &e) {
        printHeader(36, "Grouping", filename, e.span);

        printExpr(*e.inner, filename, next, "", false);
      },
  };

  std::visit(visitor, expr);
}

void printStmt(const Ast::Stmt &stmt, std::string_view filename) {
  printStmt(stmt, filename, "", "", false);
}

void printStmt(const Ast::Stmt &stmt, std::string_view filename,
               std::string prefix, std::string_view label, bool isLeft) {
  startPrint(prefix, label, isLeft);
  std::string next = nextPrefix(prefix, isLeft);

  const auto visitor = overloads{
      [&](const Ast::Block &s) {
        printHeader(219, "Block", filename, s.span);

        for (size_t i = 0; i < s.statements.size(); i++) {
          printStmt(s.statements.at(i), filename, next, "",
                    i != s.statements.size() - 1);
        }
      },
      [&](const Ast::Let &s) {
        printHeader(39, std::format("Let {}", s.name), filename, s.span);

        printExpr(s.initializer, filename, next, "", false);
      },
      [&](const Ast::If &s) {
        printHeader(181, "If", filename, s.span);

        printExpr(s.cond, filename, next, "cond", true);
        printStmt(*s.thenStatement, filename, next, "then",
                  s.elseStatement.has_value());
        if (s.elseStatement.has_value()) {
          printStmt(*s.elseStatement->get(), filename, next, "else", false);
        }
      },
      [&](const Ast::Return &s) {
        printHeader(111, "Return", filename, s.span);

        if (s.value.has_value()) {
          printExpr(s.value.value(), filename, next, "", false);
        }
      },
      [&](const Ast::ExprStmt &s) {
        printHeader(36, "Expr Stmt", filename, s.span);

        printExpr(s.expression, filename, next, "", false);
      },
  };

  std::visit(visitor, stmt);
}

void printDecl(const Ast::Decl &decl, std::string_view filename) {
  printDecl(decl, filename, "", "", false);
}

void printDecl(const Ast::Decl &decl, std::string_view filename,
               std::string prefix, std::string_view label, bool isLeft) {
  startPrint(prefix, label, isLeft);
  std::string next = nextPrefix(prefix, isLeft);

  const auto visitor = overloads{
      [&](const Ast::Function &d) {
        std::string header{std::format("Function {}", d.name)};

        header += "(";

        for (uint32_t i = 0; i < d.params.size(); i++) {
          auto &param = d.params.at(i);
          header += std::format("{}: {}", param.name, param.type);

          if (i != d.params.size() - 1) {
            header += ", ";
          }
        }

        header += "): ";
        header += d.type;

        printHeader(219, header, filename, d.span);
        printBlock(d.body, filename, next, "", false);
      },
  };

  std::visit(visitor, decl);
}

void printBlock(const Ast::Block &block, std::string_view filename) {
  printBlock(block, filename, "", "", false);
}

void printBlock(const Ast::Block &block, std::string_view filename,
                std::string prefix, std::string_view label, bool isLeft) {
  startPrint(prefix, label, isLeft);
  std::string next = nextPrefix(prefix, isLeft);

  printHeader(219, "Block", filename, block.span);

  for (size_t i = 0; i < block.statements.size(); i++) {
    printStmt(block.statements.at(i), filename, next, "",
              i != block.statements.size() - 1);
  }
}

} // namespace Printer