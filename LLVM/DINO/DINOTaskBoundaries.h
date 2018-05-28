#include "DINO.h"


class DINOTaskBoundaries : public llvm::ModulePass {

public:
  static char ID;
  llvm::LoopInfo *LI;

  DINOTaskBoundaries(char &ID);
  DINOTaskBoundaries();
  ~DINOTaskBoundaries();
  virtual void getAnalysisUsage(llvm::AnalysisUsage &Info) const;
  virtual bool runOnModule(llvm::Module &M);
  virtual bool doFinalization(llvm::Module &CG);
  bool doInitialization (llvm::Module &CG);
  virtual llvm::StringRef getPassName () const;
  
  void FindTaskBoundariesIPA(llvm::Module &M, llvm::Function &Caller, llvm::Function &Callee, DINOGlobal::BasicBlockSet &TBs);
  void FindMementosTaskBoundariesIPA(llvm::Module &M, llvm::Function &Caller, llvm::Function &Callee, DINOGlobal::BasicBlockSet &TBs);
  void FindTaskBoundaries(llvm::Module &M, llvm::Function &F, llvm::Instruction &SI, DINOGlobal::BasicBlockSet &TBs);
  void FindMementosTaskBoundaries(llvm::Module &M, llvm::Function &F, llvm::Instruction &SI, DINOGlobal::BasicBlockSet &TBs);
    
  bool getTaskBoundariesFromInsn (llvm::Instruction &SI,
                                            DINOGlobal::BasicBlockSet &TBs);
  bool getMementosTaskBoundariesFromInsn (llvm::Instruction &SI,
                                            DINOGlobal::BasicBlockSet &TBs);

  bool getTaskBoundariesFromCall (llvm::CallInst &CI, 
                                           DINOGlobal::BasicBlockSet &TBs);

private:        
  std::map<llvm::Function *, DINOGlobal::GlobalVarSet> NVVars;
  std::map<llvm::Function *, DINOGlobal::InstList> NVStores;

};

