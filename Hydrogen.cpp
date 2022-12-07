/**
 * @author Ashwin K J
 * @file
 */
#include "Diff_Mapping.hpp"
#include "Get_Input.hpp"
#include "Graph.hpp"
#include "Graph_Line.hpp"
#include "Graph_Function.hpp"
#include "Graph_Instruction.hpp"
#include "MVICFG.hpp"
#include "Module.hpp"
#include <chrono>

using namespace hydrogen_framework;

bool node_list_contains(std::list<Graph_Instruction*> &edges, Graph_Instruction* edge) {
	for (auto e : edges) {
		if(e == edge) return true;
	}
	return false;
}

bool edge_has_relevant_version(Graph_Edge* edge, int version) {
	for(auto v : edge->getEdgeVersions())
		if(v == version) return true;
	return false;
}

bool is_back_edge(Graph_Edge* edge, Graph_Instruction* node) {
	return edge->getEdgeTo() == node;
}

/**
 * We perform a depth first search of the tree, counting the number of relevant nodes and edges.
 * Our number of paths depends on branches, so we count it as "the total number of edges leaving a node which aren't the first edge to leave that node"
 * This can be found with num_edges - num_nodes + 2, because then each node accounts for one outgoing edge, and the excess are counted as branches.
 * We add two to this. 1 because a straight line counts as a path, so we need to offset our count. 1 more to account for the exit node.
*/
int num_paths(Graph* g) {
	auto edges = g->getGraphEdges();
  auto functions = g->getGraphFunctions();
	std::list<Graph_Instruction*> nodes_visited = {};
	std::list<Graph_Instruction*> stack = {};
  std::list<Graph_Function*> dead_func = {};
  std::list<Graph_Function*> used_func = {};

  // Use for finding dead code in conditional statements and functions
	Graph_Instruction* node = g->findVirtualEntry("main");
	nodes_visited.push_back(node);
	Graph_Instruction* exit_node = g->findVirtualExit("main");
	while(true) {
    auto edges = node->getInstructionEdges();
    
    for (auto e : edges) {
      if (!is_back_edge(e, node)) {
        if (!node_list_contains(nodes_visited, e->getEdgeTo())) {
          stack.push_back(e->getEdgeTo());
        }
      }
    }

    for (auto f : functions) {
      int func_used = 0;

      // Go through all edges in instruction and check which function they are associated with, add to used function list
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

    if (stack.empty()) {
      break;
    }
    else {
      node = stack.front();
      stack.pop_front();
      nodes_visited.push_back(node);
    }
	}

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
  // Dead code stored in dead_code in the form on lines of code

  std::cout << "Unused functions:\n";
  for (auto f : dead_func) {
    std::cout << f->getFunctionName() << "\n";
  }

	return nodes_visited.size() + 2;
}

/**
 * Main function
 */
int main(int argc, char *argv[]) {
  /* Getting the input */
  if (argc < 2) {
    std::cerr << "Insufficient arguments\n"
              << "The correct format is as follows:\n"
              << "<Path-to-Module1> <Path-to-Module2> .. <Path-to-ModuleN> :: "
              << "<Path-to-file1-for-Module1> .. <Path-to-fileN-for-Module1> :: "
              << "<Path-to-file2-for-Module2> .. <Path-to-fileN-for-Module2> ..\n"
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
  std::list<Module *> mod = framework.getModules();
  /* Create ICFG */
  unsigned graphVersion = 1;
  Module *firstMod = mod.front();
  Graph *MVICFG = buildICFG(firstMod, graphVersion);
  /* Start timer */
  auto mvicfgStart = std::chrono::high_resolution_clock::now();
  /* Create MVICFG */
  // TODO: Traverse the graph (Called MVICFG currently) and do analysis.
  /* Stop timer */
  auto mvicfgStop = std::chrono::high_resolution_clock::now();
  auto mvicfgBuildTime = std::chrono::duration_cast<std::chrono::milliseconds>(mvicfgStop - mvicfgStart);

  int paths = num_paths(MVICFG);

  MVICFG->printGraph("MVICFG");
  std::cout << "Finished Building CFG in " << mvicfgBuildTime.count() << "ms\n";
  /* Write output to file */
  std::ofstream rFile("Result.txt", std::ios::trunc);
  if (!rFile.is_open()) {
    std::cerr << "Unable to open file for printing the output\n";
    return 5;
  } // End check for Result file
  rFile << "Input Args:\n";
  for (auto i = 0; i < argc; ++ i) {
    rFile << argv[i] << "  ";
  } // End loop for writing arguments
  rFile << "\n";
  rFile << "Finished Building CFG in " << mvicfgBuildTime.count() << "ms\n";
  rFile.close();
  return 0;
} // End main