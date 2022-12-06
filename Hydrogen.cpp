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
int num_paths(Graph* g, int version) {
	auto edges = g->getGraphEdges();
	std::list<Graph_Instruction*> nodes_visited = {};
	std::list<Graph_Instruction*> stack = {};

	int n_edges = 0;

	Graph_Instruction* node = g->findVirtualEntry("main");
	nodes_visited.push_back(node);
	Graph_Instruction* exit_node = g->findVirtualExit("main");
	while(true) {
		auto edges = node->getInstructionEdges();
		for(auto e : edges) {
			// we don't want to consider edges that lead backward, or that don't exist for our version.
			if(!is_back_edge(e, node) && edge_has_relevant_version(e, version)) {
				n_edges++;
				if(!node_list_contains(nodes_visited, e->getEdgeTo()))
					stack.push_back(e->getEdgeTo());
			};
		}
		
		if(stack.empty()) {
			break;
		} else {
			node = stack.front();
			stack.pop_front();
			nodes_visited.push_back(node);
		}
	}

	std::cout << "graph num edges: " << n_edges << ", graph num nodes: " << nodes_visited.size() << std::endl;
	std::cout << "version: " << version << ", paths " << n_edges - nodes_visited.size() + 2 << std::endl;

	return n_edges - nodes_visited.size() + 2;
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
  for (auto iterModule = mod.begin(), iterModuleEnd = mod.end(); iterModule != iterModuleEnd; ++iterModule) {
    auto iterModuleNext = std::next(iterModule);
    /* Proceed as long as there is a next module */
    if (iterModuleNext != iterModuleEnd) {
      /* Container for added and deleted MVICFG lines */
      std::list<Graph_Line *> addedLines;
      std::list<Graph_Line *> deletedLines;
      std::map<Graph_Line *, Graph_Line *> matchedLines; /**<Map From ICFG Graph_Line to MVICFG Graph_Line */
      std::list<Diff_Mapping> diffMap = generateLineMapping(*iterModule, *iterModuleNext);
      Graph *ICFG = buildICFG(*iterModuleNext, ++graphVersion);
      for (auto iter : diffMap) {
        /* iter.printFileInfo(); */
        std::list<Graph_Line *> iterAdd = addToMVICFG(MVICFG, ICFG, iter, graphVersion);
        std::list<Graph_Line *> iterDel = deleteFromMVICFG(MVICFG, ICFG, iter, graphVersion);
        std::map<Graph_Line *, Graph_Line *> iterMatch = matchedInMVICFG(MVICFG, ICFG, iter, graphVersion);
        addedLines.insert(addedLines.end(), iterAdd.begin(), iterAdd.end());
        deletedLines.insert(deletedLines.end(), iterDel.begin(), iterDel.end());
        matchedLines.insert(iterMatch.begin(), iterMatch.end());
      } // End loop for diffMap
      /* Update Map Edges */
      getEdgesForAddedLines(MVICFG, ICFG, addedLines, diffMap, graphVersion);
      /* Update the matched lines to get new temporary variable mapping for old lines */
      updateMVICFGVersion(MVICFG, addedLines, deletedLines, diffMap, graphVersion);
      /* Update Map Version */
      MVICFG->setGraphVersion(graphVersion);
    } // End check for iterModuleEnd
  } // End loop for Module
  /* Stop timer */
  auto mvicfgStop = std::chrono::high_resolution_clock::now();
  auto mvicfgBuildTime = std::chrono::duration_cast<std::chrono::milliseconds>(mvicfgStop - mvicfgStart);

  // Here we do a little print out of net path changes.
  int max_version = MVICFG->getGraphVersion();
  for(int i = 2; i <= max_version; i++) {
	int paths_before = num_paths(MVICFG, i - 1);
	int paths_after = num_paths(MVICFG, i);
	int paths_diff = paths_after - paths_before;
	std::cout << "From version " << i - 1 << " to version " << i;
	std::cout << ", " << (paths_diff < 0 ? paths_diff * -1 : paths_diff);
	std::cout << " paths were " << (paths_diff < 0 ? "removed." : "added.") << std::endl;
  }

  MVICFG->printGraph("MVICFG");
  std::cout << "Finished Building MVICFG in " << mvicfgBuildTime.count() << "ms\n";
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
  rFile << "Finished Building MVICFG in " << mvicfgBuildTime.count() << "ms\n";
  rFile.close();
  return 0;
} // End main