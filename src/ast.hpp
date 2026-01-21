#pragma once

#include "span.hpp"
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
  Or,
  And,
};

struct String;
struct Char;
struct Number;
struct Boolean;
struct StructInit;
struct Get;
struct Ident;
struct Binary;
struct Call;
struct Assign;
struct Grouping;

using Expr = std::variant<String, Char, Number, Boolean, StructInit, Get, Ident,
                          Binary, Call, Assign, Grouping>;

struct String {
  Span span;
  std::string_view value;
};

struct Char {
  Span span;
  char value;
};

struct Number {
  Span span;
  int32_t value;
};

struct Boolean {
  Span span;
  bool value;
};

struct FieldInit {
  std::string_view name;
  std::unique_ptr<Expr> value;
};

struct StructInit {
  Span span;
  std::string_view name;
  std::vector<FieldInit> fields;
};

struct Get {
  Span span;
  std::unique_ptr<Expr> expr;
  std::string_view name;
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
  std::unique_ptr<Expr> function;
  std::vector<Expr> arguments;
};

struct Assign {
  Span span;
  std::string_view variable;
  std::unique_ptr<Expr> value;
};

struct Grouping {
  Span span;
  std::unique_ptr<Expr> inner;
};

struct Block;
struct Let;
struct If;
struct While;
struct For;
struct Return;
struct ExprStmt;

using Stmt = std::variant<Block, Let, If, While, For, Return, ExprStmt>;

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
  Expr condition;
  std::unique_ptr<Stmt> thenStatement;
  std::optional<std::unique_ptr<Stmt>> elseStatement;
};

struct While {
  Span span;
  Expr condition;
  std::unique_ptr<Stmt> body;
};

struct For {
  Span span;
  std::unique_ptr<Stmt> initializer;
  Expr condition;
  Expr update;
  std::unique_ptr<Stmt> body;
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
struct Struct;

using Decl = std::variant<Struct, Function>;

struct Field {
  std::string_view name;
  std::string_view type;
};

struct Struct {
  Span span;
  std::string_view name;
  std::vector<Field> fields;
};

struct Parameter {
  std::string_view name;
  std::string_view type;
};

struct Function {
  Span span;
  std::string_view name;
  std::vector<Parameter> params;
  std::string_view returnType;
  Block body;
};

} // namespace Ast