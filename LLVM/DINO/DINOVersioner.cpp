#include <llvm/Pass.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include "llvm/IR/InstIterator.h"
#include <llvm/Support/raw_ostream.h>
#include "DINO.h"
#include "DINOVersioner.h"
#include "DINOTaskBoundaries.h"

using namespace llvm;

bool DINOVersioner::doInitialization (Module &M) {

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
bool DINOVersioner::runOnModule (Module &M) {
    module = &M;

    DINOTaskBoundaries DTBsPass;
    DTBsPass.doInitialization(M);
    DTBsPass.runOnModule(M);

    // For each store to a nonvolatile variable, traverse the CFG backward to
    // find the task boundaries that can flow to the store.  At these task
    // boundaries, add versioning.
    //
    // TODO Extension: reduce the number of NV vars we version.  Scan forward
    // looking for loads from the var in question; stop if we hit the store we
    // started from to find the TBs. See NOTES below for more on this.

    std::map< BasicBlock *,std::set<std::string> > versionLedger = 
        std::map< BasicBlock *, std::set<std::string> >();

    for (auto &F : M.getFunctionList()) {

        if (DINOGlobal::shouldIgnoreFunction(F)){
            continue;
        }

        NVStores[&F] = DINOGlobal::InstList();
        DINOGlobal::getNVStores(F, NVStores[&F]);
        for (auto &I : NVStores[&F] ) {

            if (StoreInst *SI = dyn_cast<StoreInst>(I)) {
                DINOGlobal::BasicBlockSet TBs = DINOGlobal::BasicBlockSet();

                /*BUG: We've added stuff before the task boundary instruction
                       so the code that finds task boundaries doesn't find
                       task boundaries.  The fix is to make it look at all
                       insts in the block, not just at the first one.
                  UPDATE: FIXED.
                */
                DTBsPass.FindTaskBoundaries(M, F, *SI, TBs);
                for( auto &T : TBs ){

                    if( versionLedger.find( T ) == 
                        versionLedger.end() ){ 
                      versionLedger[ T ]  = std::set<std::string>();
                    }
            
                    if( versionLedger[ T ].find( getVarFromStore(*SI)->getName() ) ==
                        versionLedger[ T ].end() ){

                      if( versioningRequired(*T, *SI) ){
#ifdef DINO_VERBOSE
                        outs() << "Before Versioning: " << *T << "\n";
#endif //DINO_VERBOSE
                        insertVersioning(*T, *SI);
#ifdef DINO_VERBOSE
                        outs() << "After Versioning: " << *T << "\n";
#endif //DINO_VERBOSE
                      }
                      
                      versionLedger[ T ].insert( getVarFromStore(*SI)->getName() );
  
                    }

                }

            }

        }

    }

    DTBsPass.doFinalization(M);
    return false;
}

GlobalVariable *DINOVersioner::getVarFromStore (StoreInst &SI)
{
    return dyn_cast<GlobalVariable>(SI.getOperand(1));
}


bool DINOVersioner::findReadForwardSearch( llvm::BasicBlock &B,
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

bool DINOVersioner::reachesReadOfGlobalVar( llvm::BasicBlock &TB,
                                            llvm::GlobalVariable *GV ){

  DINOGlobal::BasicBlockSet visited = DINOGlobal::BasicBlockSet();
  return findReadForwardSearch( TB, GV, visited);

}

bool DINOVersioner::versioningRequired(llvm::BasicBlock &TB,
                                     llvm::StoreInst &SI) {

  llvm::GlobalVariable *GV = getVarFromStore(SI);
  if( reachesReadOfGlobalVar( TB, GV ) ){

    return true;

  }

  return false;

}

void DINOVersioner::insertVersioning(llvm::BasicBlock &TB,
                                     llvm::StoreInst &SI) {

    Type *Pty = SI.getPointerOperand()->getType();
    Type *Ty = Pty->getPointerElementType();

    IRBuilder<> Builder(module->getContext());

    GlobalVariable *GV = getVarFromStore(SI);
    Twine newName = "__volatile_copy_of_" + GV->getName();

    llvm::BasicBlock &funcHeader = TB.getParent()->front();
    Builder.SetInsertPoint(&funcHeader,funcHeader.getFirstInsertionPt());
    AllocaInst *AI = NULL;
    
    if( VolatileCopies.find(TB.getParent()) == 
          VolatileCopies.end() ){
      VolatileCopies[TB.getParent()] = std::map<llvm::GlobalVariable *, llvm::AllocaInst *>();
    }
    if( VolatileCopies[TB.getParent()].find(GV) == 
          VolatileCopies[TB.getParent()].end() ){
      VolatileCopies[TB.getParent()][GV] = Builder.CreateAlloca(Ty, 0, newName);
    }
    AI = VolatileCopies[TB.getParent()][GV];

    // Allocate space on the stack, load the NV var, and store the NV
    // var's contents into the stack var.

    Instruction *I = NULL; 
    for ( auto &TI : TB ){
      if( DINOGlobal::isTaskBoundaryInstruction(TI) ){
        I = dyn_cast<Instruction>(&TI); 
        break;
      }
    }
    assert(I != NULL);
#ifdef DINO_VERBOSE
    outs() << "Inserting versioning at [" << *I << "]\n";
#endif
    Builder.SetInsertPoint(I);
    LoadInst *LI = Builder.CreateLoad(SI.getPointerOperand());
    Value *theVal = dyn_cast<Value>(LI);   
    Builder.CreateStore(theVal, AI);

    for ( auto &OI : TB ){
      bool isRestore = DINOGlobal::isRestoreInstruction(OI);
      llvm::BasicBlock::iterator iit(OI);
      iit++; 
      if (isRestore) {
        Builder.SetInsertPoint(&TB,iit);
        LoadInst *loadVolatile = Builder.CreateLoad(AI);
        Value *vol = dyn_cast<Value>(loadVolatile);
        Builder.CreateStore(vol, SI.getPointerOperand());
        return;
      }
    }

    // Look for the call to __dino_restore() and insert StoreInsts that copy
    // from the allocas created above.
    for (succ_iterator RI = succ_begin(&TB), E = succ_end(&TB); RI != E; ++RI) {

        for (auto &I : (*RI)->getInstList()) {
            bool isRestore = DINOGlobal::isRestoreInstruction(I);
            llvm::BasicBlock::iterator iit(I);
            iit++; 
            if (isRestore) {
              Builder.SetInsertPoint(*RI,iit);
              LoadInst *loadVolatile = Builder.CreateLoad(AI);
              Value *vol = dyn_cast<Value>(loadVolatile);
              Builder.CreateStore(vol, SI.getPointerOperand());
              return;
            }

        }

    }

}

bool DINOVersioner::doFinalization (Module &M) {
    return false;
}

char DINOVersioner::ID = 0;

DINOVersioner::DINOVersioner () : ModulePass(ID) {
}

DINOVersioner::~DINOVersioner () {
}

void DINOVersioner::getAnalysisUsage (AnalysisUsage &AU) const {
  AU.addPreserved<LoopInfoWrapperPass>();
}

const char *DINOVersioner::getPassName () const {
    return "DINO Versioning Transformation";
}

ModulePass *llvm::createDINOVersioner () {
    return new DINOVersioner();
}

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
