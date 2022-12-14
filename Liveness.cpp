/**
 * @authors Benjamin Goodall, Brennan Seymour, Marios Tsekitsidis, Garrett Westenskow
 */
#include "Liveness.hpp"
#include "Graph.hpp"
#include "Graph_Edge.hpp"
#include "Graph_Function.hpp"
#include "Graph_Instruction.hpp"
#include "Graph_Line.hpp"
#include "Module.hpp"

namespace hydrogen_framework {

/**
 * Returns true if edges contains edge, and false otherwise
 */
bool nodeListContains(std::list<Graph_Instruction *> &edges, Graph_Instruction *edge) {
  for (auto e : edges) {
    if (e == edge)
      return true;
  }
  return false;
}

/**
 * Returns true if the edge ends at this node.
 */
bool isBackEdge(Graph_Edge *edge, Graph_Instruction *node) { return edge->getEdgeTo() == node; }

/**
 * Checks if a variable changes from its initial value before it is used.
 */
bool checkIfVariableChanged(std::list<Graph_Instruction *> nodes_visited, std::list<Graph_Instruction *> stack,
                            Graph_Instruction *node, std::string current_var) {
  int num_stores = 0;
  bool used_in_comparison = false;
  std::list<std::string> checked_instructions = {};

  while (true) {
    auto edges = node->getInstructionEdges();

    for (auto e : edges) {
      if (!isBackEdge(e, node)) {
        if (!nodeListContains(nodes_visited, e->getEdgeTo())) {
          stack.push_back(e->getEdgeTo());
        }
      }
    }

    std::string current_var_append = current_var + ",";
    for (auto e : edges) {
      std::string from_label = e->getEdgeFrom()->getInstructionLabel();
      std::string to_label = e->getEdgeTo()->getInstructionLabel();

      // Check if from_label contains the current variable
      if (from_label.find(current_var_append) != std::string::npos) {

        // Check if instruction was already checked
        bool checked = false;
        for (auto i : checked_instructions) {
          if (from_label.compare(i) == 0) {
            checked = true;
          }
        }

        // Check if current_var is used in a store operation. If so, increment num_stores.
        if (from_label.find("store") != std::string::npos && !checked) {
          checked_instructions.push_back(from_label);
          num_stores++;
        }

        // Check if current_var is used in a load operation. If so, check if the next operation is a comparison. If so,
        // set used_in_comparsion to true.
        if (from_label.find("load") != std::string::npos) {
          std::string reg = from_label.substr(from_label.find("%"), from_label.find(" =") - 2);
          std::string reg_append = reg + " ";

          if (!used_in_comparison) {
            if (from_label.find(reg_append) != std::string::npos && to_label.find("icmp") != std::string::npos) {
              std::string check = to_label.substr(to_label.find("="));
              check = check.substr(0, check.find(","));
              std::string check_reg = check.substr(check.find("%"));

              if (check_reg.compare(reg) == 0) {
                used_in_comparison = true;

                if (num_stores < 2) {
                  return false;
                }
              }
            }
          }
        }
      }
    }

    if (stack.empty()) {
      break;
    } else {
      node = stack.front();
      stack.pop_front();
      nodes_visited.push_back(node);
    }
  }

  if (num_stores >= 2 && used_in_comparison) {
    return true;
  }
  if (num_stores < 2 && used_in_comparison) {
    return false;
  }
  return false;
}

/**
 * We perform a depth first search of the tree, counting the number of relevant nodes and edges.
 * Our number of paths depends on branches, so we count it as "the total number of edges leaving a node which aren't the
 * first edge to leave that node" This can be found with num_edges - num_nodes + 2, because then each node accounts for
 * one outgoing edge, and the excess are counted as branches. We add two to this. 1 because a straight line counts as a
 * path, so we need to offset our count. 1 more to account for the exit node.
 */
void findDeadCode(Graph *g) {
  auto edges = g->getGraphEdges();
  auto functions = g->getGraphFunctions();
  std::list<Graph_Instruction *> nodes_visited = {};
  std::list<Graph_Instruction *> stack = {};
  std::list<Graph_Function *> dead_func = {};
  std::list<Graph_Function *> used_func = {};
  std::list<std::string> checked_variables = {};

  // Use for finding dead code in conditional statements and functions
  Graph_Instruction *node = g->findVirtualEntry("main");
  nodes_visited.push_back(node);
  Graph_Instruction *exit_node = g->findVirtualExit("main");

  std::cout << "~~~~~~~~~~ Dead Code Detection ~~~~~~~~~~" << std::endl;
  while (true) {
    auto edges = node->getInstructionEdges();

    for (auto e : edges) {
      if (!isBackEdge(e, node)) {
        if (!nodeListContains(nodes_visited, e->getEdgeTo())) {
          stack.push_back(e->getEdgeTo());
        }
      }
    }

    for (auto f : functions) {
      int func_used = 0;

      // Go through all edges in instruction and check which function they are associated with, add to used function
      // list
      for (auto e : edges) {
        Graph_Line *from = e->getEdgeFrom()->getGraphLine();
        Graph_Line *to = e->getEdgeTo()->getGraphLine();

        Graph_Function *func_from = from->getGraphFunction();
        Graph_Function *func_to = to->getGraphFunction();

        if (func_from == f || func_to == f) {
          bool in_list = false;
          for (auto u : used_func) {
            if (u == f) {
              in_list = true;
            }
          }

          if (!in_list) {
            used_func.push_back(f);
          }

          continue;
        }
      }
    }

    // Look for variable declarations. If variable is not changed before it is used in conditional, the code in the
    // conditional is flagged as being potentially dead code.
    for (auto e : edges) {
      std::string from_label = e->getEdgeFrom()->getInstructionLabel();
      std::string current_var;

      // Find variable being declared
      if (from_label.find("llvm.dbg.declare") != std::string::npos) {
        std::string f = from_label.substr(from_label.find("%"));
        current_var = f.substr(0, f.find(","));

        bool in_list = false;
        for (auto v : checked_variables) {
          if (v.compare(current_var) == 0) {
            in_list = true;
          }
        }

        if (!in_list) {
          checked_variables.push_back(current_var);
          bool changed = checkIfVariableChanged(nodes_visited, stack, node, current_var);

          if (changed) {
            std::cout << "Variable " << current_var << " is changed before comparison" << std::endl;
          } else {
            std::cout << "Variable " << current_var << " is not changed before comparison" << std::endl;
          }
        }
      }
    }

    if (stack.empty()) {
      break;
    } else {
      node = stack.front();
      stack.pop_front();
      nodes_visited.push_back(node);
    }
  }

  // Compare used function list with complete function list. Add functions that are not used to dead function list.
  bool in_list = false;
  for (auto f : functions) {
    for (auto u : used_func) {
      if (u == f) {
        in_list = true;
      }
    }

    if (!in_list) {
      dead_func.push_back(f);
    }
  }

  // Dead functions stored in dead_func

  std::cout << "~~~~~~~~~~ Unused Functions: ~~~~~~~~~~" << std::endl;
  for (auto f : dead_func) {
    std::cout << f->getFunctionName() << std::endl;
  }
}

/**
 * Returns true if list contains str, and false otherwise
 */
bool stringListContains(const std::list<std::string> &list, const std::string &str) {
  for (auto s : list) {
    if (s.compare(str) == 0)
      return true;
  }
  return false;
}

/**
 * Returns true if a and b have the same elements in the same order, and false otherwise.
 */
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

/**
 * Gets the line number of an instruction, or zero if its not stored.
 */
unsigned int getLine(llvm::Instruction &inst) {
  const llvm::DebugLoc &dbgInfo = inst.getDebugLoc();
  auto addr = std::addressof(dbgInfo);
  if ((void *)dbgInfo == NULL) {
    return 0;
  }

  return dbgInfo->getLine();
}

/**
 * Returns the name of the given BasicBlock, or "unnamed" if there is no name
 */
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

/**
 * Sets the passes gen and kill variables to their starting values for the given BasicBlock block.
 * killUnused is a subset of kill, excluding variables which are read later in this block.
 */
void analyzeBlock(llvm::BasicBlock &block, std::list<std::string> &gen, std::list<std::string> &kill,
                  std::list<std::string> &killUnused) {
  for (auto &inst : block) {
    // For each instruction in the BasicBlock
    unsigned int opcode = inst.getOpcode();

    if (opcode == llvm::Instruction::Call)
      continue; // We don't learn anything from function calls, skip this instruciton

    unsigned int numOperands = inst.getNumOperands();
    for (int opIndex = numOperands; opIndex > 0;) {
      opIndex--; // Done like this because 1: right to left is useful and 2: numOperands is unsigned, so numOperands - 1
                 // is dangerous.
      llvm::Value *op = inst.getOperand(opIndex);
      if (op == NULL || !op->hasName()) {
        continue;
      }

      std::string varName = op->getName().str();
      // ignore retval, it's used as a function's return value.
      if (varName.compare("retval") == 0) {
        continue;
      }

      // Consider these operations to be added to GEN.
      if (opcode == llvm::Instruction::Load) {
        // Only add vars not already in gen, and which haven't yet been assigned to (in kill)
        if (!stringListContains(gen, varName)) {
          if (stringListContains(kill, varName)) {
            // this var was set, now used. Take it out of killUnused
            killUnused.remove(varName);
          } else {
            // this var was used, but hasn't yet been set in this block.
            gen.push_back(varName);
          }
        }
      }

      // Consider these operations to be added to KILL. They set a variable (second operand of store)
      if (opcode == llvm::Instruction::Store && opIndex == 1) {
        if (!stringListContains(kill, varName)) {
          kill.push_back(varName);
          killUnused.push_back(varName);
        }
      }
    }
  }
}

/*
 * Format a nice looking string for printing a list of strings
 */
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

/**
 * main function that handles the livenessAnalysis portion of our code on a given module mod
 */
void livenessAnalysis(Module *mod) {

  std::unique_ptr<llvm::Module> &modPtr = mod->getPtr();

  for (llvm::Function &func : (*modPtr)) {
    // We break the module into functions, and get a list of the current function's BasicBlocks
    llvm::Function::BasicBlockListType &blocks = func.getBasicBlockList();

    // These represent our relevant string sets for each block.
    // The set for each block lives at its index.
    std::map<llvm::BasicBlock *, std::list<std::string>> genMap;
    std::map<llvm::BasicBlock *, std::list<std::string>> killMap;
    std::map<llvm::BasicBlock *, std::list<std::string>> killUnusedMap;
    std::map<llvm::BasicBlock *, std::list<std::string>> inMap;
    std::map<llvm::BasicBlock *, std::list<std::string>> outMap;

    // This loop sets the initial values of GEN and KILL for each block.
    for (llvm::BasicBlock &block : blocks) {
      std::list<std::string> gen;
      std::list<std::string> kill;
      std::list<std::string> killUnused;
      analyzeBlock(block, gen, kill, killUnused);

      genMap[&block] = gen;
      killMap[&block] = kill;
      killUnusedMap[&block] = killUnused;
      std::list<std::string> empty;
      inMap[&block] = empty;
      outMap[&block] = empty;
    }

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

        if (!stringListsEqual(oldIn, liveIn)) {
          changed = true;
        }
      }
    }

