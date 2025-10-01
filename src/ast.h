#pragma once

#include <memory>
#include <variant>

enum struct Operator { Add, Sub, Mul, Div };

struct Number;
struct Binary;

using Expr = std::variant<Binary, Number>;

struct Number {
  int value;

  Number(const int value);
};

struct Binary {
  std::unique_ptr<Expr> left;
  Operator op;
  std::unique_ptr<Expr> right;

  Binary(std::unique_ptr<Expr> l, Operator op, std::unique_ptr<Expr> r);
};
