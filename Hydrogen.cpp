/**
 * @author Ashwin K J
 * @file
 */
#include "Diff_Mapping.hpp"
#include "Get_Input.hpp"
#include "Graph.hpp"
#include "Graph_Function.hpp"
#include "Graph_Instruction.hpp"
#include "Graph_Line.hpp"
#include "MVICFG.hpp"
#include "Module.hpp"
#include <chrono>
#include "llvm/IR/CFG.h"

using namespace hydrogen_framework;

bool stringListContains(const std::list<std::string> &list, const std::string &str) {
  for (auto s : list) {
    if (s.compare(str) == 0)
      return true;
  }
  return false;
}

unsigned int getLine(llvm::Instruction &inst) {
  const llvm::DebugLoc &dbgInfo = inst.getDebugLoc();
  auto addr = std::addressof(dbgInfo);
  if ((void *)dbgInfo == NULL) {
    return 0;
  }

  return dbgInfo->getLine();
}

/**
 * Appends everything in a that is not also in b to out.
 */
void setDiff(const std::list<std::string> &a, const std::list<std::string> &b, std::list<std::string> &out) {
  for (const std::string &ai : a) {
    if (!stringListContains(b, ai)) {
      out.push_back(ai);
    }
  }
}

/**
 * Appends everything that is in either a or b to out, with no duplicates.
 * the contents of a are appended first.
 */
void setUnion(const std::list<std::string> &a, const std::list<std::string> &b, std::list<std::string> &out) {
  for (auto ai : a) {
    out.push_back(ai);
  }
  setDiff(b, a, out);
}

void analyzeBlock(llvm::BasicBlock &block, std::list<std::string> &gen, std::list<std::string> &kill) {
  // auto instList = block.getInstList();
  for (auto &inst : block) {

    unsigned int opcode = inst.getOpcode();

    if (opcode == llvm::Instruction::Call)
      continue;

    unsigned int numOperands = inst.getNumOperands();
    for (int opIndex = numOperands; opIndex > 0;) {
      opIndex--;
      llvm::Value *op = inst.getOperand(opIndex);
      if (op == NULL || !op->hasName()) {
        continue;
      }

      //std::cout << "line " << getLine(inst) << " op: " << inst.getOpcodeName() << " index: " << opIndex << std::endl;

      std::string varName = op->getName().str();
      // ignore retval, it's used as a function's return value.
      if (varName.compare("retval") == 0) {
        continue;
      }
      
      // Consider these operations to be added to GEN. They use a variable
      if (opcode == llvm::Instruction::Load) {
        if (!stringListContains(gen, varName) && !stringListContains(kill, varName)) {
          //std::cout << "added to GEN: " << varName << std::endl;
          gen.push_back(varName);
        }
      }

      // Consider these operations to be added to KILL. They set a variable (second operand of store)
      if (opcode == llvm::Instruction::Store && opIndex == 1) {
        if (!stringListContains(kill, varName)) {
          //std::cout << "added to KILL: " << varName << std::endl;
          kill.push_back(varName);
        }
      }
    }
  }

  //NOTE: The code below causes a bug, since some things might be used before being killed WITHIN a given Basic Block.
  //This does check for some errors, but I feel being able to say x = x+1 requires x be definied is more important.

  // Make sure anything in gen which is also killed in this block gets cropped out.
  //for (auto k : kill) {
  //  gen.remove(k);
  //}
}

