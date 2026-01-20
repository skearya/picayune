#pragma once

#include "ast.hpp"
#include "span.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

namespace TAst {

struct TypeID {
  uint32_t id;

  bool operator==(const TypeID &) const = default;
};

struct TVoid;
struct TString;
struct TChar;
struct TInt;
struct TBoolean;
struct TStruct;
struct TFunction;

using Type =
    std::variant<TVoid, TString, TChar, TInt, TBoolean, TStruct, TFunction>;

struct TVoid {
  bool operator==(const TVoid &) const = default;
};

struct TString {
  bool operator==(const TString &) const = default;
};

struct TChar {
  bool operator==(const TChar &) const = default;
};

struct TInt {
  bool operator==(const TInt &) const = default;
};

struct TBoolean {
  bool operator==(const TBoolean &) const = default;
};

struct Field {
  TypeID type;
  std::string_view name;

  bool operator==(const Field &) const = default;
};

struct TStruct {
  std::string_view name;
  std::vector<Field> fields;

  bool operator==(const TStruct &) const = default;
};

struct Parameter {
  TypeID type;
  std::string_view name;

  bool operator==(const Parameter &) const = default;
};

struct TFunction {
  std::vector<Parameter> parameters;
  TypeID returnType;

  bool operator==(const TFunction &) const = default;
};

struct String;
struct Char;
struct Number;
struct Boolean;
struct StructInit;
struct Ident;
struct Binary;
struct Call;
struct Assign;
struct Grouping;

using Expr = std::variant<String, Char, Number, Boolean, StructInit, Ident,
                          Binary, Call, Assign, Grouping>;

struct String {
  TypeID type;
  Span span;
  std::string_view value;
};

struct Char {
  TypeID type;
  Span span;
  char value;
};

struct Number {
  TypeID type;
  Span span;
  int32_t value;
};

struct Boolean {
  TypeID type;
  Span span;
  bool value;
};

struct FieldInit {
  std::string_view name;
  std::unique_ptr<Expr> value;
};

struct StructInit {
  TypeID type;
  Span span;
  std::string_view name;
  std::vector<FieldInit> fields;
};

struct Ident {
  TypeID type;
  Span span;
  std::string_view name;
};

struct Binary {
  TypeID type;
  Span span;
  std::unique_ptr<Expr> left;
  Ast::Operator op;
  std::unique_ptr<Expr> right;
};

struct Call {
  TypeID type;
  Span span;
  std::unique_ptr<Expr> function;
  std::vector<Expr> arguments;
};

struct Assign {
  TypeID type;
  Span span;
  std::string_view variable;
  std::unique_ptr<Expr> value;
};

struct Grouping {
  TypeID type;
  Span span;
  std::unique_ptr<Expr> inner;
};

struct Block;
struct Let;
struct If;
struct While;
struct Return;
struct ExprStmt;

using Stmt = std::variant<Block, Let, If, While, Return, ExprStmt>;

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

struct Return {
  Span span;
  std::optional<Expr> value;
};

struct ExprStmt {
  Span span;
  Expr expression;
};

struct Struct;
struct Function;

using Decl = std::variant<Struct, Function>;

struct Struct {
  Span span;
  std::string_view name;
  std::vector<Field> fields;
};

struct Function {
  Span span;
  std::string_view name;
  std::vector<Parameter> params;
  TypeID returnType;
  Block body;
};

template <typename T> TAst::TypeID getTypeID(const T &arg) {
  return std::visit([](const auto &node) { return node.type; }, arg);
}

} // namespace TAst
