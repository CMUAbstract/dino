#include "DINO.h"
#include "DINOTaskSplit.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/CFG.h"
#include <cstdio>
#include <iostream>

using namespace llvm;

DINOTaskSplit::DINOTaskSplit()
    : ModulePass(ID)
{ 

#ifdef DINO_VERBOSE
  llvm::outs() << "Starting the Task Splitter Pass\n";
#endif //DINO_VERBOSE

}

DINOTaskSplit::~DINOTaskSplit ()
{ }

bool DINOTaskSplit::doInitialization (Module &M)
{
    return false;
}

// Recognize stores to nonvolatile locations.
bool DINOTaskSplit::runOnModule (Module &M)
{

    // For each Instruction in each Function, note whether it's a store to
    // a variable we just marked as nonvolatile.
    for (auto &F : M.getFunctionList()) {

      std::set<llvm::Instruction *> NVStores;
      bool changing;

      do{

        changing = false;

        for(auto &B : F){
 
          auto TI = B.begin();

          for(auto &I : B){
     
            if(DINOGlobal::isTaskBoundaryInstruction(I)){
              
              if( &I == TI ){ /*It is already at the front of a block*/
                continue; 
              }

              B.splitBasicBlock(I);

              changing = true;

              break;
  
            }
   
          }       

          if( changing ){
            break;
          } 
  
        }

      }while(changing);

    }

    return false;
}




void DINOTaskSplit::getAnalysisUsage (AnalysisUsage &AU) const
{
  /*Modifies the CFG!*/

}

bool DINOTaskSplit::doFinalization (Module &M)
{
    return false;
}

const char *DINOTaskSplit::getPassName () const {
    return "DINO Task Splitting Analysis";
}

char DINOTaskSplit::ID = 0;

ModulePass *llvm::createDINOTaskSplit() {
    return new DINOTaskSplit();
}
