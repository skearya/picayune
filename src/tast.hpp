#pragma once

#include "ast.hpp"
#include "span.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

namespace TAst {

struct TString;
struct TChar;
struct TInt;
struct TBoolean;
struct TVoid;

using Type = std::variant<TString, TChar, TInt, TBoolean, TVoid>;

struct TString {};
struct TChar {};
struct TInt {};
struct TBoolean {};
struct TVoid {};

struct String;
struct Char;
struct Number;
struct Boolean;
struct Ident;
struct Binary;
struct Call;
struct Assign;
struct Grouping;

using Expr = std::variant<String, Char, Number, Boolean, Ident, Binary, Call,
                          Assign, Grouping>;

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

struct Assign {
  Type type;
  Span span;
  std::string_view variable;
  std::unique_ptr<Expr> value;
};

struct Grouping {
  Type type;
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
        using K = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<K, String>) {
          return TString{};
        } else if constexpr (std::is_same_v<K, Char>) {
          return TChar{};
        } else if constexpr (std::is_same_v<K, Number>) {
          return TInt{};
        } else if constexpr (std::is_same_v<K, Boolean>) {
          return TBoolean{};
        } else {
          return node.type;
        }
      },
      arg);
}

} // namespace TAst
