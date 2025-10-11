#include "printer.h"
#include "../tast.h"
#include "../utils.h"
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <print>
#include <string_view>
#include <variant>

namespace Printer {

void startPrint(std::string_view prefix, std::string_view label, bool isLeft) {
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

void printHeader(uint8_t color, std::string_view label,
                 std::optional<TAst::Type> type, std::string_view filename,
                 const Span &span) {
  std::print("\033[38:5:{}m", color);
  std::print("{} ", label);
  std::print("\033[39m");

  if (type.has_value()) {
    std::print("\033[90m");
    std::print(": ");
    std::print("\033[39m");

    std::print("\033[38:5:6m");
    std::print("{} - ", typeName(type.value()));
    std::print("\033[39m");
  } else {
    std::print("\033[90m");
    std::print("- ");
    std::print("\033[39m");
  }

  std::print("\033[90m");
  std::print("\033[3m");
  std::print("[{}:{}:{}]", filename, span.line, span.start + 1);
  std::print("\033[23m");
  std::print("\033[39m");

  std::println();
}

std::string nextPrefix(std::string prefix, bool isLeft) {
  return prefix + (isLeft ? "│   " : "    ");
}

const char *operatorName(const Ast::Operator &op) {
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
  }
}

const char *typeName(const TAst::Type &type) {
  return std::visit(overloads{[](const TAst::TInt &) { return "Int"; },
                              [](const TAst::TBoolean &) { return "Boolean"; },
                              [](const TAst::TVoid &) { return "Void"; }},
                    type);
}

} // namespace Printer