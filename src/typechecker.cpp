#include "typechecker.hpp"
#include "ast.hpp"
#include "type.hpp"
#include "utils.hpp"
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <optional>
#include <print>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

TypeChecker::TypeChecker(Storage &s)
    : storage{s}, types{}, environments{{}}, currentFunctionReturnType{},
      voidId{storage.add(Type::TVoid{})},
      stringId{storage.add(Type::TString{})},
      charId{storage.add(Type::TChar{})}, intId{storage.add(Type::TInt{})},
      booleanId{storage.add(Type::TBoolean{})} {
  types.insert({"void", voidId});
  types.insert({"string", stringId});
  types.insert({"char", charId});
  types.insert({"int", intId});
  types.insert({"boolean", booleanId});
}

void TypeChecker::check(const std::vector<Ast::DeclId> &program) {
  for (const auto &declId : program) {
    const Ast::Decl &decl = storage.get(declId);

    if (const auto *structDecl = std::get_if<Ast::Struct>(&decl.node)) {
      Type::TypeID typeId = storage.add(Type::TStruct{structDecl->name, {}});

      types.insert({structDecl->name, typeId});
    }
  }

  for (const auto &declId : program) {
    const Ast::Decl &decl = storage.get(declId);

    if (const auto *structDecl = std::get_if<Ast::Struct>(&decl.node)) {
      std::vector<Type::Field> fields;

      for (const auto &field : structDecl->fields) {
        std::optional<Type::TypeID> typeId = lookupType(field.type);

        if (!typeId.has_value()) {
          throw std::runtime_error("Struct field type is not defined");
        }

        fields.push_back(Type::Field{field.name, typeId.value()});
      }

      auto &structType = std::get<Type::TStruct>(
          storage.getMut(lookupType(structDecl->name).value()));

      structType.fields = fields;
    }
  }

  for (const auto &declId : program) {
    const Ast::Decl &decl = storage.get(declId);

    if (const auto *function = std::get_if<Ast::Function>(&decl.node)) {
      std::vector<Type::Parameter> params;

      for (const auto &param : function->params) {
        std::optional<Type::TypeID> typeId = lookupType(param.type);

        if (!typeId.has_value()) {
          throw std::runtime_error("Parameter type is not defined");
        }

        params.push_back(Type::Parameter{param.name, typeId.value()});
      }

      std::optional<Type::TypeID> returnTypeId =
          lookupType(function->returnType);

      if (!returnTypeId.has_value()) {
        throw std::runtime_error("Return type is not defined");
      }

      Type::TypeID typeId =
          storage.add(Type::TFunction{std::move(params), returnTypeId.value()});

      environments.back().insert({function->name, typeId});
    }
  }

  for (const auto &declId : program) {
    const Ast::Decl &decl = storage.get(declId);

    if (const auto *function = std::get_if<Ast::Function>(&decl.node)) {
      const auto &functionType = std::get<Type::TFunction>(
          storage.get(lookupVar(function->name).value()));

      std::unordered_map<std::string_view, Type::TypeID> params;

      for (size_t i = 0; i < function->params.size(); i++) {
        params.insert(
            {function->params[i].name, functionType.parameters[i].type});
      }

      environments.push_back(std::move(params));
      currentFunctionReturnType = functionType.returnType;

      checkStmt(function->body);

      environments.pop_back();

      if (functionType.returnType != voidId && !doesReturn(function->body)) {
        throw std::runtime_error(
            "Function does not return value in all branches");
      }
    }
  }
}

Type::TypeID TypeChecker::checkExpr(Ast::ExprId exprId) {
  const Ast::Expr &expr = storage.get(exprId);

  return std::visit(*this, expr.node);
}

Type::TypeID TypeChecker::operator()(const Ast::String &) { return stringId; }

Type::TypeID TypeChecker::operator()(const Ast::Char &) { return charId; }

Type::TypeID TypeChecker::operator()(const Ast::Number &) { return intId; }

Type::TypeID TypeChecker::operator()(const Ast::Boolean &) { return booleanId; }

