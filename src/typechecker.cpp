#include "typechecker.h"
#include "ast.h"
#include "tast.h"
#include "utils.h"
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

std::vector<TAst::Decl>
TypeChecker::check(const std::vector<Ast::Decl> &program) {
  std::vector<PartialDecl> partialTypedDecls{};

  for (auto &decl : program) {
    partialTypedDecls.push_back(std::visit(*this, decl));
  }

  std::vector<TAst::Decl> typedDecls{};

  for (auto &decl : partialTypedDecls) {
    TAst::Decl typedDecl =
        std::visit(overloads{[this](const PartialFunction &node) {
                     return TAst::Function{node.span, node.type, node.name,
                                           node.params, checkBlock(node.body)};
                   }},
                   decl);

    typedDecls.push_back(std::move(typedDecl));
  }

  return typedDecls;
}

PartialDecl TypeChecker::operator()(const Ast::Function &function) {
  std::vector<TAst::Parameter> typedParams{};

  for (auto &param : function.params) {
    typedParams.push_back(TAst::Parameter{identToType(param.type), param.name});
  }

  return PartialFunction{function.span, TAst::TVoid{}, function.name,
                         std::move(typedParams), function.body};
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
  }

  return TAst::Binary{type, node.span,
                      std::make_unique<TAst::Expr>(std::move(left)), node.op,
                      std::make_unique<TAst::Expr>(std::move(right))};
}

TAst::Expr TypeChecker::operator()(const Ast::Call &_) {
  throw std::runtime_error("TODO: Call");
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
  auto cond = checkExpr(node.cond);
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

TAst::Stmt TypeChecker::operator()(const Ast::Return &node) {
  return TAst::Return{node.span, node.value.transform([this](auto &expr) {
                        return checkExpr(expr);
                      })};
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