void livenessAnalysis(Module *mod) {

  std::unique_ptr<llvm::Module> &modPtr = mod->getPtr();

  for (llvm::Function &func : (*modPtr)) {
    llvm::Function::BasicBlockListType &blocks = func.getBasicBlockList();

    int numInst = 0;

    // These represent our relevant string sets for each block.
    // The set for each block lives at its index.
    std::map<llvm::BasicBlock*, std::list<std::string>> genMap;
    std::map<llvm::BasicBlock*, std::list<std::string>> killMap;
    std::map<llvm::BasicBlock*, std::list<std::string>> inMap;
    std::map<llvm::BasicBlock*, std::list<std::string>> outMap;

    //for(int i = 0; i < blocks.size(); i++) {
    //	std::cout << "set length for block " << i << " : " << genMap[i].size();
    //}

    //int blockNum = 0;
    for (llvm::BasicBlock &block : blocks) {
      //std::cout << "block num: " << blockNum << std::endl;
      std::list<std::string> gen;
      std::list<std::string> kill;
      analyzeBlock(block, gen, kill);

      genMap[&block] = gen;
      killMap[&block] = kill;
      inMap[&block] = genMap[&block];
      //blockNum++;
    }

    //std::cout << "Entering Loop\n";
    bool changed = true;
    while (changed) {
      //std::cout << "Loop Start\n";
      changed = false;
      for (llvm::BasicBlock &block : blocks)
      {
        //std::cout << "Inner Loop Start\n";
        std::list<std::string> temp = inMap.find(&block) == inMap.end() ? std::list<std::string>() : inMap.find(&block)->second;
        std::list<std::string> in = inMap.find(&block) == inMap.end() ? std::list<std::string>() : inMap.find(&block)->second;
        std::list<std::string> out = outMap.find(&block) == outMap.end() ? std::list<std::string>() : outMap.find(&block)->second;
        for (llvm::BasicBlock *successor : successors(&block))
        {
          //std::cout << "Successors looking\n";
          std::list<std::string> needed = inMap.find(successor) == inMap.end() ? std::list<std::string>() : inMap.find(successor)->second;
          for (std::string e : needed)
          {
            if (!stringListContains(out, e))
            {
              out.push_back(e);
            }
          }
          outMap[&block] = out;
        }
        outMap[&block] = out; //For the end block, so it shows up.
        for (std::string check : out)
        {
          if (!stringListContains(in, check) && (killMap.find(&block) == killMap.end()|| !stringListContains(killMap.find(&block)->second, check)))
          {
            in.push_back(check);
          }
        }
        inMap[&block] = in;
        for (std::string element : in)
        {
          if (!stringListContains(temp, element))
          {
            changed = true;
          }
        }
      }
    }

    // Now we have a bunch of sets that tells us when we can stop caring about a variable's value.
    // Analyze the sets for variables that do not appear in any IN values to find useless variables.
    std::list<std::string> usedVariables;
    std::list<std::string> setVariables;
    std::cout << "Generated results for the function " << func.getName().str() << std::endl;
    std::cout << "Values for GEN\n";
    for (auto iterator = genMap.begin(); iterator != genMap.end(); ++iterator)
    {
      std::cout << "BasicBlock Name: " << iterator->first->getName().str() << "\nWith contents: {";
      std::string contents = "";
      for (auto listElement : iterator->second)
      {
        contents += listElement + ",";
        if (!stringListContains(usedVariables, listElement))
        {
          usedVariables.push_back(listElement);
        }
      }
      if (contents.size() > 0)
      {
        contents.pop_back();
      }
      std::cout << contents << "}\n";
    }
    std::cout << "Values for KILL\n";
    for (auto iterator = killMap.begin(); iterator != killMap.end(); ++iterator)
    {
      std::cout << "BasicBlock Name: " << iterator->first->getName().str() << "\nWith contents: {";
      std::string contents = "";
      for (auto listElement : iterator->second)
      {
        contents += listElement + ",";
        if (!stringListContains(setVariables, listElement))
        {
          setVariables.push_back(listElement);
        }
      }
      if (contents.size() > 0)
      {
        contents.pop_back();
      }
      std::cout << contents << "}\n";
    }
    std::cout << "Values for IN\n";
    for (auto iterator = inMap.begin(); iterator != inMap.end(); ++iterator)
    {
      std::cout << "BasicBlock Name: " << iterator->first->getName().str() << "\nWith contents: {";
      std::string contents = "";
      for (auto listElement : iterator->second)
      {
        contents += listElement + ",";
      }
      if (contents.size() > 0)
      {
        contents.pop_back();
      }
      std::cout << contents << "}\n";
    }
    std::cout << "Values for OUT\n";
    for (auto iterator = outMap.begin(); iterator != outMap.end(); ++iterator)
    {
      std::cout << "BasicBlock Name: " << iterator->first->getName().str() << "\nWith contents: {";
      std::string contents = "";
      for (auto listElement : iterator->second)
      {
        contents += listElement + ",";
      }
      if (contents.size() > 0)
      {
        contents.pop_back();
      }
      
      std::cout << contents << "}\n";
    } 

    std::string contents = "";
    for (auto variable : usedVariables)
    {
      if (!stringListContains(setVariables, variable))
      {
        contents += variable + ",";
      }
    }
    if (contents.size() > 0)
    {
      contents.pop_back();
      std::cout << "Variables used without being set: {" << contents << "}" << std::endl;
    }
    contents = "";
    for (auto variable : setVariables)
    {
      if (!stringListContains(usedVariables, variable))
      {
        contents += variable + ",";
      }
    }
    if (contents.size() > 0)
    {
      contents.pop_back();
      std::cout << "Variables set without being (instantly) used: {" << contents << "}" << std::endl;
    }
  }
}

