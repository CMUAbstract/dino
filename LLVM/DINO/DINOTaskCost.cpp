#include "DINO.h"
#include "DINOTaskCost.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/CFG.h"
#include <llvm/Analysis/CallGraph.h>
#include <cstdio>
#include <iostream>

using namespace llvm;

DINOTaskCost::DINOTaskCost() : ModulePass(ID)
{ 
    std::map< llvm::Function *, std::set<llvm::Function *> > callersOf =
    std::map< llvm::Function *, std::set<llvm::Function *> >();
}

DINOTaskCost::~DINOTaskCost ()
{ }

bool DINOTaskCost::doInitialization (Module &M)
{
    boundariesInFunc = std::map< llvm::Function *, std::set<llvm::BasicBlock *> >();
    return false;
}
        
void DINOTaskCost::findAllCallers( Module &M){

  for( auto &F : M ){
      
    for(auto &B : F){

      for(auto &I : B){
    
        Function *OtherF = NULL; 

        CallInst *CI = dyn_cast<CallInst>(&I);
        if( CI != NULL ) {

          OtherF = CI->getCalledFunction(); 

        }
        
        InvokeInst *II = dyn_cast<InvokeInst>(&I);
        if( II != NULL ) {

          OtherF = II->getCalledFunction(); 

        }

        if( OtherF != NULL ){

          if( callersOf.find(OtherF) == callersOf.end() ){
            callersOf[OtherF] = std::set<Function *>();
          }

          callersOf[OtherF].insert(&F);

        } 
      
      }

    }

  }

}

void DINOTaskCost::SuggestMovingToCaller(llvm::Function &F, llvm::Function &C){

  //outs().changeColor(llvm::raw_ostream::Colors::YELLOW);
  outs() << "[DINO Suggestion] ";
  //outs().resetColor();
  outs() << "Moving task boundary from ";
  //outs().changeColor(llvm::raw_ostream::Colors::RED,true,false);
  outs() << F.getName();
  //outs().resetColor();
  outs() << " to ";
  //outs().changeColor(llvm::raw_ostream::Colors::GREEN,true,false);
  outs() << C.getName();
  //outs().resetColor();
  outs() << " will reduce checkpoint size by ";
  //outs().changeColor(llvm::raw_ostream::Colors::RED,true,false);
  outs() << (allocaCostOf[&F] - allocaCostOf[&C]);
  //outs().resetColor();
  outs() << "\n";

}

void DINOTaskCost::findBoundariesInFunctions (llvm::Module &M)
{
    if (!boundariesInFunc.empty())
        return;

    IsTaskBoundaryPredicate isTaskBoundary = IsTaskBoundaryPredicate();
    for (auto &F : M.getFunctionList()) {
        boundariesInFunc[&F] = std::set<llvm::BasicBlock *>();
        for(auto &B : F){
            if(isTaskBoundary(B)){
                boundariesInFunc[&F].insert(&B);
            }
        }
    }
}

void DINOTaskCost::analyzeStackSize (llvm::Module &M)
{
    findBoundariesInFunctions(M);

    for (auto &F : M.getFunctionList()) {

        unsigned long totalAllocationSize = 0;
        for(auto &B : F){

            for(auto &I : B){

                AllocaInst *AI = dyn_cast<AllocaInst>(&I);
                if( AI != NULL ){

                    unsigned long thisAllocApproxWordSize = 1;
                    if( AI->isArrayAllocation() ){

                        Value *ASizeVal = AI->getArraySize();
                        ConstantInt *ASizeCInt = dyn_cast<ConstantInt>(ASizeVal);
                        if(ASizeCInt != NULL){

                            /*Constant Int size*/
                            thisAllocApproxWordSize *= ASizeCInt->getLimitedValue();

                        }

                    }

                    totalAllocationSize += thisAllocApproxWordSize;

                }

            }

        }

#ifdef DINO_VERBOSE
        outs() << "Boundaries in Function " << F.getName() << " cost " << totalAllocationSize << "\n";
#endif
        allocaCostOf[&F] = totalAllocationSize;

    }

    for( auto &P : boundariesInFunc ){

        Function *F = P.first;
        std::set<llvm::BasicBlock *> &TBs = P.second;
        
        for( auto &TB : TBs ){
            
            for( auto &C : callersOf[F] ){
                
                if( allocaCostOf.find(C) == allocaCostOf.end() ||
                   allocaCostOf.find(F) == allocaCostOf.end() ){ 
                    continue; 
                }
                
                if( allocaCostOf[C] < allocaCostOf[F] ){
                    
                    SuggestMovingToCaller(*F, *C);
                    
                }            
                
            }
            
        }
        
        
    }

}

void DINOTaskCost::analyzePeripheralUse (Module &M)
{
    llvm::outs() << "Analyzing peripheral use\n";
    for (auto &F: M.getFunctionList()) {
        for (auto &B: F) {
            for (auto &I: B) {
                StringRef periphName;

                /* on MSP430FR5969, peripherals are addressible via 0x0-0xFFF
                   declarations like:
                     extern volatile unsigned char PADIR_L __asm__("__PADIR_L")
                   result in:
                     store volatile i8 %4, i8* @"\01__PADIR_L", align 1
                   and the PADIR_L name isn't translated until the assembler has
                   a shot at it (in this case PADIR_L=0x0204) */

                if (StoreInst *SI = dyn_cast<StoreInst>(&I)) {
                    StringRef varName = SI->getPointerOperand()->getName();
                    if (varName.find(0x01) == 0) {
                        periphName = varName.substr(1);
                    }
                }

                else if (LoadInst *LI = dyn_cast<LoadInst>(&I)) {
                    StringRef varName = LI->getPointerOperand()->getName();
                    if (varName.find(0x01) == 0) {
                        periphName = varName.substr(1);
                    }
                }

                if (! periphName.empty()) {
                    llvm::outs() << "Peripheral Use Detected in Task. (" << periphName << ")\n";
                }
            }
        }
    }
}

bool DINOTaskCost::runOnModule (Module &M)
{
    llvm::outs() << "Starting the Task Boundary Cost Analysis\n";

    findAllCallers(M);

    analyzeStackSize(M);

    analyzePeripheralUse(M);

    return false;

}




void DINOTaskCost::getAnalysisUsage (AnalysisUsage &AU) const
{
  /*Modifies the CFG!*/
  AU.addRequired<CallGraphWrapperPass>();
  AU.addPreserved<LoopInfoWrapperPass>();

}

bool DINOTaskCost::doFinalization (Module &M)
{
    return false;
}

const char *DINOTaskCost::getPassName () const {
    return "DINO Task Boundary Cost Analysis";
}

char DINOTaskCost::ID = 0;

static RegisterPass<DINOTaskCost> X("dino--task-cost",
    "DINO: analyse task boundary cost");

#ifdef AUTORUN_PASSES
ModulePass *llvm::createDINOTaskCost() {
    return new DINOTaskCost();
}
#endif // AUTORUN_PASSES
