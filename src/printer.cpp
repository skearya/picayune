#include "printer.h"
#include "ast.h"
#include "span.h"
#include "utils.h"
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <print>
#include <string>
#include <string_view>

void startPrint(std::string_view prefix, std::string_view label, bool isLeft) {
  std::print("\033[90m");

  if (prefix.empty()) {
    std::print("> ");
  } else {
    std::print("{}", prefix);
    std::print("{}", isLeft ? "├───" : "╰───");

    if (!label.empty()) {
      std::print("{}: ", label);
    }
  }

  std::print("\033[39m");
}

void printHeader(uint8_t color, std::string_view label,
                 std::string_view filename, const Span &span) {
  std::print("\033[38:5:{}m", color);
  std::print("{}", label);
  std::print("\033[39m");

  std::print("\033[90m");
  std::print(" - ");
  std::print("\033[39m");

  std::print("\033[38:5:6m");
  std::print("\033[3m");
  std::print("[{}:{}:{}]", filename, span.line, span.start + 1);
  std::print("\033[23m");
  std::print("\033[39m");

  std::println();
}

void printExpr(const Ast::Expr &expr, std::string_view filename) {
  printExpr(expr, filename, "", "", false);
}

void printExpr(const Ast::Expr &expr, std::string_view filename,
               std::string prefix, std::string_view label, bool isLeft) {
  startPrint(prefix, label, isLeft);

  const auto visitor = overloads{
      [&](const Ast::Number &e) {
        printHeader(219, "Number " + std::to_string(e.value), filename, e.span);
      },
      [&](const Ast::Boolean &e) {
        printHeader(39, "Boolean " + std::to_string(e.value), filename, e.span);
      },
      [&](const Ast::Ident &e) {
        printHeader(181, std::format("Ident {}", e.name), filename, e.span);
      },
      [&](const Ast::Binary &e) {
        printHeader(111, std::format("Binary {}", operatorName(e.op)), filename,
                    e.span);

        printExpr(*e.left, filename, prefix + (isLeft ? "│   " : "    "), "",
                  true);
        printExpr(*e.right, filename, prefix + (isLeft ? "│   " : "    "), "",
                  false);
      },
      [&](const Ast::Call &e) {
        printHeader(111, std::format("Call {}", e.function), filename, e.span);

        for (size_t i = 0; i < e.arguments.size(); i++) {
          printExpr(e.arguments.at(i), filename,
                    prefix + (isLeft ? "│   " : "    "),
                    std::format("arg {}", i),
                    e.arguments.size() == 1 ? false : i == 0);
        }
      },
      [&](const Ast::Grouping &e) {
        printHeader(36, "Grouping", filename, e.span);

        printExpr(*e.inner, filename, prefix + (isLeft ? "│   " : "    "), "",
                  false);
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

  const auto visitor = overloads{
      [&](const Ast::Block &s) {
        printHeader(219, "Block", filename, s.span);

        for (size_t i = 0; i < s.statements.size(); i++) {
          printStmt(s.statements.at(i), filename,
                    prefix + (isLeft ? "│   " : "    "), "",
                    s.statements.size() == 1 ? false : i == 0);
        }
      },
      [&](const Ast::Let &s) { printHeader(39, "Let", filename, s.span); },
      [&](const Ast::If &s) {
        printHeader(181, "If", filename, s.span);

        printExpr(s.cond, filename, prefix + (isLeft ? "│   " : "    "), "cond",
                  true);
        printStmt(*s.thenStatement, filename,
                  prefix + (isLeft ? "│   " : "    "), "then",
                  s.elseStatement.has_value());
        if (s.elseStatement.has_value()) {
          printStmt(*s.elseStatement->get(), filename,
                    prefix + (isLeft ? "│   " : "    "), "else", false);
        }
      },
      [&](const Ast::Return &s) {
        printHeader(111, "Return", filename, s.span);

        if (s.value.has_value()) {
          printExpr(s.value.value(), filename,
                    prefix + (isLeft ? "│   " : "    "), "", false);
        }
      },
      [&](const Ast::ExprStmt &s) {
        printHeader(36, "Expr Stmt", filename, s.span);

        printExpr(s.expression, filename, prefix + (isLeft ? "│   " : "    "),
                  "", false);
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
        header += d.returnType;

        printHeader(219, header, filename, d.span);
        printBlock(d.body, filename, prefix + (isLeft ? "│   " : "    "), "",
                   false);
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

  printHeader(219, "Block", filename, block.span);

  for (size_t i = 0; i < block.statements.size(); i++) {
    printStmt(block.statements.at(i), filename,
              prefix + (isLeft ? "│   " : "    "), "",
              block.statements.size() == 1 ? false : i == 0);
  }
}

const char *operatorName(const Ast::Operator &op) {
  switch (op) {
  case Ast::Operator::Add:
    return "Add";
  case Ast::Operator::Sub:
    return "Sub";
  case Ast::Operator::Mul:
    return "Mul";
  case Ast::Operator::Div:
    return "Div";
  case Ast::Operator::Eq:
    return "Eq";
  case Ast::Operator::NotEq:
    return "NotEq";
  case Ast::Operator::Lt:
    return "Lt";
  case Ast::Operator::LtEq:
    return "LtEq";
  case Ast::Operator::Gt:
    return "Gt";
  case Ast::Operator::GtEq:
    return "GtEq";
  }
}
