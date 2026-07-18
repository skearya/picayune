#include "llvm.hpp"
#include "../ast.hpp"
#include "../tast.hpp"
#include "../utils.hpp"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APInt.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/TargetParser/SubtargetFeature.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <llvm/Support/Alignment.h>
#include <llvm/Support/CodeGen.h>
#include <print>
#include <ranges>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

LLVMCodegen::LLVMCodegen(Storage &ts)
    : ts{ts}, context{}, module{"Codegen", context}, builder{context},
      functions{}, values{} {}

void LLVMCodegen::codegen(const std::vector<TAst::Decl> &program) {
  for (const auto &decl : program) {
    if (const auto *node = std::get_if<TAst::Struct>(&decl)) {
      llvm::StructType::create(context, node->name);
    }
  }

  for (const auto &decl : program) {
    if (const auto *node = std::get_if<TAst::Struct>(&decl)) {
      auto structType = llvm::StructType::getTypeByName(context, node->name);

      std::vector<llvm::Type *> fields;

      for (const auto &field : node->fields) {
        fields.push_back(typeIDToLLVM(field.type));
      }

      structType->setBody(fields);
    }
  }

  for (const auto &decl : program) {
    if (const auto *node = std::get_if<TAst::Function>(&decl)) {
      std::vector<llvm::Type *> functionParams;

      for (const auto &param : node->params) {
        functionParams.push_back(typeIDToLLVM(param.type));
      }

      auto functionType = llvm::FunctionType::get(
          typeIDToLLVM(node->returnType), functionParams, false);

      auto function = llvm::Function::Create(
          functionType, llvm::Function::ExternalLinkage, node->name, module);

      for (size_t i = 0; auto &param : function->args()) {
        param.setName(node->params[i].name);
      }

      functions.insert({node->name, function});
    }
  }

  for (const auto &decl : program) {
    if (const auto *node = std::get_if<TAst::Function>(&decl)) {
      auto function = functions.at(node->name);
      auto block = llvm::BasicBlock::Create(context, "entry", function);

      builder.SetInsertPoint(block);

      for (auto &arg : function->args()) {
        auto alloca =
            createEntryBlockAlloca(function, arg.getType(), arg.getName());

        builder.CreateStore(&arg, alloca);
      }

      codegenBlock(node->body);

      llvm::EliminateUnreachableBlocks(*function);

      // If we have a `void` function that doesn't have an explicit
      // return, add it.
      if (node->returnType == ts.voidTypeID &&
          !function->back().getTerminator()) {
        builder.CreateRetVoid();
      }

      values.clear();

      if (llvm::verifyFunction(*function, &llvm::errs())) {
        std::println("WARNING: Function {} failed verification ^^^",
                     node->name);
        std::println();
      } else {
        std::println("Function {} verified with no errors.", node->name);
        std::println();
      }
    }
  }

  module.print(llvm::errs(), nullptr);
  std::println();

  if (llvm::verifyModule(module, &llvm::errs())) {
    std::println("WARNING: Module failed verification ^^^");
    std::println();
  } else {
    std::println("Module verified with no errors.");
    std::println();
  }

  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeAllAsmPrinters();

  auto targetTriple = llvm::sys::getDefaultTargetTriple();
  module.setTargetTriple(llvm::Triple{targetTriple});

  std::string error;
  auto target =
      llvm::TargetRegistry::lookupTarget(module.getTargetTriple(), error);

  if (!target) {
    llvm::errs() << error;
    return;
  }

  auto CPU = llvm::sys::getHostCPUName();

  llvm::SubtargetFeatures features;
  llvm::StringMap<bool> hostFeatures = llvm::sys::getHostCPUFeatures();

  for (const auto &F : hostFeatures) {
    features.AddFeature(F.first(), F.second);
  }

  auto relocModel = llvm::Reloc::PIC_;
  auto codeModel = llvm::CodeModel::Small;
  auto optLevel = llvm::CodeGenOptLevel::Default;

  llvm::TargetOptions opt;

  auto targetMachine = target->createTargetMachine(
      llvm::Triple{targetTriple}, CPU, features.getString(), opt, relocModel,
      codeModel, optLevel);

  module.setDataLayout(targetMachine->createDataLayout());

  auto filename = "output.o";
  std::error_code EC;
  llvm::raw_fd_ostream filestream{filename, EC, llvm::sys::fs::OF_None};

  if (EC) {
    llvm::errs() << "Could not open file: " << EC.message();
    return;
  }

  llvm::legacy::PassManager pass;

  if (targetMachine->addPassesToEmitFile(pass, filestream, nullptr,
                                         llvm::CodeGenFileType::ObjectFile)) {
    llvm::errs() << "targetMachine can't emit a file of this type";
    return;
  }

  pass.run(module);
  filestream.flush();

  llvm::outs() << "Wrote " << filename << "\n";
}

