#include "ast.h"
#include "span.h"
#include "utils.h"
#include <cstdint>
#include <memory>
#include <print>

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

void printAst(const Expr &expr, uint32_t depth) {
  for (uint32_t i = 0; i < depth * 2; i++) {
    std::print("-");
  }

  const auto visitor = overloads{
      [](const Number &e) { std::println("Number {}", e.value); },
      [depth](const Binary &e) {
        std::println("Binary {}", operatorName(e.op));

        printAst(*e.left, depth + 1);
        printAst(*e.right, depth + 1);
      },
  };

  std::visit(visitor, expr);
}
