#pragma once

#include "span.hpp"
#include <cstdint>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

namespace Ast {

struct ExprId {
  uint32_t id;

  bool operator==(const ExprId &) const = default;
};

struct StmtId {
  uint32_t id;

  bool operator==(const StmtId &) const = default;
};

struct DeclId {
  uint32_t id;

  bool operator==(const DeclId &) const = default;
};

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

/* Expressions */

struct String {
  std::string_view value;
};

struct Char {
  char value;
};

struct Number {
  int32_t value;
};

struct Boolean {
  bool value;
};

struct FieldInit {
  std::string_view name;
  ExprId value;
};

struct StructInit {
  std::string_view name;
  std::vector<FieldInit> fields;
};

struct Get {
  ExprId expr;
  std::string_view field;
};

struct Ident {
  std::string_view name;
};

struct Binary {
  ExprId left;
  Operator op;
  ExprId right;
};

struct Call {
  ExprId function;
  std::vector<ExprId> arguments;
};

struct Assign {
  std::string_view variable;
  ExprId value;
};

struct Grouping {
  ExprId inner;
};

using ExprVariant = std::variant<String, Char, Number, Boolean, StructInit, Get,
                                 Ident, Binary, Call, Assign, Grouping>;

/* Statements */

struct Block {
  std::vector<StmtId> statements;
};

struct Let {
  std::string_view name;
  ExprId initializer;
};

struct If {
  ExprId condition;
  StmtId thenStatement;
  std::optional<StmtId> elseStatement;
};

struct While {
  ExprId condition;
  StmtId body;
};

struct For {
  StmtId initializer;
  ExprId condition;
  ExprId update;
  StmtId body;
};

struct Return {
  std::optional<ExprId> value;
};

struct ExprStmt {
  ExprId expression;
};

using StmtVariant = std::variant<Block, Let, If, While, For, Return, ExprStmt>;

/* Declarations */

struct Field {
  std::string_view name;
  std::string_view type;
};

struct Struct {
  std::string_view name;
  std::vector<Field> fields;
};

struct Parameter {
  std::string_view name;
  std::string_view type;
};

struct Function {
  std::string_view name;
  std::vector<Parameter> params;
  std::string_view returnType;
  Block body;
};

using DeclVariant = std::variant<Struct, Function>;

/* Wrapper Types */

struct Expr {
  Span span;
  ExprVariant node;
};

struct Stmt {
  Span span;
  StmtVariant node;
};

struct Decl {
  Span span;
  DeclVariant node;
};

} // namespace Ast