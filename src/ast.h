#pragma once

#include "span.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

namespace Ast {

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
struct Call;
struct Grouping;

using Expr = std::variant<Number, Boolean, Ident, Binary, Call, Grouping>;

struct Number {
  Span span;
  int32_t value;
};

struct Boolean {
  Span span;
  bool value;
};

struct Ident {
  Span span;
  std::string_view name;
};

struct Binary {
  Span span;
  std::unique_ptr<Expr> left;
  Operator op;
  std::unique_ptr<Expr> right;
};

struct Call {
  Span span;
  std::string_view function;
  std::vector<Expr> arguments;
};

struct Grouping {
  Span span;
  std::unique_ptr<Expr> inner;
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
};

struct Let {
  Span span;
  std::string_view name;
  Expr initializer;
};

struct If {
  Span span;
  Expr cond;
  std::unique_ptr<Stmt> thenStatement;
  std::optional<std::unique_ptr<Stmt>> elseStatement;
};

struct Return {
  Span span;
  std::optional<Expr> value;
};

struct ExprStmt {
  Span span;
  Expr expression;
};

struct Function;

using Decl = std::variant<Function>;

struct Parameter {
  std::string_view name;
  std::string_view type;
};

struct Function {
  Span span;
  std::string_view name;
  std::vector<Parameter> params;
  std::string_view type;
  Block body;
};

} // namespace Ast