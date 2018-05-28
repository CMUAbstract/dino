#include "DINO.h"
#include <llvm/Analysis/CallGraph.h>

class DINOTaskCost: public llvm::ModulePass {

    public:

        static char ID;
        DINOTaskCost();
        virtual ~DINOTaskCost();
        virtual llvm::StringRef getPassName() const;

        virtual bool doInitialization (llvm::Module &M);
        virtual bool runOnModule (llvm::Module &M);
        virtual bool doFinalization (llvm::Module &M);
        virtual void getAnalysisUsage (llvm::AnalysisUsage &AU) const;

    private:
        std::map< llvm::Function *, std::set<llvm::BasicBlock *> > boundariesInFunc;
        std::map< llvm::Function *, std::set<llvm::Function *> > callersOf;
        std::map< llvm::Function *, unsigned long > allocaCostOf;
        void analyzeStackSize (llvm::Module &M);
        void analyzePeripheralUse (llvm::Module &M);
        void findAllCallers (llvm::Module &M);
        void findBoundariesInFunctions (llvm::Module &M);
        void SuggestMovingToCaller (llvm::Function &F, llvm::Function &C);

};
