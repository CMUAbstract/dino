#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/InstIterator.h"
#include "DINOTaskBoundaries.h"
using namespace llvm;

DINOTaskBoundaries::DINOTaskBoundaries(char &ID) : llvm::ModulePass(ID){

}

DINOTaskBoundaries::DINOTaskBoundaries() : llvm::ModulePass(ID){

}

DINOTaskBoundaries::~DINOTaskBoundaries(){

}


void DINOTaskBoundaries::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.addPreserved<LoopInfoWrapperPass>();
}

void FindCallers(llvm::Module &M, llvm::Function &F, DINOGlobal::FuncList &Callers){

  /*Iterate through all caller functions in the CallGraph*/
  for (auto &Fs : M){
  
    for(auto &BBs : Fs){

      for(auto &Is : BBs){
  
        CallInst *CI = dyn_cast<CallInst>(&Is);
        if( CI == NULL ){ continue; }

        Function *CF = CI->getCalledFunction();
        if( CF == NULL ){ continue; }

        if( CF == &F ){
          Callers.push_back( &Fs );
        }

      }

    }

  }

}  

bool DINOTaskBoundaries::getTaskBoundariesFromCall (CallInst &CI, DINOGlobal::BasicBlockSet &TBs) {

    /*TBs Collects the Task Boundaries for this store instruction*/
    IsTaskBoundaryPredicate collect = IsTaskBoundaryPredicate();
    IsTaskBoundaryOrFunctionEntryPredicate stop = IsTaskBoundaryOrFunctionEntryPredicate();
    BasicBlock *CB = CI.getParent();

    DINOGlobal::BasicBlockList visited = DINOGlobal::BasicBlockList();

    DINOGlobal::CollectBBsPredicated(*CB, visited, TBs,
                         collect, /*Collection Predicate*/
                         stop);/*Traversal Stop Predicate*/

    return !stop.hitTaskBoundary;

}

bool DINOTaskBoundaries::getTaskBoundariesFromInsn (Instruction &SI, DINOGlobal::BasicBlockSet &TBs) {
    /*TBs Collects the Task Boundaries for this store instruction*/
    IsTaskBoundaryPredicate collect = IsTaskBoundaryPredicate();
    IsTaskBoundaryOrFunctionEntryPredicate stop = IsTaskBoundaryOrFunctionEntryPredicate();
    BasicBlock *SB = SI.getParent();

    DINOGlobal::BasicBlockList visited = DINOGlobal::BasicBlockList();
    DINOGlobal::CollectBBsPredicated(*SB, visited, TBs,
                         collect, /*Collection Predicate*/
                         stop);/*Traversal Stop Predicate*/

    return !stop.hitTaskBoundary;
}

bool DINOTaskBoundaries::getMementosTaskBoundariesFromInsn (Instruction &SI, DINOGlobal::BasicBlockSet &TBs) {
  
    BBPredicate *collect = NULL;
    BasicBlock *SB = SI.getParent();

    Function *F = SB->getParent();
    if( F != NULL ){
      IsReturnOrBackedgePredicate *rbcollect = new IsReturnOrBackedgePredicate(LI);
      collect = (BBPredicate*) rbcollect;
    }else{
      IsReturnPredicate *rcollect = new IsReturnPredicate();
      collect = (BBPredicate*) rcollect;
    }
    IsMainEntryPredicate stop = IsMainEntryPredicate();

    DINOGlobal::BasicBlockList visited = DINOGlobal::BasicBlockList();
    DINOGlobal::CollectBBsPredicated(*SB, visited, TBs,
                         *collect, 
                         stop);

    return false;
}


void DINOTaskBoundaries::FindTaskBoundariesIPA(llvm::Module &M, llvm::Function &Caller, llvm::Function &Callee, DINOGlobal::BasicBlockSet &TBs){

  for( auto &CB : Caller ){

    for( auto &CBI : CB ){
  
      CallInst *CI = dyn_cast<CallInst>(&CBI);
      if(CI == NULL){
        continue;
      }

      Function *CalledFunc = CI->getCalledFunction();
      if( CalledFunc != &Callee){
        continue;
      }


      bool escapesFunction = getTaskBoundariesFromCall(*CI, TBs);
      
      if( escapesFunction ){

        DINOGlobal::FuncList Callers = DINOGlobal::FuncList();

        FindCallers(M,Caller,Callers);

        /*Do this search in each caller of this caller*/
        for (auto &C : Callers) {

          FindTaskBoundariesIPA(M,*C,Caller,TBs); 

        }

      }

    }

  }

}

