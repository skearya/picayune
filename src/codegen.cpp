#include "codegen.hpp"
#include "ast.hpp"
#include "tast.hpp"
#include "utils.hpp"
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
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <cassert>
#include <cstddef>
#include <print>
#include <stdexcept>
#include <string_view>
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

                 functions.insert({node.name, function});
               }},
               decl);
  }

  for (const auto &decl : program) {
    std::visit(
        overloads{[this](const TAst::Function &node) {
          auto function = functions.at(node.name);

          auto block = llvm::BasicBlock::Create(context, "entry", function);

          builder.SetInsertPoint(block);

          values.clear();

          for (auto &arg : function->args()) {
            auto alloca =
                createEntryBlockAlloca(function, arg.getType(), arg.getName());

            builder.CreateStore(&arg, alloca);
          }

          codegenBlock(node.body);

          llvm::EliminateUnreachableBlocks(*function);

          // If we have a `void` function that doesn't have an explicit return,
          // add it.
          if (node.type.index() == TAst::Type{TAst::TVoid{}}.index() &&
              !function->back().getTerminator()) {
            builder.CreateRetVoid();
          }

          if (llvm::verifyFunction(*function, &llvm::errs())) {
            std::println("WARNING: Function {} failed verification ^^^",
                         node.name);
          } else {
            std::println("Function {} verified with no errors.", node.name);
          }

          std::println();

          function->print(llvm::errs());
          std::println();
        }},
        decl);
  }

  if (llvm::verifyModule(module, &llvm::errs())) {
    std::println("WARNING: Module failed verification ^^^");
  } else {
    std::println("Module verified with no errors.");
  }

  std::println();

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

  auto CPU = "generic";
  auto features = "";

  llvm::TargetOptions opt;
  auto targetMachine = target->createTargetMachine(
      llvm::Triple{targetTriple}, CPU, features, opt, llvm::Reloc::PIC_);

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

llvm::Value *LLVMCodegen::operator()(const TAst::Number &node) {
  return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), node.value,
                                true);
}

llvm::Value *LLVMCodegen::operator()(const TAst::Boolean &node) {
  return llvm::ConstantInt::get(llvm::Type::getInt1Ty(context), node.value);
}

llvm::Value *LLVMCodegen::operator()(const TAst::Ident &node) {
  auto alloca = values.at(node.name);
  auto allocatype = alloca->getAllocatedType();

  return builder.CreateLoad(allocatype, alloca, node.name);
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
      // If we came from "start" (early exited), OR is true, otherwise the value
      // of right.
      phi->addIncoming(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(context), true),
          startBlock);
      phi->addIncoming(right, elseBlock);
    } else {
      // If we came from "start" (early exited), AND is false, otherwise the
      // value of right.
      phi->addIncoming(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(context), false),
          startBlock);
      phi->addIncoming(right, elseBlock);
    }

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

  return builder.CreateCall(functions.at(node.function), args, "calltmp");
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
  auto type = convertType(TAst::getType(node.initializer));

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