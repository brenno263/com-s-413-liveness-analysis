#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>
#include <sts/stat.h>

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		//Need an argument, output something relevant here.
		return 1;
	}
	//Insert checks that argv[1] is LLVM IR code here.

	/*Begin thing you can perhaps pull into a different function for loading a file*/
	string file = argv[1]; 
	llvm::StringRef modulePath(file);
	llvm::SMDiagnostic error;
	llvm::LLVMContext modContext;
	std::unique_ptr<llvm::Module> modulePointer = llvm::parseIRFile(modulePath, error, modContext);
	if(!modulePointer)
	{
		string errorMessage;
		llvm::raw_string_ostream output(errorMessage);
		std::cerr << "Error in parsting the " << file << "\n";
		return 3; /*if turned into a function, make this something like "return false"*/
	}
	if (llvm::verifyModule(*modulePointer, &llvm::errs()) != 0) {
		std::cerr << "Error in verifying the Module : " << file << "\n";
		return 3;/*if turned into a function, make this something like "return false"*/
	}
	/*Return true if split into its own function*/
	/*End section on loading a file*/
	return 0;
}
