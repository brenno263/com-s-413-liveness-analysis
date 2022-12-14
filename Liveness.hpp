/**
 * @authors Benjamin Goodall, Brennan Seymour, Marios Tsekitsidis, Garrett Westenskow
*/
#ifndef LIVENESS_H
#define LIVENESS_H

#include "llvm/IR/CFG.h"
namespace hydrogen_framework {

/* Forward declaration */
class Graph;
class Module;

void findDeadCode(Graph *g);

void livenessAnalysis(Module *mod);

}

#endif
