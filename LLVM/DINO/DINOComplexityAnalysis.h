#include "DINO.h"

class DINOComplexityAnalysis : public llvm::ModulePass {

public:
    static char ID;
    DINOComplexityAnalysis ();
    virtual ~DINOComplexityAnalysis ();
    virtual llvm::StringRef getPassName() const;

    virtual bool doInitialization (llvm::Module &M);
    virtual bool runOnModule (llvm::Module &M);
    virtual bool doFinalization (llvm::Module &M);
    virtual void getAnalysisUsage (llvm::AnalysisUsage &AU) const;

    bool isNVStore (llvm::Instruction &I);
    llvm::GlobalVariable *getVarFromStore (llvm::StoreInst &SI);
    void insertVersioning (llvm::BasicBlock &TB, llvm::StoreInst &SI);

    bool findReadForwardSearch( llvm::BasicBlock &B, llvm::GlobalVariable *V,
                                           DINOGlobal::BasicBlockSet &visited);

private:
    llvm::Module *module;
    llvm::Function *verfn;
    std::map<llvm::Function *, DINOGlobal::GlobalVarSet> NVVars;
    std::map<llvm::Function *, DINOGlobal::InstList> NVStores;

};

