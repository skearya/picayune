#include "printer.hpp"
#include "../utils.hpp"
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <print>
#include <string_view>
#include <variant>

Printer::Printer(Storage &storage, std::string_view filename)
    : storage{storage}, filename{filename} {}

void Printer::printExpr(Ast::ExprId exprId) {
  printExpr(exprId, "", "", false);
}

void Printer::printExpr(Ast::ExprId exprId, std::string prefix,
                        std::string_view label, bool isLeft) {
  startPrint(prefix, label, isLeft);
  std::string next = nextPrefix(prefix, isLeft);

  const Ast::Expr &expr = storage.get(exprId);
  uint8_t color = colors[expr.node.index()];

  std::optional<Type::TypeID> type = std::nullopt;

  std::visit(
      overloads{
          [&](const Ast::String &node) {
            printHeader(color, std::format("String {}", node.value), type,
                        expr.span);
          },
          [&](const Ast::Char &node) {
            printHeader(color, std::format("Char {}", node.value), type,
                        expr.span);
          },
          [&](const Ast::Number &node) {
            printHeader(color, "Number " + std::to_string(node.value), type,
                        expr.span);
          },
          [&](const Ast::Boolean &node) {
            printHeader(
                color, std::format("Boolean {}", node.value ? "true" : "false"),
                type, expr.span);
          },
          [&](const Ast::StructInit &node) {
            printHeader(color, std::format("StructInit {}", node.name), type,
                        expr.span);

            for (size_t i = 0; i < node.fields.size(); i++) {
              auto &field = node.fields.at(i);

              printExpr(field.value, next, field.name,
                        node.fields.size() == 1 ? false : i == 0);
            }
          },
          [&](const Ast::Get &node) {
            printHeader(color, std::format("Get '{}'", node.field), type,
                        expr.span);

            printExpr(node.expr, next, "expr", false);
          },
          [&](const Ast::Ident &node) {
            printHeader(color, std::format("Ident {}", node.name), type,
                        expr.span);
          },
          [&](const Ast::Binary &node) {
            printHeader(color, std::format("Binary {}", operatorName(node.op)),
                        type, expr.span);

            printExpr(node.left, next, "", true);
            printExpr(node.right, next, "", false);
          },
          [&](const Ast::Call &node) {
            printHeader(color, std::format("Call"), type, expr.span);

            printExpr(node.function, next, "callee",
                      node.arguments.size() != 0);

            for (size_t i = 0; i < node.arguments.size(); i++) {
              printExpr(node.arguments.at(i), next, std::format("arg {}", i),
                        node.arguments.size() == 1 ? false : i == 0);
            }
          },
          [&](const Ast::Assign &node) {
            printHeader(color, std::format("Assign \"{}\"", node.variable),
                        type, expr.span);

            printExpr(node.value, next, "value", false);
          },
          [&](const Ast::Grouping &node) {
            printHeader(color, "Grouping", type, expr.span);

            printExpr(node.inner, next, "", false);
          }},
      expr.node);
}

void Printer::printStmt(Ast::StmtId stmtId) {
  printStmt(stmtId, "", "", false);
}

void Printer::printStmt(Ast::StmtId stmtId, std::string prefix,
                        std::string_view label, bool isLeft) {
  startPrint(prefix, label, isLeft);
  std::string next = nextPrefix(prefix, isLeft);

  const Ast::Stmt &stmt = storage.get(stmtId);
  uint8_t color = colors[colors.size() - 1 - stmt.node.index()];

  std::visit(
      overloads{[&](const Ast::Block &node) {
                  printHeader(color, "Block", std::nullopt, stmt.span);

                  for (size_t i = 0; i < node.statements.size(); i++) {
                    printStmt(node.statements.at(i), next, "",
                              i != node.statements.size() - 1);
                  }
                },
                [&](const Ast::Let &node) {
                  printHeader(color, std::format("Let {}", node.name),
                              std::nullopt, stmt.span);

                  printExpr(node.initializer, next, "", false);
                },
                [&](const Ast::If &node) {
                  printHeader(color, "If", std::nullopt, stmt.span);

                  printExpr(node.condition, next, "cond", true);
                  printStmt(node.thenStatement, next, "then",
                            node.elseStatement.has_value());
                  if (node.elseStatement.has_value()) {
                    printStmt(node.elseStatement.value(), next, "else", false);
                  }
                },
                [&](const Ast::While &node) {
                  printHeader(color, "While", std::nullopt, stmt.span);

                  printExpr(node.condition, next, "cond", true);
                  printStmt(node.body, next, "body", false);
                },
                [&](const Ast::For &node) {
                  printHeader(color, "For", std::nullopt, stmt.span);

                  printStmt(node.initializer, next, "init", true);
                  printExpr(node.condition, next, "cond", true);
                  printExpr(node.update, next, "update", true);
                  printStmt(node.body, next, "body", false);
                },
                [&](const Ast::Return &node) {
                  printHeader(color, "Return", std::nullopt, stmt.span);

                  if (node.value.has_value()) {
                    printExpr(node.value.value(), next, "", false);
                  }
                },
                [&](const Ast::ExprStmt &node) {
                  printHeader(color, "Expr Stmt", std::nullopt, stmt.span);

                  printExpr(node.expression, next, "", false);
                }},
      stmt.node);
}

