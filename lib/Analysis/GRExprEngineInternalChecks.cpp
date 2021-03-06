//=-- GRExprEngineInternalChecks.cpp - Builtin GRExprEngine Checks---*- C++ -*-=
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the BugType classes used by GRExprEngine to report
//  bugs derived from builtin checks in the path-sensitive engine.
//
//===----------------------------------------------------------------------===//

#include "clang/Analysis/PathSensitive/BugReporter.h"
#include "clang/Analysis/PathSensitive/GRExprEngine.h"
#include "clang/Analysis/PathSensitive/CheckerVisitor.h"
#include "clang/Analysis/PathDiagnostic.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace clang::bugreporter;

//===----------------------------------------------------------------------===//
// Utility functions.
//===----------------------------------------------------------------------===//

template <typename ITERATOR> inline
ExplodedNode* GetNode(ITERATOR I) {
  return *I;
}

template <> inline
ExplodedNode* GetNode(GRExprEngine::undef_arg_iterator I) {
  return I->first;
}

//===----------------------------------------------------------------------===//
// Bug Descriptions.
//===----------------------------------------------------------------------===//

namespace {

class VISIBILITY_HIDDEN BuiltinBugReport : public RangedBugReport {
public:
  BuiltinBugReport(BugType& bt, const char* desc,
                   ExplodedNode *n)
  : RangedBugReport(bt, desc, n) {}
  
  BuiltinBugReport(BugType& bt, const char *shortDesc, const char *desc,
                   ExplodedNode *n)
  : RangedBugReport(bt, shortDesc, desc, n) {}  
  
  void registerInitialVisitors(BugReporterContext& BRC,
                               const ExplodedNode* N);
};  
  
class VISIBILITY_HIDDEN BuiltinBug : public BugType {
  GRExprEngine &Eng;
protected:
  const std::string desc;
public:
  BuiltinBug(GRExprEngine *eng, const char* n, const char* d)
    : BugType(n, "Logic errors"), Eng(*eng), desc(d) {}

  BuiltinBug(GRExprEngine *eng, const char* n)
    : BugType(n, "Logic errors"), Eng(*eng), desc(n) {}
  
  virtual void FlushReportsImpl(BugReporter& BR, GRExprEngine& Eng) = 0;

  void FlushReports(BugReporter& BR) { FlushReportsImpl(BR, Eng); }
  
  virtual void registerInitialVisitors(BugReporterContext& BRC,
                                       const ExplodedNode* N,
                                       BuiltinBugReport *R) {}
  
  template <typename ITER> void Emit(BugReporter& BR, ITER I, ITER E);
};
  
  
template <typename ITER>
void BuiltinBug::Emit(BugReporter& BR, ITER I, ITER E) {
  for (; I != E; ++I) BR.EmitReport(new BuiltinBugReport(*this, desc.c_str(),
                                                         GetNode(I)));
}  

void BuiltinBugReport::registerInitialVisitors(BugReporterContext& BRC,
                                               const ExplodedNode* N) {
  static_cast<BuiltinBug&>(getBugType()).registerInitialVisitors(BRC, N, this);
}  
  
class VISIBILITY_HIDDEN NullDeref : public BuiltinBug {
public:
  NullDeref(GRExprEngine* eng)
    : BuiltinBug(eng,"Null dereference", "Dereference of null pointer") {}

  void FlushReportsImpl(BugReporter& BR, GRExprEngine& Eng) {
    Emit(BR, Eng.null_derefs_begin(), Eng.null_derefs_end());
  }
  
  void registerInitialVisitors(BugReporterContext& BRC,
                               const ExplodedNode* N,
                               BuiltinBugReport *R) {
    registerTrackNullOrUndefValue(BRC, GetDerefExpr(N), N);
  }
};
  
class VISIBILITY_HIDDEN NilReceiverStructRet : public BuiltinBug {
public:
  NilReceiverStructRet(GRExprEngine* eng) :
    BuiltinBug(eng, "'nil' receiver with struct return type") {}

