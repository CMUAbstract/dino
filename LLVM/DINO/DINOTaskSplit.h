#include "DINO.h"

class DINOTaskSplit: public llvm::ModulePass {
    public:
        static char ID;
        DINOTaskSplit();
        virtual ~DINOTaskSplit();
		virtual llvm::StringRef getPassName () const;

        virtual bool doInitialization (llvm::Module &M);
        virtual bool runOnModule (llvm::Module &M);
        virtual bool doFinalization (llvm::Module &M);
        virtual void getAnalysisUsage (llvm::AnalysisUsage &AU) const;

        bool isTaskBoundaryInstruction(llvm::Instruction &I);

};
