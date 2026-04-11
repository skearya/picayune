#include "printer.hpp"
#include "../tast.hpp"
#include "../utils.hpp"
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <print>
#include <span>
#include <string_view>
#include <variant>

Printer::Printer(std::string_view filename) : filename{filename}, typeArena{} {}

Printer::Printer(std::string_view filename, std::span<TAst::Type> typeArena)
    : filename{filename}, typeArena{typeArena} {}

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
                          std::optional<TAst::TypeID> typeID,
                          const Span &span) {
  std::print("\033[38:5:{}m", color);
  std::print("{}", label);
  std::print("\033[39m");

  if (typeID.has_value()) {
    std::print("\033[90m");
    std::print(" : ");
    std::print("\033[39m");

    std::print("\033[38:5:6m");
    std::print("{}", typeName(typeID.value()));
    std::print("\033[39m");
  }

  std::print("\033[37m");
  std::print(" - ");
  std::print("\033[39m");

  std::print("\033[90m");
  std::print("\033[3m");
  std::print("[{}:{}:{}]", filename, span.line, span.start + 1);
  std::print("\033[23m");
  std::print("\033[39m");

  std::println();
}

void Printer::printStructField(
    std::string_view label,
    std::variant<std::string_view, TAst::TypeID> value) {
  std::print("\033[39m");
  std::print("{}", label);

  std::print("\033[90m");
  std::print(" : ");
  std::print("\033[39m");

  std::visit(
      overloads{[](const std::string_view &name) { std::print("{}", name); },
                [this](const TAst::TypeID &typeID) {
                  std::print("\033[38:5:6m");
                  std::print("{}", typeName(typeID));
                  std::print("\033[39m");
                }},
      value);

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

std::string_view Printer::typeName(TAst::TypeID typeID) {
  return std::visit(
      overloads{
          [](const TAst::TVoid &) -> std::string_view { return "Void"; },
          [](const TAst::TString &) -> std::string_view { return "String"; },
          [](const TAst::TChar &) -> std::string_view { return "Char"; },
          [](const TAst::TInt &) -> std::string_view { return "Int"; },
          [](const TAst::TBoolean &) -> std::string_view { return "Boolean"; },
          [](const TAst::TStruct &structt) { return structt.name; },
          [](const TAst::TFunction &) -> std::string_view {
            return "Function";
          },
      },
      typeArena[typeID.id]);
}