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

Binary::Binary(Span span, std::unique_ptr<Expr> l, Operator op,
               std::unique_ptr<Expr> r)
    : span(span), left(std::move(l)), op(op), right(std::move(r)) {}

Span getSpan(const Expr &expr) {
  struct Visitor {
    Span operator()(const Number &e) { return e.span; };
    Span operator()(const Binary &e) { return e.span; };
  };

  return std::visit(Visitor{}, expr);
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

  const int colors[] = {
      219,
      181,
      111,
      36,
  };

  const auto visitor = overloads{
      [&](const Number &e) {
        std::print("\033[38:5:{}m", colors[0]);
        std::print("Number {} - ", e.value);
        std::print("\033[39m");
        printLocation();
        std::println();
      },
      [&](const Binary &e) {
        std::print("\033[38:5:{}m", colors[1]);
        std::print("Binary {} - ", operatorName(e.op));
        std::print("\033[39m");
        printLocation();
        std::println();

        printAst(*e.left, filename, prefix + (isLeft ? "│   " : "    "), true);
        printAst(*e.right, filename, prefix + (isLeft ? "│   " : "    "),
                 false);
      },
  };

  std::visit(visitor, expr);
}