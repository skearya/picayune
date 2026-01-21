#include "typechecker.hpp"
#include "ast.hpp"
#include "tast.hpp"
#include "utils.hpp"
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <print>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

TypeChecker::TypeChecker()
    : typeArena{}, types{}, functions{}, environments{{}},
      currentFunction{nullptr}, voidTypeID{internType(TAst::TVoid{})},
      stringTypeID{internType(TAst::TString{})},
      charTypeID{internType(TAst::TChar{})},
      intTypeID{internType(TAst::TInt{})},
      booleanTypeID{internType(TAst::TBoolean{})} {
  types.insert({"void", voidTypeID});
  types.insert({"string", stringTypeID});
  types.insert({"char", charTypeID});
  types.insert({"int", intTypeID});
  types.insert({"boolean", booleanTypeID});
}

std::vector<TAst::Decl>
TypeChecker::check(const std::vector<Ast::Decl> &program) {
  std::vector<TAst::Decl> decls;

  for (const auto &decl : program) {
    if (const auto *structt = std::get_if<Ast::Struct>(&decl)) {
      auto typeId = internType(TAst::TStruct{structt->name, {}});

      types.insert({structt->name, typeId});
    }
  }

  for (const auto &decl : program) {
    if (const auto *structt = std::get_if<Ast::Struct>(&decl)) {
      std::vector<TAst::Field> fields;

      for (const auto &param : structt->fields) {
        auto typeId = lookupType(param.type);

        if (!typeId.has_value()) {
          throw std::runtime_error("Field type is not defined");
        }

        fields.push_back(TAst::Field{typeId.value(), param.name});
      }

      auto &type = typeArena[types.at(structt->name).id];
      auto &structType = std::get<TAst::TStruct>(type);

      structType.name = structt->name;
      structType.fields = fields;

      decls.push_back(
          TAst::Struct{structt->span, structt->name, std::move(fields)});
    }
  }

  for (const auto &decl : program) {
    if (const auto *function = std::get_if<Ast::Function>(&decl)) {
      std::vector<TAst::Parameter> params;

      for (const auto &param : function->params) {
        auto typeId = lookupType(param.type);

        if (!typeId.has_value()) {
          throw std::runtime_error("Parameter type is not defined");
        }

        params.push_back(TAst::Parameter{typeId.value(), param.name});
      }

      auto returnTypeId = lookupType(function->returnType);

      if (!returnTypeId.has_value()) {
        throw std::runtime_error("Return type is not defined");
      }

      auto typeId =
          internType(TAst::TFunction{std::move(params), returnTypeId.value()});

      functions.insert({function->name, typeId});
    }
  }

  for (const auto &decl : program) {
    if (const auto *function = std::get_if<Ast::Function>(&decl)) {
      auto type = typeArena[functions.at(function->name).id];
      auto functionType = std::get<TAst::TFunction>(type);

      currentFunction = &functionType;

      environments.push_back({});

      for (size_t i = 0; i < function->params.size(); i++) {
        environments.back().insert(
            {function->params[i].name, functionType.parameters[i].type});
      }

      auto block = checkBlock(function->body);

      environments.pop_back();

      if (functionType.returnType != voidTypeID && !doesBlockReturn(block)) {
        throw std::runtime_error(
            "Function does not return value in all branches");
      }

      decls.push_back(TAst::Function{
          function->span, function->name, functionType.parameters,
          functionType.returnType, std::move(block)});
    }
  }

  return decls;
}

TAst::Expr TypeChecker::checkExpr(const Ast::Expr &node) {
  return std::visit(*this, node);
}

TAst::Expr TypeChecker::operator()(const Ast::String &node) {
  return TAst::String{stringTypeID, node.span, node.value};
}

TAst::Expr TypeChecker::operator()(const Ast::Char &node) {
  return TAst::Char{charTypeID, node.span, node.value};
}

TAst::Expr TypeChecker::operator()(const Ast::Number &node) {
  return TAst::Number{intTypeID, node.span, node.value};
}

TAst::Expr TypeChecker::operator()(const Ast::Boolean &node) {
  return TAst::Boolean{booleanTypeID, node.span, node.value};
}