llvm::Value *LLVMCodegen::codegenExpr(const TAst::Expr &node) {
  return std::visit(*this, node);
}

llvm::Value *LLVMCodegen::operator()(const TAst::String &node) {
  return builder.CreateGlobalString(node.value);
}

llvm::Value *LLVMCodegen::operator()(const TAst::Char &node) {
  return llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), node.value,
                                true);
}

llvm::Value *LLVMCodegen::operator()(const TAst::Number &node) {
  return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), node.value,
                                true);
}

llvm::Value *LLVMCodegen::operator()(const TAst::Boolean &node) {
  return llvm::ConstantInt::get(llvm::Type::getInt1Ty(context), node.value);
}

llvm::Value *LLVMCodegen::operator()(const TAst::StructInit &node) {
  auto type = typeIDToLLVM(node.type);

  auto function = builder.GetInsertBlock()->getParent();
  auto alloca = createEntryBlockAlloca(function, type, node.name);

  for (const auto &field : node.fields) {
    const auto *structType = std::get_if<TAst::TStruct>(&ts.getType(node.type));

    if (structType == nullptr) {
      throw std::runtime_error(
          "TAst::StructInit does not produce struct type?");
    }

    auto structField =
        std::ranges::find_if(structType->fields, [&](const auto &structField) {
          return structField.name == field.name;
        });

    if (structField == std::ranges::end(structType->fields)) {
      throw std::runtime_error("Struct field does not exist?");
    }

    auto index = std::ranges::distance(std::ranges::begin(structType->fields),
                                       structField);

    auto location = builder.CreateStructGEP(type, alloca, index, field.name);
    auto value = codegenExpr(*field.value);

    builder.CreateStore(value, location);
  }

  return alloca;
}

llvm::Value *LLVMCodegen::operator()(const TAst::Get &node) {
  auto structTypeID = getTypeID(*node.expr);
  auto structType = std::get_if<TAst::TStruct>(&ts.getType(structTypeID));

  if (structType == nullptr) {
    throw std::runtime_error("TAst::Get not operating on struct type?");
  }

  auto structField =
      std::ranges::find_if(structType->fields, [&](const auto &structField) {
        return structField.name == node.field;
      });

  if (structField == std::ranges::end(structType->fields)) {
    throw std::runtime_error("Struct field does not exist?");
  }

  auto index = std::ranges::distance(std::ranges::begin(structType->fields),
                                     structField);

  auto expr = codegenExpr(*node.expr);

  auto location = builder.CreateStructGEP(typeIDToLLVM(structTypeID), expr,
                                          index, node.field);

  return builder.CreateLoad(typeIDToLLVM(structField->type), location,
                            structField->name);
}

llvm::Value *LLVMCodegen::operator()(const TAst::Ident &node) {
  auto alloca = values.at(node.name);
  auto allocaType = alloca->getAllocatedType();

  return builder.CreateLoad(allocaType, alloca, node.name);
}

