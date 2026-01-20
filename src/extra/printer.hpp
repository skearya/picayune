#pragma once

#include "../tast.hpp"
#include <array>
#include <cstddef>
#include <format>
#include <optional>
#include <print>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

const std::array colors = {39, 219, 45, 220, 154, 225, 121, 213, 105};

struct Printer {
  const std::string_view filename;
  const std::span<TAst::Type> typeArena;

  Printer(std::string_view filename);
  Printer(std::string_view filename, std::span<TAst::Type> typeArena);

  void startPrint(std::string_view prefix, std::string_view label, bool isLeft);
  void printHeader(uint8_t color, std::string_view label,
                   std::optional<TAst::TypeID> typeID, const Span &span);

  std::string nextPrefix(std::string prefix, bool isLeft);
  std::string_view operatorName(Ast::Operator op);
  std::string_view typeName(TAst::TypeID typeID);

  template <typename T>
  void printExpr(const T &expr, std::string prefix, std::string_view label,
                 bool isLeft) {
    startPrint(prefix, label, isLeft);
    std::string next = nextPrefix(prefix, isLeft);

    std::visit(
        [&](const auto &node) {
          using K = std::decay_t<decltype(node)>;

          std::optional<TAst::TypeID> type = std::nullopt;

          if constexpr (requires() { node.type; }) {
            type = std::optional{node.type};
          }

          if constexpr (std::is_same_v<K, Ast::String> ||
                        std::is_same_v<K, TAst::String>) {
            printHeader(colors[expr.index()],
                        std::format("String {}", node.value), type, node.span);
          } else if constexpr (std::is_same_v<K, Ast::Char> ||
                               std::is_same_v<K, TAst::Char>) {
            printHeader(colors[expr.index()],
                        std::format("Char {}", node.value), type, node.span);
          } else if constexpr (std::is_same_v<K, Ast::Number> ||
                               std::is_same_v<K, TAst::Number>) {
            printHeader(colors[expr.index()],
                        "Number " + std::to_string(node.value), type,
                        node.span);
          } else if constexpr (std::is_same_v<K, Ast::Boolean> ||
                               std::is_same_v<K, TAst::Boolean>) {
            printHeader(
                colors[expr.index()],
                std::format("Boolean {}", node.value ? "true" : "false"), type,
                node.span);
          } else if constexpr (std::is_same_v<K, Ast::StructInit> ||
                               std::is_same_v<K, TAst::StructInit>) {
            printHeader(colors[expr.index()],
                        std::format("StructInit {}", node.name), type,
                        node.span);

            for (size_t i = 0; i < node.fields.size(); i++) {
              auto &field = node.fields.at(i);

              printExpr(*field.value, next, field.name,
                        node.fields.size() == 1 ? false : i == 0);
            }
          } else if constexpr (std::is_same_v<K, Ast::Ident> ||
                               std::is_same_v<K, TAst::Ident>) {
            printHeader(colors[expr.index()],
                        std::format("Ident {}", node.name), type, node.span);
          } else if constexpr (std::is_same_v<K, Ast::Binary> ||
                               std::is_same_v<K, TAst::Binary>) {
            printHeader(colors[expr.index()],
                        std::format("Binary {}", operatorName(node.op)), type,
                        node.span);

            printExpr(*node.left, next, "", true);
            printExpr(*node.right, next, "", false);
          } else if constexpr (std::is_same_v<K, Ast::Call> ||
                               std::is_same_v<K, TAst::Call>) {
            printHeader(colors[expr.index()],
                        std::format("Call {}", node.function), type, node.span);

            for (size_t i = 0; i < node.arguments.size(); i++) {
              printExpr(node.arguments.at(i), next, std::format("arg {}", i),
                        node.arguments.size() == 1 ? false : i == 0);
            }
          } else if constexpr (std::is_same_v<K, Ast::Assign> ||
                               std::is_same_v<K, TAst::Assign>) {
            printHeader(colors[expr.index()],
                        std::format("Assign \"{}\"", node.variable), type,
                        node.span);

            printExpr(*node.value, next, "value", false);
          } else if constexpr (std::is_same_v<K, Ast::Grouping> ||
                               std::is_same_v<K, TAst::Grouping>) {
            printHeader(colors[expr.index()], "Grouping", type, node.span);

            printExpr(*node.inner, next, "", false);
          } else {
            static_assert(false, "Non exhaustive printExpr()");
          }
        },
        expr);
  }

  template <typename T> void printExpr(const T &expr) {
    printExpr(expr, "", "", false);
  }

