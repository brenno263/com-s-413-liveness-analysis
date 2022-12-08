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
    for (unsigned int opIndex = 0; opIndex < numOperands; opIndex++) {
      llvm::Value *op = inst.getOperand(opIndex);
      if (op == NULL || !op->hasName()) {
        continue;
      }

      std::cout << "line " << getLine(inst) << " op: " << inst.getOpcodeName() << " index: " << opIndex << std::endl;

      std::string varName = op->getName().str();
      // ignore retval, it's used as a function's return value.
      if (varName.compare("retval") == 0) {
        continue;
      }

      // Consider these operations to be added to GEN. They use a variable
      if (opcode == llvm::Instruction::Load) {
        if (!stringListContains(gen, varName)) {
          std::cout << "added to GEN: " << varName << std::endl;
          gen.push_back(varName);
        }
      }

      // Consider these operations to be added to KILL. They set a variable (second operand of store)
      if (opcode == llvm::Instruction::Store && opIndex == 1) {
        if (!stringListContains(kill, varName)) {
          std::cout << "added to KILL: " << varName << std::endl;
          kill.push_back(varName);
        }
      }
    }
  }

  // Make sure anything in gen which is also killed in this block gets cropped out.
  for (auto k : kill) {
    gen.remove(k);
  }
}

void livenessAnalysis(Module *mod) {

  std::unique_ptr<llvm::Module> &modPtr = mod->getPtr();
  // TODO: Initialize containers for IN, OUT, KILL, and GEN, with enough space for every (or less space and use a
  // broader space).

  for (llvm::Function &func : (*modPtr)) {
	llvm::Function::BasicBlockListType &blocks = func.getBasicBlockList();


    // These represent our relevant string sets for each block.
    // The set for each block lives at its index.
    std::vector<std::list<std::string>> genMap(blocks.size(), std::list<std::string>());
    std::vector<std::list<std::string>> killMap(blocks.size(), std::list<std::string>());
    std::vector<std::list<std::string>> inMap(blocks.size(), std::list<std::string>());
    std::vector<std::list<std::string>> outMap(blocks.size(), std::list<std::string>());

	for(int i = 0; i < blocks.size(); i++) {
		std::cout << "set length for block " << i << " : " << genMap[i].size();
	}

    bool changed = true;
    while (changed) {
      int blockNum = 0;
      for (llvm::BasicBlock &block : blocks) {
        std::cout << "block num: " << blockNum << std::endl;
        std::list<std::string> gen;
        std::list<std::string> kill;

        analyzeBlock(block, gen, kill);

        blockNum++;
      }
    }

    // for (auto l : f.getFunctionLines())
    // {
    //   for (auto i : l.getLineInstructions())
    //   {
    //     //TODO: Set GEN for i to include any variable used in i
    //     //TODO: Set KILL for i to include to any variable assigned/reassigned/free()'d in i.
    //   }
    // }
  }
  //   Graph_Instruction *exit = g.findVirtualExit("main");
  //   // TODO: Set OUT for exit to be the empty set
  //   do {
  //     bool changed = false;
  //     for (auto f : g->getGraphFunctions()) {
  //       for (auto l : f->getFunctionLines()) {
  //         for (auto i : l->getLineInstructions()) {
  //           // TODO: Store i's IN set as a temp variable
  //           // TODO: Set i's IN set to be i's GEN set
  //           for (auto edge : i->getInstructionEdges()) {
  //             if (edge->getEdgeFrom()->getInstructionID() == i->getInstructionID()) {
  //               Graph_Instruction *other = edge->getEdgeTo();
  //               // TODO: Add every element in other's IN set to i's OUT set (no need to add duplicates)
  //             }
  //           }
  //           // TODO: If an element is in i's OUT set but NOT it's KILL set, add it to i's IN set (ignoring
  //           duplicates).

  //           // TODO: Compare i's IN set with the temp variable. If they differ, set changed to be true. If not, leave
  //           // changed alone.
  //           if (false) {
  //             changed = true;
  //           }
  //         }
  //       }
  //     }
  //   } while (changed);

  // Now we have a bunch of sets that tells us when we can stop caring about a variable's value.
  // Analyze the sets for variables that do not appear in any IN values to find useless variables.
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
  livenessAnalysis(mod);
  /* Create CFG */
  unsigned graphVersion = 1;
  Graph *CFG = buildICFG(mod, graphVersion);
  /* Start timer */
  auto analysisStart = std::chrono::high_resolution_clock::now();
  // TODO: Traverse the graph (CFG) and do analysis on it.
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
