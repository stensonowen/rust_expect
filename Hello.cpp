/*
 * How instructions move
 *  Consecutive instructions within each basic block can be swapped 
 *      Normally, all load instructions move up and stores move down
 *      We assume all instructions are of the form load-binop-store
 *      A Load is not moved up if it is preceded by a store 
 *          which shares a source/destination
 *      A Store is not moved down if it is followed by a load
 *          which shares a source/destination
 *      If an instruction is not recognized (e.g. alloca) we don't mess with it
 *
 */

#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;

namespace {
    static uint32_t TrueWeight = 2000;
    static uint32_t FalseWeight = 1;
    static StringRef FnName= "__builtin_expect_";

    CallInst *descendedFromLikelyCall(BranchInst &BI) {
        //determines whether a branch instruction was generated 
        // because of a "__builtin_expect" function call
        //If it is descended from a "likely" call, return the function
        //otherwise return null
        
        //not branching based on a condition
        if(BI.isUnconditional()) { return NULL; }

        Value *Cond = BI.getCondition();
        CmpInst *Comp = dyn_cast<CmpInst>(Cond);

        //not branching based on a comparison
        //comparison should be signed not equal
        if(Comp == NULL || Comp->getPredicate() != CmpInst::ICMP_NE) { 
            return NULL; 
        }

        //check all operands for call to __builtin_expect
        CallInst *Call = NULL;
        for(unsigned int i=0; i<Comp->getNumOperands(); ++i) {
            if((Call = dyn_cast<CallInst>(Comp->getOperand(i)))) {
                break;
            }
        }

        //check function name contains term "__builtin_expect"
        //we might be branching because of a different function
        StringRef fn_name = Call->getCalledFunction()->getName();
        if(fn_name.find(FnName) == StringRef::npos) {
            return NULL;
        }

        return Call;
    }

    bool calledCorrectly(CallInst &CI) {
        //determine whether programmer used expect call correctly
        //second option must be constant and not variable
        //  and must be zero or one ??
        if(dyn_cast<ConstantInt>(CI.getArgOperand(1)) == NULL) {
            errs() << "Called using a variable instead of a constant for second arg\n";
            return false;
        }

        return true;
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
            Node = MDB.createBranchWeights(TrueWeight, FalseWeight);
        } else {
            Node = MDB.createBranchWeights(FalseWeight, TrueWeight);
        }


        if(ExpectedValue->isZero()) {
            //ordinarily the "true" option is the default next basic block
            //eliminate a cache miss if it's the other way around
            //is this how LLVM actually works?  maybe.
            BI.swapSuccessors();
        }

        BI.setMetadata(LLVMContext::MD_prof, Node);

        errs() << "Changed Something!\n";

    }

    struct IdentifyBranches: public InstVisitor<IdentifyBranches> {
        void visitBranchInst(BranchInst &BI) {
            //TODO: BI.swapSuccessors() on __builtin_expect(_,0)
            CallInst *likely_call = descendedFromLikelyCall(BI);

            //not the result of a `likely`
            if(likely_call == NULL) { return; }
            //called function incorrectly
            if(calledCorrectly(*likely_call) == false) { return; }

            adjustLikelihood(BI, *likely_call);

            /*
            if(likely) {
                //(likely->getIntrinsicID()==Intrinsic::expect) ??
                errs() << likely->getName() << " !!\n";
            } else {
                errs() << "Branch not from a likely: ";
                BI.dump();
            }*/
            //errs() << "Found a call inst in parent:\n";
            //Call->dump();

        }
    };
    
    struct Hello : public ModulePass {
        static char ID;
        Hello() : ModulePass(ID) {}

        bool runOnModule(Module &M) {
            IdentifyBranches IB;
            IB.visit(M);
            return true;
        }
    };
}


char Hello::ID = 0;
static RegisterPass<Hello> X("hello", "Hello World Pass");