llvm::Value *LLVMCodegen::operator()(const TAst::Binary &node) {
  auto left = codegenExpr(*node.left);

  if (node.op == Ast::Operator::Or || node.op == Ast::Operator::And) {
    auto function = builder.GetInsertBlock()->getParent();

    auto startBlock = llvm::BasicBlock::Create(context, "start", function);
    auto elseBlock = llvm::BasicBlock::Create(context, "else");
    auto mergeBlock = llvm::BasicBlock::Create(context, "merge");

    builder.CreateBr(startBlock);

    builder.SetInsertPoint(startBlock);

    if (node.op == Ast::Operator::Or) {
      // If left = true, OR is true, otherwise have to evaluate right.
      builder.CreateCondBr(left, mergeBlock, elseBlock);
    } else {
      // If left = false, AND is true, otherwise have to evaluate right.
      builder.CreateCondBr(left, elseBlock, mergeBlock);
    }

    function->insert(function->end(), elseBlock);
    builder.SetInsertPoint(elseBlock);
    auto right = codegenExpr(*node.right);
    builder.CreateBr(mergeBlock);

    function->insert(function->end(), mergeBlock);
    builder.SetInsertPoint(mergeBlock);
    auto phi =
        builder.CreatePHI(llvm::Type::getInt1Ty(context), 2,
                          node.op == Ast::Operator::Or ? "ortmp" : "andtmp");

    if (node.op == Ast::Operator::Or) {
      // If we came from "start" (early exited), OR is true, otherwise the
      // value of right.
      phi->addIncoming(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(context), true),
          startBlock);
    } else {
      // If we came from "start" (early exited), AND is false, otherwise
      // the value of right.
      phi->addIncoming(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(context), false),
          startBlock);
    }

    phi->addIncoming(right, elseBlock);

    return phi;
  }

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
  case Ast::Operator::Or:
  case Ast::Operator::And:
    throw std::runtime_error("Already handled above?");
  }
}

llvm::Value *LLVMCodegen::operator()(const TAst::Call &node) {
  std::vector<llvm::Value *> args;

  for (const auto &arg : node.arguments) {
    args.push_back(codegenExpr(arg));
  }

  if (const auto *functionName = std::get_if<TAst::Ident>(&*node.function)) {
    return builder.CreateCall(functions.at(functionName->name), args,
                              "calltmp");
  } else {
    throw std::runtime_error("Expected ident callee");
  }
}

llvm::Value *LLVMCodegen::operator()(const TAst::Assign &node) {
  auto variable = values.at(node.variable);
  auto value = codegenExpr(*node.value);

  builder.CreateStore(value, variable);

  return value;
}

llvm::Value *LLVMCodegen::operator()(const TAst::Grouping &node) {
  return codegenExpr(*node.inner);
}

void LLVMCodegen::codegenStmt(const TAst::Stmt &node) {
  std::visit(*this, node);
}

void LLVMCodegen::operator()(const TAst::Block &node) { codegenBlock(node); }

void LLVMCodegen::operator()(const TAst::Let &node) {
  auto type = typeIDToLLVM(getTypeID(node.initializer));

  auto function = builder.GetInsertBlock()->getParent();
  auto alloca = createEntryBlockAlloca(function, type, node.name);

  auto value = codegenExpr(node.initializer);

  builder.CreateStore(value, alloca);
}

