#pragma once

#include "span.h"
#include <cstdint>
#include <memory>
#include <variant>

enum struct Operator { Add, Sub, Mul, Div };

struct Number;
struct Binary;

using Expr = std::variant<Binary, Number>;

struct Number {
  Span span;
  int32_t value;

  Number(Span span, int32_t value);
};

struct Binary {
  Span span;
  std::unique_ptr<Expr> left;
  Operator op;
  std::unique_ptr<Expr> right;

  Binary(Span span, std::unique_ptr<Expr> l, Operator op,
         std::unique_ptr<Expr> r);
};

Span getSpan(const Expr &expr);

void printAst(const Expr &expr, std::string_view filename);
void printAst(const Expr &expr, std::string_view filename, std::string prefix,
              bool isLeft);