  void FlushReportsImpl(BugReporter& BR, GRExprEngine& Eng) {
    for (GRExprEngine::nil_receiver_struct_ret_iterator
          I=Eng.nil_receiver_struct_ret_begin(),
          E=Eng.nil_receiver_struct_ret_end(); I!=E; ++I) {

      std::string sbuf;
      llvm::raw_string_ostream os(sbuf);
      PostStmt P = cast<PostStmt>((*I)->getLocation());
      const ObjCMessageExpr *ME = cast<ObjCMessageExpr>(P.getStmt());
      os << "The receiver in the message expression is 'nil' and results in the"
            " returned value (of type '"
         << ME->getType().getAsString()
         << "') to be garbage or otherwise undefined.";

      BuiltinBugReport *R = new BuiltinBugReport(*this, os.str().c_str(), *I);
      R->addRange(ME->getReceiver()->getSourceRange());
      BR.EmitReport(R);
    }
  }
  
  void registerInitialVisitors(BugReporterContext& BRC,
                               const ExplodedNode* N,
                               BuiltinBugReport *R) {
    registerTrackNullOrUndefValue(BRC, GetReceiverExpr(N), N);
  }
};

class VISIBILITY_HIDDEN NilReceiverLargerThanVoidPtrRet : public BuiltinBug {
public:
  NilReceiverLargerThanVoidPtrRet(GRExprEngine* eng) :
    BuiltinBug(eng,
               "'nil' receiver with return type larger than sizeof(void *)") {}
  
  void FlushReportsImpl(BugReporter& BR, GRExprEngine& Eng) {
    for (GRExprEngine::nil_receiver_larger_than_voidptr_ret_iterator
         I=Eng.nil_receiver_larger_than_voidptr_ret_begin(),
         E=Eng.nil_receiver_larger_than_voidptr_ret_end(); I!=E; ++I) {
      
      std::string sbuf;
      llvm::raw_string_ostream os(sbuf);
      PostStmt P = cast<PostStmt>((*I)->getLocation());
      const ObjCMessageExpr *ME = cast<ObjCMessageExpr>(P.getStmt());
      os << "The receiver in the message expression is 'nil' and results in the"
      " returned value (of type '"
      << ME->getType().getAsString()
      << "' and of size "
      << Eng.getContext().getTypeSize(ME->getType()) / 8
      << " bytes) to be garbage or otherwise undefined.";
      
      BuiltinBugReport *R = new BuiltinBugReport(*this, os.str().c_str(), *I);
      R->addRange(ME->getReceiver()->getSourceRange());
      BR.EmitReport(R);
    }
  }    
  void registerInitialVisitors(BugReporterContext& BRC,
                               const ExplodedNode* N,
                               BuiltinBugReport *R) {
    registerTrackNullOrUndefValue(BRC, GetReceiverExpr(N), N);
  }
};
  
class VISIBILITY_HIDDEN UndefinedDeref : public BuiltinBug {
public:
  UndefinedDeref(GRExprEngine* eng)
    : BuiltinBug(eng,"Dereference of undefined pointer value") {}
  
  void FlushReportsImpl(BugReporter& BR, GRExprEngine& Eng) {
    Emit(BR, Eng.undef_derefs_begin(), Eng.undef_derefs_end());
  }
  
  void registerInitialVisitors(BugReporterContext& BRC,
                               const ExplodedNode* N,
                               BuiltinBugReport *R) {
    registerTrackNullOrUndefValue(BRC, GetDerefExpr(N), N);
  }
};

class VISIBILITY_HIDDEN DivZero : public BuiltinBug {
public:
  DivZero(GRExprEngine* eng)
    : BuiltinBug(eng,"Division-by-zero",
                 "Division by zero or undefined value.") {}
  
  void FlushReportsImpl(BugReporter& BR, GRExprEngine& Eng) {
    Emit(BR, Eng.explicit_bad_divides_begin(), Eng.explicit_bad_divides_end());
  }
  
  void registerInitialVisitors(BugReporterContext& BRC,
                               const ExplodedNode* N,
                               BuiltinBugReport *R) {
    registerTrackNullOrUndefValue(BRC, GetDenomExpr(N), N);
  }
};
  
class VISIBILITY_HIDDEN UndefResult : public BuiltinBug {
public:
  UndefResult(GRExprEngine* eng) : BuiltinBug(eng,"Undefined result",
                             "Result of operation is undefined.") {}
  
  void FlushReportsImpl(BugReporter& BR, GRExprEngine& Eng) {
    Emit(BR, Eng.undef_results_begin(), Eng.undef_results_end());
  }
};
  
class VISIBILITY_HIDDEN BadCall : public BuiltinBug {
public:
  BadCall(GRExprEngine *eng)
  : BuiltinBug(eng, "Invalid function call",
        "Called function pointer is a null or undefined pointer value") {}
  