TAst::Expr TypeChecker::operator()(const Ast::StructInit &node) {
  auto typeId = lookupType(node.name);

  if (!typeId.has_value()) {
    throw std::runtime_error("Struct does not exist");
  }

  auto type = typeArena[typeId.value().id];

  if (auto *structType = std::get_if<TAst::TStruct>(&type)) {
    std::vector<TAst::FieldInit> fieldInits;

    for (const auto &fieldInit : node.fields) {
      auto structField = std::ranges::find_if(
          structType->fields, [&](const auto &structField) {
            return structField.name == fieldInit.name;
          });

      if (structField == std::end(structType->fields)) {
        throw std::runtime_error("Struct field does not exist");
      }

      auto fieldValue = checkExpr(*fieldInit.value);
      auto fieldValueTypeId = getTypeID(fieldValue);

      if (fieldValueTypeId == structField->type) {
        fieldInits.emplace_back(fieldInit.name, std::make_unique<TAst::Expr>(
                                                    std::move(fieldValue)));
      } else {
        throw std::runtime_error("Mismatched struct field type");
      }
    }

    return TAst::StructInit{typeId.value(), node.span, node.name,
                            std::move(fieldInits)};
  } else {
    throw std::runtime_error("Type is not a struct");
  }
}

TAst::Expr TypeChecker::operator()(const Ast::Get &node) {
  auto expr = checkExpr(*node.expr);
  auto exprTypeId = getTypeID(expr);
  auto exprType = typeArena[exprTypeId.id];

  if (const auto *structType = std::get_if<TAst::TStruct>(&exprType)) {
    auto structField =
        std::ranges::find_if(structType->fields, [&](const auto &structField) {
          return structField.name == node.name;
        });

    if (structField == std::end(structType->fields)) {
      throw std::runtime_error(
          "Tried getting struct field that does not exist");
    }

    return TAst::Get{structField->type, node.span,
                     std::make_unique<TAst::Expr>(std::move(expr)), node.name};
  } else {
    throw std::runtime_error("Requested field from non-struct");
  }
}

TAst::Expr TypeChecker::operator()(const Ast::Ident &node) {
  auto typeId = lookupVar(node.name);

  if (!typeId.has_value()) {
    throw std::runtime_error("Identifier is not defined");
  }

  return TAst::Ident{typeId.value(), node.span, node.name};
}

TAst::Expr TypeChecker::operator()(const Ast::Binary &node) {
  auto left = std::visit(*this, *node.left);
  auto right = std::visit(*this, *node.right);

  auto leftTypeId = getTypeID(left);
  auto rightTypeId = getTypeID(right);

  if (leftTypeId != rightTypeId) {
    throw std::runtime_error("Expected binary operands to be the same type");
  }

  TAst::TypeID type;

  switch (node.op) {
  case Ast::Operator::Add:
  case Ast::Operator::Sub:
  case Ast::Operator::Mul:
  case Ast::Operator::Div:
    if (leftTypeId != intTypeID) {
      throw std::runtime_error(
          "Expected arithmetic operator to be used with numbers");
    }

    type = intTypeID;
    break;
  case Ast::Operator::Eq:
  case Ast::Operator::NotEq:
    type = booleanTypeID;
    break;
  case Ast::Operator::Lt:
  case Ast::Operator::LtEq:
  case Ast::Operator::Gt:
  case Ast::Operator::GtEq:
    if (leftTypeId != intTypeID) {
      throw std::runtime_error(
          "Expected comparison operator to be used with numbers");
    }

    type = booleanTypeID;
    break;
  case Ast::Operator::Or:
  case Ast::Operator::And:
    if (leftTypeId != booleanTypeID) {
      throw std::runtime_error(
          "Expected logical operator to be used with booleans");
    }

    type = booleanTypeID;
    break;
  }

  return TAst::Binary{type, node.span,
                      std::make_unique<TAst::Expr>(std::move(left)), node.op,
                      std::make_unique<TAst::Expr>(std::move(right))};
}

TAst::Expr TypeChecker::operator()(const Ast::Call &node) {
  auto callee = checkExpr(*node.function);
  auto calleeTypeId = getTypeID(callee);
  auto calleeType = typeArena[calleeTypeId.id];

  if (const auto *functionType = std::get_if<TAst::TFunction>(&calleeType)) {
    if (node.arguments.size() != functionType->parameters.size()) {
      throw std::runtime_error("Function call arity mismatch");
    }

    std::vector<TAst::Expr> arguments;

    for (size_t i = 0; i < functionType->parameters.size(); i++) {
      auto arg = checkExpr(node.arguments.at(i));
      auto param = functionType->parameters.at(i);

      auto argTypeId = getTypeID(arg);
      auto paramTypeId = param.type;

      if (argTypeId != paramTypeId) {
        throw std::runtime_error("Function call argument type mismatch");
      }

      arguments.push_back(std::move(arg));
    }

    return TAst::Call{functionType->returnType, node.span,
                      std::make_unique<TAst::Expr>(std::move(callee)),
                      std::move(arguments)};
  } else {
    throw std::runtime_error("Called non-function type");
  }
}

