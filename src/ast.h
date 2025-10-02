#pragma once

#include "span.h"
#include <cstdint>
#include <memory>
#include <string_view>
#include <variant>

enum struct Operator {
  Add,
  Sub,
  Mul,
  Div,
  Eq,
  NotEq,
  Lt,
  LtEq,
  Gt,
  GtEq,
};

struct Number;
struct Boolean;
struct Ident;
struct Binary;
struct Grouping;

using Expr = std::variant<Number, Boolean, Ident, Binary, Grouping>;

struct Number {
  Span span;
  int32_t value;

  Number(Span span, int32_t value);
};

struct Boolean {
  Span span;
  bool value;

  Boolean(Span span, bool value);
};

struct Ident {
  Span span;
  std::string_view name;

  Ident(Span span, std::string_view name);
};

struct Binary {
  Span span;
  std::unique_ptr<Expr> left;
  Operator op;
  std::unique_ptr<Expr> right;

  Binary(Span span, std::unique_ptr<Expr> l, Operator op,
         std::unique_ptr<Expr> r);
};

struct Grouping {
  Span span;
  std::unique_ptr<Expr> inner;

  Grouping(Span span, std::unique_ptr<Expr> inner);
};

Span getSpan(const Expr &expr);

void printAst(const Expr &expr, std::string_view filename);
void printAst(const Expr &expr, std::string_view filename, std::string prefix,
              bool isLeft);
