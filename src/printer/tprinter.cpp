#include "tprinter.h"
#include "../tast.h"
#include "../utils.h"
#include "shared.h"
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <print>
#include <string>
#include <string_view>
#include <variant>

namespace TPrinter {

void printHeaderWithType(uint8_t color, std::string_view label, TAst::Type type,
                         std::string_view filename, const Span &span) {
  std::print("\033[38:5:{}m", color);
  std::print("{} ", label);
  std::print("\033[39m");

  std::print("\033[90m");
  std::print(": ");
  std::print("\033[39m");

  std::print("\033[38:5:6m");
  std::print("{} - ", typeName(type));
  std::print("\033[39m");

  std::print("\033[90m");
  std::print("\033[3m");
  std::print("[{}:{}:{}]", filename, span.line, span.start + 1);
  std::print("\033[23m");
  std::print("\033[39m");

  std::println();
}

void printExpr(const TAst::Expr &expr, std::string_view filename) {
  printExpr(expr, filename, "", "", false);
}

void printExpr(const TAst::Expr &expr, std::string_view filename,
               std::string prefix, std::string_view label, bool isLeft) {
  startPrint(prefix, label, isLeft);
  std::string next = nextPrefix(prefix, isLeft);

  const auto visitor = overloads{
      [&](const TAst::Number &e) {
        printHeaderWithType(219, "Number " + std::to_string(e.value),
                            TAst::TInt{}, filename, e.span);
      },
      [&](const TAst::Boolean &e) {
        printHeaderWithType(
            39, std::format("Boolean {}", e.value ? "true" : "false"),
            TAst::TBoolean{}, filename, e.span);
      },
      [&](const TAst::Ident &e) {
        printHeaderWithType(181, std::format("Ident {}", e.name), e.type,
                            filename, e.span);
      },
      [&](const TAst::Binary &e) {
        printHeaderWithType(111, std::format("Binary {}", operatorName(e.op)),
                            e.type, filename, e.span);

        printExpr(*e.left, filename, next, "", true);
        printExpr(*e.right, filename, next, "", false);
      },
      [&](const TAst::Call &e) {
        printHeaderWithType(111, std::format("Call {}", e.function), e.type,
                            filename, e.span);

        for (size_t i = 0; i < e.arguments.size(); i++) {
          printExpr(e.arguments.at(i), filename, next, std::format("arg {}", i),
                    e.arguments.size() == 1 ? false : i == 0);
        }
      },
      [&](const TAst::Grouping &e) {
        printHeaderWithType(36, "Grouping", e.type, filename, e.span);

        printExpr(*e.inner, filename, next, "", false);
      },
  };

  std::visit(visitor, expr);
}

void printStmt(const TAst::Stmt &stmt, std::string_view filename) {
  printStmt(stmt, filename, "", "", false);
}

void printStmt(const TAst::Stmt &stmt, std::string_view filename,
               std::string prefix, std::string_view label, bool isLeft) {
  startPrint(prefix, label, isLeft);
  std::string next = nextPrefix(prefix, isLeft);

  const auto visitor = overloads{
      [&](const TAst::Block &s) {
        printHeader(219, "Block", filename, s.span);

        for (size_t i = 0; i < s.statements.size(); i++) {
          printStmt(s.statements.at(i), filename, next, "",
                    i != s.statements.size() - 1);
        }
      },
      [&](const TAst::Let &s) {
        printHeader(39, std::format("Let {}", s.name), filename, s.span);

        printExpr(s.initializer, filename, next, "", false);
      },
      [&](const TAst::If &s) {
        printHeader(181, "If", filename, s.span);

        printExpr(s.cond, filename, next, "cond", true);
        printStmt(*s.thenStatement, filename, next, "then",
                  s.elseStatement.has_value());
        if (s.elseStatement.has_value()) {
          printStmt(*s.elseStatement->get(), filename, next, "else", false);
        }
      },
      [&](const TAst::Return &s) {
        printHeader(111, "Return", filename, s.span);

        if (s.value.has_value()) {
          printExpr(s.value.value(), filename, next, "", false);
        }
      },
      [&](const TAst::ExprStmt &s) {
        printHeader(36, "Expr Stmt", filename, s.span);

        printExpr(s.expression, filename, next, "", false);
      },
  };

  std::visit(visitor, stmt);
}

void printDecl(const TAst::Decl &decl, std::string_view filename) {
  printDecl(decl, filename, "", "", false);
}

void printDecl(const TAst::Decl &decl, std::string_view filename,
               std::string prefix, std::string_view label, bool isLeft) {
  startPrint(prefix, label, isLeft);
  std::string next = nextPrefix(prefix, isLeft);

  const auto visitor = overloads{
      [&](const TAst::Function &d) {
        std::string header{std::format("Function {}", d.name)};

        header += "(";

        for (uint32_t i = 0; i < d.params.size(); i++) {
          auto &param = d.params.at(i);
          header += std::format("{}: {}", param.name, typeName(param.type));

          if (i != d.params.size() - 1) {
            header += ", ";
          }
        }

        header += "): ";
        header += typeName(d.type);

        printHeader(219, header, filename, d.span);
        printBlock(d.body, filename, next, "", false);
      },
  };

  std::visit(visitor, decl);
}

void printBlock(const TAst::Block &block, std::string_view filename) {
  printBlock(block, filename, "", "", false);
}

void printBlock(const TAst::Block &block, std::string_view filename,
                std::string prefix, std::string_view label, bool isLeft) {
  startPrint(prefix, label, isLeft);
  std::string next = nextPrefix(prefix, isLeft);

  printHeader(219, "Block", filename, block.span);

  for (size_t i = 0; i < block.statements.size(); i++) {
    printStmt(block.statements.at(i), filename, next, "",
              i != block.statements.size() - 1);
  }
}

const char *typeName(const TAst::Type &type) {
  return std::visit(overloads{[](const TAst::TInt &) { return "Int"; },
                              [](const TAst::TBoolean &) { return "Boolean"; },
                              [](const TAst::TVoid &) { return "Void"; }},
                    type);
}

} // namespace TPrinter