/**
 * Main function
 */
int main(int argc, char *argv[]) {
  /* Getting the input */
  if (argc < 2) {
    std::cerr << "Insufficient arguments\n"
              << "The correct format is as follows:\n"
              << "<Path-to-Bytecode> :: "
              << "<Path-to-file1-for-Bytecode> .. <Path-to-fileN-for-Bytecode>"
              << "\n"
              << "Note that '::' is the demarcation\n";
    return 1;
  } // End check for min argument
  Hydrogen framework;
  if (!framework.validateInputs(argc, argv)) {
    return 2;
  } // End check for valid Input
  if (!framework.processInputs(argc, argv)) {
    return 3;
  } // End check for processing Inputs
  // mod is the bytecode we're making the ICFG for.
  Module *mod = framework.getModules().front();
  
  /* Create CFG */
  unsigned graphVersion = 1;
  Graph *CFG = buildICFG(mod, graphVersion);
  /* Start timer */
  auto analysisStart = std::chrono::high_resolution_clock::now();
  livenessAnalysis(mod);
  /* Stop timer */
  auto analysisStop = std::chrono::high_resolution_clock::now();
  auto analysisTime = std::chrono::duration_cast<std::chrono::milliseconds>(analysisStop - analysisStart);

  // Here we do a little print out of net path changes.
  // int max_version = MVICFG->getGraphVersion();
  // for(int i = 2; i <= max_version; i++) {
  // int paths_before = num_paths(MVICFG, i - 1);
  // int paths_after = num_paths(MVICFG, i);
  // int paths_diff = paths_after - paths_before;
  // std::cout << "From version " << i - 1 << " to version " << i;
  // std::cout << ", " << (paths_diff < 0 ? paths_diff * -1 : paths_diff);
  // std::cout << " paths were " << (paths_diff < 0 ? "removed." : "added.") << std::endl;
  //}

  CFG->printGraph("CFG");
  std::cout << "Finished Analyzing CFG in " << analysisTime.count() << "ms\n";
  /* Write output to file */
  std::ofstream rFile("Result.txt", std::ios::trunc);
  if (!rFile.is_open()) {
    std::cerr << "Unable to open file for printing the output\n";
    return 5;
  } // End check for Result file
  rFile << "Input Args:\n";
  for (auto i = 0; i < argc; ++i) {
    rFile << argv[i] << "  ";
  } // End loop for writing arguments
  rFile << "\n";
  rFile << "Finished Analyzing CFG in " << analysisTime.count() << "ms\n";
  rFile.close();
  return 0;
} // End main
