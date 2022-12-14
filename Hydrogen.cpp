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
#include "Liveness.hpp"
#include <chrono>

using namespace hydrogen_framework;

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
  findDeadCode(CFG);
  /* Stop timer */
  auto analysisStop = std::chrono::high_resolution_clock::now();
  auto analysisTime = std::chrono::duration_cast<std::chrono::milliseconds>(analysisStop - analysisStart);

  CFG->printGraph("CFG");
  std::cout << std::endl << "Finished Analyzing CFG in " << analysisTime.count() << "ms\n" << std::endl << std::endl;
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