Type::TypeID TypeChecker::operator()(const Ast::StructInit &node) {
  std::optional<Type::TypeID> typeId = lookupType(node.name);

  if (!typeId.has_value()) {
    throw std::runtime_error("Struct does not exist");
  }

  const auto &type = storage.get(typeId.value());

  if (const auto *structType = std::get_if<Type::TStruct>(&type)) {
    if (structType->fields.size() != node.fields.size()) {
      throw std::runtime_error("Struct initialization error");
    }

    for (const auto &fieldInit : node.fields) {
      auto fieldType =
          std::ranges::find_if(structType->fields, [&](const auto &fieldType) {
            return fieldType.name == fieldInit.name;
          });

      if (fieldType == std::ranges::end(structType->fields)) {
        throw std::runtime_error("Struct field does not exist");
      }

      Type::TypeID fieldInitTypeId = checkExpr(fieldInit.value);

      if (fieldType->type != fieldInitTypeId) {
        throw std::runtime_error("Mismatched struct field type");
      }
    }

    return typeId.value();
  } else {
    throw std::runtime_error("Type is not a struct");
  }
}

Type::TypeID TypeChecker::operator()(const Ast::Get &node) {
  Type::TypeID exprTypeId = checkExpr(node.expr);
  const auto &exprType = storage.get(exprTypeId);

  if (const auto *structType = std::get_if<Type::TStruct>(&exprType)) {
    auto structField =
        std::ranges::find_if(structType->fields, [&](const auto &fieldType) {
          return fieldType.name == node.field;
        });

    if (structField == std::ranges::end(structType->fields)) {
      throw std::runtime_error(
          "Tried getting struct field that does not exist");
    }

    return structField->type;
  } else {
    throw std::runtime_error("Requested field from non-struct");
  }
}

Type::TypeID TypeChecker::operator()(const Ast::Ident &node) {
  std::optional<Type::TypeID> typeId = lookupVar(node.name);

  if (!typeId.has_value()) {
    throw std::runtime_error("Identifier is not defined");
  }

  return typeId.value();
}

Type::TypeID TypeChecker::operator()(const Ast::Binary &node) {
  Type::TypeID leftTypeId = checkExpr(node.left);
  Type::TypeID rightTypeId = checkExpr(node.right);

  if (leftTypeId != rightTypeId) {
    throw std::runtime_error("Expected binary operands to be the same type");
  }

  switch (node.op) {
  case Ast::Operator::Add:
  case Ast::Operator::Sub:
  case Ast::Operator::Mul:
  case Ast::Operator::Div:
    if (leftTypeId != intId) {
      throw std::runtime_error(
          "Expected arithmetic operator to be used with numbers");
    }

    return intId;
  case Ast::Operator::Eq:
  case Ast::Operator::NotEq:
    return booleanId;
  case Ast::Operator::Lt:
  case Ast::Operator::LtEq:
  case Ast::Operator::Gt:
  case Ast::Operator::GtEq:
    if (leftTypeId != intId) {
      throw std::runtime_error(
          "Expected comparison operator to be used with numbers");
    }

    return booleanId;
  case Ast::Operator::Or:
  case Ast::Operator::And:
    if (leftTypeId != booleanId) {
      throw std::runtime_error(
          "Expected logical operator to be used with booleans");
    }

    return booleanId;
  }
}

Type::TypeID TypeChecker::operator()(const Ast::Call &node) {
  Type::TypeID callee = checkExpr(node.function);
  const auto &calleeType = storage.get(callee);

  if (const auto *functionType = std::get_if<Type::TFunction>(&calleeType)) {
    if (node.arguments.size() != functionType->parameters.size()) {
      throw std::runtime_error("Function call arity mismatch");
    }

    for (size_t i = 0; i < functionType->parameters.size(); i++) {
      Type::TypeID arg = checkExpr(node.arguments.at(i));
      const auto &param = functionType->parameters.at(i);

      if (arg != param.type) {
        throw std::runtime_error("Function call argument type mismatch");
      }
    }

    return functionType->returnType;
  } else {
    throw std::runtime_error("Called non-function type");
  }
}

