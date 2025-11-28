#include "typechecker.hpp"
#include "ast.hpp"
#include "tast.hpp"
#include "utils.hpp"
#include <cstddef>
#include <memory>
#include <optional>
#include <print>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

std::vector<TAst::Decl>
TypeChecker::check(const std::vector<Ast::Decl> &program) {
  for (const auto &decl : program) {
    PartialDecl partialDecl = std::visit(
        overloads{[](const Ast::Function &function) {
          std::vector<TAst::Parameter> typedParams{};

          for (auto &param : function.params) {
            typedParams.push_back(
                TAst::Parameter{identToType(param.type), param.name});
          }

          return PartialFunction{function.span, identToType(function.type),
                                 function.name, std::move(typedParams),
                                 function.body};
        }},
        decl);

    partialDeclarations.push_back(partialDecl);
  }

  std::vector<TAst::Decl> typedDecls;

  for (const auto &decl : partialDeclarations) {
    TAst::Decl typedDecl = std::visit(
        overloads{[this](const PartialFunction &node) {
          currentFunction = &node;

          environments.push_back(
              std::unordered_map<std::string_view, TAst::Type>{});

          for (const auto &param : node.params) {
            environments.back()[param.name] = param.type;
          }

          auto block = checkBlock(node.body);

          environments.pop_back();

          if (node.type.index() != TAst::Type{TAst::TVoid{}}.index() &&
              !doesBlockReturn(block)) {
            throw std::runtime_error(
                "Function does not return value in all branches");
          }

          return TAst::Function{node.span, node.type, node.name, node.params,
                                std::move(block)};
        }},
        decl);

    typedDecls.push_back(std::move(typedDecl));
  }

  return typedDecls;
}

TAst::Expr TypeChecker::checkExpr(const Ast::Expr &node) {
  return std::visit(*this, node);
}

TAst::Expr TypeChecker::operator()(const Ast::Number &node) {
  return TAst::Number{node.span, node.value};
}

TAst::Expr TypeChecker::operator()(const Ast::Boolean &node) {
  return TAst::Boolean{node.span, node.value};
}

TAst::Expr TypeChecker::operator()(const Ast::Ident &node) {
  auto type = lookup(node.name);

  if (!type.has_value()) {
    throw std::runtime_error("Identifier is not defined");
  }

  return TAst::Ident{*type, node.span, node.name};
}

TAst::Expr TypeChecker::operator()(const Ast::Binary &node) {
  auto left = std::visit(*this, *node.left);
  auto right = std::visit(*this, *node.right);

  auto lefttype = TAst::getType(left);
  auto righttype = TAst::getType(right);

  if (lefttype.index() != righttype.index()) {
    throw std::runtime_error("Expected binary operands to be the same type");
  }

  TAst::Type type;

  switch (node.op) {
  case Ast::Operator::Add:
  case Ast::Operator::Sub:
  case Ast::Operator::Mul:
  case Ast::Operator::Div:
    if (!std::holds_alternative<TAst::TInt>(lefttype)) {
      throw std::runtime_error(
          "Expected arithmetic operator to be used with numbers");
    }

    type = TAst::TInt{};
    break;
  case Ast::Operator::Eq:
  case Ast::Operator::NotEq:
    type = TAst::TBoolean{};
    break;

  case Ast::Operator::Lt:
  case Ast::Operator::LtEq:
  case Ast::Operator::Gt:
  case Ast::Operator::GtEq:
    if (!std::holds_alternative<TAst::TInt>(lefttype)) {
      throw std::runtime_error(
          "Expected comparison operator to be used with numbers");
    }

    type = TAst::TBoolean{};
    break;
  case Ast::Operator::Or:
  case Ast::Operator::And:
    if (!std::holds_alternative<TAst::TBoolean>(lefttype)) {
      throw std::runtime_error(
          "Expected logical operator to be used with booleans");
    }

    type = TAst::TBoolean{};
    break;
  }

  return TAst::Binary{type, node.span,
                      std::make_unique<TAst::Expr>(std::move(left)), node.op,
                      std::make_unique<TAst::Expr>(std::move(right))};
}

TAst::Expr TypeChecker::operator()(const Ast::Call &node) {
  auto function = lookupFunction(node.function);

  if (!function.has_value()) {
    throw std::runtime_error("Tried to call undefined function");
  }

  const auto &functiontype = function.value()->type;
  const auto &params = function.value()->params;

  if (node.arguments.size() != params.size()) {
    throw std::runtime_error("Function call arity mismatch");
  }

  std::vector<TAst::Expr> arguments;

  for (size_t i = 0; i < params.size(); i++) {
    auto arg = checkExpr(node.arguments.at(i));
    auto argtype = TAst::getType(arg);

    if (argtype.index() != params.at(i).type.index()) {
      throw std::runtime_error("Function call argument type mismatch");
    }

    arguments.push_back(std::move(arg));
  }

  return TAst::Call{functiontype, node.span, node.function,
                    std::move(arguments)};
}

TAst::Expr TypeChecker::operator()(const Ast::Assign &node) {
  auto targettype = lookup(node.variable);

  if (!targettype.has_value()) {
    throw std::runtime_error("Assignment target is not defined");
  }

  auto value = checkExpr(*node.value);
  auto valuetype = TAst::getType(value);

  if (targettype.value().index() != valuetype.index()) {
    throw std::runtime_error("Assignment value doesn't match expected type");
  }

  return TAst::Assign{valuetype, node.span, node.variable,
                      std::make_unique<TAst::Expr>(std::move(value))};
}

