#include "ast.h"
#include "span.h"
#include "utils.h"
#include <cstdint>
#include <format>
#include <memory>
#include <print>
#include <string_view>
#include <utility>

Number::Number(Span span, int32_t value) : span(span), value(value) {};

Boolean::Boolean(Span span, bool value) : span(span), value(value) {};

Ident::Ident(Span span, std::string_view name) : span(span), name(name) {};

Binary::Binary(Span span, std::unique_ptr<Expr> l, Operator op,
               std::unique_ptr<Expr> r)
    : span(span), left(std::move(l)), op(op), right(std::move(r)) {}

Grouping::Grouping(Span span, std::unique_ptr<Expr> inner)
    : span(span), inner(std::move(inner)) {}

Span getSpan(const Expr &expr) {
  const auto visitor = overloads{
      [](const Number &e) { return e.span; },
      [](const Boolean &e) { return e.span; },
      [](const Binary &e) { return e.span; },
      [](const Ident &e) { return e.span; },
      [](const Grouping &e) { return e.span; },
  };

  return std::visit(visitor, expr);
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

void printAst(const Expr &expr, std::string_view filename) {
  printAst(expr, filename, "", false);
}

void printAst(const Expr &expr, std::string_view filename, std::string prefix,
              bool isLeft) {
  std::print("\033[90m");

  if (prefix.empty()) {
    std::print("> ");
  } else {
    std::print("{}", prefix);
    std::print("{}", isLeft ? "├───" : "╰───");
  }

  std::print("\033[39m");

  const auto printLocation = [&]() {
    auto span = getSpan(expr);

    std::print("\033[38:5:6m"); // Light White
    std::print("\033[3m");      // Italic
    std::print("[{}:{}:{}]", filename, span.line, span.start + 1);
    std::print("\033[23m"); // End Italic
    std::print("\033[39m"); // End Light White
  };

  const auto visitor = overloads{
      [&](const Number &e) {
        std::print("\033[38:5:{}m", 219);
        std::print("Number {} - ", e.value);
        std::print("\033[39m");
        printLocation();
        std::println();
      },
      [&](const Boolean &e) {
        std::print("\033[38:5:{}m", 39);
        std::print("Boolean {} - ", e.value);
        std::print("\033[39m");
        printLocation();
        std::println();
      },
      [&](const Ident &e) {
        std::print("\033[38:5:{}m", 181);
        std::print("Ident {} - ", e.name);
        std::print("\033[39m");
        printLocation();
        std::println();
      },
      [&](const Binary &e) {
        std::print("\033[38:5:{}m", 111);
        std::print("Binary {} - ", operatorName(e.op));
        std::print("\033[39m");
        printLocation();
        std::println();

        printAst(*e.left, filename, prefix + (isLeft ? "│   " : "    "), true);
        printAst(*e.right, filename, prefix + (isLeft ? "│   " : "    "),
                 false);
      },
      [&](const Grouping &e) {
        std::print("\033[38:5:{}m", 36);
        std::print("Grouping - ");
        std::print("\033[39m");
        printLocation();
        std::println();

        printAst(*e.inner, filename, prefix + (isLeft ? "│   " : "    "),
                 false);
      },
  };

  std::visit(visitor, expr);
}