void DINOTaskBoundaries::FindMementosTaskBoundariesIPA(llvm::Module &M, llvm::Function &Caller, llvm::Function &Callee, DINOGlobal::BasicBlockSet &TBs){

  for( auto &CB : Caller ){

    for( auto &CBI : CB ){
  
      CallInst *CI = dyn_cast<CallInst>(&CBI);
      if(CI == NULL){
        continue;
      }

      Function *CalledFunc = CI->getCalledFunction();
      if( CalledFunc != &Callee){
        continue;
      }

      FindMementosTaskBoundaries(M,Caller,CBI,TBs);

    }

  }

}

void DINOTaskBoundaries::FindMementosTaskBoundaries(llvm::Module &M, llvm::Function &F, llvm::Instruction &SI, DINOGlobal::BasicBlockSet &TBs){

  bool escapesFunction = getMementosTaskBoundariesFromInsn(SI, TBs);

  if( escapesFunction ){

    DINOGlobal::FuncList Callers = DINOGlobal::FuncList();

    FindCallers(M,F,Callers);

    for (auto &C : Callers) {
    
      FindMementosTaskBoundariesIPA(M,*C,F,TBs); 

    }

  }

}

void DINOTaskBoundaries::FindTaskBoundaries(llvm::Module &M, llvm::Function &F, llvm::Instruction &SI, DINOGlobal::BasicBlockSet &TBs){
  /*Strategy: 
              Search locally first.  If any path escapes on its way to a task boundary
              then we search inter-procedurally.  
          
              That means recursively calling FindTaskBoundariesIPA(...) on all the 
              functions that call this function, continuing the search for a task boundary.  
              We may hit entry of main and that will be the end of the SCC. We should 
              report that as a warning (?) 
  */
  
  /*Local search*/             
  bool escapesFunction = getTaskBoundariesFromInsn(SI, TBs);

  /*If local search reaches the function entry point*/
  if( escapesFunction ){

    /*Find the callers of the function we're in*/
    DINOGlobal::FuncList Callers = DINOGlobal::FuncList();

    FindCallers(M,F,Callers);
    

    /*Do this search in each caller*/
    for (auto &C : Callers) {
    
      FindTaskBoundariesIPA(M,*C,F,TBs); 

    }

  }

}

bool DINOTaskBoundaries::runOnModule(llvm::Module &M) {

  unsigned num_task_boundaries = 0;
  for(auto &F : M){

    NVStores[&F] = DINOGlobal::InstList();
    DINOGlobal::getNVStores(F, NVStores[&F]);

    for (auto &I : NVStores[&F] ) {

      if (llvm::StoreInst *SI = dyn_cast<StoreInst>(I)) {

        DINOGlobal::BasicBlockSet TBs = DINOGlobal::BasicBlockSet();
        FindTaskBoundaries(M, F, *SI, TBs);

#ifdef DINO_VERBOSE
        if( TBs.size() > 0 ){
          outs() << "Task boundaries for " << F.getName() << "(){ ... [ " << *SI << " ] ... }\n";
          for( auto &T : TBs ){
            outs() << (*T) << "\n";
          }
        }
#endif //DINO_VERBOSE

      }

    }

  }

  return false;

}

bool DINOTaskBoundaries::doFinalization(llvm::Module &M){
    return false;
}

bool DINOTaskBoundaries::doInitialization (llvm::Module &M)
{
    return false;
}


const char *DINOTaskBoundaries::getPassName () const {
    return "DINO Interprocedural Task Boundary Collection Analysis";
}

char DINOTaskBoundaries::ID = 0;

static RegisterPass<DINOTaskBoundaries> X("dino--task-boundaries",
    "DINO: perform interprocedural task boundary collection analysis");

#ifdef AUTORUN_PASSES
llvm::ModulePass *llvm::createDINOTaskBoundaries(){
  return new DINOTaskBoundaries();
}
#endif // AUTORUN_PASSES
