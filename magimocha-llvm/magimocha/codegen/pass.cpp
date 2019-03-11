#include "pass.h"
#include "llvm/IR/LegacyPassManager.h"
using namespace llvm;
namespace tig::magimocha::codegen {
	bool writeObjectFileWithLegacyPass(Module* TheModule,TargetMachine* TheTargetMachine,raw_fd_ostream& dest) {

		legacy::PassManager pass;
		auto FileType = TargetMachine::CGFT_ObjectFile;

		if (TheTargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
			errs() << "TheTargetMachine can't emit a file of this type";
			return true;
		}

		pass.run(*TheModule);
		dest.flush();
		return false;
	}
}
