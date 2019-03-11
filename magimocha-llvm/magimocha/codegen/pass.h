#pragma once
#include "llvm/IR/Module.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/raw_ostream.h"
namespace tig::magimocha::codegen {
	bool writeObjectFileWithLegacyPass(llvm::Module* TheModule, llvm::TargetMachine* TheTargetMachine, llvm::raw_fd_ostream& dest);
}