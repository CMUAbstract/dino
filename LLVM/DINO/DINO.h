#ifndef _________________DINO_H__
#define _________________DINO_H__

#include <llvm/Pass.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Dominators.h>
#include <llvm/Analysis/PostDominators.h>
#include <llvm/Analysis/LoopInfo.h>
#include <map>
#include <set>

#undef DINO_VERBOSE

namespace llvm {
    ModulePass *createDINOTaskCost();
    ModulePass *createDINOTaskSplit();
    ModulePass *createDINOTaskBoundaries();
    ModulePass *createDINOVersioner ();
    ModulePass *createDINOComplexityAnalysis();
}

extern const char *NVGlobalPrefix;
extern const char *DINOTaskBoundaryFunctionName;
extern const char *DINOVersionFunctionName;
extern const char *DINOPrefix;



class BBPredicate
{

  public:
    virtual bool operator ()(llvm::BasicBlock &B) = 0;

};


class DINOGlobal{
public:
    typedef std::set<llvm::BasicBlock *> BasicBlockSet;
    typedef std::vector<llvm::Function *> FuncList;
    typedef std::vector<llvm::Instruction *> InstList;
    typedef std::vector<llvm::BasicBlock *> BasicBlockList;
    typedef std::vector<llvm::GlobalVariable *> GlobalVarSet;

    static bool isTaskBoundaryInstruction(llvm::Instruction &I);
    static bool isFuncEntryInstruction(llvm::Instruction &I);
    static bool isRestoreInstruction(llvm::Instruction &I);
    static bool shouldIgnoreFunction (const llvm::Function &F);

    /*TODO: Move getTaskBoundaries* to TaskBoundaries*/
  
    static bool isCallTo (llvm::Instruction &I, llvm::StringRef fname);
    static bool isNVStore (llvm::Instruction &I);
    static void getNVStores (llvm::Function &F, InstList &Insts);
    static llvm::GlobalVariable *getVarFromStore (llvm::StoreInst &SI);
    static void CollectBBsPredicated(llvm::BasicBlock &BB,
                                     BasicBlockList &visited,
                                     BasicBlockSet &BL,
                                     BBPredicate &Collect,
                                     BBPredicate &Stop);

};

class IsMainEntryPredicate : public BBPredicate {
public:

  virtual bool operator() (llvm::BasicBlock &BB){
 
    llvm::Function *F = BB.getParent();
    if( F != NULL ){ /*has function*/

      llvm::StringRef fn = F->getName();
      if( fn.find("main") == 0 ){ /*name contains "main"*/ 

        if( &BB == &(F->getEntryBlock()) ){
          llvm::outs() << "Entry block of main\n" << BB << "\n";
          return true;
  
        }

      }

    }
    return false;

  }

};

class IsReturnPredicate : public BBPredicate {

public: 
  virtual bool operator() (llvm::BasicBlock &B){

    for( auto &I : B ){

      llvm::ReturnInst *RI = llvm::dyn_cast<llvm::ReturnInst>(&I);
      if( RI ){  return true; }

    }
    return false;

  }

};

class IsReturnOrBackedgePredicate : public BBPredicate {

    llvm::LoopInfo *LI;

public:

    IsReturnOrBackedgePredicate(llvm::LoopInfo *li){
      this->LI = li;
    }

    virtual bool operator() (llvm::BasicBlock &B){

        llvm::Function *F = B.getParent();
        if( F != NULL ){
          llvm::Module *M = F->getParent();
          if( M != NULL ){
            if( LI->isLoopHeader(&B) ){ return true; }
          }
        }
        for( auto &I : B ){

          llvm::CallInst *RI = llvm::dyn_cast<llvm::CallInst>(&I);
          if( RI ){ return true; }
           
        }
        return false;
    }
};

class IsTaskBoundaryPredicate : public BBPredicate {
public:
    virtual bool operator() (llvm::BasicBlock &B){
        for( auto &I : B ){
          if( DINOGlobal::isTaskBoundaryInstruction( I ) ){
            return true;
          }
        }
        return false;
    }
};

class IsTaskBoundaryOrFunctionEntryPredicate : public BBPredicate {

  public:
    bool hitTaskBoundary;

    IsTaskBoundaryOrFunctionEntryPredicate(){
      hitTaskBoundary = false;
    }

    virtual bool operator() (llvm::BasicBlock &B){

      IsTaskBoundaryPredicate isTaskBoundary = IsTaskBoundaryPredicate();
      if( isTaskBoundary( B ) ){
        hitTaskBoundary = true;
        return true;
      }

      auto I = B.begin();
      if( DINOGlobal::isFuncEntryInstruction( *I ) ){
        hitTaskBoundary = false;
        return true;
      }

      return false;
    }

};

#endif // _________________DINO_H__