    // Now we have a bunch of sets that tells us when we can stop caring about a variable's value.
    // Analyze the sets for variables that do not appear in any IN values to find useless variables.
    std::list<std::string> unsetVariables;
    std::list<std::string> unusedVariables;

    // anything on the in-set of our first block is being used somewhere but never set.
    if (inMap.size() > 0) {
      foldSetUnion(unsetVariables, inMap.begin()->second);
    }

    std::cout << "~~~~~~~~~~ Generated results for the function " << func.getName().str() << " ~~~~~~~~~~" << std::endl;
    for (auto iterator = genMap.begin(); iterator != genMap.end(); ++iterator) {
      llvm::BasicBlock *block = iterator->first;
      llvm::Instruction &startingInstruction = block->front();
      //   std::cout << "~~~ Block with name: " << getBlockName(block) << " line: " << getLine(startingInstruction) <<
      //  "~~~" << std::endl;

      auto genList = genMap.find(block)->second;
      auto killList = killMap.find(block)->second;
      auto killUnusedList = killUnusedMap.find(block)->second;
      auto inList = inMap.find(block)->second;
      auto outList = outMap.find(block)->second;

      //   std::cout << "Values for GEN: {" << concatStringList(genList) << "}" << std::endl;
      //   std::cout << "Values for KILL: {" << concatStringList(killList) << "}" << std::endl;
      //   std::cout << "Values for KILL-UNUSED: {" << concatStringList(killUnusedList) << "}" << std::endl;
      //   std::cout << "Values for IN: {" << concatStringList(inList) << "}" << std::endl;
      //   std::cout << "Values for OUT: {" << concatStringList(outList) << "}" << std::endl;

      // Anything in the killUnused list which isn't in the out list is going unused.
      appendSetDiff(killUnusedList, outList, unusedVariables);
    }

    // std::cout << "~~~ Report ~~~" << std::endl;

    if (unsetVariables.size() > 0)
      std::cout << "Variables used without being set: {" << concatStringList(unsetVariables) << "}" << std::endl;
    if (unusedVariables.size() > 0)
      std::cout << "Variables assigned a value that not used later: {" << concatStringList(unusedVariables) << "}"
                << std::endl;
    if (unsetVariables.size() == 0 && unusedVariables.size() == 0) {
      std::cout << "No issues found by Liveness Analysis" << std::endl;
    }
  }
}

} // namespace hydrogen_framework