TAst::Expr TypeChecker::operator()(const Ast::Grouping &node) {
  auto inner = checkExpr(*node.inner);
  auto innertype = TAst::getType(inner);

  return TAst::Grouping{innertype, node.span,
                        std::make_unique<TAst::Expr>(std::move(inner))};
}

TAst::Stmt TypeChecker::checkStmt(const Ast::Stmt &node) {
  return std::visit(*this, node);
}

TAst::Stmt TypeChecker::operator()(const Ast::Block &node) {
  return checkBlock(node);
}

TAst::Stmt TypeChecker::operator()(const Ast::Let &node) {
  auto init = checkExpr(node.initializer);
  auto inittype = TAst::getType(init);

  environments.back()[node.name] = inittype;

  return TAst::Let{node.span, node.name, std::move(init)};
}

TAst::Stmt TypeChecker::operator()(const Ast::If &node) {
  auto cond = checkExpr(node.condition);
  auto condtype = TAst::getType(cond);

  if (!std::holds_alternative<TAst::TBoolean>(condtype)) {
    throw std::runtime_error(
        "Expected if statement's condition's type to be 'bool'");
  }

  auto thenStatement = checkStmt(*node.thenStatement);
  auto elseStatement = node.elseStatement.transform([this](auto &stmt) {
    return std::make_unique<TAst::Stmt>(checkStmt(*stmt));
  });

  return TAst::If{node.span, std::move(cond),
                  std::make_unique<TAst::Stmt>(std::move(thenStatement)),
                  std::move(elseStatement)};
}

TAst::Stmt TypeChecker::operator()(const Ast::While &node) {
  auto cond = checkExpr(node.condition);
  auto condtype = TAst::getType(cond);

  if (!std::holds_alternative<TAst::TBoolean>(condtype)) {
    throw std::runtime_error(
        "Expected while statement's condition's type to be 'bool'");
  }

  auto body = checkStmt(*node.body);

  return TAst::While{node.span, std::move(cond),
                     std::make_unique<TAst::Stmt>(std::move(body))};
}

TAst::Stmt TypeChecker::operator()(const Ast::For &node) {
  auto init = checkStmt(*node.initializer);

  auto cond = checkExpr(node.condition);
  auto condtype = TAst::getType(cond);

  if (!std::holds_alternative<TAst::TBoolean>(condtype)) {
    throw std::runtime_error(
        "Expected for statement's condition's type to be 'bool'");
  }

  auto update = checkExpr(node.update);

  auto body = checkStmt(*node.body);

  std::vector<TAst::Stmt> statements{};

  statements.push_back(std::move(init));
  statements.push_back(
      TAst::While{node.span, std::move(cond),
                  std::make_unique<TAst::Stmt>(std::move(body))});

  return TAst::Block{node.span, std::move(statements)};
}

TAst::Stmt TypeChecker::operator()(const Ast::Return &node) {
  auto value =
      node.value.transform([this](auto &expr) { return checkExpr(expr); });

  auto returntype = currentFunction->type;

  if (value.has_value()) {
    auto valuetype = TAst::getType(value.value());

    if (valuetype.index() != returntype.index()) {
      throw std::runtime_error(
          "Expected return type to match function return type");
    }
  } else {
    if (!std::holds_alternative<TAst::TVoid>(returntype)) {
      throw std::runtime_error("Expected return value to exist");
    }
  }

  return TAst::Return{node.span, std::move(value)};
}

TAst::Stmt TypeChecker::operator()(const Ast::ExprStmt &node) {
  return TAst::ExprStmt{node.span, checkExpr(node.expression)};
}

TAst::Block TypeChecker::checkBlock(const Ast::Block &node) {
  environments.push_back(std::unordered_map<std::string_view, TAst::Type>{});

  std::vector<TAst::Stmt> body;

  for (auto &stmt : node.statements) {
    body.push_back(checkStmt(stmt));
  }

  environments.pop_back();

  return TAst::Block{node.span, std::move(body)};
}

std::optional<TAst::Type> TypeChecker::lookup(std::string_view name) {
  for (auto env = environments.rbegin(); env != environments.rend(); env++) {
    if (env->contains(name)) {
      return env->at(name);
    }
  }

  return std::nullopt;
}

std::optional<const PartialFunction *>
TypeChecker::lookupFunction(std::string_view name) {
  for (const auto &decl : partialDeclarations) {
    if (const auto *function = std::get_if<PartialFunction>(&decl)) {
      if (function->name == name) {
        return std::optional{function};
      }
    }
  }

  return std::nullopt;
}

TAst::Type identToType(std::string_view ident) {
  if (ident == "int") {
    return TAst::TInt{};
  } else if (ident == "bool") {
    return TAst::TBoolean{};
  } else if (ident == "void") {
    return TAst::TVoid{};
  } else {
    throw std::runtime_error("Couldn't resolve type");
  }
}

bool doesBlockReturn(const TAst::Block &node) {
  for (size_t i = 0; i < node.statements.size(); i++) {
    if (doesReturn(node.statements.at(i))) {
      if (i != node.statements.size() - 1) {
        std::println("Warning: Code after return statement is unreachable");
      }

      return true;
    }
  }

  return false;
}

bool doesReturn(const TAst::Stmt &node) {
  return std::visit(
      overloads{[](const TAst::Block &node) { return doesBlockReturn(node); },
                [](const TAst::Let &) { return false; },
                [](const TAst::If &node) {
                  return node.elseStatement.has_value() &&
                         doesReturn(*node.thenStatement) &&
                         doesReturn(*node.elseStatement.value());
                },
                [](const TAst::While &) { return false; },
                [](const TAst::Return &) { return true; },
                [](const TAst::ExprStmt &) { return false; }},
      node);
}