TAst::Expr TypeChecker::operator()(const Ast::Assign &node) {
  auto targetTypeId = lookupVar(node.variable);

  if (!targetTypeId.has_value()) {
    throw std::runtime_error("Assignment target is not defined");
  }

  auto value = checkExpr(*node.value);
  auto valueTypeId = getTypeID(value);

  if (targetTypeId != valueTypeId) {
    throw std::runtime_error("Assignment value doesn't match expected type");
  }

  return TAst::Assign{valueTypeId, node.span, node.variable,
                      std::make_unique<TAst::Expr>(std::move(value))};
}

TAst::Expr TypeChecker::operator()(const Ast::Grouping &node) {
  auto inner = checkExpr(*node.inner);
  auto innerTypeId = getTypeID(inner);

  return TAst::Grouping{innerTypeId, node.span,
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
  auto initTypeId = getTypeID(init);

  environments.back().insert({node.name, initTypeId});

  return TAst::Let{node.span, node.name, std::move(init)};
}

TAst::Stmt TypeChecker::operator()(const Ast::If &node) {
  auto cond = checkExpr(node.condition);
  auto condTypeId = getTypeID(cond);

  if (condTypeId != booleanTypeID) {
    throw std::runtime_error(
        "Expected if statement's condition's type to be 'bool'");
  }

  auto thenStatement = checkStmt(*node.thenStatement);
  auto elseStatement = node.elseStatement.transform([this](const auto &stmt) {
    return std::make_unique<TAst::Stmt>(checkStmt(*stmt));
  });

  return TAst::If{node.span, std::move(cond),
                  std::make_unique<TAst::Stmt>(std::move(thenStatement)),
                  std::move(elseStatement)};
}

TAst::Stmt TypeChecker::operator()(const Ast::While &node) {
  auto cond = checkExpr(node.condition);
  auto condTypeId = getTypeID(cond);

  if (condTypeId != booleanTypeID) {
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
  auto condTypeId = getTypeID(cond);

  if (condTypeId != booleanTypeID) {
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
  auto returnTypeId = currentFunction->returnType;

  if (node.value.has_value()) {
    auto value = checkExpr(node.value.value());
    auto valueTypeId = getTypeID(value);

    if (valueTypeId != returnTypeId) {
      throw std::runtime_error(
          "Expected return type to match function return type");
    }

    return TAst::Return{node.span, std::move(value)};
  } else {
    if (returnTypeId != internType(TAst::TVoid{})) {
      throw std::runtime_error("Expected return value to exist");
    }

    return TAst::Return{node.span, std::nullopt};
  }
}

TAst::Stmt TypeChecker::operator()(const Ast::ExprStmt &node) {
  return TAst::ExprStmt{node.span, checkExpr(node.expression)};
}

TAst::Block TypeChecker::checkBlock(const Ast::Block &node) {
  environments.push_back(std::unordered_map<std::string_view, TAst::TypeID>{});

  std::vector<TAst::Stmt> body;

  for (auto &stmt : node.statements) {
    body.push_back(checkStmt(stmt));
  }

  environments.pop_back();

  return TAst::Block{node.span, std::move(body)};
}

std::optional<TAst::TypeID> TypeChecker::lookupVar(std::string_view name) {
  for (auto env = environments.rbegin(); env != environments.rend(); env++) {
    if (env->contains(name)) {
      return env->at(name);
    }
  }

  if (functions.contains(name)) {
    return functions.at(name);
  }

  return std::nullopt;
}

std::optional<TAst::TypeID> TypeChecker::lookupType(std::string_view ident) {
  if (types.contains(ident)) {
    return types.at(ident);
  }

  return std::nullopt;
}

TAst::TypeID TypeChecker::internType(TAst::Type type) {
  for (size_t i = 0; i < typeArena.size(); i++) {
    if (type == typeArena[i]) {
      return TAst::TypeID{static_cast<uint32_t>(i)};
    }
  }

  typeArena.push_back(std::move(type));

  return TAst::TypeID{static_cast<uint32_t>(typeArena.size() - 1)};
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
