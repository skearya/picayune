#pragma once

#include "span.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

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

struct Block;
struct Let;
struct If;
struct Return;
struct ExprStmt;

using Stmt = std::variant<Block, Let, If, Return, ExprStmt>;

struct Block {
  Span span;
  std::vector<Stmt> statements;

  Block(Span span, std::vector<Stmt> statements);
};

struct Let {
  Span span;
  std::string_view name;
  Expr initializer;

  Let(Span span, std::string_view name, Expr initializer);
};

struct If {
  Span span;
  Expr cond;
  std::unique_ptr<Stmt> thenStatement;
  std::optional<std::unique_ptr<Stmt>> elseStatement;

  If(Span span, Expr cond, std::unique_ptr<Stmt> thenStatement,
     std::optional<std::unique_ptr<Stmt>> elseStatement);
};

struct Return {
  Span span;
  std::optional<Expr> value;

  Return(Span span, std::optional<Expr> value);
};

struct ExprStmt {
  Span span;
  Expr expression;

  ExprStmt(Span span, Expr expression);
};

template <typename T> Span getSpan(const T &arg) {
  return std::visit([](const auto &e) { return e.span; }, arg);
}

void printExpr(const Expr &expr, std::string_view filename);
void printExpr(const Expr &expr, std::string_view filename, std::string prefix,
               bool isLeft);

void printStmt(const Stmt &stmt, std::string_view filename);
void printStmt(const Stmt &stmt, std::string_view filename, std::string prefix,
               bool isLeft);

const char *operatorName(const Operator &op);