  void FlushReportsImpl(BugReporter& BR, GRExprEngine& Eng) {
    Emit(BR, Eng.bad_calls_begin(), Eng.bad_calls_end());
  }
  
  void registerInitialVisitors(BugReporterContext& BRC,
                               const ExplodedNode* N,
                               BuiltinBugReport *R) {
    registerTrackNullOrUndefValue(BRC, GetCalleeExpr(N), N);
  }
};


class VISIBILITY_HIDDEN ArgReport : public BuiltinBugReport {
  const Stmt *Arg;
public:
  ArgReport(BugType& bt, const char* desc, ExplodedNode *n,
         const Stmt *arg)
  : BuiltinBugReport(bt, desc, n), Arg(arg) {}
  
  ArgReport(BugType& bt, const char *shortDesc, const char *desc,
                   ExplodedNode *n, const Stmt *arg)
  : BuiltinBugReport(bt, shortDesc, desc, n), Arg(arg) {}  
  
  const Stmt *getArg() const { return Arg; }    
};

class VISIBILITY_HIDDEN BadArg : public BuiltinBug {
public:  
  BadArg(GRExprEngine* eng) : BuiltinBug(eng,"Uninitialized argument",  
    "Pass-by-value argument in function call is undefined.") {}

  BadArg(GRExprEngine* eng, const char* d)
    : BuiltinBug(eng,"Uninitialized argument", d) {}
  
  void FlushReportsImpl(BugReporter& BR, GRExprEngine& Eng) {
    for (GRExprEngine::UndefArgsTy::iterator I = Eng.undef_arg_begin(),
         E = Eng.undef_arg_end(); I!=E; ++I) {
      // Generate a report for this bug.
      ArgReport *report = new ArgReport(*this, desc.c_str(), I->first,
                                        I->second);
      report->addRange(I->second->getSourceRange());
      BR.EmitReport(report);
    }
  }

  void registerInitialVisitors(BugReporterContext& BRC,
                               const ExplodedNode* N,
                               BuiltinBugReport *R) {
    registerTrackNullOrUndefValue(BRC, static_cast<ArgReport*>(R)->getArg(),
                                  N);
  }  
};
  
class VISIBILITY_HIDDEN BadMsgExprArg : public BadArg {
public:
  BadMsgExprArg(GRExprEngine* eng) 
    : BadArg(eng,"Pass-by-value argument in message expression is undefined"){}
  
  void FlushReportsImpl(BugReporter& BR, GRExprEngine& Eng) {
    for (GRExprEngine::UndefArgsTy::iterator I=Eng.msg_expr_undef_arg_begin(),
         E = Eng.msg_expr_undef_arg_end(); I!=E; ++I) {      
      // Generate a report for this bug.
      ArgReport *report = new ArgReport(*this, desc.c_str(), I->first,
                                        I->second);
      report->addRange(I->second->getSourceRange());
      BR.EmitReport(report);
    }    
  } 
};
  
class VISIBILITY_HIDDEN BadReceiver : public BuiltinBug {
public:  
  BadReceiver(GRExprEngine* eng)
  : BuiltinBug(eng,"Uninitialized receiver",
               "Receiver in message expression is an uninitialized value") {}
  
  void FlushReportsImpl(BugReporter& BR, GRExprEngine& Eng) {
    for (GRExprEngine::ErrorNodes::iterator I=Eng.undef_receivers_begin(),
         End = Eng.undef_receivers_end(); I!=End; ++I) {
      
      // Generate a report for this bug.
      BuiltinBugReport *report = new BuiltinBugReport(*this, desc.c_str(), *I);
      ExplodedNode* N = *I;
      const Stmt *S = cast<PostStmt>(N->getLocation()).getStmt();
      const Expr* E = cast<ObjCMessageExpr>(S)->getReceiver();
      assert (E && "Receiver cannot be NULL");
      report->addRange(E->getSourceRange());
      BR.EmitReport(report);
    }
  }

  void registerInitialVisitors(BugReporterContext& BRC,
                               const ExplodedNode* N,
                               BuiltinBugReport *R) {
    registerTrackNullOrUndefValue(BRC, GetReceiverExpr(N), N);
  } 
};

class VISIBILITY_HIDDEN RetStack : public BuiltinBug {
public:
  RetStack(GRExprEngine* eng)
    : BuiltinBug(eng, "Return of address to stack-allocated memory") {}
  
