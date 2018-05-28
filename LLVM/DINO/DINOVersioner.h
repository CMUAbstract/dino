#include "DINO.h"

class DINOVersioner : public llvm::ModulePass {

public:
    static char ID;
    DINOVersioner ();
    virtual ~DINOVersioner ();
    virtual llvm::StringRef getPassName() const;

    virtual bool doInitialization (llvm::Module &M);
    virtual bool runOnModule (llvm::Module &M);
    virtual bool doFinalization (llvm::Module &M);
    virtual void getAnalysisUsage (llvm::AnalysisUsage &AU) const;

    void insertRestoreCalls(llvm::Module &M, llvm::Function *restoreFunc);
    bool isNVStore (llvm::Instruction &I);
    llvm::GlobalVariable *getVarFromStore (llvm::StoreInst &SI);
    void insertVersioning (llvm::BasicBlock &TB, llvm::StoreInst &SI);

    bool findReadForwardSearch( llvm::BasicBlock &B, llvm::GlobalVariable *V,
                                           DINOGlobal::BasicBlockSet &visited);
    bool reachesReadOfGlobalVar( llvm::BasicBlock &TB, llvm::GlobalVariable *GV );
    bool versioningRequired(llvm::BasicBlock &TB, llvm::StoreInst &SI);

private:
    llvm::Module *module;
    llvm::Function *verfn;
    std::map<llvm::Function *, DINOGlobal::GlobalVarSet> NVVars;
    std::map<llvm::Function *, DINOGlobal::InstList> NVStores;
    std::map<llvm::Function *, std::map<llvm::GlobalVariable *, llvm::AllocaInst *> > VolatileCopies;

};

