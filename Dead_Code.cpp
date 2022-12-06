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