  void FlushReportsImpl(BugReporter& BR, GRExprEngine& Eng) {
    for (GRExprEngine::ret_stackaddr_iterator I=Eng.ret_stackaddr_begin(),
         End = Eng.ret_stackaddr_end(); I!=End; ++I) {

      ExplodedNode* N = *I;
      const Stmt *S = cast<PostStmt>(N->getLocation()).getStmt();
      const Expr* E = cast<ReturnStmt>(S)->getRetValue();
      assert(E && "Return expression cannot be NULL");
      
      // Get the value associated with E.
      loc::MemRegionVal V = cast<loc::MemRegionVal>(N->getState()->getSVal(E));
      
      // Generate a report for this bug.
      std::string buf;
      llvm::raw_string_ostream os(buf);
      SourceRange R;
      
      // Check if the region is a compound literal.
      if (const CompoundLiteralRegion* CR = 
            dyn_cast<CompoundLiteralRegion>(V.getRegion())) {
        
        const CompoundLiteralExpr* CL = CR->getLiteralExpr();
        os << "Address of stack memory associated with a compound literal "
              "declared on line "
            << BR.getSourceManager()
                    .getInstantiationLineNumber(CL->getLocStart())
            << " returned.";
        
        R = CL->getSourceRange();
      }
      else if (const AllocaRegion* AR = dyn_cast<AllocaRegion>(V.getRegion())) {
        const Expr* ARE = AR->getExpr();
        SourceLocation L = ARE->getLocStart();
        R = ARE->getSourceRange();
        
        os << "Address of stack memory allocated by call to alloca() on line "
           << BR.getSourceManager().getInstantiationLineNumber(L)
           << " returned.";
      }      
      else {        
        os << "Address of stack memory associated with local variable '"
           << V.getRegion()->getString() << "' returned.";
      }
      
      RangedBugReport *report = new RangedBugReport(*this, os.str().c_str(), N);
      report->addRange(E->getSourceRange());
      if (R.isValid()) report->addRange(R);
      BR.EmitReport(report);
    }
  }
};
  
class VISIBILITY_HIDDEN RetUndef : public BuiltinBug {
public:
  RetUndef(GRExprEngine* eng) : BuiltinBug(eng, "Uninitialized return value",
              "Uninitialized or undefined value returned to caller.") {}
  
  void FlushReportsImpl(BugReporter& BR, GRExprEngine& Eng) {
    Emit(BR, Eng.ret_undef_begin(), Eng.ret_undef_end());
  }
  
  void registerInitialVisitors(BugReporterContext& BRC,
                               const ExplodedNode* N,
                               BuiltinBugReport *R) {
    registerTrackNullOrUndefValue(BRC, GetRetValExpr(N), N);
  }    
};

class VISIBILITY_HIDDEN UndefBranch : public BuiltinBug {
  struct VISIBILITY_HIDDEN FindUndefExpr {
    GRStateManager& VM;
    const GRState* St;
    
    FindUndefExpr(GRStateManager& V, const GRState* S) : VM(V), St(S) {}
    
    Expr* FindExpr(Expr* Ex) {      
      if (!MatchesCriteria(Ex))
        return 0;
      
      for (Stmt::child_iterator I=Ex->child_begin(), E=Ex->child_end();I!=E;++I)
        if (Expr* ExI = dyn_cast_or_null<Expr>(*I)) {
          Expr* E2 = FindExpr(ExI);
          if (E2) return E2;
        }
      
      return Ex;
    }
    
    bool MatchesCriteria(Expr* Ex) { return St->getSVal(Ex).isUndef(); }
  };
  
public:
  UndefBranch(GRExprEngine *eng)
    : BuiltinBug(eng,"Use of uninitialized value",
                 "Branch condition evaluates to an uninitialized value.") {}
  