  template <typename T>
  void printStmt(const T &stmt, std::string prefix, std::string_view label,
                 bool isLeft) {
    startPrint(prefix, label, isLeft);
    std::string next = nextPrefix(prefix, isLeft);

    std::visit(
        [&](const auto &node) {
          using K = std::decay_t<decltype(node)>;

          if constexpr (std::is_same_v<K, Ast::Block> ||
                        std::is_same_v<K, TAst::Block>) {
            printHeader(colors[colors.size() - 1 - stmt.index()], "Block",
                        std::nullopt, node.span);

            for (size_t i = 0; i < node.statements.size(); i++) {
              printStmt(node.statements.at(i), next, "",
                        i != node.statements.size() - 1);
            }
          } else if constexpr (std::is_same_v<K, Ast::Let> ||
                               std::is_same_v<K, TAst::Let>) {
            printHeader(colors[colors.size() - 1 - stmt.index()],
                        std::format("Let {}", node.name), std::nullopt,
                        node.span);

            printExpr(node.initializer, next, "", false);
          } else if constexpr (std::is_same_v<K, Ast::If> ||
                               std::is_same_v<K, TAst::If>) {
            printHeader(colors[colors.size() - 1 - stmt.index()], "If",
                        std::nullopt, node.span);

            printExpr(node.condition, next, "cond", true);
            printStmt(*node.thenStatement, next, "then",
                      node.elseStatement.has_value());
            if (node.elseStatement.has_value()) {
              printStmt(*node.elseStatement->get(), next, "else", false);
            }
          } else if constexpr (std::is_same_v<K, Ast::While> ||
                               std::is_same_v<K, TAst::While>) {
            printHeader(colors[colors.size() - 1 - stmt.index()], "While",
                        std::nullopt, node.span);

            printExpr(node.condition, next, "cond", true);
            printStmt(*node.body, next, "body", false);
          } else if constexpr (std::is_same_v<K, Ast::For>) {
            printHeader(colors[colors.size() - 1 - stmt.index()], "For",
                        std::nullopt, node.span);

            printStmt(*node.initializer, next, "init", true);
            printExpr(node.condition, next, "cond", true);
            printExpr(node.update, next, "update", true);
            printStmt(*node.body, next, "body", false);
          } else if constexpr (std::is_same_v<K, Ast::Return> ||
                               std::is_same_v<K, TAst::Return>) {
            printHeader(colors[colors.size() - 1 - stmt.index()], "Return",
                        std::nullopt, node.span);

            if (node.value.has_value()) {
              printExpr(node.value.value(), next, "", false);
            }
          } else if constexpr (std::is_same_v<K, Ast::ExprStmt> ||
                               std::is_same_v<K, TAst::ExprStmt>) {
            printHeader(colors[colors.size() - 1 - stmt.index()], "Expr Stmt",
                        std::nullopt, node.span);

            printExpr(node.expression, next, "", false);
          } else {
            static_assert(false, "Non exhaustive printStmt()");
          }
        },
        stmt);
  }

  template <typename T> void printStmt(const T &stmt) {
    printStmt(stmt, "", "", false);
  }

  template <typename T>
  void printDecl(const T &decl, std::string prefix, std::string_view label,
                 bool isLeft) {
    startPrint(prefix, label, isLeft);
    std::string next = nextPrefix(prefix, isLeft);

    std::visit(
        [&](const auto &node) {
          using K = std::decay_t<decltype(node)>;

          if constexpr (std::is_same_v<K, Ast::Function> ||
                        std::is_same_v<K, TAst::Function>) {
            std::optional<TAst::TypeID> returnTypeID = std::nullopt;

            if constexpr (std::is_same_v<K, TAst::Function>) {
              returnTypeID = std::optional{node.returnType};
            }

            std::string header{std::format("Function {}(", node.name)};

            for (size_t i = 0; i < node.params.size(); i++) {
              const auto &param = node.params.at(i);

              if constexpr (std::is_same_v<K, Ast::Function>) {
                header += std::format("{}: {}", param.name, param.type);
              } else {
                header +=
                    std::format("{}: {}", param.name, typeName(param.type));
              }

              if (i != node.params.size() - 1) {
                header += ", ";
              }
            }

            header += ")";

            printHeader(colors[decl.index()], header, returnTypeID, node.span);
            printBlock(node.body, next, "", false);
          } else if constexpr (std::is_same_v<K, Ast::Struct> ||
                               std::is_same_v<K, TAst::Struct>) {
            printHeader(colors[decl.index()],
                        std::format("Struct {}", node.name), std::nullopt,
                        node.span);

            for (size_t i = 0; i < node.fields.size(); i++) {
              const auto &param = node.fields.at(i);

              std::string next = nextPrefix(prefix, false);

              if constexpr (std::is_same_v<K, Ast::Struct>) {
                startPrint(next, "", i != node.fields.size() - 1);
                printHeader(colors[0],
                            std::format("{}: {}", param.name, param.type),
                            std::nullopt, node.span);
              } else {
                startPrint(next, "", i != node.fields.size() - 1);
                printHeader(colors[0], param.name, param.type, node.span);
              }
            }
          } else {
            static_assert(false, "Non exhaustive printDecl()");
          }
        },
        decl);
  }

  template <typename T> void printDecl(const T &decl) {
    printDecl(decl, "", "", false);
  }

  template <typename T>
  void printBlock(const T &block, std::string prefix, std::string_view label,
                  bool isLeft) {
    startPrint(prefix, label, isLeft);
    std::string next = nextPrefix(prefix, isLeft);

    printHeader(colors[0], "Block", std::nullopt, block.span);

    for (size_t i = 0; i < block.statements.size(); i++) {
      printStmt(block.statements.at(i), next, "",
                i != block.statements.size() - 1);
    }
  }

  template <typename T> void printBlock(const T &block) {
    printBlock(block, "", "", false);
  }

  template <typename T> void printProgram(const T &prog) {
    if constexpr (std::is_same_v<T, std::vector<Ast::Decl>>) {
      std::println("> Parsed AST");
    } else {
      std::println("> Typed AST");
    }

    for (const auto &node : prog) {
      printDecl(node);
    }
  }
};