void Printer::printDecl(Ast::DeclId declId) {
  printDecl(declId, "", "", false);
}

void Printer::printDecl(Ast::DeclId declId, std::string prefix,
                        std::string_view label, bool isLeft) {
  startPrint(prefix, label, isLeft);
  std::string next = nextPrefix(prefix, isLeft);

  const Ast::Decl &decl = storage.get(declId);
  uint8_t color = colors[decl.node.index()];

  std::visit(
      overloads{[&](const Ast::Struct &node) {
                  printHeader(color, std::format("Struct {}", node.name),
                              std::nullopt, decl.span);

                  for (size_t i = 0; i < node.fields.size(); i++) {
                    const auto &param = node.fields.at(i);

                    std::string next = nextPrefix(prefix, false);

                    startPrint(next, "", i != node.fields.size() - 1);
                    printStructField(param.name, param.type);
                  }
                },
                [&](const Ast::Function &node) {
                  std::optional<Type::TypeID> returnTypeId = std::nullopt;

                  std::string header{std::format("Function {}(", node.name)};

                  for (size_t i = 0; i < node.params.size(); i++) {
                    const auto &param = node.params.at(i);

                    header += std::format("{}: {}", param.name, param.type);

                    if (i != node.params.size() - 1) {
                      header += ", ";
                    }
                  }

                  header += ")";

                  printHeader(color, header, returnTypeId, decl.span);
                  printStmt(node.body, next, "body", false);
                }},
      decl.node);
}

void Printer::printProgram(const std::vector<Ast::DeclId> &prog) {
  std::println("> AST");

  for (const auto &node : prog) {
    printDecl(node);
  }
}

void Printer::startPrint(std::string_view prefix, std::string_view label,
                         bool isLeft) {
  std::print("\033[90m");

  if (prefix.empty()) {
    std::print("> ");
  } else {
    std::print("{}", prefix);
    std::print("{}", isLeft ? "├───" : "╰───");

    if (!label.empty()) {
      std::print("{}: ", label);
    }
  }

  std::print("\033[39m");
}

void Printer::printHeader(uint8_t color, std::string_view label,
                          std::optional<Type::TypeID> typeID,
                          const Span &span) {
  std::print("\033[38;5;{}m", color);
  std::print("{}", label);
  std::print("\033[39m");

  if (typeID.has_value()) {
    std::print("\033[90m");
    std::print(" : ");
    std::print("\033[39m");

    std::print("\033[38:5:6m");
    std::print("{}", "TODO");
    std::print("\033[39m");
  }

  std::print("\033[90m");
  std::print("\033[3m");
  std::print(" - [{}:{}:{}]", filename, span.line, span.start + 1);
  std::print("\033[23m");
  std::print("\033[39m");

  std::println();
}

void Printer::printStructField(std::string_view label, std::string_view value) {
  std::print("{}", label);

  std::print("\033[90m");
  std::print(" : ");
  std::print("\033[39m");

  std::print("{}", value);

  std::println();
}

std::string Printer::nextPrefix(std::string prefix, bool isLeft) {
  return prefix + (isLeft ? "│   " : "    ");
}

std::string_view Printer::operatorName(Ast::Operator op) {
  switch (op) {
  case Ast::Operator::Add:
    return "Add";
  case Ast::Operator::Sub:
    return "Sub";
  case Ast::Operator::Mul:
    return "Mul";
  case Ast::Operator::Div:
    return "Div";
  case Ast::Operator::Eq:
    return "Eq";
  case Ast::Operator::NotEq:
    return "NotEq";
  case Ast::Operator::Lt:
    return "Lt";
  case Ast::Operator::LtEq:
    return "LtEq";
  case Ast::Operator::Gt:
    return "Gt";
  case Ast::Operator::GtEq:
    return "GtEq";
  case Ast::Operator::Or:
    return "Or";
  case Ast::Operator::And:
    return "And";
  }
}