  void FlushReportsImpl(BugReporter& BR, GRExprEngine& Eng) {
    for (GRExprEngine::undef_branch_iterator I=Eng.undef_branches_begin(),
         E=Eng.undef_branches_end(); I!=E; ++I) {

      // What's going on here: we want to highlight the subexpression of the
      // condition that is the most likely source of the "uninitialized
      // branch condition."  We do a recursive walk of the condition's
      // subexpressions and roughly look for the most nested subexpression
      // that binds to Undefined.  We then highlight that expression's range.
      BlockEdge B = cast<BlockEdge>((*I)->getLocation());
      Expr* Ex = cast<Expr>(B.getSrc()->getTerminatorCondition());
      assert (Ex && "Block must have a terminator.");

      // Get the predecessor node and check if is a PostStmt with the Stmt
      // being the terminator condition.  We want to inspect the state
      // of that node instead because it will contain main information about
      // the subexpressions.
      assert (!(*I)->pred_empty());

      // Note: any predecessor will do.  They should have identical state,
      // since all the BlockEdge did was act as an error sink since the value
      // had to already be undefined.
      ExplodedNode *N = *(*I)->pred_begin();
      ProgramPoint P = N->getLocation();
      const GRState* St = (*I)->getState();

      if (PostStmt* PS = dyn_cast<PostStmt>(&P))
        if (PS->getStmt() == Ex)
          St = N->getState();

      FindUndefExpr FindIt(Eng.getStateManager(), St);
      Ex = FindIt.FindExpr(Ex);

      ArgReport *R = new ArgReport(*this, desc.c_str(), *I, Ex);
      R->addRange(Ex->getSourceRange());
      BR.EmitReport(R);
    }
  }
  
  void registerInitialVisitors(BugReporterContext& BRC,
                               const ExplodedNode* N,
                               BuiltinBugReport *R) {
    registerTrackNullOrUndefValue(BRC, static_cast<ArgReport*>(R)->getArg(),
                                  N);
  }
};

class VISIBILITY_HIDDEN OutOfBoundMemoryAccess : public BuiltinBug {
public:
  OutOfBoundMemoryAccess(GRExprEngine* eng)
    : BuiltinBug(eng,"Out-of-bounds memory access",
                     "Load or store into an out-of-bound memory position.") {}

  void FlushReportsImpl(BugReporter& BR, GRExprEngine& Eng) {
    Emit(BR, Eng.explicit_oob_memacc_begin(), Eng.explicit_oob_memacc_end());
  }
};
  
class VISIBILITY_HIDDEN BadSizeVLA : public BuiltinBug {
public:
  BadSizeVLA(GRExprEngine* eng) :
    BuiltinBug(eng, "Bad variable-length array (VLA) size") {}
  
  void FlushReportsImpl(BugReporter& BR, GRExprEngine& Eng) {
    for (GRExprEngine::ErrorNodes::iterator
          I = Eng.ExplicitBadSizedVLA.begin(),
          E = Eng.ExplicitBadSizedVLA.end(); I!=E; ++I) {

      // Determine whether this was a 'zero-sized' VLA or a VLA with an
      // undefined size.
      ExplodedNode* N = *I;
      PostStmt PS = cast<PostStmt>(N->getLocation());      
      const DeclStmt *DS = cast<DeclStmt>(PS.getStmt());
      VarDecl* VD = cast<VarDecl>(*DS->decl_begin());
      QualType T = Eng.getContext().getCanonicalType(VD->getType());
      VariableArrayType* VT = cast<VariableArrayType>(T);
      Expr* SizeExpr = VT->getSizeExpr();
      
      std::string buf;
      llvm::raw_string_ostream os(buf);
      os << "The expression used to specify the number of elements in the "
            "variable-length array (VLA) '"
         << VD->getNameAsString() << "' evaluates to ";
      
      bool isUndefined = N->getState()->getSVal(SizeExpr).isUndef();
      
      if (isUndefined)
        os << "an undefined or garbage value.";
      else
        os << "0. VLAs with no elements have undefined behavior.";
      
      std::string shortBuf;
      llvm::raw_string_ostream os_short(shortBuf);
      os_short << "Variable-length array '" << VD->getNameAsString() << "' "
               << (isUndefined ? "garbage value for array size"
                   : "has zero elements (undefined behavior)");

      ArgReport *report = new ArgReport(*this, os_short.str().c_str(),
                                        os.str().c_str(), N, SizeExpr);

      report->addRange(SizeExpr->getSourceRange());
      BR.EmitReport(report);
    }
  }
  
