#pragma once

#include "ast.h"
#include "span.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

namespace TAst {

struct TInt;
struct TBoolean;
struct TVoid;

using Type = std::variant<TInt, TBoolean, TVoid>;

struct TInt {};

struct TBoolean {};

struct TVoid {};

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
  Type type;
  Span span;
  std::string_view name;
};

struct Binary {
  Type type;
  Span span;
  std::unique_ptr<Expr> left;
  Ast::Operator op;
  std::unique_ptr<Expr> right;
};

struct Call {
  Type type;
  Span span;
  std::string_view function;
  std::vector<Expr> arguments;
};

struct Grouping {
  Type type;
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
  Type type;
  std::string_view name;
};

struct Function {
  Span span;
  Type type;
  std::string_view name;
  std::vector<Parameter> params;
  Block body;
};

template <typename T> Type getType(const T &arg) {
  return std::visit(
      [](const auto &node) -> Type {
        using t = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<t, Number>) {
          return TInt{};
        } else if constexpr (std::is_same_v<t, Boolean>) {
          return TBoolean{};
        } else {
          return node.type;
        }
      },
      arg);
}

} // namespace TAst