void LLVMCodegen::operator()(const TAst::If &node) {
  auto cond = codegenExpr(node.condition);

  auto function = builder.GetInsertBlock()->getParent();

  if (node.elseStatement.has_value()) {
    auto thenBlock = llvm::BasicBlock::Create(context, "then", function);
    auto elseBlock = llvm::BasicBlock::Create(context, "else");
    auto mergeBlock = llvm::BasicBlock::Create(context, "merge");

    builder.CreateCondBr(cond, thenBlock, elseBlock);

    builder.SetInsertPoint(thenBlock);
    codegenStmt(*node.thenStatement);
    createBreakIfUnterminated(mergeBlock);

    function->insert(function->end(), elseBlock);
    builder.SetInsertPoint(elseBlock);
    codegenStmt(*node.elseStatement.value());
    createBreakIfUnterminated(mergeBlock);

    function->insert(function->end(), mergeBlock);
    builder.SetInsertPoint(mergeBlock);
  } else {
    auto thenBlock = llvm::BasicBlock::Create(context, "then", function);
    auto mergeBlock = llvm::BasicBlock::Create(context, "merge");

    builder.CreateCondBr(cond, thenBlock, mergeBlock);

    builder.SetInsertPoint(thenBlock);
    codegenStmt(*node.thenStatement);
    createBreakIfUnterminated(mergeBlock);

    function->insert(function->end(), mergeBlock);
    builder.SetInsertPoint(mergeBlock);
  }
}

void LLVMCodegen::operator()(const TAst::While &node) {
  auto function = builder.GetInsertBlock()->getParent();

  auto startBlock = llvm::BasicBlock::Create(context, "start", function);
  auto bodyBlock = llvm::BasicBlock::Create(context, "body");
  auto mergeBlock = llvm::BasicBlock::Create(context, "merge");

  builder.CreateBr(startBlock);

  builder.SetInsertPoint(startBlock);
  auto cond = codegenExpr(node.condition);
  builder.CreateCondBr(cond, bodyBlock, mergeBlock);

  function->insert(function->end(), bodyBlock);
  builder.SetInsertPoint(bodyBlock);
  codegenStmt(*node.body);
  createBreakIfUnterminated(startBlock);

  function->insert(function->end(), mergeBlock);
  builder.SetInsertPoint(mergeBlock);
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

    // Code after a terminator is always useless.
    if (builder.GetInsertBlock()->getTerminator()) {
      break;
    }
  }
}

llvm::AllocaInst *LLVMCodegen::createEntryBlockAlloca(llvm::Function *function,
                                                      llvm::Type *type,
                                                      llvm::StringRef name) {
  auto builder = llvm::IRBuilder<>{&function->getEntryBlock(),
                                   function->getEntryBlock().begin()};

  auto alloc = builder.CreateAlloca(type, nullptr, name);
  values.insert({name, alloc});

  return alloc;
};

bool LLVMCodegen::createBreakIfUnterminated(llvm::BasicBlock *dest) {
  if (builder.GetInsertBlock()->getTerminator()) {
    return false;
  } else {
    builder.CreateBr(dest);
    return true;
  }
}

llvm::Type *LLVMCodegen::typeIDToLLVM(TAst::TypeID typeID) {
  return std::visit(
      overloads{
          [&](const TAst::TString &) -> llvm::Type * {
            return llvm::PointerType::getUnqual(context);
          },
          [&](const TAst::TChar &) -> llvm::Type * {
            return llvm::Type::getInt8Ty(context);
          },
          [&](const TAst::TInt &) -> llvm::Type * {
            return llvm::Type::getInt32Ty(context);
          },
          [&](const TAst::TBoolean &) -> llvm::Type * {
            return llvm::Type::getInt1Ty(context);
          },
          [&](const TAst::TVoid &) -> llvm::Type * {
            return llvm::Type::getVoidTy(context);
          },
          [&](const TAst::TStruct &node) -> llvm::Type * {
            return llvm::StructType::getTypeByName(context, node.name);
          },
          [&](const TAst::TFunction &node) -> llvm::Type * {
            std::vector<llvm::Type *> parameters;

            for (const auto &parameter : node.parameters) {
              parameters.push_back(typeIDToLLVM(parameter.type));
            }

            auto returnType = typeIDToLLVM(node.returnType);

            return llvm::FunctionType::get(returnType, parameters, false);
          },
      },
      ts.getType(typeID));
}