  void registerInitialVisitors(BugReporterContext& BRC,
                               const ExplodedNode* N,
                               BuiltinBugReport *R) {
    registerTrackNullOrUndefValue(BRC, static_cast<ArgReport*>(R)->getArg(),
                                  N);
  }
};

//===----------------------------------------------------------------------===//
// __attribute__(nonnull) checking

class VISIBILITY_HIDDEN CheckAttrNonNull :
    public CheckerVisitor<CheckAttrNonNull> {

  BugType *BT;
  
public:
  CheckAttrNonNull() : BT(0) {}
  ~CheckAttrNonNull() {}

  const void *getTag() {
    static int x = 0;
    return &x;
  }

  void PreVisitCallExpr(CheckerContext &C, const CallExpr *CE) {
    const GRState *state = C.getState();
    const GRState *originalState = state;
    
    // Check if the callee has a 'nonnull' attribute.
    SVal X = state->getSVal(CE->getCallee());
    
    const FunctionDecl* FD = X.getAsFunctionDecl();
    if (!FD)
      return;

    const NonNullAttr* Att = FD->getAttr<NonNullAttr>();    
    if (!Att)
      return;
    
    // Iterate through the arguments of CE and check them for null.
    unsigned idx = 0;
    
    for (CallExpr::const_arg_iterator I=CE->arg_begin(), E=CE->arg_end(); I!=E;
         ++I, ++idx) {
    
      if (!Att->isNonNull(idx))
        continue;
      
      ConstraintManager &CM = C.getConstraintManager();
      const GRState *stateNotNull, *stateNull;
      llvm::tie(stateNotNull, stateNull) = CM.AssumeDual(state,
                                                         state->getSVal(*I));
      
      if (stateNull && !stateNotNull) {
        // Generate an error node.  Check for a null node in case
        // we cache out.
        if (ExplodedNode *errorNode = C.generateNode(CE, stateNull, true)) {
                  
          // Lazily allocate the BugType object if it hasn't already been
          // created. Ownership is transferred to the BugReporter object once
          // the BugReport is passed to 'EmitWarning'.
          if (!BT)
            BT = new BugType("Argument with 'nonnull' attribute passed null",
                             "API");
          
          EnhancedBugReport *R =
            new EnhancedBugReport(*BT,
                                  "Null pointer passed as an argument to a "
                                  "'nonnull' parameter", errorNode);
          
          // Highlight the range of the argument that was null.
          const Expr *arg = *I;
          R->addRange(arg->getSourceRange());
          R->addVisitorCreator(registerTrackNullOrUndefValue, arg);
          
          // Emit the bug report.
          C.EmitReport(R);
        }
        
        // Always return.  Either we cached out or we just emitted an error.
        return;
      }
      
      // If a pointer value passed the check we should assume that it is
      // indeed not null from this point forward.
      assert(stateNotNull);
      state = stateNotNull;
    }
   
    // If we reach here all of the arguments passed the nonnull check.
    // If 'state' has been updated generated a new node.
    if (state != originalState)
      C.addTransition(C.generateNode(CE, state));
  }
};
} // end anonymous namespace

//===----------------------------------------------------------------------===//
// Check registration.
//===----------------------------------------------------------------------===//

void GRExprEngine::RegisterInternalChecks() {
  // Register internal "built-in" BugTypes with the BugReporter. These BugTypes
  // are different than what probably many checks will do since they don't
  // create BugReports on-the-fly but instead wait until GRExprEngine finishes
  // analyzing a function.  Generation of BugReport objects is done via a call
  // to 'FlushReports' from BugReporter.
  BR.Register(new NullDeref(this));
  BR.Register(new UndefinedDeref(this));
  BR.Register(new UndefBranch(this));
  BR.Register(new DivZero(this));
  BR.Register(new UndefResult(this));
  BR.Register(new BadCall(this));
  BR.Register(new RetStack(this));
  BR.Register(new RetUndef(this));
  BR.Register(new BadArg(this));
  BR.Register(new BadMsgExprArg(this));
  BR.Register(new BadReceiver(this));
  BR.Register(new OutOfBoundMemoryAccess(this));
  BR.Register(new BadSizeVLA(this));
  BR.Register(new NilReceiverStructRet(this));
  BR.Register(new NilReceiverLargerThanVoidPtrRet(this));
  
  // The following checks do not need to have their associated BugTypes
  // explicitly registered with the BugReporter.  If they issue any BugReports,
  // their associated BugType will get registered with the BugReporter
  // automatically.  Note that the check itself is owned by the GRExprEngine
  // object.
  registerCheck(new CheckAttrNonNull());
}
