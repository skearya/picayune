#include "../ast.h"
#include <print>

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

std::string nextPrefix(std::string prefix, bool isLeft) {
  return prefix + (isLeft ? "│   " : "    ");
}

void printHeader(uint8_t color, std::string_view label,
                 std::string_view filename, const Span &span) {
  std::print("\033[38:5:{}m", color);
  std::print("{}", label);
  std::print("\033[39m");

  std::print("\033[90m");
  std::print(" - ");
  std::print("\033[39m");

  std::print("\033[90m");
  std::print("\033[3m");
  std::print("[{}:{}:{}]", filename, span.line, span.start + 1);
  std::print("\033[23m");
  std::print("\033[39m");

  std::println();
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