Type::TypeID TypeChecker::operator()(const Ast::Assign &node) {
  std::optional<Type::TypeID> target = lookupVar(node.variable);

  if (!target.has_value()) {
    throw std::runtime_error("Assignment target is not defined");
  }

  Type::TypeID value = checkExpr(node.value);

  if (target != value) {
    throw std::runtime_error("Assignment value doesn't match expected type");
  }

  return value;
}

Type::TypeID TypeChecker::operator()(const Ast::Grouping &node) {
  Type::TypeID inner = checkExpr(node.inner);

  return inner;
}

void TypeChecker::checkStmt(const Ast::StmtId stmtId) {
  const Ast::Stmt &stmt = storage.get(stmtId);

  return std::visit(*this, stmt.node);
}

void TypeChecker::operator()(const Ast::Block &node) {
  environments.push_back({});

  for (auto &stmt : node.statements) {
    checkStmt(stmt);
  }

  environments.pop_back();
}

void TypeChecker::operator()(const Ast::Let &node) {
  Type::TypeID init = checkExpr(node.initializer);

  environments.back().insert({node.name, init});
}

void TypeChecker::operator()(const Ast::If &node) {
  Type::TypeID cond = checkExpr(node.condition);

  if (cond != booleanId) {
    throw std::runtime_error(
        "Expected if statement's condition's type to be 'bool'");
  }

  checkStmt(node.thenStatement);

  if (node.elseStatement.has_value()) {
    checkStmt(node.elseStatement.value());
  }
}

void TypeChecker::operator()(const Ast::While &node) {
  Type::TypeID cond = checkExpr(node.condition);

  if (cond != booleanId) {
    throw std::runtime_error(
        "Expected while statement's condition's type to be 'bool'");
  }

  checkStmt(node.body);
}

void TypeChecker::operator()(const Ast::For &node) {
  checkStmt(node.initializer);

  Type::TypeID cond = checkExpr(node.condition);

  if (cond != booleanId) {
    throw std::runtime_error(
        "Expected for statement's condition's type to be 'bool'");
  }

  checkExpr(node.update);

  checkStmt(node.body);
}

void TypeChecker::operator()(const Ast::Return &node) {
  if (node.value.has_value()) {
    Type::TypeID value = checkExpr(node.value.value());

    if (value != currentFunctionReturnType) {
      throw std::runtime_error(
          "Expected return type to match function return type");
    }
  } else if (currentFunctionReturnType != voidId) {
    throw std::runtime_error("Expected return value to exist");
  }
}

void TypeChecker::operator()(const Ast::ExprStmt &node) {
  checkExpr(node.expression);
}

std::optional<Type::TypeID> TypeChecker::lookupVar(std::string_view name) {
  for (auto env = environments.rbegin(); env != environments.rend(); env++) {
    if (env->contains(name)) {
      return env->at(name);
    }
  }

  return std::nullopt;
}

std::optional<Type::TypeID> TypeChecker::lookupType(std::string_view ident) {
  if (types.contains(ident)) {
    return types.at(ident);
  }

  return std::nullopt;
}

bool TypeChecker::doesBlockReturn(const Ast::Block &node) {
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

bool TypeChecker::doesReturn(Ast::StmtId stmtId) {
  const Ast::Stmt &stmt = storage.get(stmtId);

  return std::visit(
      overloads{[&](const Ast::Block &node) { return doesBlockReturn(node); },
                [&](const Ast::Let &) { return false; },
                [&](const Ast::If &node) {
                  return node.elseStatement.has_value() &&
                         doesReturn(node.thenStatement) &&
                         doesReturn(node.elseStatement.value());
                },
                [&](const Ast::While &) { return false; },
                [&](const Ast::For &) { return false; },
                [&](const Ast::Return &) { return true; },
                [&](const Ast::ExprStmt &) { return false; }},
      stmt.node);
}
