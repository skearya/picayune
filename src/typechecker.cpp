#include "typechecker.h"
#include "ast.h"
#include "tast.h"
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

void TypeChecker::check(const Ast::Expr &node) { std::visit(*this, node); }

std::optional<TAst::Type> TypeChecker::lookup(std::string_view name) {
  for (auto env = environment.rbegin(); env != environment.rend(); env++) {
    if (env->contains(name)) {
      return env->at(name);
    }
  }

  return std::nullopt;
}

TAst::Type TypeChecker::operator()(const Ast::Number &_) {
  return TAst::TInt{};
}

TAst::Type TypeChecker::operator()(const Ast::Boolean &_) {
  return TAst::TBoolean{};
}

TAst::Type TypeChecker::operator()(const Ast::Ident &node) {
  auto var = lookup(node.name);

  if (!var.has_value()) {
    throw std::runtime_error("");
  }

  return *var;
}

TAst::Type TypeChecker::operator()(const Ast::Binary &node) {
  auto left = std::visit(*this, *node.left);
  auto right = std::visit(*this, *node.right);

  if (left.index() != right.index()) {
    throw std::runtime_error("Expected binary operands to be the same type");
  }

  switch (node.op) {
  case Ast::Operator::Add:
  case Ast::Operator::Sub:
  case Ast::Operator::Mul:
  case Ast::Operator::Div:
    if (!std::holds_alternative<TAst::TInt>(left)) {
      throw std::runtime_error(
          "Expected arithmetic operator to be used with numbers");
    }

    return TAst::TInt{};
  case Ast::Operator::Eq:
  case Ast::Operator::NotEq:
    return TAst::TBoolean{};

  case Ast::Operator::Lt:
  case Ast::Operator::LtEq:
  case Ast::Operator::Gt:
  case Ast::Operator::GtEq:
    if (!std::holds_alternative<TAst::TInt>(left)) {
      throw std::runtime_error(
          "Expected comparison operator to be used with numbers");
    }

    return TAst::TBoolean{};
  }
}

TAst::Type TypeChecker::operator()(const Ast::Call &_) {
  throw std::runtime_error("");
}

TAst::Type TypeChecker::operator()(const Ast::Grouping &node) {
  return std::visit(*this, *node.inner);
}