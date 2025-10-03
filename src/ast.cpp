#include "ast.h"
#include "span.h"
#include "utils.h"
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <print>
#include <string_view>
#include <utility>

// Expressions

Number::Number(Span span, int32_t value) : span(span), value(value) {};

Boolean::Boolean(Span span, bool value) : span(span), value(value) {};

Ident::Ident(Span span, std::string_view name) : span(span), name(name) {};

Binary::Binary(Span span, std::unique_ptr<Expr> l, Operator op,
               std::unique_ptr<Expr> r)
    : span(span), left(std::move(l)), op(op), right(std::move(r)) {}

Grouping::Grouping(Span span, std::unique_ptr<Expr> inner)
    : span(span), inner(std::move(inner)) {}

// Statements

Block::Block(Span span, std::vector<Stmt> statements)
    : span(span), statements(std::move(statements)) {}

Let::Let(Span span, std::string_view name, Expr initializer)
    : span(span), name(name), initializer(std::move(initializer)) {}

If::If(Span span, Expr cond, std::unique_ptr<Stmt> thenStatement,
       std::optional<std::unique_ptr<Stmt>> elseStatement)
    : span(span), cond(std::move(cond)),
      thenStatement(std::move(thenStatement)),
      elseStatement(std::move(elseStatement)) {}

Return::Return(Span span, std::optional<Expr> value)
    : span(span), value(std::move(value)) {}

ExprStmt::ExprStmt(Span span, Expr expression)
    : span(span), expression(std::move(expression)) {}

const char *operatorName(const Operator &op) {
  switch (op) {
  case Operator::Add:
    return "Add";
  case Operator::Sub:
    return "Sub";
  case Operator::Mul:
    return "Mul";
  case Operator::Div:
    return "Div";
  case Operator::Eq:
    return "Eq";
  case Operator::NotEq:
    return "NotEq";
  case Operator::Lt:
    return "Lt";
  case Operator::LtEq:
    return "LtEq";
  case Operator::Gt:
    return "Gt";
  case Operator::GtEq:
    return "GtEq";
  }
}

void printLocation(std::string_view filename, const Span &span) {
  std::print("\033[38:5:6m");
  std::print("\033[3m");
  std::print("[{}:{}:{}]", filename, span.line, span.start + 1);
  std::print("\033[23m");
  std::print("\033[39m");
};

void printExpr(const Expr &expr, std::string_view filename) {
  printExpr(expr, filename, "", false);
}

void printExpr(const Expr &expr, std::string_view filename, std::string prefix,
               bool isLeft) {
  std::print("\033[90m");

  if (prefix.empty()) {
    std::print("> ");
  } else {
    std::print("{}", prefix);
    std::print("{}", isLeft ? "├───" : "╰───");
  }

  std::print("\033[39m");

  const auto visitor = overloads{
      [&](const Number &e) {
        std::print("\033[38:5:{}m", 219);
        std::print("Number {} - ", e.value);
        std::print("\033[39m");
        printLocation(filename, e.span);
        std::println();
      },
      [&](const Boolean &e) {
        std::print("\033[38:5:{}m", 39);
        std::print("Boolean {} - ", e.value);
        std::print("\033[39m");
        printLocation(filename, e.span);
        std::println();
      },
      [&](const Ident &e) {
        std::print("\033[38:5:{}m", 181);
        std::print("Ident {} - ", e.name);
        std::print("\033[39m");
        printLocation(filename, e.span);
        std::println();
      },
      [&](const Binary &e) {
        std::print("\033[38:5:{}m", 111);
        std::print("Binary {} - ", operatorName(e.op));
        std::print("\033[39m");
        printLocation(filename, e.span);
        std::println();

        printExpr(*e.left, filename, prefix + (isLeft ? "│   " : "    "), true);
        printExpr(*e.right, filename, prefix + (isLeft ? "│   " : "    "),
                  false);
      },
      [&](const Grouping &e) {
        std::print("\033[38:5:{}m", 36);
        std::print("Grouping - ");
        std::print("\033[39m");
        printLocation(filename, e.span);
        std::println();

        printExpr(*e.inner, filename, prefix + (isLeft ? "│   " : "    "),
                  false);
      },
  };

  std::visit(visitor, expr);
}

void printStmt(const Stmt &stmt, std::string_view filename) {
  printStmt(stmt, filename, "", false);
}

void printStmt(const Stmt &stmt, std::string_view filename, std::string prefix,
               bool isLeft) {
  std::print("\033[90m");

  if (prefix.empty()) {
    std::print("> ");
  } else {
    std::print("{}", prefix);
    std::print("{}", isLeft ? "├───" : "╰───");
  }

  std::print("\033[39m");

  const auto visitor = overloads{
      [&](const Block &s) {
        std::print("\033[38:5:{}m", 219);
        std::print("Block - ");
        std::print("\033[39m");
        printLocation(filename, s.span);
        std::println();

        for (size_t i = 0; i < s.statements.size(); i++) {
          printStmt(s.statements.at(i), filename,
                    prefix + (isLeft ? "│   " : "    "),
                    s.statements.size() == 1 ? false : i == 0);
          ;
        }
      },
      [&](const Let &s) {
        std::print("\033[38:5:{}m", 39);
        std::print("Let {} - ", s.name);
        std::print("\033[39m");
        printLocation(filename, s.span);
        std::println();
      },
      [&](const If &s) {
        std::print("\033[38:5:{}m", 181);
        std::print("If - ");
        std::print("\033[39m");
        printLocation(filename, s.span);
        std::println();

        printExpr(s.cond, filename, prefix + (isLeft ? "│   " : "    "), true);
        printStmt(*s.thenStatement, filename,
                  prefix + (isLeft ? "│   " : "    "),
                  s.elseStatement.has_value());
        if (s.elseStatement.has_value()) {
          printStmt(*s.elseStatement->get(), filename,
                    prefix + (isLeft ? "│   " : "    "), false);
        }
      },
      [&](const Return &s) {
        std::print("\033[38:5:{}m", 111);
        std::print("Return - ");
        std::print("\033[39m");
        printLocation(filename, s.span);
        std::println();

        if (s.value.has_value()) {
          printExpr(s.value.value(), filename,
                    prefix + (isLeft ? "│   " : "    "), false);
        }
      },
      [&](const ExprStmt &s) {
        std::print("\033[38:5:{}m", 36);
        std::print("Expr Stmt - ");
        std::print("\033[39m");
        printLocation(filename, s.span);
        std::println();

        printExpr(s.expression, filename, prefix + (isLeft ? "│   " : "    "),
                  false);
      },
  };

  std::visit(visitor, stmt);
}