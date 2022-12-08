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
#include "llvm/IR/CFG.h"
#include <chrono>

using namespace hydrogen_framework;

bool stringListContains(const std::list<std::string> &list, const std::string &str) {
  for (auto s : list) {
    if (s.compare(str) == 0)
      return true;
  }
  return false;
}

bool stringListsEqual(const std::list<std::string> &a, const std::list<std::string> &b) {
  if (a.size() != b.size()) {
    return false;
  }
  auto ait = a.begin();
  auto bit = b.begin();
  while (ait != a.end()) {
    if ((*ait).compare(*bit) != 0) {
      return false;
    }
    ++ait;
    ++bit;
  }
  return true;
}

unsigned int getLine(llvm::Instruction &inst) {
  const llvm::DebugLoc &dbgInfo = inst.getDebugLoc();
  auto addr = std::addressof(dbgInfo);
  if ((void *)dbgInfo == NULL) {
    return 0;
  }

  return dbgInfo->getLine();
}

std::string getBlockName(llvm::BasicBlock *b) {
  if (b->hasName()) {
    return b->getName().str();
  }
  return std::string("unnamed");
}

/**
 * Appends everything in a that is not also in b to out.
 */
void appendSetDiff(std::list<std::string> &a, std::list<std::string> &b, std::list<std::string> &out) {
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
void appendSetUnion(std::list<std::string> &a, std::list<std::string> &b, std::list<std::string> &out) {
  appendSetDiff(a, out, out);
  appendSetDiff(b, out, out);
}

/**
 * Union the set onto our first operand.
 */
void foldSetUnion(std::list<std::string> &fold, std::list<std::string> &a) { appendSetDiff(a, fold, fold); }

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

      // std::cout << "line " << getLine(inst) << " op: " << inst.getOpcodeName() << " index: " << opIndex << std::endl;

      std::string varName = op->getName().str();
      // ignore retval, it's used as a function's return value.
      if (varName.compare("retval") == 0) {
        continue;
      }

      // Consider these operations to be added to GEN. They use a variable
      if (opcode == llvm::Instruction::Load) {
        if (!stringListContains(gen, varName)) {
          // std::cout << "added to GEN: " << varName << std::endl;
          gen.push_back(varName);
        }
      }

      // Consider these operations to be added to KILL. They set a variable (second operand of store)
      if (opcode == llvm::Instruction::Store && opIndex == 1) {
        if (!stringListContains(kill, varName)) {
          // std::cout << "added to KILL: " << varName << std::endl;
          kill.push_back(varName);
        }
      }
    }
  }

  // NOTE: The code below causes a bug, since some things might be used before being killed WITHIN a given Basic Block.
  // This does check for some errors, but I feel being able to say x = x+1 requires x be definied is more important.

  // Make sure anything in gen which is also killed in this block gets cropped out.
  // for (auto k : kill) {
  //  gen.remove(k);
  //}
}

std::string concatStringList(std::list<std::string> &list) {
  std::string out("");
  for (auto s : list) {
    out.append(s);
    out.append(", ");
  }
  // trim the trailing comma + space if necessary.
  if (out.length() > 0) {
    out.pop_back();
    out.pop_back();
  }
  return out;
}

