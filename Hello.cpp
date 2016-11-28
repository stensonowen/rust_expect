/* 
 * Basic language-agnostic(-ish) PoC implementation of `__builtin_expect` 
 *  Identifies function by name 
 *  Adjusts weights for branches which depend on the result of such a function
 *
 * Most of the work done by an InstVisitor and ModulePass
 */


#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/MDBuilder.h"
//#include "llvm/IR/IRBuilder.h"
//#include "llvm/IR/Constants.h"
//#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;

namespace {
    //We might benefit from tweaking these values
    //These are the defaults used by C's __builtin_expect
    static uint32_t TrueWeight = 2000;
    static uint32_t FalseWeight = 1;
    //TODO: change back to "__builtin_expect"
    //  a trailing underscore makes this a bit easier to test w/ C rather than Rust
    static StringRef FnName= "__builtin_expect_";

    CallInst *descendedFromLikelyCall(BranchInst &BI) {
        //determines whether a branch instruction was generated 
        // because of a "__builtin_expect" function call
        //If it is descended from a "likely" call, return the function
        //otherwise return null
        
        //not branching based on a condition
        if(BI.isUnconditional()) { 
            return NULL; 
        }

        //condition is not based on a call
        CallInst *Call = dyn_cast<CallInst>(BI.getCondition());
        if(Call == NULL) {
            return NULL;
        }

        //We must check if the function name *contains* FnName if we allow 
        //  Rust to mangle function names, which is sometimes necessary
        //  (e.g. generics)
        //StringRef fn_name = Call->getCalledFunction()->getName();
        //if(fn_name.find(FnName) == StringRef::npos) {
        
        //check function name is term "__builtin_expect_"
        if(Call->getCalledFunction()->getName() != FnName) {
            return NULL;
        }

        return Call;
    }

    bool calledCorrectly(CallInst &CI) {
        //determine whether programmer used expect call correctly
        //second option must be constant and not variable
        //  and must be zero or one ??
        ConstantInt *expect = dyn_cast<ConstantInt>(CI.getArgOperand(1));
        if(expect == NULL) {
            errs() << "Called using a variable instead of a constant for second arg\n";
            return false;
        } else if (!expect->isOne() && !expect->isZero()) {
            /* For now we require the expected value to be a bool
             * In theory it should be able to be generic though,
             *  but that adds complexity
             */
            errs() << "Expected value must be either true or false\n";
            return false;
        } else {
            return true;
        }
    }

    void adjustLikelihood(BranchInst &BI, CallInst &CI) {
        //Make sure you call calledCorrectly first
        //otherwise ExpectedValue might be NULL and we could segfault
        ConstantInt *ExpectedValue = dyn_cast<ConstantInt>(CI.getArgOperand(1));

        //adjust metadata
        MDBuilder MDB(CI.getContext());
        MDNode *Node;

        // If expect value is equal to 1 it means that we are more likely to take
        // branch 0, in other case more likely is branch 1.
        if (ExpectedValue->isOne()) {
            Node = MDB.createBranchWeights(FalseWeight, TrueWeight);
        } else {
            Node = MDB.createBranchWeights(TrueWeight, FalseWeight);
        }

        BI.setMetadata(LLVMContext::MD_prof, Node);
    }

    struct IdentifyBranches: public InstVisitor<IdentifyBranches> {
        void visitBranchInst(BranchInst &BI) {
            //decide whether this is an important Branch
            CallInst *likely_call = descendedFromLikelyCall(BI);

            //it's not the result of a `__builtin_expect`
            if(likely_call == NULL) { return; }
            //user called function incorrectly
            if(calledCorrectly(*likely_call) == false) { return; }

            //change branch weight metadata
            adjustLikelihood(BI, *likely_call);
            errs() << "Changed Something!\n";
        }
    };
    
    struct Hello : public ModulePass {
        static char ID;
        Hello() : ModulePass(ID) {}

        bool runOnModule(Module &M) {
            //Run BranchInst visitor against all the branches in each module
            IdentifyBranches IB;
            IB.visit(M);
            return true;
        }
    };
}


char Hello::ID = 0;
static RegisterPass<Hello> X("hello", "Hello World Pass");


