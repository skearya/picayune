#pragma once

#include "tast.hpp"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Instructions.h>
#include <string_view>
#include <unordered_map>
#include <vector>

struct LLVMCodegen {
  llvm::LLVMContext context;
  llvm::Module module;
  llvm::IRBuilder<> builder;
  std::unordered_map<std::string_view, llvm::Function *> functions;
  std::unordered_map<std::string_view, llvm::AllocaInst *> values;

  LLVMCodegen();

  void codegen(const std::vector<TAst::Decl> &program);

  llvm::Value *codegenExpr(const TAst::Expr &node);

  llvm::Value *operator()(const TAst::Number &node);
  llvm::Value *operator()(const TAst::Boolean &node);
  llvm::Value *operator()(const TAst::Ident &node);
  llvm::Value *operator()(const TAst::Binary &node);
  llvm::Value *operator()(const TAst::Call &node);
  llvm::Value *operator()(const TAst::Assign &node);
  llvm::Value *operator()(const TAst::Grouping &node);

  void codegenStmt(const TAst::Stmt &node);

  void operator()(const TAst::Block &node);
  void operator()(const TAst::Let &node);
  void operator()(const TAst::If &node);
  void operator()(const TAst::Return &node);
  void operator()(const TAst::ExprStmt &node);

  void codegenBlock(const TAst::Block &node);

  llvm::Type *convertType(const TAst::Type &type);

  llvm::AllocaInst *createEntryBlockAlloca(llvm::Function *function,
                                           llvm::Type *type,
                                           llvm::StringRef name);
};
