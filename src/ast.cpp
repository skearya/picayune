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

// Declarations

Parameter::Parameter(std::string_view name, std::string_view type)
    : name(name), type(type) {}

Function::Function(Span span, std::string_view name,
                   std::vector<Parameter> params, std::string_view returnType,
                   Block body)
    : span(span), name(name), params(params), returnType(returnType),
      body(std::move(body)) {}

void printHeader(uint8_t color, std::string_view label,
                 std::string_view filename, const Span &span) {
  std::print("\033[38:5:{}m", color);
  std::print("{} - ", label);
  std::print("\033[39m");

  std::print("\033[38:5:6m");
  std::print("\033[3m");
  std::print("[{}:{}:{}]", filename, span.line, span.start + 1);
  std::print("\033[23m");
  std::print("\033[39m");

  std::println();
}

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
        printHeader(219, "Number " + std::to_string(e.value), filename, e.span);
      },
      [&](const Boolean &e) {
        printHeader(39, "Boolean " + std::to_string(e.value), filename, e.span);
      },
      [&](const Ident &e) {
        printHeader(181, std::format("Ident {}", e.name), filename, e.span);
      },
      [&](const Binary &e) {
        printHeader(111, std::format("Binary {}", operatorName(e.op)), filename,
                    e.span);

        printExpr(*e.left, filename, prefix + (isLeft ? "│   " : "    "), true);
        printExpr(*e.right, filename, prefix + (isLeft ? "│   " : "    "),
                  false);
      },
      [&](const Grouping &e) {
        printHeader(36, "Grouping", filename, e.span);

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
        printHeader(219, "Block", filename, s.span);

        for (size_t i = 0; i < s.statements.size(); i++) {
          printStmt(s.statements.at(i), filename,
                    prefix + (isLeft ? "│   " : "    "),
                    s.statements.size() == 1 ? false : i == 0);
          ;
        }
      },
      [&](const Let &s) { printHeader(39, "Let", filename, s.span); },
      [&](const If &s) {
        printHeader(181, "If", filename, s.span);

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
        printHeader(111, "Return", filename, s.span);

        if (s.value.has_value()) {
          printExpr(s.value.value(), filename,
                    prefix + (isLeft ? "│   " : "    "), false);
        }
      },
      [&](const ExprStmt &s) {
        printHeader(36, "Expr Stmt", filename, s.span);

        printExpr(s.expression, filename, prefix + (isLeft ? "│   " : "    "),
                  false);
      },
  };

  std::visit(visitor, stmt);
}

void printDecl(const Decl &decl, std::string_view filename) {
  printDecl(decl, filename, "", false);
}

void printDecl(const Decl &decl, std::string_view filename, std::string prefix,
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
      [&](const Function &d) {
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
        printBlock(d.body, filename, prefix + (isLeft ? "│   " : "    "),
                   false);
      },
  };

  std::visit(visitor, decl);
}

void printBlock(const Block &block, std::string_view filename) {
  printBlock(block, filename, "", false);
}

void printBlock(const Block &block, std::string_view filename,
                std::string prefix, bool isLeft) {
  std::print("\033[90m");

  if (prefix.empty()) {
    std::print("> ");
  } else {
    std::print("{}", prefix);
    std::print("{}", isLeft ? "├───" : "╰───");
  }

  std::print("\033[39m");

  printHeader(219, "Block", filename, block.span);

  for (size_t i = 0; i < block.statements.size(); i++) {
    printStmt(block.statements.at(i), filename,
              prefix + (isLeft ? "│   " : "    "),
              block.statements.size() == 1 ? false : i == 0);
  }
}

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
