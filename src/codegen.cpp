#include "codegen.h"
#include "ast.h"
#include "tast.h"
#include "utils.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APInt.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"
#include <cassert>
#include <cstddef>
#include <unordered_map>
#include <variant>
#include <vector>

LLVMCodegen::LLVMCodegen()
    : context{}, module{"Codegen", context}, builder{context}, functions{},
      values{} {}

void LLVMCodegen::codegen(const std::vector<TAst::Decl> &program) {
  for (const auto &decl : program) {
    std::visit(overloads{[this](const TAst::Function &node) {
                 std::vector<llvm::Type *> functionparams;

                 for (const auto &param : node.params) {
                   functionparams.push_back(convertType(param.type));
                 }

                 auto functiontype = llvm::FunctionType::get(
                     convertType(node.type), functionparams, false);

                 auto function = llvm::Function::Create(
                     functiontype, llvm::Function::ExternalLinkage, node.name,
                     module);

                 for (size_t i = 0; auto &param : function->args()) {
                   param.setName(node.params[i].name);
                 }

                 functions[node.name] = function;
               }},
               decl);
  }

  for (const auto &decl : program) {
    std::visit(overloads{[this](const TAst::Function &node) {
                 auto function = functions[node.name];

                 auto block =
                     llvm::BasicBlock::Create(context, "entry", function);

                 builder.SetInsertPoint(block);

                 values.clear();

                 for (auto &arg : function->args()) {
                   auto alloca = createEntryBlockAlloca(function, arg.getType(),
                                                        arg.getName());

                   builder.CreateStore(&arg, alloca);

                   values[arg.getName()] = alloca;
                 }

                 codegenBlock(node.body);

                 llvm::verifyFunction(*function);

                 function->print(llvm::errs());
               }},
               decl);
  }
}

llvm::Value *LLVMCodegen::codegenExpr(const TAst::Expr &node) {
  return std::visit(*this, node);
}

llvm::Value *LLVMCodegen::operator()(const TAst::Number &node) {
  return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), node.value,
                                true);
}

llvm::Value *LLVMCodegen::operator()(const TAst::Boolean &node) {
  return llvm::ConstantInt::get(llvm::Type::getInt1Ty(context), node.value);
}

llvm::Value *LLVMCodegen::operator()(const TAst::Ident &node) {
  auto alloca = values[node.name];

  return builder.CreateLoad(alloca->getAllocatedType(), alloca, node.name);
}

llvm::Value *LLVMCodegen::operator()(const TAst::Binary &node) {
  auto left = codegenExpr(*node.left);
  auto right = codegenExpr(*node.right);

  switch (node.op) {
  case Ast::Operator::Add:
    return builder.CreateAdd(left, right, "addtmp");
  case Ast::Operator::Sub:
    return builder.CreateSub(left, right, "subtmp");
  case Ast::Operator::Mul:
    return builder.CreateMul(left, right, "multmp");
  case Ast::Operator::Div:
    return builder.CreateSDiv(left, right, "divtmp");
  case Ast::Operator::Eq:
    return builder.CreateICmpEQ(left, right, "eqtmp");
  case Ast::Operator::NotEq:
    return builder.CreateICmpNE(left, right, "noteqtmp");
  case Ast::Operator::Lt:
    return builder.CreateICmpSLT(left, right, "lttmp");
  case Ast::Operator::LtEq:
    return builder.CreateICmpSLE(left, right, "ltEqtmp");
  case Ast::Operator::Gt:
    return builder.CreateICmpSGT(left, right, "gttmp");
  case Ast::Operator::GtEq:
    return builder.CreateICmpSGE(left, right, "gteqtmp");
  }
}

llvm::Value *LLVMCodegen::operator()(const TAst::Call &node) {
  std::vector<llvm::Value *> args;

  for (const auto &arg : node.arguments) {
    args.push_back(codegenExpr(arg));
  }

  return builder.CreateCall(functions[node.function], args, "calltmp");
}

llvm::Value *LLVMCodegen::operator()(const TAst::Grouping &node) {
  return codegenExpr(*node.inner);
}

void LLVMCodegen::codegenStmt(const TAst::Stmt &node) {
  std::visit(*this, node);
}

void LLVMCodegen::operator()(const TAst::Block &node) { codegenBlock(node); }

void LLVMCodegen::operator()(const TAst::Let &node) {
  auto function = builder.GetInsertBlock()->getParent();
  auto type = convertType(TAst::getType(node.initializer));

  auto alloca = createEntryBlockAlloca(function, type, node.name);

  builder.CreateStore(codegenExpr(node.initializer), alloca);
}

void LLVMCodegen::operator()(const TAst::If &node) {
  auto cond = codegenExpr(node.cond);

  auto function = builder.GetInsertBlock()->getParent();

  if (node.elseStatement.has_value()) {
    auto thenBlock = llvm::BasicBlock::Create(context, "then", function);
    auto elseBlock = llvm::BasicBlock::Create(context, "else", function);
    auto mergeBlock = llvm::BasicBlock::Create(context, "merge", function);

    builder.CreateCondBr(cond, thenBlock, elseBlock);

    builder.SetInsertPoint(thenBlock);
    codegenStmt(*node.thenStatement);
    builder.CreateBr(mergeBlock);

    builder.SetInsertPoint(elseBlock);
    codegenStmt(*node.elseStatement.value());
    builder.CreateBr(mergeBlock);

    builder.SetInsertPoint(mergeBlock);
  } else {
    auto thenBlock = llvm::BasicBlock::Create(context, "then", function);
    auto mergeBlock = llvm::BasicBlock::Create(context, "merge", function);

    builder.CreateCondBr(cond, thenBlock, mergeBlock);

    builder.SetInsertPoint(thenBlock);
    codegenStmt(*node.thenStatement);
    builder.CreateBr(mergeBlock);

    builder.SetInsertPoint(mergeBlock);
  }
}

void LLVMCodegen::operator()(const TAst::Return &node) {
  if (node.value.has_value()) {
    builder.CreateRet(codegenExpr(*node.value));
  } else {
    builder.CreateRetVoid();
  }
}

void LLVMCodegen::operator()(const TAst::ExprStmt &node) {
  codegenExpr(node.expression);
}

void LLVMCodegen::codegenBlock(const TAst::Block &node) {
  for (const auto &stmt : node.statements) {
    codegenStmt(stmt);
  }
}

llvm::Type *LLVMCodegen::convertType(const TAst::Type &type) {
  return std::visit(overloads{
                        [this](const TAst::TInt &) -> llvm::Type * {
                          return llvm::Type::getInt32Ty(context);
                        },
                        [this](const TAst::TBoolean &) -> llvm::Type * {
                          return llvm::Type::getInt1Ty(context);
                        },
                        [this](const TAst::TVoid &) -> llvm::Type * {
                          return llvm::Type::getVoidTy(context);
                        },
                    },
                    type);
}

llvm::AllocaInst *LLVMCodegen::createEntryBlockAlloca(llvm::Function *function,
                                                      llvm::Type *type,
                                                      llvm::StringRef name) {
  auto builder = llvm::IRBuilder<>{&function->getEntryBlock(),
                                   function->getEntryBlock().begin()};

  return builder.CreateAlloca(type, nullptr, name);
};