void livenessAnalysis(Module *mod) {

  std::unique_ptr<llvm::Module> &modPtr = mod->getPtr();

  for (llvm::Function &func : (*modPtr)) {
    llvm::Function::BasicBlockListType &blocks = func.getBasicBlockList();

    int numInst = 0;

    // These represent our relevant string sets for each block.
    // The set for each block lives at its index.
    std::map<llvm::BasicBlock *, std::list<std::string>> genMap;
    std::map<llvm::BasicBlock *, std::list<std::string>> killMap;
    std::map<llvm::BasicBlock *, std::list<std::string>> inMap;
    std::map<llvm::BasicBlock *, std::list<std::string>> outMap;

    // for(int i = 0; i < blocks.size(); i++) {
    //	std::cout << "set length for block " << i << " : " << genMap[i].size();
    // }

    // int blockNum = 0;
    for (llvm::BasicBlock &block : blocks) {
      // std::cout << "block num: " << blockNum << std::endl;
      std::list<std::string> gen;
      std::list<std::string> kill;
      analyzeBlock(block, gen, kill);

      genMap[&block] = gen;
      killMap[&block] = kill;
      std::list<std::string> empty;
      inMap[&block] = empty;
      outMap[&block] = empty;
      // blockNum++;
    }

    // std::cout << "Entering Loop\n";
    bool changed = true;
    while (changed) {
      changed = false;
      for (auto blockIter = blocks.begin(); blockIter != blocks.end(); ++blockIter) {
        auto &block = *blockIter;

        auto gen = genMap.find(&block)->second;
        auto kill = killMap.find(&block)->second;

        // Copy out our oldIn so we can compare at the end to detect change.
        std::list<std::string> oldIn;
        for (auto str : inMap.find(&block)->second) {
          oldIn.push_back(str);
        }

        // LIVEout[s] = Union for p in successors of LIVEin[p].
        // UNLESS we're the last block. Then enforce that out is empty
        std::list<std::string> outFold;
        for (llvm::BasicBlock *successor : successors(&block)) {
          //   std::cout << "Successors looking" << std::endl;

          auto succLiveIn = inMap.find(successor)->second;
          foldSetUnion(outFold, succLiveIn);
        }
        outMap[&block] = outFold;

        // LIVEin[s] = GEN[s] Union (LIVEout[s] - KILL[s])
        std::list<std::string> liveIn;
        appendSetDiff(outFold, kill, liveIn);
        foldSetUnion(liveIn, gen);
        inMap[&block] = liveIn;

        // std::cout << "old in: " << concatStringList(oldIn) << std::endl;
        // std::cout << "live in: " << concatStringList(liveIn) << std::endl;
        // std::cout << "live out: " << concatStringList(outFold) << std::endl;
        // std::cout << "kill: " << concatStringList(kill) << std::endl;
        // std::cout << "gen: " << concatStringList(gen) << std::endl;

        if (!stringListsEqual(oldIn, liveIn)) {
          changed = true;
        }
      }
    }

    // Now we have a bunch of sets that tells us when we can stop caring about a variable's value.
    // Analyze the sets for variables that do not appear in any IN values to find useless variables.
    std::list<std::string> usedVariables;
    std::list<std::string> setVariables;
    std::list<std::string> unusedVariables;

    std::cout << "~~~~~~~~~~ Generated results for the function " << func.getName().str() << " ~~~~~~~~~~" << std::endl;
    for (auto iterator = genMap.begin(); iterator != genMap.end(); ++iterator) {
      llvm::BasicBlock *block = iterator->first;
      llvm::Instruction &startingInstruction = block->front();
      std::cout << "~~~ Block with name: " << getBlockName(block) << " line: " << getLine(startingInstruction) << " ~~~"
                << std::endl;

      auto genList = genMap.find(block)->second;
      auto killList = killMap.find(block)->second;
      auto inList = inMap.find(block)->second;
      auto outList = outMap.find(block)->second;

      std::cout << "Values for GEN: {" << concatStringList(genList) << "}" << std::endl;
      std::cout << "Values for KILL: {" << concatStringList(killList) << "}" << std::endl;
      std::cout << "Values for IN: {" << concatStringList(inList) << "}" << std::endl;
      std::cout << "Values for OUT: {" << concatStringList(outList) << "}" << std::endl;

      // Anything in the kill list which isn't in the out list is going unused.
      appendSetDiff(killList, outList, unusedVariables);

      for (auto str : genList) {
        if (!stringListContains(usedVariables, str)) {
          usedVariables.push_back(str);
        }
      }
      for (auto str : killList) {
        if (!stringListContains(setVariables, str)) {
          setVariables.push_back(str);
        }
      }
    }

    std::list<std::string> usedButNotSet;
    appendSetDiff(usedVariables, setVariables, usedButNotSet);

    std::list<std::string> setButNotUsed;
    appendSetDiff(setVariables, usedVariables, setButNotUsed);

	std::cout << "~~~ Report ~~~" << std::endl;

    std::cout << "Variables used without being set: {" << concatStringList(usedButNotSet) << "}" << std::endl;
    std::cout << "Variables set without being (instantly) used: {" << concatStringList(setButNotUsed) << "}"
              << std::endl;
    std::cout << "Variables set without being (EVER) used: {" << concatStringList(unusedVariables) << "}" << std::endl;
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
