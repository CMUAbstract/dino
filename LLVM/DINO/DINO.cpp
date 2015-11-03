#include <string.h>
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "DINO.h"

using namespace llvm;

// The runtime must define this function.
const char *DINOTaskBoundaryFunctionName = "__dino_task_boundary";

const char *DINORestoreFunctionName = "__mementos_checkpoint";

const char *DINOPrefix = "__dino_";

// Nonvolatile globals need to be prefixed with this name for us to recognize
// them as such.
const char *NVGlobalPrefix = "__NV_";
const char *NVGlobalSection = ".nv_vars";

namespace {
    static void registerDINO (const llvm::PassManagerBuilder &, llvm::legacy::PassManagerBase &PM) {
        PM.add(llvm::createDINOTaskSplit());
        PM.add(llvm::createDINOTaskBoundaries());
        PM.add(llvm::createDINOVersioner());
        PM.add(llvm::createDINOTaskCost());
        //PM.add(llvm::createDINOComplexityAnalysis());
    }

    // Run DINO pass after all other optimizations.
    static llvm::RegisterStandardPasses
        RegisterDINO(llvm::PassManagerBuilder::EP_EnabledOnOptLevel0, registerDINO);
}

llvm::GlobalVariable *DINOGlobal::getVarFromStore (llvm::StoreInst &SI)
{
    return dyn_cast<llvm::GlobalVariable>(SI.getPointerOperand());
}

/* Returns true if I stores to a nonvolatile variable. */
bool DINOGlobal::isNVStore (Instruction &I)
{

    StoreInst *SI = dyn_cast<StoreInst>(&I);
    if (!SI){

        return false;

    }

    GlobalVariable *GV = getVarFromStore(*SI);
    if (!GV){

        return false;

    }

    std::string sec(""); 
    llvm::GlobalValue *GA = dyn_cast<llvm::GlobalValue>(GV);
    /*If it is a global value*/
    if(GA != NULL){ 

      /*If it has a specified section*/
      if( !StringRef(GA->getSection()).empty() ){
        sec = GA->getSection();
#ifdef DINO_VERBOSE
        outs() << "Got a GV with a section: <" << GA->getSection() << ">\n";
        if( sec.compare(NVGlobalSection) == 0) {
          outs() << "Got an NV var: " << GV->getName() << "\n";
          return true;
        }
#endif
      }

    }

    /* Note: a var beginning with literal ASCII 0x01 is one that has been
     * specifically pinned to an address like this:
     *   extern volatile unsigned int foo asm("0xf000");
     */
    if( (GV->getName().find(NVGlobalPrefix) == 0) ||
        (GV->getName().find("\x01" "0x") == 0)    ||
        ( sec.compare(NVGlobalSection) == 0 ) ){

      return true; 

    }

    return false;


}

void DINOGlobal::getNVStores (Function &F,
                                   DINOGlobal::InstList &Insts)
{

    unsigned numStores = 0;
    for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
        if (isNVStore(*I)) {
            Insts.push_back(&*I);
            ++numStores;
        }
    }

#ifdef DINO_VERBOSE
    if( numStores > 0 ){
      outs() << "Function " << F.getName() << " has " << numStores << " NV store(s)\n";
    }
#endif //DINO_VERBOSE

}



bool DINOGlobal::isFuncEntryInstruction(llvm::Instruction &I){

  llvm::BasicBlock *B = I.getParent();
  llvm::Function *F = B->getParent();
  llvm::BasicBlock &FuncEntryBlock = F->front();
  llvm::Instruction &FuncEntryInst = FuncEntryBlock.front();
  
  if( &I == &FuncEntryInst ){
    return true;
  }
  return false;

}

bool DINOGlobal::isCallTo(llvm::Instruction &I, StringRef fname) {
    llvm::CallInst *call = llvm::dyn_cast<llvm::CallInst>(&I);
    if (!call){
        return false;
    }

    llvm::Value *func = call->getCalledValue();
    if (!func){
        return false;
    }

    return !func->getName().compare(fname);

}

bool DINOGlobal::isTaskBoundaryInstruction(llvm::Instruction &I){
    return isCallTo(I, DINOTaskBoundaryFunctionName);
}

bool DINOGlobal::isRestoreInstruction(llvm::Instruction &I) {

    return isCallTo(I, DINORestoreFunctionName);

}

/**
 * Starting with an Instruction, traverse the CFG backward on all possible
 * paths, stopping on each path when we hit a task boundary.  Return the set of
 * these task boundaries, sorted by distance (closest first).
 */
void DINOGlobal::CollectBBsPredicated(BasicBlock &BB,
                                      BasicBlockList &visited,
                                      BasicBlockSet &BL,
                                      BBPredicate &Collect,
                                      BBPredicate &Stop){
    for( auto &VB : visited ){
      if( &BB == VB ){
        return;
      }
    }


    if( Collect(BB) ){
        BL.insert(&BB);
    } 

    if( Stop(BB) ){
        return;
    }
   
    /*This fixed a bug where visited was carried along other future
      paths, rather than being popped off after this recursive call
      was complete
    */ 
    BasicBlockList nextVisited(visited);
    nextVisited.push_back(&BB);

    for (pred_iterator PI = pred_begin(&BB), E = pred_end(&BB); PI != E; ++PI) {
        CollectBBsPredicated(*(*PI), nextVisited, BL, Collect, Stop);
    }

}


bool DINOGlobal::shouldIgnoreFunction (const llvm::Function &F) {
    if (F.getName().find(DINOPrefix) == 0) {

#ifdef DINO_VERBOSE
        llvm::outs() << "Skipping DINO function " << F.getName() << "\n";
#endif //DINO_VERBOSE

        return true;
    }
    return false;
}
