#include <llvm/Pass.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include "llvm/IR/InstIterator.h"
#include <llvm/Support/raw_ostream.h>
#include "DINO.h"
#include "DINOComplexityAnalysis.h"
#include "DINOTaskBoundaries.h"

using namespace llvm;

bool DINOComplexityAnalysis::doInitialization (Module &M) {
    outs() << "DINO Complexity Analysis\n";
    return false;
}

/**
 * Outline:
 * Phase 1: In any order, mark all stores to NV variables.  Each function is
 *   associated with the set of NV variables it touches, and the set of
 *   StoreInsts that touch them.  These functions are provided by the
 *   DINOStoreFilter pass.
 * Phase 2: From each StoreInst, traverse the CFG backward along all paths.  On
 * each path:
 *   - If traversal encounters a task boundary, insert versioning (a fresh copy)
 *     of the var just before the task boundary (so that the version will be
 *     checkpointed when the task boundary fires), and stop.
 *   - If traversal encounters a function entry, add the var to the set of vars
 *     that the function touches, and stop.
 */
bool DINOComplexityAnalysis::runOnModule (Module &M) {

    module = &M;

    DINOTaskBoundaries DTBsPass;
    DTBsPass.doInitialization(M);
    DTBsPass.runOnModule(M);

    for (auto &F : M.getFunctionList()) {

      if( !F.isDeclaration() ){
        LoopInfoWrapperPass &LI = getAnalysis<LoopInfoWrapperPass>(F);
        DTBsPass.LI = &LI.getLoopInfo();
      }

      if (DINOGlobal::shouldIgnoreFunction(F)){
          continue;
      }

      for (auto &B : F.getBasicBlockList() ) {

        for (auto &I : B ) {

          DINOGlobal::BasicBlockSet TBs = DINOGlobal::BasicBlockSet();
  #undef MEMENTOS_TASK_BOUNDARIES
  #if defined(MEMENTOS_TASK_BOUNDARIES)
          DTBsPass.FindMementosTaskBoundaries(M, F, I, TBs);
  #else
          DTBsPass.FindTaskBoundaries(M, F, I, TBs);
  #endif 
          outs() << TBs.size() << "\n";

        }

      }

    }

    DTBsPass.doFinalization(M);
    return false;
}

GlobalVariable *DINOComplexityAnalysis::getVarFromStore (StoreInst &SI)
{
    return dyn_cast<GlobalVariable>(SI.getOperand(1));
}


bool DINOComplexityAnalysis::findReadForwardSearch( llvm::BasicBlock &B,
                                           llvm::GlobalVariable *V,
                                           DINOGlobal::BasicBlockSet &visited){

  /*Base Case*/
  /*Case 0: on a cyclic path where we've already traversed*/
  if( visited.find( &B ) != visited.end() ){

    return false;

  }
  
  DINOGlobal::BasicBlockSet newVisited = DINOGlobal::BasicBlockSet(visited);
  newVisited.insert(&B);

  /*Search this block*/
  for( auto &I : B ){

    /*Base Case*/
    /*Case 1: Found a load instruction*/
    llvm::LoadInst *LI = dyn_cast<LoadInst>(&I);
    if( LI != NULL ){

      llvm::GlobalVariable *LV = dyn_cast<GlobalVariable>(LI->getPointerOperand());
      if( LV != NULL ){ 
   
        if( LV->getName().equals(V->getName()) ){
    
          return true;
    
        } 

      }

    }

    /*Recursive Case*/
    /*Case 2: Found a callsite -- need to go IPA*/ 
    llvm::CallInst *CI = dyn_cast<CallInst>(&I);
    if( CI != NULL ){

      llvm::Function *F = CI->getCalledFunction(); 
      if( F != NULL ){
    
        /*What does f->empty mean?*/ 
        if( F->empty() == false ){ 

          auto &FB = F->front();

          if(findReadForwardSearch(FB,V,newVisited)){

            return true;

          }

        }


      }


    }
  
    /*Base Case*/ 
    /*Case 3: Found a task boundary -- haven't returned yet?  No read.*/
    if( DINOGlobal::isTaskBoundaryInstruction( I ) ){

      /*visited.size() == 0 on the first iteration, where we expect a task boundary*/
      if( visited.size() > 0 ){
        return false;
      }

    }


    /*Base Case*/
    /*Case 4: Found a write to the NV Glob - overwrites the pre-fail
      write; OK to not version*/
    llvm::StoreInst *SI = dyn_cast<StoreInst>(&I);
    if( SI != NULL ){
 
      llvm::GlobalVariable *SV = dyn_cast<GlobalVariable>(SI->getPointerOperand());
      if( SV != NULL ){ 
     
        if( SV->getName().find(V->getName()) == 0 ){

          return false;

        }

      }

    }
 
  } /*end of this block*/

 
  /*Recursive Case*/ 
  /*Case 5: No load in that block -- recursively visit successors*/
  for( succ_iterator SIt = succ_begin(&B), SEn = succ_end(&B); SIt != SEn; ++SIt){

    llvm::BasicBlock *Suc = *SIt;
    if ( findReadForwardSearch(*Suc,V,newVisited) ){

      return true;

    }

  }

  /*Basiest of Base Cases*/
  /*Case 6: End of function/no more successors*/
  return false;

}

bool DINOComplexityAnalysis::doFinalization (Module &M) {
    return false;
}

char DINOComplexityAnalysis::ID = 0;

DINOComplexityAnalysis::DINOComplexityAnalysis () : ModulePass(ID) {
}

DINOComplexityAnalysis::~DINOComplexityAnalysis () {
}

void DINOComplexityAnalysis::getAnalysisUsage (AnalysisUsage &AU) const {

  AU.addRequired<LoopInfoWrapperPass>();
  AU.addPreserved<LoopInfoWrapperPass>();

}

llvm::StringRef DINOComplexityAnalysis::getPassName () const {
    return "DINO Path Complexity Analyzer";
}

static RegisterPass<DINOComplexityAnalysis> X("dino--path-complexity",
    "DINO: analyse path complexity");

#ifdef AUTORUN_PASSES
ModulePass *llvm::createDINOComplexityAnalysis () {
    return new DINOComplexityAnalysis();
}
#endif // AUTORUN_PASSES

/*
 NOTES:

 For each task boundary, t, in a function:
 For each block, b, reachable from the task boundary:
 If b contains a write to a NV var, x:
 For each block, b', reachable from the task boundary:
 If b' contains a use of x,
 Add x to VERSIONS

 Output: VERSIONS

 */
