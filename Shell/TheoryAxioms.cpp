
/*
 * File TheoryAxioms.cpp.
 *
 * This file is part of the source code of the software program
 * Vampire. It is protected by applicable
 * copyright laws.
 *
 * This source code is distributed under the licence found here
 * https://vprover.github.io/license.html
 * and in the source directory
 *
 * In summary, you are allowed to use Vampire for non-commercial
 * purposes but not allowed to distribute, modify, copy, create derivatives,
 * or use in competitions. 
 * For other uses of Vampire please contact developers for a different
 * licence, which we will make an effort to provide. 
 */
/**
 * @file TheoryAxioms.cpp
 * Implements class TheoryAxioms.
 */

#include "Lib/Environment.hpp"
#include "Lib/Stack.hpp"

#include "Kernel/Clause.hpp"
#include "Kernel/EqHelper.hpp"
#include "Kernel/Formula.hpp"
#include "Kernel/FormulaUnit.hpp"
#include "Kernel/Inference.hpp"
#include "Kernel/Problem.hpp"
#include "Kernel/Signature.hpp"
#include "Kernel/Sorts.hpp"
#include "Kernel/Term.hpp"
#include "Kernel/TermIterators.hpp"
#include "Kernel/Theory.hpp"

#include "Property.hpp"
#include "SymCounter.hpp"
#include "TheoryAxioms.hpp"
#include "Options.hpp"
#include "Lib/DHMap.hpp"
#include "Kernel/Sorts.hpp"

using namespace Lib;
using namespace Kernel;
using namespace Shell;


/**
 * Add the unit @c to @c problem and output it, if the option show_theory_axioms is on.
 * @since 11/11/2013 Manchester
 * @author Andrei Voronkov
 */
void TheoryAxioms::addAndOutputTheoryUnit(Unit* unit, unsigned level)
{
  CALL("TheoryAxioms::addAndOutputTheoryUnit");

  static Options::TheoryAxiomLevel opt_level = env.options->theoryAxioms();
  // if the theory axioms are some or off (want this case for some things like fool) and the axiom is not
  // a cheap one then don't add it
  if(opt_level != Options::TheoryAxiomLevel::ON && level != CHEAP){ return; }


  if (env.options->showTheoryAxioms()) {
    cout << "% Theory " << (unit->isClause() ? "clause" : "formula" ) << ": " << unit->toString() << "\n";
  }
  if(unit->isClause()){ 
    static_cast<Clause*>(unit)->setTheoryDescendant(true); 
  } else {
    _prb.reportFormulasAdded();
  }
  UnitList::push(unit, _prb.units());
} // addAndOutputTheoryUnit

/**
 * Add the theory unit clause with literal @c lit to @c problem.
 * @since 11/11/2013, Manchester: output of the clause added
 * @author Andrei Voronkov
 */
void TheoryAxioms::addTheoryUnitClause(Literal* lit, unsigned level)
{
  CALL("TheoryAxioms::addTheoryUnitClause");
  addTheoryUnitClause(lit, new Inference(Inference::THEORY), level);
} // addTheoryUnitClause

void TheoryAxioms::addTheoryUnitClause(Literal* lit, Inference* inf, unsigned level)
{
  CALL("TheoryAxioms::addTheoryUnitClause");
  Clause* unit = Clause::fromIterator(getSingletonIterator(lit), Unit::AXIOM, inf);
  addAndOutputTheoryUnit(unit, level);
} // addTheoryUnitClause

void TheoryAxioms::addTheoryNonUnitClause(Literal* lit1, Literal* lit2,unsigned level)
{
  CALL("TheoryAxioms::addTheoryNonUnitClause");
  LiteralStack lits;
  ASS(lit1);
  lits.push(lit1);
  ASS(lit2);
  lits.push(lit2);
  Clause* cl = Clause::fromStack(lits, Unit::AXIOM, new Inference(Inference::THEORY));
  addAndOutputTheoryUnit(cl, level);
} // addTheoryNonUnitCLause

void TheoryAxioms::addTheoryNonUnitClause(Literal* lit1, Literal* lit2, Literal* lit3,unsigned level)
{
  CALL("TheoryAxioms::addTheoryNonUnitClause");
  LiteralStack lits;
  ASS(lit1);
  lits.push(lit1);
  ASS(lit2);
  lits.push(lit2);
  if (lit3) {
    lits.push(lit3);
  }
  Clause* cl = Clause::fromStack(lits, Unit::AXIOM, new Inference(Inference::THEORY));
  addAndOutputTheoryUnit(cl, level);
} // addTheoryNonUnitCLause

void TheoryAxioms::addTheoryNonUnitClause(Literal* lit1, Literal* lit2, Literal* lit3,Literal* lit4,unsigned level)
{
  CALL("TheoryAxioms::addTheoryNonUnitClause");
  LiteralStack lits;
  ASS(lit1);
  lits.push(lit1);
  ASS(lit2);
  lits.push(lit2);
  ASS(lit3);
  lits.push(lit3);
  ASS(lit4);
  lits.push(lit4);
  Clause* cl = Clause::fromStack(lits, Unit::AXIOM, new Inference(Inference::THEORY));
  addAndOutputTheoryUnit(cl, level);
} // addTheoryNonUnitCLause

/**
 * Add the axiom f(X,Y)=f(Y,X).
 * @since 11/11/2013, Manchester: modified
 * @author Andrei Voronkov
 */
void TheoryAxioms::addCommutativity(Interpretation op)
{
  CALL("TheoryAxioms::addCommutativity");
  ASS(theory->isFunction(op));
  ASS_EQ(theory->getArity(op),2);

  unsigned f = env.signature->getInterpretingSymbol(op);
  unsigned srt = theory->getOperationSort(op);
  TermList x(0,false);
  TermList y(1,false);
  TermList fxy(Term::create2(f,x,y));
  TermList fyx(Term::create2(f,y,x));
  Literal* eq = Literal::createEquality(true,fxy,fyx,srt);
  addTheoryUnitClause(eq, EXPENSIVE);
} // addCommutativity

/**
 * Add the axiom f(X,Y)=f(Y,X).
 * Intended for bitvectors.
 * @since 17/02/2018, Vienna
 * @author David Damestani
 */
void TheoryAxioms::addBitVectorCommutativity(Interpretation op, unsigned size)
{
    CALL("TheoryAxioms::addBitVectorCommutativity");
    ASS(theory->isFunction(op));
    ASS(theory->isPolymorphic(op));
    ASS_EQ(theory->getArity(op),2);
    unsigned srt = env.sorts->addBitVectorSort(size);
    unsigned argSorts[2] = {srt,srt};
    
    unsigned f = env.signature->getInterpretingSymbol(op,OperatorType::getFunctionType(2,argSorts,srt));
    TermList x(0,false);
    TermList y(1,false);
    TermList fxy(Term::create2(f,x,y));
    TermList fyx(Term::create2(f,y,x));
    Literal* eq = Literal::createEquality(true,fxy,fyx,srt);
    addTheoryUnitClause(eq, CHEAP);
}

/**
 * Add axiom f(X,f(Y,Z))=f(f(X,Y),Z).
 * @since 11/11/2013, Manchester: modified
 * @author Andrei Voronkov
 */
void TheoryAxioms::addAssociativity(Interpretation op)
{
  CALL("TheoryAxioms::addCommutativity");
  ASS(theory->isFunction(op));
  ASS_EQ(theory->getArity(op),2);

  unsigned f = env.signature->getInterpretingSymbol(op);
  unsigned srt = theory->getOperationSort(op);
  TermList x(0,false);
  TermList y(1,false);
  TermList z(2,false);
  TermList fxy(Term::create2(f,x,y));
  TermList fyz(Term::create2(f,y,z));
  TermList fx_fyz(Term::create2(f,x,fyz));
  TermList f_fxy_z(Term::create2(f,fxy,z));
  Literal* eq = Literal::createEquality(true, fx_fyz,f_fxy_z, srt);
  addTheoryUnitClause(eq, EXPENSIVE);
} // addAsssociativity


/**
 * Add axiom f(X,e)=X.
 * @since 11/11/2013, Manchester: modified
 * @author Andrei Voronkov
 */
void TheoryAxioms::addRightIdentity(Interpretation op, TermList e)
{
  CALL("TheoryAxioms::addRightIdentity");
  ASS(theory->isFunction(op));
  ASS_EQ(theory->getArity(op),2);

  unsigned f = env.signature->getInterpretingSymbol(op);
  unsigned srt = theory->getOperationSort(op);
  TermList x(0,false);
  TermList fxe(Term::create2(f,x,e));
  Literal* eq = Literal::createEquality(true,fxe,x,srt);
  addTheoryUnitClause(eq, EXPENSIVE);
} // addRightIdentity

/**
 * Add axiom f(e,X)=X.
 */
void TheoryAxioms::addLeftIdentity(Interpretation op, TermList e)
{
  CALL("TheoryAxioms::addLeftIdentity");
  ASS(theory->isFunction(op));
  ASS_EQ(theory->getArity(op),2);

  unsigned f = env.signature->getInterpretingSymbol(op);
  unsigned srt = theory->getOperationSort(op);
  TermList x(0,false);
  TermList fex(Term::create2(f,e,x));
  Literal* eq = Literal::createEquality(true,fex,x,srt);
  addTheoryUnitClause(eq, EXPENSIVE);
} // addLeftIdentity

/**
 * Add axioms for commutative group with addition @c op, inverse @c inverse and unit @c e:
 * <ol>
 * <li>f(X,Y)=f(Y,X) (commutativity)</li>
 * <li>f(X,f(Y,Z))=f(f(X,Y),Z) (associativity)</li>
 * <li>f(X,e)=X (right identity)</li>
 * <li>i(f(x,y)) = f(i(y),i(x))</li>
 * <li>f(x,i(x))=e (right inverse)</li>
 * </ol>
 * @since 11/11/2013, Manchester: modified
 * @author Andrei Voronkov
 */
void TheoryAxioms::addCommutativeGroupAxioms(Interpretation op, Interpretation inverse, TermList e)
{
  CALL("TheoryAxioms::addCommutativeGroupAxioms");

  ASS(theory->isFunction(op));
  ASS_EQ(theory->getArity(op),2);
  ASS(theory->isFunction(inverse));
  ASS_EQ(theory->getArity(inverse),1);

  addCommutativity(op);
  addAssociativity(op);
  addRightIdentity(op,e);

  // i(f(x,y)) = f(i(y),i(x))
  unsigned f = env.signature->getInterpretingSymbol(op);
  unsigned i = env.signature->getInterpretingSymbol(inverse);
  unsigned srt = theory->getOperationSort(op);
  ASS_EQ(srt, theory->getOperationSort(inverse));

  TermList x(0,false);
  TermList y(1,false);
  TermList fxy(Term::create2(f,x,y));
  TermList ix(Term::create1(i,x));
  TermList iy(Term::create1(i,y));
  TermList i_fxy(Term::create1(i,fxy));
  TermList f_iy_ix(Term::create2(f,iy,ix));
  Literal* eq1 = Literal::createEquality(true,i_fxy,f_iy_ix,srt);
  addTheoryUnitClause(eq1, EXPENSIVE);

  // f(x,i(x))=e
  TermList fx_ix(Term::create2(f,x,ix));
  Literal* eq2 = Literal::createEquality(true,fx_ix,e,srt);
  addTheoryUnitClause(eq2, EXPENSIVE);
} // TheoryAxioms::addCommutativeGroupAxioms

/**
 * Add axiom op(op(x,i(y)),y) = x
 * e.g. (x+(-y))+y = x
 */
void TheoryAxioms::addRightInverse(Interpretation op, Interpretation inverse)
{
  CALL("TheoryAxioms::addRightInverse");

  TermList x(0,false);
  TermList y(0,false);
  unsigned f = env.signature->getInterpretingSymbol(op);
  unsigned i = env.signature->getInterpretingSymbol(inverse);
  unsigned srt = theory->getOperationSort(op);
  ASS_EQ(srt, theory->getOperationSort(inverse));

  TermList iy(Term::create1(i,y));
  TermList xiy(Term::create2(f,x,iy));
  TermList xiyy(Term::create2(f,xiy,y));
  Literal* eq = Literal::createEquality(true,xiyy,x,srt);
  addTheoryUnitClause(eq, EXPENSIVE);
}

/**
 * Add axiom ~op(X,X)
 */
void TheoryAxioms::addNonReflexivity(Interpretation op)
{
  CALL("TheoryAxioms::addNonReflexivity");

  ASS(!theory->isFunction(op));
  ASS_EQ(theory->getArity(op),2);

  unsigned opPred = env.signature->getInterpretingSymbol(op);
  TermList x(0,false);
  Literal* l11 = Literal::create2(opPred, false, x, x);
  addTheoryUnitClause(l11, CHEAP);
} // addNonReflexivity

void TheoryAxioms::addPolyMorphicNonReflexivity(Interpretation op, OperatorType* type)
{
  CALL("TheoryAxioms::addPolyMorphicNonReflexivity");

  ASS(!theory->isFunction(op));
  ASS_EQ(theory->getArity(op),2);

  unsigned opPred = env.signature->getInterpretingSymbol(op, type);
  TermList x(0,false);
  Literal* l11 = Literal::create2(opPred, false, x, x);
  addTheoryUnitClause(l11, CHEAP);
} // addNonReflexivity

/**
 * Add axiom ~op(X,Y) | ~op(Y,Z) | op(X,Z)
 */
void TheoryAxioms::addTransitivity(Interpretation op)
{
  CALL("TheoryAxioms::addTransitivity");
  ASS(!theory->isFunction(op));
  ASS_EQ(theory->getArity(op),2);

  unsigned opPred = env.signature->getInterpretingSymbol(op);
  TermList x(0,false);
  TermList y(1,false);
  TermList v3(2,false);

  Literal* nonL12 = Literal::create2(opPred, false, x, y);
  Literal* nonL23 = Literal::create2(opPred, false, y, v3);
  Literal* l13 = Literal::create2(opPred, true, x, v3);

  addTheoryNonUnitClause(nonL12, nonL23, l13,CHEAP);
}

/**
 * Add axiom less(X,Y) | less(Y,X) | X=Y
 */
void TheoryAxioms::addOrderingTotality(Interpretation less)
{
  CALL("TheoryAxioms::addOrderingTotality");
  ASS(!theory->isFunction(less));
  ASS_EQ(theory->getArity(less),2);

  unsigned opPred = env.signature->getInterpretingSymbol(less);
  TermList x(0,false);
  TermList y(1,false);

  Literal* l12 = Literal::create2(opPred, true, x, y);
  Literal* l21 = Literal::create2(opPred, true, y, x);

  unsigned srt = theory->getOperationSort(less);
  Literal* eq = Literal::createEquality(true,x,y,srt);

  addTheoryNonUnitClause(l12, l21,eq,CHEAP);
}

/**
 * Add axioms of reflexivity, transitivity and total ordering for predicate @c less
 */
void TheoryAxioms::addTotalOrderAxioms(Interpretation less)
{
  CALL("TheoryAxioms::addTotalOrderAxioms");

  addNonReflexivity(less);
  addTransitivity(less);
  addOrderingTotality(less);
}

/**
 * Add axiom ~less(X,Y) | less(X+Z,Y+Z)
 */
void TheoryAxioms::addMonotonicity(Interpretation less, Interpretation addition)
{
  CALL("TheoryAxioms::addMonotonicity");
  ASS(!theory->isFunction(less));
  ASS_EQ(theory->getArity(less),2);
  ASS(theory->isFunction(addition));
  ASS_EQ(theory->getArity(addition),2);

  unsigned lessPred = env.signature->getInterpretingSymbol(less);
  unsigned addFun = env.signature->getInterpretingSymbol(addition);
  TermList x(0,false);
  TermList y(1,false);
  TermList v3(2,false);
  TermList xPv3(Term::create2(addFun, x,v3));
  TermList yPv3(Term::create2(addFun, y,v3));
  Literal* nonLe = Literal::create2(lessPred, false, x, y);
  Literal* leAdded = Literal::create2(lessPred, true, xPv3, yPv3);

  addTheoryNonUnitClause(nonLe, leAdded,EXPENSIVE);
}

/**
 * Add the axiom $less(X,$sum(X,1))
 *
 * Taken from SPASS+T work
 */
void TheoryAxioms::addPlusOneGreater(Interpretation plus, TermList oneElement,
                                     Interpretation less)
{
  CALL("TheoryAxioms::addPlusOneGreater");
  ASS(!theory->isFunction(less));
  ASS_EQ(theory->getArity(less),2);
  ASS(theory->isFunction(plus));
  ASS_EQ(theory->getArity(plus),2);

  unsigned lessPred = env.signature->getInterpretingSymbol(less);
  unsigned addFun = env.signature->getInterpretingSymbol(plus);
  TermList x(0,false);

  TermList xPo(Term::create2(addFun,x,oneElement));
  Literal* xPo_g_x = Literal::create2(lessPred,true,x,xPo);
  addTheoryUnitClause(xPo_g_x,CHEAP);
}

/**
 * Add axioms for addition, unary minus and ordering
 */
void TheoryAxioms::addAdditionAndOrderingAxioms(Interpretation plus, Interpretation unaryMinus,
    TermList zeroElement, TermList oneElement, Interpretation less)
{
  CALL("TheoryAxioms::addAdditionAndOrderingAxioms");

  addCommutativeGroupAxioms(plus, unaryMinus, zeroElement);
  addTotalOrderAxioms(less);
  addMonotonicity(less, plus);

  // y < x+one | x<y
  unsigned plusFun = env.signature->getInterpretingSymbol(plus);
  unsigned lessPred = env.signature->getInterpretingSymbol(less);
  TermList x(0,false);
  TermList y(1,false);
  Literal* xLy = Literal::create2(lessPred,true,x,y);
  TermList xP(Term::create2(plusFun,x,oneElement));
  Literal* yLxP = Literal::create2(lessPred,true,y,xP);
  addTheoryNonUnitClause(xLy,yLxP,EXPENSIVE);

  // add that --x = x
  unsigned varSort = theory->getOperationSort(unaryMinus);
  unsigned unaryMinusFun = env.signature->getInterpretingSymbol(unaryMinus);
  TermList mx(Term::create1(unaryMinusFun,x));
  TermList mmx(Term::create1(unaryMinusFun,mx));
  Literal* mmxEqx = Literal::createEquality(true,mmx,x,varSort);
  addTheoryUnitClause(mmxEqx, EXPENSIVE);

}

/* Add that
 *
 * p(concat(s x) concat(ts tx) -> p(s,ts)
 *
 * e.g: bvuge(concat(s x) concat(ts tx)) -> bvuge(s ts)
 *
 *	where srt0 is the sort for s and ts
 *	and srt1 is the sort for x and tx
 *
 *
 * */

void TheoryAxioms::addPredicateOnConcatArgsImpliesPredicateConcatFirstArg(unsigned srt0, unsigned srt1, unsigned resultSrt, Interpretation predicate)
{
	CALL("TheoryAxioms::addPredicateOnConcatArgsImpliesPredicateConcatFirstArg");

	TermList x(0,false);
	TermList s(1,false);
	TermList tx(2,false);
	TermList ts(3,false);

	unsigned bvuge_concat = env.signature->getInterpretingSymbol(predicate,OperatorType::getPredicateType({resultSrt,resultSrt}));
	unsigned bvuge_s_ts = env.signature->getInterpretingSymbol(predicate,OperatorType::getPredicateType({srt0,srt0}));

	unsigned argSorts[2] = {srt0,srt1};
	unsigned concat = env.signature->getInterpretingSymbol(Interpretation::CONCAT,OperatorType::getFunctionType(2,argSorts,resultSrt));

	//LHS
	// p(concat(s x) concat(ts tx)
	//concat(s x)

	TermList concat_s_x(Term::create2(concat,s,x));
	//concat(ts tx)

	TermList concat_ts_tx(Term::create2(concat,ts,tx));

	Formula* p_concats = new AtomicFormula(Literal::create2(bvuge_concat,true,concat_s_x,concat_ts_tx));

	//RHS
	//bvuge(s ts)

	Formula* p_s_ts = new AtomicFormula(Literal::create2(bvuge_s_ts,true,s,ts));

	Formula* implication0 = new BinaryFormula(IMP, p_concats, p_s_ts);

	addAndOutputTheoryUnit(new FormulaUnit(implication0, new Inference(Inference::THEORY), Unit::AXIOM),CHEAP);


}



// called when encountered:
// concat x s
// concat(srt0 srt1) -> resultSrt
// (bvuge s ts) AND (bvsge x tx) -> bvsge (concat(x s) concat(tx ts))
void TheoryAxioms::addConcatAxiom1(unsigned srt0, unsigned srt1, unsigned resultSrt)
{
	CALL("TheoryAxioms::addConcatAxiom1");
	// where x and tx are of the same sort and s and ts are of the same sort
	// (bvuge s ts) AND (bvsge x tx) -> bvsge (concat(x s) concat(tx ts))

	TermList x(0,false);
    TermList s(1,false);
    TermList tx(2,false);
    TermList ts(3,false);

    // LHS

    unsigned bvuge = env.signature->getInterpretingSymbol(Interpretation::BVUGE,OperatorType::getPredicateType({srt1,srt1}));
    // (bvuge s ts)
    Formula* bvuge_s_ts = new AtomicFormula(Literal::create2(bvuge,true,s,ts));

    unsigned bvsge1 = env.signature->getInterpretingSymbol(Interpretation::BVSGE,OperatorType::getPredicateType({srt0,srt0}));
    // (bvsge x tx)
    Formula* bvsge_x_tx = new AtomicFormula(Literal::create2(bvsge1,true,x,tx));

    // (bvuge s ts) AND (bvsge x tx)
    FormulaList* argLst = nullptr;
    FormulaList::push(bvuge_s_ts,argLst);
    FormulaList::push(bvsge_x_tx,argLst);
    Formula* conjunct = new JunctionFormula(AND,argLst);
    // LHS END

    // RHS
    // bvsge (concat(x s) concat(tx ts))
    unsigned argSorts[2] = {srt0,srt1};
    unsigned concat = env.signature->getInterpretingSymbol(Interpretation::CONCAT,OperatorType::getFunctionType(2,argSorts,resultSrt));

    //concat(x s)
    TermList concat_x_s(Term::create2(concat,x,s));

    //concat(tx ts)
    TermList concat_tx_ts(Term::create2(concat,tx,ts));

    //bvsge (concat(x s) concat(tx ts))
    unsigned bvsge2 = env.signature->getInterpretingSymbol(Interpretation::BVSGE,OperatorType::getPredicateType({resultSrt,resultSrt}));
    Formula* bvsge_con_con = new AtomicFormula(Literal::create2(bvsge2,true,concat_x_s,concat_tx_ts));
    //RHS END

    Formula* ax = new BinaryFormula(IMP, conjunct, bvsge_con_con);

    addAndOutputTheoryUnit(new FormulaUnit(ax, new Inference(Inference::THEORY), Unit::AXIOM),CHEAP);
}

// called when encountered:
// concat x s
// concat(srt0 srt1) -> resultSrt
// (bvult s ts) AND (bvsgt x tx) -> bvsge (concat(x s) concat(tx ts))
void TheoryAxioms::addConcatAxiom2(unsigned srt0, unsigned srt1, unsigned resultSrt)
{
	CALL("TheoryAxioms::addConcatAxiom2");
	// where x and tx are of the same sort and s and ts are of the same sort
	// (bvult s ts) AND (bvsgt x tx) -> bvsge (concat(x s) concat(tx ts))

	TermList x(0,false);
    TermList s(1,false);
    TermList tx(2,false);
    TermList ts(3,false);

    // LHS
    unsigned bvult = env.signature->getInterpretingSymbol(Interpretation::BVULT,OperatorType::getPredicateType({srt1,srt1}));
    // (bvult s ts)
    Formula* bvult_s_ts = new AtomicFormula(Literal::create2(bvult,true,s,ts));

    unsigned bvsgt = env.signature->getInterpretingSymbol(Interpretation::BVSGT,OperatorType::getPredicateType({srt0,srt0}));
    // (bvsgt x tx)
    Formula* bvsgt_x_tx = new AtomicFormula(Literal::create2(bvsgt,true,x,tx));

    // (bvult s ts) AND (bvsgt x tx)
    FormulaList* argLst = nullptr;
    FormulaList::push(bvult_s_ts,argLst);
    FormulaList::push(bvsgt_x_tx,argLst);
    Formula* conjunct = new JunctionFormula(AND,argLst);
    // LHS END

    // RHS
    // bvsge (concat(x s) concat(tx ts))
    unsigned argSorts[2] = {srt0,srt1};
    unsigned concat = env.signature->getInterpretingSymbol(Interpretation::CONCAT,OperatorType::getFunctionType(2,argSorts,resultSrt));

    //concat(x s)
    TermList concat_x_s(Term::create2(concat,x,s));

    //concat(tx ts)
    TermList concat_tx_ts(Term::create2(concat,tx,ts));

    //bvsge (concat(x s) concat(tx ts))
    unsigned bvsge2 = env.signature->getInterpretingSymbol(Interpretation::BVSGE,OperatorType::getPredicateType({resultSrt,resultSrt}));
    Formula* bvsge_con_con = new AtomicFormula(Literal::create2(bvsge2,true,concat_x_s,concat_tx_ts));
    //RHS END

    Formula* ax = new BinaryFormula(IMP, conjunct, bvsge_con_con);

    addAndOutputTheoryUnit(new FormulaUnit(ax, new Inference(Inference::THEORY), Unit::AXIOM),CHEAP);
}


// e.g. (bvsge x s) -> (bvsgt x s OR x = s)
// !(bvsge x s) OR (bvsgt x s) (x = s)
void TheoryAxioms::isPredicateWithEqualRemovedOrEqualAxiom(Interpretation completePredicate, Interpretation predicateWithEqualRemoved, unsigned size)
{
	CALL("TheoryAxioms::isPredicateWithEqualRemovedOrEqualAxiom");


	TermList x(0,false);
	TermList s(1,false);

	unsigned srt = env.sorts->addBitVectorSort(size);

	unsigned bvsgt = env.signature->getInterpretingSymbol(predicateWithEqualRemoved,OperatorType::getPredicateType({srt,srt}));
	unsigned bvsge = env.signature->getInterpretingSymbol(completePredicate,OperatorType::getPredicateType({srt,srt}));
	// !(bvsge x s)
	Literal* bvsge_x_s = Literal::create2(bvsge, false,x,s);
	// (bvsgt x s)
	Literal* bvsgt_x_s = Literal::create2(bvsgt, true,x,s);
	// x = s
	Literal* xEs = Literal::createEquality(true,x,s,srt);

	addTheoryNonUnitClause(bvsge_x_s,bvsgt_x_s,xEs,CHEAP);

}


// add that x + y = z -> (y = z - x AND x = z - y)
//  (x + y != z) OR (y = z + -(x))
// AND
// (x + y != z) OR x = z + (-y)
void TheoryAxioms::addSomeAdditionAxiom(unsigned srt)
{
	CALL("TheoryAxioms::addSomeAdditionAxiom");

	TermList x(0,false);
	TermList y(1,false);
	TermList z(2,false);

	unsigned arg[2] = {srt,srt};
	unsigned bvadd = env.signature->getInterpretingSymbol(Interpretation::BVADD,OperatorType::getFunctionType(2,arg,srt));
	unsigned bvsub = env.signature->getInterpretingSymbol(Interpretation::BVSUB,OperatorType::getFunctionType(2,arg,srt));
	unsigned bvneg = env.signature->getInterpretingSymbol(Interpretation::BVNEG,OperatorType::getFunctionType(2,arg,srt));

	// x + y != z
	TermList xPy(Term::create2(bvadd,x,y));
	Literal* l1 = Literal::createEquality(false,xPy,z,srt);

	// y = z + -(x)
	TermList mx(Term::create1(bvneg,x));
	TermList zPmx(Term::create2(bvadd,z,mx));
	Literal* l2 = Literal::createEquality(true,y,zPmx,srt);

	addTheoryNonUnitClause(l1,l2,CHEAP); // (x + y != z) OR (y = z + -(x))

	// x = z + (-y)
	TermList my(Term::create1(bvneg,y));
	TermList zPMy(Term::create2(bvadd,z,my));
	Literal* l3 = Literal::createEquality(true,x,zPMy,srt);

	addTheoryNonUnitClause(l1,l3,CHEAP); // (x + y != z) OR x = z + (-y)

}

// add that
// x + 1 != x +(-1)
// AND
// x + 1 != x
void TheoryAxioms::addAdditionByOneAxioms(unsigned srt)
{
	CALL("TheoryAxioms::addSomeAdditionAxiom");

	TermList x(0,false);
	TermList y(1,false);
	TermList z(2,false);

	unsigned arg[2] = {srt,srt};
	unsigned bvadd = env.signature->getInterpretingSymbol(Interpretation::BVADD,OperatorType::getFunctionType(2,arg,srt));
	unsigned bvneg = env.signature->getInterpretingSymbol(Interpretation::BVNEG,OperatorType::getFunctionType(1,arg,srt));
	unsigned size = env.sorts->getBitVectorSort(srt)->getSize();

	TermList max(theory->representConstant(BitVectorOperations::getSignedMaxBVCT(size)));
	TermList one(theory->representConstant(BitVectorOperations::getOneBVCT(size)));

	// lhs
	// x + 1
	TermList xP1(Term::create2(bvadd,x,one));
	// -1
	TermList neg1(Term::create1(bvneg,one));
	// x +(-1)
	TermList xPneg1(Term::create2(bvadd,x,neg1));

	Literal* ax = Literal::createEquality(false,xP1,xPneg1,srt);

	addTheoryUnitClause(ax,CHEAP);

	// add that x + 1 != x
	Literal* ax2 = Literal::createEquality(false,xP1,x,srt);
	addTheoryUnitClause(ax2,CHEAP);

}


// add that
// u(u(x)) = x
// for example: !(!x) = x
void TheoryAxioms::addUnaryFunctionAppliedTwiceEqualsArgument(Interpretation f, unsigned srt)
{
	CALL("TheoryAxioms::addUnaryFunctionAppliedTwiceEqualsArgument");

	TermList x(0,false);
	unsigned arg[2] = {srt};

	unsigned bvneg = env.signature->getInterpretingSymbol(f,OperatorType::getFunctionType(1,arg,srt));

	//!x
	TermList nx(Term::create1(bvneg,x));
	//!(!x)
	TermList nnx(Term::create1(bvneg,nx));

	Literal* ax = Literal::createEquality(true,nnx,x,srt);
	addTheoryUnitClause(ax,CHEAP);

}

// P(x constant)
void TheoryAxioms::addSimplePolyMorphicPredicateWithConstantAxiom(unsigned srt, Interpretation p, TermList constant, bool swapArguments, bool polarity, bool commutative)
{
	CALL("TheoryAxioms::addSimplePolyMorphicPredicateWithConstantAxiom");

	unsigned pred = env.signature->getInterpretingSymbol(p,OperatorType::getPredicateType({srt,srt}));
	TermList x(0,false);


	TermList args[2] = { x, constant };
	if(swapArguments) { swap(args[0], args[1]); }


	Literal* ax = Literal::create(pred, 2, polarity, false, args);
	addTheoryUnitClause(ax,CHEAP);
}

/*
 * Add that
 *
 *( p(x tx) && p(s ts) ) -> ( p (concat(x s ) concat (tx ts) ) )
 * !p(x tx) OR !p(s ts) OR ( p (concat(x s ) concat (tx ts) ) )
 * where srt0 is the sort for x and tx
 *  and srt1 is the sort for s and ts
 */
void TheoryAxioms::addConcatArgsPredicateImpliesWholePredicate(Interpretation predicate, unsigned srt0, unsigned srt1, unsigned resultSort)
{
	CALL("TheoryAxioms::addConcatArgsPredicateImpliesWholePredicate");

	TermList x(0,false);
	TermList tx(1,false);
	TermList s(2,false);
	TermList ts(3,false);

	unsigned arg[2] = {srt0,srt1};
	unsigned concat = env.signature->getInterpretingSymbol(Interpretation::CONCAT,OperatorType::getFunctionType(2,arg,resultSort));
	unsigned predX_TX = env.signature->getInterpretingSymbol(predicate,OperatorType::getPredicateType({srt0,srt0}));
	unsigned predS_TS = env.signature->getInterpretingSymbol(predicate,OperatorType::getPredicateType({srt1,srt1}));
    unsigned predConcat = env.signature->getInterpretingSymbol(predicate,OperatorType::getPredicateType({resultSort,resultSort}));

    //!p(x tx)
	Literal* nxPtx = Literal::create2(predX_TX,false,x,tx);
	//!p(s ts)
	Literal* nsPts = Literal::create2(predS_TS,false,s,ts);

	//( p (concat(x s ) concat (tx ts) ) )
	// concat(x s )
	TermList xCs(Term::create2(concat,x,s));
	//concat (tx ts)
	TermList txCts(Term::create2(concat,tx,ts));
	Literal* con = Literal::create2(predConcat,true,xCs,txCts);

	addTheoryNonUnitClause(nxPtx,nsPts,con,CHEAP);

}

/*
 * Add that
 *
 *( p(x tx) && s = ts ) -> ( p (concat(x s ) concat (tx ts) ) )
 *( !p(x tx) OR (s != ts) OR (p (concat(x s ) concat (tx ts)))
 *( where srt0 is the sort for x and tx
 *  and srt1 is the sort for s and ts
 */
void TheoryAxioms::addConcatArgsPredicateImpliesWholePredicateVariation(Interpretation predicate, unsigned srt0, unsigned srt1, unsigned resultSort)
{
	CALL("TheoryAxioms::addConcatArgsPredicateImpliesWholePredicate");

	TermList x(0,false);
	TermList tx(1,false);
	TermList s(2,false);
	TermList ts(3,false);

	unsigned arg[2] = {srt0,srt1};
	unsigned concat = env.signature->getInterpretingSymbol(Interpretation::CONCAT,OperatorType::getFunctionType(2,arg,resultSort));
	unsigned predX_TX = env.signature->getInterpretingSymbol(predicate,OperatorType::getPredicateType({srt0,srt0}));
	unsigned predConcat = env.signature->getInterpretingSymbol(predicate,OperatorType::getPredicateType({resultSort,resultSort}));

	// lhs
	//!p(x tx)
	Literal* nxPtx = Literal::create2(predX_TX,false,x,tx);
	// s != ts
	Literal* sNets = Literal::createEquality(false,s,ts,srt1);

	//( p (concat(x s ) concat (tx ts) ) )
	// concat(x s )
	TermList xCs(Term::create2(concat,x,s));
	//concat (tx ts)
	TermList txCts(Term::create2(concat,tx,ts));
	Literal* con = Literal::create2(predConcat,true,xCs,txCts);

	addTheoryNonUnitClause(nxPtx,sNets,con,CHEAP);

}


/*
 *  Add that
 *  *  (s!=ts OR x!=tx ) <-> ( concat (s x) != concat (ts tx) )
 *
 *  where srt0 is the sort for s and ts
 *  	  srt1 is the sort for x and tx
 *
 *  -> side
 *	(s!=ts OR x!=tx) -> ( concat (s x) != concat (ts tx) )
 *
 *	s=ts OR ( concat (s x) != concat (ts tx) )
 *	AND
 *  x = tx OR ( concat (s x) != concat (ts tx) )
 *
 *  <- side
 *	( concat (s x) != concat (ts tx) ) -> (s!=ts OR x!=tx )
 *	concat (s x) = concat (ts tx) OR s!=ts OR x!=tx
 *

 *
 * */

void TheoryAxioms::addConcatArgumentsNotEqualEquivalentToConcatResultsNotEqual(unsigned srt0, unsigned srt1, unsigned resultSort)
{
	CALL("TheoryAxioms::addConcatArgumentsNotEqualEquivalentToConcatResultsNotEqual");

	TermList x(0,false);
	TermList tx(1,false);
	TermList s(2,false);
	TermList ts(3,false);

	unsigned arg[2] = {srt0,srt1};
	unsigned concat = env.signature->getInterpretingSymbol(Interpretation::CONCAT,OperatorType::getFunctionType(2,arg,resultSort));

	// ------------------------------------
	// -> side
	// s=ts OR ( concat (s x) != concat (ts tx) )
	// AND
	// x = tx OR ( concat (s x) != concat (ts tx) )

	// s=ts
	Literal* sEts = Literal::createEquality(true,s,ts,srt0);
	// x=tx
	Literal* xEtx = Literal::createEquality(true,x,tx,srt1);

	//concat (s x)
	TermList xCs(Term::create2(concat,s,x));
	//concat (ts tx)
	TermList txCts(Term::create2(concat,ts,tx));
	//concat (s x) != concat (ts tx)
	Literal* ncon = Literal::createEquality(false,xCs,txCts,resultSort);

	// s=ts OR ( concat (s x) != concat (ts tx) )
	addTheoryNonUnitClause(sEts,ncon,CHEAP);
	// x = tx OR ( concat (s x) != concat (ts tx) )
	addTheoryNonUnitClause(xEtx,ncon,CHEAP);

	// ------------------------------------
	// <- side
	// concat (s x) = concat (ts tx) OR s!=ts OR x!=tx

	// s!=ts
	Literal* sNEts = Literal::createEquality(false,s,ts,srt0);
	// x!=tx
	Literal* xNEtx = Literal::createEquality(false,x,tx,srt1);

	//concat (s x) = concat (ts tx)
	Literal* con = Literal::createEquality(true,xCs,txCts,resultSort);
	addTheoryNonUnitClause(sNEts,xNEtx,con,CHEAP);

	// ------------------------------------
}


// P(x y) OR Q(x y)
void TheoryAxioms::addPolyMorphicClauseAxiom(unsigned srt, Interpretation p1, bool swapArguments1, bool polarity1, Interpretation p2, bool swapArguments2, bool polarity2)
{
	CALL("TheoryAxioms::addPolyMorphicClauseAxiom");

	unsigned pred1 = env.signature->getInterpretingSymbol(p1,OperatorType::getPredicateType({srt,srt}));
	unsigned pred2 = env.signature->getInterpretingSymbol(p2,OperatorType::getPredicateType({srt,srt}));

	TermList x(0,false);
	TermList y(1,false);

	TermList args[2] = { x, y };
	if(swapArguments1) { swap(args[0], args[1]); }
	Literal* l1 = Literal::create(pred1, 2, polarity1, false, args);

	TermList args2[2] = { x, y };
	if(swapArguments2) { swap(args2[0], args2[1]); }
	Literal* l2 = Literal::create(pred2, 2, polarity2, false, args2);

	addTheoryNonUnitClause(l1,l2,CHEAP);
}

// add
// P(x,c)
void TheoryAxioms::addPolyMorphicLiteralWithConstantAxiom(unsigned srt, Interpretation pred1, TermList constant,bool swapArguments, bool polarity)
{
	unsigned pred = env.signature->getInterpretingSymbol(pred1,OperatorType::getPredicateType({srt,srt}));

	TermList x(0,false);

	TermList args[2] = { x, constant };
	if(swapArguments) { swap(args[0], args[1]); }
	Literal* l1 = Literal::create(pred, 2, polarity, false, args);

	addTheoryUnitClause(l1,CHEAP);
}


// add that x = y -> p(x y)
// cnf : x != y OR p(x y)
void TheoryAxioms::addEqualsImpliesBinaryPredicate(Interpretation itp, unsigned srt)
{
	CALL("addEqualsImpliesBinaryPredicate(Interpretation itp, unsigned srt)");
	TermList x(0,false);
	TermList y(1,false);

	unsigned arg[2] = {srt,srt};
	unsigned pred = env.signature->getInterpretingSymbol(itp,OperatorType::getPredicateType({srt,srt}));

	Literal* xNey = Literal::createEquality(false,x,y,srt);
	Literal* xPy = Literal::create2(pred,true,x,y);

	addTheoryNonUnitClause(xNey,xPy,CHEAP);

}



/* Add that
 * p(f(x,y),x)
 *
 * e.g. bvule(bvand(x,y),x)
 * */
void TheoryAxioms::predicateTrueForArgumentsOfAFunction(unsigned srt, Interpretation func, Interpretation pred)
{
	CALL("TheoryAxioms::predicateTrueForArgumentsOfAFunction");

	TermList x(0,false);
	TermList y(1,false);

	unsigned f = env.signature->getInterpretingSymbol(func,OperatorType::getFunctionType({srt,srt},srt));
	unsigned p = env.signature->getInterpretingSymbol(pred,OperatorType::getPredicateType({srt,srt}));

	// f(x,y)
	TermList fxy(Term::create2(f, x, y));
	//p(f(x,y),x)
	Literal* pfxyx = Literal::create2(p, true,x,fxy);

	//p(f(x,y),y)
	Literal* pfxyy = Literal::create2(p, true,y,fxy);

	addTheoryUnitClause(pfxyx,CHEAP);

}


/* Add that x!=c -> p(c,x)
 * 	x=c OR p(c,x)
 * for example
 * x!= signedMax -> bvsgt(signedMax x)
 * */

void TheoryAxioms::addXNEqualToConstantImpliesAxiom(unsigned srt, Interpretation predicate, TermList constant, bool swapArguments)
{
	CALL("TheoryAxioms::addXNEqualToConstantImpliesAxiom");

	TermList x(0,false);
	unsigned size = env.sorts->getBitVectorSort(srt)->getSize();

	//x=MAX
	Literal* l1 = Literal::createEquality(true,x,constant,srt);

	unsigned p = env.signature->getInterpretingSymbol(predicate,OperatorType::getPredicateType({srt,srt}));

	//p(c,x)
	TermList args[2] = { constant, x };
	if(swapArguments) { swap(args[0], args[1]); }
	Literal* l2 = Literal::create(p, 2, true,false,args);
	addTheoryNonUnitClause(l1,l2,CHEAP);
}

/* Add that
 *  p(f(s, x), t) -> p(s,t)
 *  !p(f(s, x), t) OR p(s t)
 * !bvugt (bvlshr (s x) t) OR bvugt(s t)
 *  for example
 *  bvult (bvor(s x) t) -> bvult(s t)
 *
 * */

void TheoryAxioms::addTempOrAxiom2(unsigned srt, Interpretation pred,  Interpretation func)
{
	CALL("TheoryAxioms::addTempOrAxiom2");

	TermList x(0,false);
	TermList s(1,false);
	TermList t(2,false);

	unsigned p = env.signature->getInterpretingSymbol(pred,OperatorType::getPredicateType({srt,srt}));
	unsigned f = env.signature->getInterpretingSymbol(func,OperatorType::getFunctionType({srt,srt},srt));

	// !p(f(x, s), t)
	TermList fxs(Term::create2(f,s,x));
	Literal* l1 = Literal::create2(p, false,fxs,t);
	Literal* l2 = Literal::create2(p, true,s,t);

	addTheoryNonUnitClause(l1,l2,CHEAP);

}


/* Add that
 * p(s t) -> p(f(s c) t)
 * !p(s t) OR p(f(s c) t)
 * */

void TheoryAxioms::addOtherBVANDSignedPredicatesAxiom(unsigned srt, Interpretation pred, Interpretation func,
  		  TermList constant)
{
	CALL("TheoryAxioms::addOtherBVANDSignedPredicatesAxiom");

	TermList s(0,false);
	TermList t(1,false);

	unsigned p = env.signature->getInterpretingSymbol(pred,OperatorType::getPredicateType({srt,srt}));
	unsigned f = env.signature->getInterpretingSymbol(func,OperatorType::getFunctionType({srt,srt},srt));

	// !p(s t)
	Literal* nPst = Literal::create2(p, false,s,t);

	//p(f(s c) t)
	TermList fsc(Term::create2(f,s,constant));
	Literal* l = Literal::create2(p, true,fsc,t);

	addTheoryNonUnitClause(nPst,l,CHEAP);

}

/* Add that
 * f(x s) = t -> f(t s) = t
 * f(x s) != t OR f(t s) = t
 * for example
 * x & s = t -> t & s = t
 *
 * */
void TheoryAxioms::addSpecialEqualAndAxiom(unsigned srt, Interpretation func)
{
	CALL("TheoryAxioms::addSpecialEqualAndAxiom");

	TermList x(0,false);
	TermList s(1,false);
	TermList t(2,false);

	unsigned f = env.signature->getInterpretingSymbol(func,OperatorType::getFunctionType({srt,srt},srt));


	//f(x s) != t
	TermList fxs(Term::create2(f,x,s));
	Literal* l1 = Literal::createEquality(false,fxs,t,srt);

	//f(t s) = t
	TermList fts(Term::create2(f,t,s));
	Literal* l2 = Literal::createEquality(true,fts,t,srt);

	addTheoryNonUnitClause(l1,l2,CHEAP);

}


/* Add that
 * f(x s) = t -> f( g(t s) s) = t
 * f(x s) != t OR f( g(t s) s) = t
 * for example (logical shift)
 *
 * x << s = t -> (t >> s) << s = t
 * */

void TheoryAxioms::addShiftingAxiom(unsigned srt, Interpretation func1, Interpretation func2)
{
	CALL("TheoryAxioms::addShiftingAxiom");

	TermList x(0,false);
	TermList s(1,false);
	TermList t(2,false);

	unsigned f = env.signature->getInterpretingSymbol(func1,OperatorType::getFunctionType({srt,srt},srt));
	unsigned g = env.signature->getInterpretingSymbol(func2,OperatorType::getFunctionType({srt,srt},srt));

	//f(x s) != t
	TermList fxs(Term::create2(f,x,s));
	Literal* l1 = Literal::createEquality(false,fxs,t,srt);

	//f( g(t s) s) = t
	TermList gts(Term::create2(g,t,s));
	TermList fgtss(Term::create2(f,gts,s));
	Literal* l2 = Literal::createEquality(true,fgtss,t,srt);

	addTheoryNonUnitClause(l1,l2,CHEAP);

}


/* Add that
 * p( f(X S) T) -> p( f(S c) T)
 * ! p( f(X S) T) OR p( f(S c) T)
 * for example (signed)
 *
 * (x || s > t) -> (s || max > t)
 *
 * */

void TheoryAxioms::addORSignedOperatorWithConstantAxiom(unsigned srt, Interpretation pred, Interpretation func,TermList constant)
{
	CALL("TheoryAxioms::addORSignedOperatorWithConstantAxiom");

	TermList x(0,false);
	TermList s(1,false);
	TermList t(2,false);

	unsigned p = env.signature->getInterpretingSymbol(pred,OperatorType::getPredicateType({srt,srt}));
	unsigned f = env.signature->getInterpretingSymbol(func,OperatorType::getFunctionType({srt,srt},srt));


	// !p( f(X S) T)
	TermList xs(Term::create2(f,x,s));
	Literal* l1 = Literal::create2(p,false,xs,t);

	//p( f(S c) T)
	TermList fsc(Term::create2(f,s,constant));
	Literal* l2 = Literal::create2(p,true,fsc,t);

	addTheoryNonUnitClause(l1,l2,CHEAP);

}



/* Add that
 * x!=0 -> (0/x) = 0
 * x=0 OR (0/x) = 0
 * */
void TheoryAxioms::addDivisionZeroAxiom(unsigned srt)
{
	CALL("TheoryAxioms::addDivisionZeroAxiom");
	unsigned size = env.sorts->getBitVectorSort(srt)->getSize();
	TermList x(0,false);
	TermList zero(theory->representConstant(BitVectorOperations::getZeroBVCT(size)));

	unsigned bvudiv = env.signature->getInterpretingSymbol(Theory::BVUDIV,OperatorType::getFunctionType({srt,srt},srt));

	Literal* l1 = Literal::createEquality(true,x,zero,srt);

	TermList zDx(Term::create2(bvudiv,zero,x));
	Literal* l2 = Literal::createEquality(true,zDx,zero,srt);

	addTheoryNonUnitClause(l1,l2,CHEAP);
}

/* Add that
 *  x != 0 -> x/1 = x
 *  x = 0 OR x/1 = x
 * */
void TheoryAxioms::addDivisionOneAxiom(unsigned srt)
{
	CALL("TheoryAxioms::addDivisionOneAxiom");
	unsigned size = env.sorts->getBitVectorSort(srt)->getSize();
	TermList x(0,false);
	TermList one(theory->representConstant(BitVectorOperations::getOneBVCT(size)));
	TermList zero(theory->representConstant(BitVectorOperations::getZeroBVCT(size)));

	unsigned bvudiv = env.signature->getInterpretingSymbol(Theory::BVUDIV,OperatorType::getFunctionType({srt,srt},srt));

	Literal* l1 = Literal::createEquality(true,x,zero,srt);

	TermList xDo(Term::create2(bvudiv,x,one));
	Literal* l2 = Literal::createEquality(true,xDo,x,srt);

	addTheoryNonUnitClause(l1,l2,CHEAP);
}

/* bvugt(b a) -> a/b = 0
 * !bvugt(b a) OR a/b = 0
 *
 * */

void TheoryAxioms::addAnotherDivisionAxiom(unsigned srt)
{
	CALL("TheoryAxioms::addAnotherAxiom");
	unsigned size = env.sorts->getBitVectorSort(srt)->getSize();

	TermList a(0,false);
	TermList b(1,false);
	TermList zero(theory->representConstant(BitVectorOperations::getZeroBVCT(size)));

	unsigned bvugt = env.signature->getInterpretingSymbol(Theory::BVUGT,OperatorType::getPredicateType({srt,srt}));
	unsigned bvudiv = env.signature->getInterpretingSymbol(Theory::BVUDIV,OperatorType::getFunctionType({srt,srt},srt));

	Literal* l1 = Literal::create2(bvugt,false,b,a);

	TermList aDb(Term::create2(bvudiv,a,b));
	Literal* l2 = Literal::createEquality(true,aDb,zero,srt);

	addTheoryNonUnitClause(l1,l2,CHEAP);

}

/* Add that
 * x!=0 -> x/x=1
 * x=0 OR x/x=1
 *
 * */

void TheoryAxioms::addDivisionSameArgAxiom(unsigned srt)
{
	CALL("TheoryAxioms::addDivisionSameArgAxiom");
	unsigned size = env.sorts->getBitVectorSort(srt)->getSize();

	TermList x(0,false);
	TermList zero(theory->representConstant(BitVectorOperations::getZeroBVCT(size)));
	TermList one(theory->representConstant(BitVectorOperations::getOneBVCT(size)));

	unsigned bvudiv = env.signature->getInterpretingSymbol(Theory::BVUDIV,OperatorType::getFunctionType({srt,srt},srt));

	Literal* l1 = Literal::createEquality(true,x,zero,srt);

	TermList xDx(Term::create2(bvudiv,x,x));
	Literal* l2 = Literal::createEquality(true,xDx,one,srt);

	addTheoryNonUnitClause(l1,l2,CHEAP);

}

/* Add that
 *
 * (t>s) -> (s/t = 0)
 * !(t>s) OR (s/t = 0)
 *
 * */
void TheoryAxioms::addDivAxiomGT(unsigned srt)
{
	CALL("TheoryAxioms::addConfidentAxiomWow");

	unsigned bvudiv = env.signature->getInterpretingSymbol(Theory::BVUDIV,OperatorType::getFunctionType({srt,srt},srt));
	unsigned bvugt = env.signature->getInterpretingSymbol(Theory::BVUGT,OperatorType::getPredicateType({srt,srt}));
	unsigned size = env.sorts->getBitVectorSort(srt)->getSize();

	TermList t(0,false);
	TermList s(1,false);
	TermList zero(theory->representConstant(BitVectorOperations::getZeroBVCT(size)));

	Literal* l1 = Literal::create2(bvugt,false,t,s);

	TermList sDivt(Term::create2(bvudiv,s,t));
	Literal* l2 = Literal::createEquality(true,sDivt,zero,srt);

	addTheoryNonUnitClause(l1,l2,CHEAP);

}

/* Add that
 * (t=s & t!=0) -> s/t = 1
 * t!=s OR t=0 OR s/t = 1
 *
 * */
void TheoryAxioms::addDivONEAxiom(unsigned srt)
{
	CALL("TheoryAxioms::addDivONEAxiom(unsigned srt)");

	unsigned bvudiv = env.signature->getInterpretingSymbol(Theory::BVUDIV,OperatorType::getFunctionType({srt,srt},srt));
	unsigned size = env.sorts->getBitVectorSort(srt)->getSize();

	TermList t(0,false);
	TermList s(1,false);
	TermList zero(theory->representConstant(BitVectorOperations::getZeroBVCT(size)));
	TermList one(theory->representConstant(BitVectorOperations::getOneBVCT(size)));

	Literal* l1 = Literal::createEquality(false,s,t,srt);
	Literal* l2 = Literal::createEquality(true,t,zero,srt);

	TermList sDivT(Term::create2(bvudiv,s,t));
	Literal* l3 = Literal::createEquality(true,sDivT,one,srt);

	addTheoryNonUnitClause(l1,l2,l3,CHEAP);


}
/* Add that (unsigned)
 * (s>t) -> (s/t > 0)
 * !(s>t) OR (s/t > 0)
 *
 * */
void TheoryAxioms::addDivAxiomGT2(unsigned srt)
{
	CALL("addDivAxiomGT2(unsigned srt)");

	unsigned bvudiv = env.signature->getInterpretingSymbol(Theory::BVUDIV,OperatorType::getFunctionType({srt,srt},srt));
	unsigned bvugt = env.signature->getInterpretingSymbol(Theory::BVUGT,OperatorType::getPredicateType({srt,srt}));
	unsigned size = env.sorts->getBitVectorSort(srt)->getSize();

	TermList s(0,false);
	TermList t(1,false);

	TermList zero(theory->representConstant(BitVectorOperations::getZeroBVCT(size)));

	Literal* l1 = Literal::create2(bvugt,false,s,t);

	TermList sDt(Term::create2(bvudiv,s,t));
	Literal* l2 = Literal::create2(bvugt,true,sDt,zero);

	addTheoryNonUnitClause(l1,l2,CHEAP);

}


/* Add that
 * s/x = t -> (s/(s/t)) = t
 * s/x != t OR (s/(s/t)) = t
 * */
void TheoryAxioms::addTempAxiom(unsigned srt)
{
	CALL("TheoryAxioms::addDivGE(unsigned srt)");

	unsigned bvuge = env.signature->getInterpretingSymbol(Theory::BVUGE,OperatorType::getPredicateType({srt,srt}));
	unsigned bvudiv = env.signature->getInterpretingSymbol(Theory::BVUDIV,OperatorType::getFunctionType({srt,srt},srt));
	unsigned bvmul = env.signature->getInterpretingSymbol(Theory::BVMUL,OperatorType::getFunctionType({srt,srt},srt));
	unsigned size = env.sorts->getBitVectorSort(srt)->getSize();
	TermList s(0,false);
	TermList x(1,false);
	TermList t(2,false);

	TermList sDx(Term::create2(bvudiv,s,x));
	Literal* l1 = Literal::createEquality(false,sDx,t,srt);

	TermList sDt(Term::create2(bvudiv,s,t));
	TermList sDsDt(Term::create2(bvudiv,s,sDt));
	Literal* l2 = Literal::createEquality(true,sDsDt,t,srt);

	addTheoryNonUnitClause(l1,l2,CHEAP);
}

// x!=MAX -> bvugt(x+1 x)
// x=MAX OR bvugt(x+1 x)
void TheoryAxioms::addMaxAxiom(Interpretation p, unsigned srt)
{
	CALL("TheoryAxioms::addMaxAxiom(Interpretation p, unsigned srt)");

	TermList x(0,false);
	unsigned size = env.sorts->getBitVectorSort(srt)->getSize();
	TermList max(theory->representConstant(BitVectorOperations::getAllOnesBVCT(size)));
	TermList one(theory->representConstant(BitVectorOperations::getOneBVCT(size)));

	//x=MAX
	Literal* l1 = Literal::createEquality(true,x,max,srt);

    unsigned bvugt = env.signature->getInterpretingSymbol(p,OperatorType::getPredicateType({srt,srt}));
    unsigned bvadd = env.signature->getInterpretingSymbol(Theory::BVADD,OperatorType::getFunctionType({srt,srt},srt));
    // x+1
    TermList xp1(Term::create2(bvadd,x,one));
    // x+1 > x
    Literal* l2 = Literal::create2(bvugt, true,xp1,x);

    addTheoryNonUnitClause(l1,l2,CHEAP);

}

// f(X,Y) = u(b(X,Y))
void TheoryAxioms::addPolyMorphicBinaryFunctionEquivalentToUnaryFunctionAppliedToBinaryFunction(Interpretation f_i, Interpretation unary_i, Interpretation binary_i, unsigned size)
{
  CALL("TheoryAxioms::addPolyMorphicBinaryFunctionEquivalentToUnaryFunctionAppliedToBinaryFunction");

    unsigned srt = env.sorts->addBitVectorSort(size);
    unsigned argSorts[2] = {srt,srt};
    
    unsigned f = env.signature->getInterpretingSymbol(f_i,OperatorType::getFunctionType(2, argSorts,srt));
    unsigned unary = env.signature->getInterpretingSymbol(unary_i,OperatorType::getFunctionType(1, argSorts,srt));
    unsigned binary = env.signature->getInterpretingSymbol(binary_i,OperatorType::getFunctionType(2, argSorts,srt));
    
    TermList s(0,false);
    TermList t(1,false);
    
    //(bvnand s t) 
    TermList bvnand_s_t(Term::create2(f,s,t));
    
    //(bvand s t)
    TermList bvand_s_t(Term::create2(binary,s,t));
    //(bvnot (bvand s t))
    TermList bvnot_bvand_s_t(Term::create1(unary,bvand_s_t));
    
    Literal* eq1 = Literal::createEquality(true,bvnand_s_t,bvnot_bvand_s_t,srt);
    addTheoryUnitClause(eq1, EXPENSIVE);
}




// f(X,c) = X
// e.g. bvadd a 0 = a
void TheoryAxioms::addBitVectorRightIdentity(Interpretation f_i, TermList neutralElement, unsigned size)
{
    CALL("TheoryAxioms::addBitVectorRightIdentity");
    ASS(theory->isFunction(f_i));
    ASS_EQ(theory->getArity(f_i),2);
    unsigned srt = env.sorts->addBitVectorSort(size);
    unsigned  arg[2] = {srt,srt};
    
    unsigned f = env.signature->getInterpretingSymbol(f_i,OperatorType::getFunctionType(2, arg,srt));
    // f(X,c) = X
    TermList x(0,false);
    TermList x_f_Neutral(Term::create2(f, x, neutralElement));
    Literal* r = Literal::createEquality(true, x_f_Neutral, x, srt);
    addTheoryUnitClause(r, CHEAP);
}


// f(X,Y) = b(X,u(Y))
//(bvsub x y) abbreviates (bvadd x (bvneg y))
void TheoryAxioms::addPolyMorphicBinaryFunctionEquivalentToBinaryFunctionAppliedToUnaryFunction(Interpretation op, Interpretation binary, Interpretation unary, unsigned size)
{
  CALL("TheoryAxioms::addPolyMorphicBinaryFunctionEquivalentToBinaryFunctionAppliedToUnaryFunction");

    unsigned srt = env.sorts->addBitVectorSort(size);
    unsigned int argSorts[2] = {srt,srt};
    
    unsigned f = env.signature->getInterpretingSymbol(op,OperatorType::getFunctionType(2,argSorts,srt));
    unsigned b = env.signature->getInterpretingSymbol(binary,OperatorType::getFunctionType(2,argSorts,srt));
    unsigned u = env.signature->getInterpretingSymbol(unary,OperatorType::getFunctionType(1,argSorts,srt));
    
    TermList x(0,false);
    TermList y(1,false);
    
    //lhs
    // f(X,Y)
    //(bvsub X Y) 
    TermList f_x_y(Term::create2(f,x,y));
    
    //rhs
    // u(Y)
    //(bvneg Y)
    TermList u_y(Term::create1(u,y));
    // b(X,u(Y))
    // (bvadd s (bvneg t))
    TermList b_x_u_y(Term::create2(b,x,u_y));
    
    Literal* eq1 = Literal::createEquality(true,f_x_y,b_x_u_y,srt);
    addTheoryUnitClause(eq1,EXPENSIVE);
}
  
/*
 * Add that
 * p ( f(x y) x )
 * */

void TheoryAxioms::addBVUREMwithPredicateAxiom(Interpretation f, Interpretation p, unsigned srt)
{
	CALL("TheoryAxioms::addBVUREMwithPredicateAxiom");

	TermList x(0,false);
	TermList y(1,false);

	unsigned argSorts[2] = {srt,srt};
	unsigned fun = env.signature->getInterpretingSymbol(f,OperatorType::getFunctionType(2,argSorts,srt));
	unsigned pred = env.signature->getInterpretingSymbol(p,OperatorType::getPredicateType({srt,srt}));

	// f (x y )
	TermList fxy(Term::create2(fun,x,y));
	Literal* pfxy_x = Literal::create2(pred,true,fxy,x);
	addTheoryUnitClause(pfxy_x, CHEAP);

}


/* Add that
 * f(x x) = c
 * */
void TheoryAxioms::addFunctionWithSameArgumentEqualsConstant(Interpretation f, TermList constant, unsigned srt)
{
	CALL("TheoryAxioms::addFunctionWithSameArgumentEqualsConstant");

	TermList x(0,false);
	unsigned argSorts[2] = {srt,srt};
	unsigned fun = env.signature->getInterpretingSymbol(f,OperatorType::getFunctionType(2,argSorts,srt));

	TermList fxx(Term::create2(fun,x,x));

	Literal* eq1 = Literal::createEquality(true,fxx,constant,srt);
	addTheoryUnitClause(eq1, CHEAP);

}


/* Add that
 * p(f(c X) c)
 *
 * */
void TheoryAxioms::addFunctionAppliedToConstantPredicateFirstArgVariation(Interpretation f, Interpretation p, TermList constant,unsigned srt)
{
	CALL("TheoryAxioms::addFunctionAppliedToConstantPredicateFirstArg");

	TermList x(0,false);


	unsigned argSorts[2] = {srt,srt};
	unsigned fun = env.signature->getInterpretingSymbol(f,OperatorType::getFunctionType(2,argSorts,srt));
	unsigned pred = env.signature->getInterpretingSymbol(p,OperatorType::getPredicateType({srt,srt}));

	// f (c X )
	TermList fcx(Term::create2(fun,constant,x));
	Literal* pfxy_x = Literal::create2(pred,true,fcx,constant);
	addTheoryUnitClause(pfxy_x, CHEAP);

}

/* Add that
 * f(x x) = x
 *
 * */

void TheoryAxioms::addFunctionWithSameArgumentEqualArgument(Interpretation f, unsigned srt)
{
	CALL("TheoryAxioms::addFunctionWithSameArgumentEqualArgument");
	TermList x(0,false);
	unsigned argSorts[2] = {srt,srt};
	unsigned fun = env.signature->getInterpretingSymbol(f,OperatorType::getFunctionType(2,argSorts,srt));

	TermList fxx(Term::create2(fun,x,x));

	Literal* eq1 = Literal::createEquality(true,fxx,x,srt);
	addTheoryUnitClause(eq1, CHEAP);
}

// f(X,c) = d
// e.g (bvudiv x zero) = allones 
void TheoryAxioms::addPolyMorphicSpecialConstantAxiom(Interpretation op, TermList arg, TermList out,unsigned size)
{
	CALL("TheoryAxioms::addPolyMorphicSpecialConstantAxiom");

    unsigned srt = env.sorts->addBitVectorSort(size);
    unsigned argSorts[2] = {srt,srt};
    
    unsigned f = env.signature->getInterpretingSymbol(op,OperatorType::getFunctionType(2,argSorts,srt));
    
    TermList x(0,false);
    
    // f(X,c) = d
    //(bvudiv x zero) = allones 
    TermList f_x_arg(Term::create2(f,x,arg));
    
    Literal* eq1 = Literal::createEquality(true,f_x_arg,out,srt);
    addTheoryUnitClause(eq1, CHEAP);
    
}

// f(c,X) = d
// e.g (bvashr zero X) = zero
void TheoryAxioms::addPolyMorphicSpecialConstantAxiomVariation(Interpretation op, TermList arg, TermList out,unsigned size)
{
	CALL("TheoryAxioms::addPolyMorphicSpecialConstantAxiom");

    unsigned srt = env.sorts->addBitVectorSort(size);
    unsigned argSorts[2] = {srt,srt};

    unsigned f = env.signature->getInterpretingSymbol(op,OperatorType::getFunctionType(2,argSorts,srt));

    TermList x(0,false);

    // f(c,X) = d
    TermList f_x_arg(Term::create2(f,arg,x));

    Literal* eq1 = Literal::createEquality(true,f_x_arg,out,srt);
    addTheoryUnitClause(eq1, CHEAP);

}
void TheoryAxioms::addBVXNORAxiom1(Interpretation bvxnor , Interpretation bvor , Interpretation bvand, Interpretation bvnot, unsigned size)
{
  CALL("TheoryAxioms::addBVXNORAxiom1");

    unsigned srt = env.sorts->addBitVectorSort(size);
    unsigned argSorts[2] = {srt,srt};
    
    //(bvxnor s t) abbreviates (bvor (bvand s t) (bvand (bvnot s) (bvnot t)))
    unsigned _xnor = env.signature->getInterpretingSymbol(bvxnor,OperatorType::getFunctionType(2,argSorts,srt));
    unsigned _or = env.signature->getInterpretingSymbol(bvor,OperatorType::getFunctionType(2,argSorts,srt));
    unsigned _not = env.signature->getInterpretingSymbol(bvnot,OperatorType::getFunctionType(1,argSorts,srt));
    unsigned _and = env.signature->getInterpretingSymbol(bvand,OperatorType::getFunctionType(2,argSorts,srt));
    
    
    TermList s(0,false);
    TermList t(1,false);
    
    //lhs
    //(bvxnor s t) 
    TermList bvxnor_s_t(Term::create2(_xnor,s,t));
    
    // rhs
    //(bvnot t)
    TermList bvnot_t(Term::create1(_not,t));
    //(bvnot s)
    TermList bvnot_s(Term::create1(_not,s));
    //(bvand (bvnot s) (bvnot t))
    TermList bvand_bvnot_s_bvnot_t(Term::create2(_and,bvnot_t,bvnot_s));
    
    //(bvand s t)
    TermList bvand_s_t(Term::create2(_and,s,t));
    
    //rhs 
    //(bvor (bvand s t) (bvand (bvnot s) (bvnot t))
    TermList rhs(Term::create2(_or,bvand_s_t,bvand_bvnot_s_bvnot_t));
    
    Literal* eq1 = Literal::createEquality(true,bvxnor_s_t,rhs,srt);
    addTheoryUnitClause(eq1, EXPENSIVE);
    
}

void TheoryAxioms::addBVXORAxiom1(Interpretation bvxor, Interpretation bvor , Interpretation bvand, Interpretation bvnot, unsigned size)
{
  CALL("TheoryAxioms::addBVXORAxiom1");

    unsigned srt = env.sorts->addBitVectorSort(size);
    unsigned argSorts[2] = {srt,srt};
    
    //(bvxor s t) abbreviates (bvor (bvand s (bvnot t)) (bvand (bvnot s) t))
    unsigned _xor = env.signature->getInterpretingSymbol(bvxor,OperatorType::getFunctionType(2,argSorts,srt));
    unsigned _or = env.signature->getInterpretingSymbol(bvor,OperatorType::getFunctionType(2,argSorts,srt));
    unsigned _not = env.signature->getInterpretingSymbol(bvnot,OperatorType::getFunctionType(1,argSorts,srt));
    unsigned _and = env.signature->getInterpretingSymbol(bvand,OperatorType::getFunctionType(2,argSorts,srt));
    

    TermList s(0,false);
    TermList t(1,false);
    
    //(bvxor s t)
    TermList bvxor_s_t(Term::create2(_xor,s,t));
    
    
    // (bvnot s)
    TermList bvnot_s(Term::create1(_not,s));
    // (bvand (bvnot s) t))
    TermList bvand_bvnot_s_t(Term::create2(_and,bvnot_s, t));
    
    // (bvnot t)
    TermList bvnot_t(Term::create1(_not,t));
    // (bvand s (bvnot t)
    TermList bvand_s_bvnot_t(Term::create2(_and,s, bvnot_t));
    //(bvor (bvand s (bvnot t)) (bvand (bvnot s) t))
    TermList rhs(Term::create2(_or,bvand_bvnot_s_t, bvand_s_bvnot_t));
    
    
    Literal* eq1 = Literal::createEquality(true,bvxor_s_t,rhs,srt);
    addTheoryUnitClause(eq1, EXPENSIVE);
    
}

/**
 * Add axioms for addition, multiplication, unary minus and ordering
 */
void TheoryAxioms::addAdditionOrderingAndMultiplicationAxioms(Interpretation plus, Interpretation unaryMinus,
    TermList zeroElement, TermList oneElement, Interpretation less, Interpretation multiply)
{
  CALL("TheoryAxioms::addAdditionOrderingAndMultiplicationAxioms");

  unsigned srt = theory->getOperationSort(plus);
  ASS_EQ(srt, theory->getOperationSort(unaryMinus));
  ASS_EQ(srt, theory->getOperationSort(less));
  ASS_EQ(srt, theory->getOperationSort(multiply));

  addAdditionAndOrderingAxioms(plus, unaryMinus, zeroElement, oneElement, less);

  addCommutativity(multiply);
  addAssociativity(multiply);
  addRightIdentity(multiply, oneElement);

  //axiom( X0*zero==zero );
  unsigned mulFun = env.signature->getInterpretingSymbol(multiply);
  TermList x(0,false);
  TermList xMulZero(Term::create2(mulFun, x, zeroElement));
  Literal* xEqXMulZero = Literal::createEquality(true, xMulZero, zeroElement, srt);
  addTheoryUnitClause(xEqXMulZero,EXPENSIVE);

  // Distributivity
  //axiom x*(y+z) = (x*y)+(x*z)

  unsigned plusFun = env.signature->getInterpretingSymbol(plus);
  TermList y(1,false);
  TermList z(2,false);

  TermList yPz(Term::create2(plusFun,y,z));
  TermList xTyPz(Term::create2(mulFun,x,yPz));

  TermList xTy(Term::create2(mulFun,x,y));
  TermList xTz(Term::create2(mulFun,x,z));
  TermList xTyPxTz(Term::create2(plusFun,xTy,xTz));
  
  Literal* distrib = Literal::createEquality(true, xTyPz, xTyPxTz,srt);
  addTheoryUnitClause(distrib, EXPENSIVE);

  // Divisibility
  // (x != 0 & x times z = y & x times w = y) -> z = w
  // x=0 | x*z != y | x*w != y | z=w
  TermList w(3,false);
  Literal* xEz = Literal::createEquality(true,x,zeroElement,srt);
  TermList xTw(Term::create2(mulFun,x,w));
  Literal* xTznEy = Literal::createEquality(false,xTz,y,srt); 
  Literal* xTwnEy = Literal::createEquality(false,xTw,y,srt); 
  Literal* zEw = Literal::createEquality(true,z,w,srt);

  addTheoryNonUnitClause(xEz,xTznEy,xTwnEy,zEw,EXPENSIVE);
  
}

/**
 * Add axioms for integer division
 * Also modulo and abs functions
 */
void TheoryAxioms::addIntegerDivisionWithModuloAxioms(Interpretation plus, Interpretation unaryMinus, Interpretation less,
                                Interpretation multiply, Interpretation divide, Interpretation divides,
                                Interpretation modulo, Interpretation abs, TermList zeroElement,
                                TermList oneElement)
{
  CALL("TheoryAxioms::addIntegerDivisionWithModuloAxioms");


  unsigned srt = theory->getOperationSort(plus);
  ASS_EQ(srt, theory->getOperationSort(unaryMinus));
  ASS_EQ(srt, theory->getOperationSort(less));
  ASS_EQ(srt, theory->getOperationSort(multiply));
  ASS_EQ(srt, theory->getOperationSort(divide));
  ASS_EQ(srt, theory->getOperationSort(divides));
  ASS_EQ(srt, theory->getOperationSort(modulo));
  ASS_EQ(srt, theory->getOperationSort(abs));

  unsigned lessPred = env.signature->getInterpretingSymbol(less);
  unsigned umFun = env.signature->getInterpretingSymbol(unaryMinus);
  unsigned mulFun = env.signature->getInterpretingSymbol(multiply);
  unsigned divFun = env.signature->getInterpretingSymbol(divide);
  unsigned modFun = env.signature->getInterpretingSymbol(modulo);
  unsigned absFun = env.signature->getInterpretingSymbol(abs);
  unsigned plusFun = env.signature->getInterpretingSymbol(plus);

  addIntegerAbsAxioms(abs,less,unaryMinus,zeroElement);

  TermList x(1,false);
  TermList y(2,false);

  // divides
  //TODO

  Literal* yis0 = Literal::createEquality(true,y,zeroElement,srt);
  TermList modxy(Term::create2(modFun,x,y));

  //y!=0 => x = modulo(x,y) +  multiply(y,div(x,y))

  TermList divxy(Term::create2(divFun,x,y));
  TermList mulydivxy(Term::create2(mulFun,y,divxy));
  TermList sum(Term::create2(plusFun,modxy,mulydivxy));
  Literal* xeqsum = Literal::createEquality(true,x,sum,srt);
  addTheoryNonUnitClause(yis0,xeqsum,EXPENSIVE);

  // y!=0 => (0 <= mod(x,y))
  // y=0 | ~(mod(x,y) < 0)
  Literal* modxyge0 = Literal::create2(lessPred,false,modxy,zeroElement);
  addTheoryNonUnitClause(yis0,modxyge0,EXPENSIVE);

  // y!=0 => (mod(x,y) <= abs(y)-1)
  // y=0 | ~( abs(y)-1 < mod(x,y) )
  TermList absy(Term::create1(absFun,y));
  TermList m1(Term::create1(umFun,oneElement));
  TermList absym1(Term::create2(plusFun,absy,m1));
  Literal* modxyleabsym1 = Literal::create2(lessPred,false,absym1,modxy);
  addTheoryNonUnitClause(yis0,modxyleabsym1,EXPENSIVE);

}

void TheoryAxioms::addIntegerDividesAxioms(Interpretation divides, Interpretation multiply, TermList zero, TermList n)
{
  CALL("TheoryAxioms::addIntegerDividesAxioms");

#if VDEBUG
  // ASSERT n>0
  ASS(theory->isInterpretedConstant(n)); 
  IntegerConstantType nc;
  ALWAYS(theory->tryInterpretConstant(n,nc));
  ASS(nc.toInner()>0);
#endif

// ![Y] : (divides(n,Y) <=> ?[Z] : multiply(Z,n) = Y)

  unsigned srt = theory->getOperationSort(divides);
  ASS_EQ(srt, theory->getOperationSort(multiply));

  unsigned divsPred = env.signature->getInterpretingSymbol(divides);
  unsigned mulFun   = env.signature->getInterpretingSymbol(multiply);

  TermList y(1,false);
  TermList z(2,false);

// divides(n,Y) | multiply(Z,n) != Y 
  Literal* divsXY = Literal::create2(divsPred,true,n,y);
  TermList mZX(Term::create2(mulFun,z,n));
  Literal* mZXneY = Literal::createEquality(false,mZX,y,srt);
  addTheoryNonUnitClause(divsXY,mZXneY,EXPENSIVE);

// ~divides(n,Y) | multiply(skolem(n,Y),n)=Y
  Literal* ndivsXY = Literal::create2(divsPred,false,n,y);
  
  // create a skolem function with signature srt*srt>srt
  unsigned skolem = env.signature->addSkolemFunction(2);
  Signature::Symbol* sym = env.signature->getFunction(skolem);
  sym->setType(OperatorType::getFunctionType({srt,srt},srt));
  TermList skXY(Term::create2(skolem,n,y));
  TermList msxX(Term::create2(mulFun,skXY,n));
  Literal* msxXeqY = Literal::createEquality(true,msxX,y,srt);

  addTheoryNonUnitClause(ndivsXY,msxXeqY,EXPENSIVE);

}

void TheoryAxioms::addIntegerAbsAxioms(Interpretation abs, Interpretation less, 
                                       Interpretation unaryMinus, TermList zeroElement)
{
  CALL("TheoryAxioms::addIntegerAbsAxioms");

  unsigned srt = theory->getOperationSort(abs);
  ASS_EQ(srt, theory->getOperationSort(less));
  ASS_EQ(srt, theory->getOperationSort(unaryMinus));

  unsigned lessPred = env.signature->getInterpretingSymbol(less);
  unsigned absFun = env.signature->getInterpretingSymbol(abs);
  unsigned umFun = env.signature->getInterpretingSymbol(unaryMinus);

  TermList x(1,false);
  TermList absX(Term::create1(absFun,x));
  TermList mx(Term::create1(umFun,x));
  TermList absmX(Term::create1(absFun,mx));

  // If x is positive then abs(x)=x 
  // If x is negative then abs(x)=-x 

  Literal* xNeg = Literal::create2(lessPred,false,zeroElement,x); // not 0<x
  Literal* xPos = Literal::create2(lessPred,false,x,zeroElement); // not x<0

  Literal* absXeqX = Literal::createEquality(true,absX,x,srt);
  Literal* absXeqmX = Literal::createEquality(true,absX,mx,srt);

  addTheoryNonUnitClause(xNeg,absXeqX,EXPENSIVE);
  addTheoryNonUnitClause(xPos,absXeqmX,EXPENSIVE);

}


/**
 * Add axioms for quotient i.e. rat or real division 
 */
void TheoryAxioms::addQuotientAxioms(Interpretation quotient, Interpretation multiply,
    TermList zeroElement, TermList oneElement, Interpretation less)
{
  CALL("TheoryAxioms::addQuotientAxioms");

  unsigned srt = theory->getOperationSort(quotient);
  ASS_EQ(srt, theory->getOperationSort(multiply));
  ASS_EQ(srt, theory->getOperationSort(less));

  TermList x(1,false);
  TermList y(2,false);

  unsigned mulFun = env.signature->getInterpretingSymbol(multiply);
  unsigned divFun = env.signature->getInterpretingSymbol(quotient);

  Literal* guardx = Literal::createEquality(true,x,zeroElement,srt); 

  // x=0 | quotient(x,x)=1, easily derivable!
  //TermList qXX(Term::create2(quotient,x,x));
  //Literal* xQxis1 = Literal::createEquality(true,qXX,oneElement,srt);
  //addTheoryNonUnitClause(guardx,xQxis1);

  // x=0 | quotient(1,x)!=0
  TermList q1X(Term::create2(divFun,oneElement,x));
  Literal* oQxnot0 = Literal::createEquality(false,q1X,zeroElement,srt);
  addTheoryNonUnitClause(guardx,oQxnot0,EXPENSIVE);

  // quotient(x,1)=x, easily derivable!
  //TermList qX1(Term::create2(quotient,x,oneElement));
  //Literal* qx1isx = Literal::createEquality(true,qX1,x,srt);
  //addTheoryUnitClause(qx1isx);

  // x=0 | quotient(multiply(y,x),x)=y
  TermList myx(Term::create2(mulFun,y,x));
  TermList qmx(Term::create2(divFun,myx,x));
  Literal* qmxisy = Literal::createEquality(true,qmx,y,srt);
  addTheoryNonUnitClause(guardx,qmxisy,EXPENSIVE);


}

/**
 * Add axiom valid only for integer ordering: Y>X ->  Y => X+1 
 *
 * ~(x<y) | ~(y,x+1)
 */
void TheoryAxioms::addExtraIntegerOrderingAxiom(Interpretation plus, TermList oneElement,
                                                Interpretation less)
{
  CALL("TheoryAxioms::addExtraIntegerOrderingAxiom");

  unsigned lessPred = env.signature->getInterpretingSymbol(less);
  unsigned plusFun = env.signature->getInterpretingSymbol(plus);
  TermList x(0,false);
  TermList y(1,false);
  Literal* nxLy = Literal::create2(lessPred, false, x, y);
  TermList xPOne(Term::create2(plusFun, x, oneElement));
  Literal* nyLxPOne = Literal::create2(lessPred, false, y,xPOne);
  addTheoryNonUnitClause(nxLy, nyLxPOne,EXPENSIVE);
}
    
/**
 * Add axioms defining floor function
 * @author Giles
 */
void TheoryAxioms::addFloorAxioms(Interpretation floor, Interpretation less, Interpretation unaryMinus,
     Interpretation plus, TermList oneElement)
{
  CALL("TheoryAxioms::addFloorAxioms");

  unsigned lessPred = env.signature->getInterpretingSymbol(less);
  unsigned plusFun = env.signature->getInterpretingSymbol(plus);
  unsigned umFun = env.signature->getInterpretingSymbol(unaryMinus);
  unsigned floorFun = env.signature->getInterpretingSymbol(floor);
  TermList x(0,false);
  TermList floorX(Term::create1(floorFun,x));

  //axiom( floor(X) <= X )
  // is ~(X < floor(X))
  Literal* a1 = Literal::create2(lessPred, false, x, floorX);
  addTheoryUnitClause(a1,EXPENSIVE);

  //axiom( X-1 < floor(X) ) 
  TermList m1(Term::create1(umFun,oneElement));
  TermList xm1(Term::create2(plusFun, x, m1));
  Literal* a2 = Literal::create2(lessPred,true, xm1, floorX);
  addTheoryUnitClause(a2, EXPENSIVE);
} //addFloorAxioms

/**
 * Add axioms defining ceiling function
 * @author Giles
 */ 
void TheoryAxioms::addCeilingAxioms(Interpretation ceiling, Interpretation less, 
     Interpretation plus, TermList oneElement)
{
  CALL("TheoryAxioms::addCeilingAxioms");

  unsigned lessPred = env.signature->getInterpretingSymbol(less);
  unsigned plusFun = env.signature->getInterpretingSymbol(plus);
  unsigned ceilingFun = env.signature->getInterpretingSymbol(ceiling);
  TermList x(0,false);
  TermList ceilingX(Term::create1(ceilingFun,x));

  //axiom( ceiling(X) >= X ) 
  // is ~( ceiling(X) < X )
  Literal* a1 = Literal::create2(lessPred, false, ceilingX, x);
  addTheoryUnitClause(a1,EXPENSIVE);

  //axiom( ceiling(X) < X+1 ) 
  TermList xp1(Term::create2(plusFun, x, oneElement));
  Literal* a2 = Literal::create2(lessPred,true, ceilingX, xp1);
  addTheoryUnitClause(a2, EXPENSIVE);
} //addCeilingAxioms

/**
 * Add axioms defining round function
 * @author Giles
 */ 
void TheoryAxioms::addRoundAxioms(Interpretation round, Interpretation floor, Interpretation ceiling)
{
  CALL("TheoryAxioms::addRoundAxioms");
  
  //TODO... not that interesting as $round not in TPTP or translations
  // Suggested axioms:
  // round(x) = floor(x) | round(x) = ceiling(x)
  // x-0.5 > floor(x) => round(x) = ceiling(x)
  // x+0.5 < ceiling(x) => round(x) = floor(x)
  // x-0.5 = floor(x) => ?y : is_int(y) & 2*y = round(x)
  // x+0.5 = ceiling(x) => ?y : is_int(y) & 2*y = round(x)
  //NOT_IMPLEMENTED;

} //addRoundAxioms

/**
 * Add axioms defining truncate function
 * truncate is 'towards zero'
 *
 * >> x positive
 * x<0 | ~( x < tr(x) )		// x-1 < tr(x) <= x 
 * x<0 | x-1 < tr(x) 
 *
 * >> x negative
 * ~(x<0) | ~( tr(x) < x )	// x <= tr(x) < x+1 
 * ~(x<0) | tr(x) < x+1
 *
 * @author Giles
 */ 
void TheoryAxioms::addTruncateAxioms(Interpretation truncate, Interpretation less, Interpretation unaryMinus,
                      Interpretation plus, TermList zeroElement, TermList oneElement)
{
  CALL("TheoryAxioms::addTruncateAxioms");

  unsigned lessPred = env.signature->getInterpretingSymbol(less);
  unsigned plusFun = env.signature->getInterpretingSymbol(plus);
  unsigned umFun = env.signature->getInterpretingSymbol(unaryMinus);
  unsigned truncateFun = env.signature->getInterpretingSymbol(truncate);
  TermList x(0,false);
  TermList truncateX(Term::create1(truncateFun,x));

  TermList m1(Term::create1(umFun,oneElement));
  TermList xm1(Term::create2(plusFun,x,m1));
  TermList xp1(Term::create2(plusFun,x,oneElement));

  Literal* xLz = Literal::create2(lessPred,true,x,zeroElement);
  Literal* nxLz= Literal::create2(lessPred,false,x,zeroElement);

  //x<0 | ~( x < tr(x) )
  Literal* a1 = Literal::create2(lessPred,false,x,truncateX);
  addTheoryNonUnitClause(xLz,a1,EXPENSIVE);

  //x<0 | x-1 < tr(x)
  Literal* a2 = Literal::create2(lessPred,true,xm1,truncateX);
  addTheoryNonUnitClause(xLz,a2,EXPENSIVE);

  // ~(x<0) | ~( tr(x) < x )
  Literal* a3 = Literal::create2(lessPred,false,truncateX,x);
  addTheoryNonUnitClause(nxLz,a3,EXPENSIVE);

  // ~(x<0) | tr(x) < x+1
  Literal* a4 = Literal::create2(lessPred,true,truncateX,xp1);
  addTheoryNonUnitClause(nxLz,a4,EXPENSIVE);

} //addTruncateAxioms

/**
 * Adds the extensionality axiom of arrays (of type array1 or array2): 
 * select(X,sk(X,Y)) != select(Y,sk(X,Y)) | X = Y
 *
 * @author Laura Kovacs
 * @since 31/08/2012, Vienna
 * @since 11/11/2013 Manchester, updates
 * @author Andrei Voronkov
 * @since 05/01/2014 Vienna, add axiom in clause form (we need skolem function in other places)
 * @author Bernhard Kragl
*/
void TheoryAxioms::addArrayExtensionalityAxioms(unsigned arraySort, unsigned skolemFn)
{
  CALL("TheoryAxioms::addArrayExtenstionalityAxioms");

  unsigned sel = env.signature->getInterpretingSymbol(Theory::ARRAY_SELECT,Theory::getArrayOperatorType(arraySort,Theory::ARRAY_SELECT));

  Sorts::ArraySort* si = env.sorts->getArraySort(arraySort);
  unsigned rangeSort = si->getInnerSort();

  TermList x(0,false);
  TermList y(1,false);
  
  TermList sk(Term::create2(skolemFn, x, y)); //sk(x,y)
  TermList sel_x_sk(Term::create2(sel,x,sk)); //select(x,sk(x,y))
  TermList sel_y_sk(Term::create2(sel,y,sk)); //select(y,sk(x,y))
  Literal* eq = Literal::createEquality(true,x,y,arraySort); //x = y
  Literal* ineq = Literal::createEquality(false,sel_x_sk,sel_y_sk,rangeSort); //select(x,sk(x,y) != select(y,z)
  
  addTheoryNonUnitClause(eq, ineq,CHEAP);
} // addArrayExtensionalityAxiom    

/**
 * Adds the extensionality axiom of boolean arrays:
 * select(X, sk(X, Y)) <~> select(Y, sk(X, Y)) | X = Y
 *
 * @since 25/08/2015 Gothenburg
 * @author Evgeny Kotelnikov
 */
void TheoryAxioms::addBooleanArrayExtensionalityAxioms(unsigned arraySort, unsigned skolemFn)
{
  CALL("TheoryAxioms::addBooleanArrayExtenstionalityAxioms");

  OperatorType* selectType = Theory::getArrayOperatorType(arraySort,Theory::ARRAY_BOOL_SELECT);

  unsigned sel = env.signature->getInterpretingSymbol(Theory::ARRAY_BOOL_SELECT,selectType);

  TermList x(0,false);
  TermList y(1,false);

  TermList sk(Term::create2(skolemFn, x, y)); //sk(x,y)
  Formula* x_neq_y = new AtomicFormula(Literal::createEquality(false,x,y,arraySort)); //x != y

  Formula* sel_x_sk = new AtomicFormula(Literal::create2(sel, true, x, sk)); //select(x,sk(x,y))
  Formula* sel_y_sk = new AtomicFormula(Literal::create2(sel, true, y, sk)); //select(y,sk(x,y))
  Formula* sx_neq_sy = new BinaryFormula(XOR, sel_x_sk, sel_y_sk); //select(x,sk(x,y)) <~> select(y,sk(x,y))

  Formula* axiom = new QuantifiedFormula(FORALL, new Formula::VarList(0, new Formula::VarList(1, 0)),
                                         new Formula::SortList(arraySort, new Formula::SortList(arraySort,0)),
                                         new BinaryFormula(IMP, x_neq_y, sx_neq_sy));

  addAndOutputTheoryUnit(new FormulaUnit(axiom, new Inference(Inference::THEORY), Unit::AXIOM),CHEAP);
} // addBooleanArrayExtensionalityAxiom

/**
* Adds the write/select axiom of arrays (of type array1 or array2), 
 * @author Laura Kovacs
 * @since 31/08/2012, Vienna
*/
void TheoryAxioms::addArrayWriteAxioms(unsigned arraySort)
{
  CALL("TheoryAxioms::addArrayWriteAxioms");
        
  unsigned func_select = env.signature->getInterpretingSymbol(Theory::ARRAY_SELECT,Theory::getArrayOperatorType(arraySort,Theory::ARRAY_SELECT));
  unsigned func_store = env.signature->getInterpretingSymbol(Theory::ARRAY_STORE,Theory::getArrayOperatorType(arraySort,Theory::ARRAY_STORE));

  Sorts::ArraySort* si = env.sorts->getArraySort(arraySort);
  unsigned rangeSort = si->getInnerSort();
  unsigned domainSort = si->getIndexSort();

  TermList i(0,false);
  TermList j(1,false);
  TermList v(2,false);
  TermList a(3,false);
  TermList args[] = {a, i, v};
        
  //axiom (!A: arraySort, !I:domainSort, !V:rangeSort: (select(store(A,I,V), I) = V
  TermList wAIV(Term::create(func_store, 3, args)); //store(A,I,V)
  TermList sWI(Term::create2(func_select, wAIV,i)); //select(wAIV,I)
  Literal* ax = Literal::createEquality(true, sWI, v, rangeSort);
  addTheoryUnitClause(ax,CHEAP);

  //axiom (!A: arraySort, !I,J:domainSort, !V:rangeSort: (I!=J)->(select(store(A,I,V), J) = select(A,J)
  TermList sWJ(Term::create2(func_select, wAIV,j)); //select(wAIV,J)
  TermList sAJ(Term::create2(func_select, a, j)); //select(A,J)
        
  Literal* indexEq = Literal::createEquality(true, i, j, domainSort);//!(!(I=J)) === I=J
  Literal* writeEq = Literal::createEquality(true, sWJ, sAJ, rangeSort);//(select(store(A,I,V), J) = select(A,J)
  addTheoryNonUnitClause(indexEq, writeEq,CHEAP);
} //

/**
* Adds the write/select axiom of arrays (of type array1 or array2),
 * @author Laura Kovacs
 * @since 31/08/2012, Vienna
*/
void TheoryAxioms::addBooleanArrayWriteAxioms(unsigned arraySort)
{
  CALL("TheoryAxioms::addArrayWriteAxioms");

  unsigned pred_select = env.signature->getInterpretingSymbol(Theory::ARRAY_BOOL_SELECT,Theory::getArrayOperatorType(arraySort,Theory::ARRAY_BOOL_SELECT));
  unsigned func_store = env.signature->getInterpretingSymbol(Theory::ARRAY_STORE,Theory::getArrayOperatorType(arraySort,Theory::ARRAY_STORE));

  Sorts::ArraySort* si = env.sorts->getArraySort(arraySort);
  unsigned domainSort = si->getIndexSort();

  TermList a(0,false);
  TermList i(1,false);

  TermList false_(Term::foolFalse());
  TermList true_(Term::foolTrue());

  // select(store(A,I,$$true), I)
  //~select(store(A,I,$$false), I)

  for (TermList bval : {false_,true_}) {
    TermList args[] = {a, i, bval};
    TermList wAIV(Term::create(func_store, 3, args)); //store(A,I,bval)
    Literal* lit = Literal::create2(pred_select, true, wAIV,i);
    if (bval == false_) {
      lit = Literal::complementaryLiteral(lit);
    }
    Formula* ax = new AtomicFormula(lit);
    addAndOutputTheoryUnit(new FormulaUnit(ax, new Inference(Inference::THEORY), Unit::AXIOM),CHEAP);
  }

  TermList v(2,false);
  TermList j(3,false);

  TermList args[] = {a, i, v};

  //axiom (!A: arraySort, !I,J:domainSort, !V:rangeSort: (I!=J)->(select(store(A,I,V), J) <=> select(A,J)
  TermList wAIV(Term::create(func_store, 3, args)); //store(A,I,V)
  Formula* sWJ = new AtomicFormula(Literal::create2(pred_select, true, wAIV,j)); //select(wAIV,J)
  Formula* sAJ = new AtomicFormula(Literal::create2(pred_select, true, a, j)); //select(A,J)

  Formula* indexEq = new AtomicFormula(Literal::createEquality(false, i, j, domainSort));//I!=J
  Formula* writeEq = new BinaryFormula(IFF, sWJ, sAJ);//(select(store(A,I,V), J) <=> select(A,J)
  Formula* ax2 = new BinaryFormula(IMP, indexEq, writeEq);
  addAndOutputTheoryUnit(new FormulaUnit(ax2, new Inference(Inference::THEORY), Unit::AXIOM),CHEAP);
} //

//Axioms for integer division that hven't been implemented yet
//
//axiom( (ige(X0,zero) & igt(X1,zero)) --> ( ilt(X0-X1, idiv(X0,X1)*X1) & ile(idiv(X0,X1)*X1, X0) ) );
//axiom( (ilt(X0,zero) & ilt(X1,zero)) --> ( igt(X0-X1, idiv(X0,X1)*X1) & ige(idiv(X0,X1)*X1, X0) ) );
//axiom( (ige(X0,zero) & ilt(X1,zero)) --> ( ilt(X0+X1, idiv(X0,X1)*X1) & ile(idiv(X0,X1)*X1, X0) ) );
//axiom( (ilt(X0,zero) & igt(X1,zero)) --> ( igt(X0+X1, idiv(X0,X1)*X1) & ige(idiv(X0,X1)*X1, X0) ) );
//axiom( (ilt(X0,zero) & igt(X1,zero)) --> ( igt(X0+X1, idiv(X0,X1)*X1) & ige(idiv(X0,X1)*X1, X0) ) );
//axiom( (X1!=zero) --> (idiv(X0,X1)+X2==idiv(X0+(X1*X2),X1)) );


/**
 * Add theory axioms to the @b problem that are relevant to
 * units present in the problem. The problem must have been processed
 * by the InterpretedNormalizer before using this rule
 *
 * @since 11/11/2013, Manchester: bug fixes
 * @author Andrei Voronkov
 */
void TheoryAxioms::apply()
{
  CALL("TheoryAxioms::applyProperty");
  Property* prop = _prb.getProperty();
  bool modified = false;
  bool haveIntPlus =
    prop->hasInterpretedOperation(Theory::INT_PLUS) ||
    prop->hasInterpretedOperation(Theory::INT_UNARY_MINUS) ||
    prop->hasInterpretedOperation(Theory::INT_LESS) ||
    prop->hasInterpretedOperation(Theory::INT_MULTIPLY);
  bool haveIntMultiply =
    prop->hasInterpretedOperation(Theory::INT_MULTIPLY);

  bool haveIntDivision =
    prop->hasInterpretedOperation(Theory::INT_QUOTIENT_E) || // let's ignore the weird _F and _T for now!
    prop->hasInterpretedOperation(Theory::INT_REMAINDER_E) ||
    prop->hasInterpretedOperation(Theory::INT_ABS);

  bool haveIntDivides = prop->hasInterpretedOperation(Theory::INT_DIVIDES);

  bool haveIntFloor = prop->hasInterpretedOperation(Theory::INT_FLOOR);
  bool haveIntCeiling = prop->hasInterpretedOperation(Theory::INT_CEILING);
  bool haveIntRound = prop->hasInterpretedOperation(Theory::INT_ROUND);
  bool haveIntTruncate = prop->hasInterpretedOperation(Theory::INT_TRUNCATE);
  bool haveIntUnaryRoundingFunction = haveIntFloor || haveIntCeiling || haveIntRound || haveIntTruncate;

  if (haveIntPlus || haveIntUnaryRoundingFunction || haveIntDivision || haveIntDivides) {
    TermList zero(theory->representConstant(IntegerConstantType(0)));
    TermList one(theory->representConstant(IntegerConstantType(1)));
    if(haveIntMultiply || haveIntDivision || haveIntDivides) {
      addAdditionOrderingAndMultiplicationAxioms(Theory::INT_PLUS, Theory::INT_UNARY_MINUS, zero, one,
						 Theory::INT_LESS, Theory::INT_MULTIPLY);
      if(haveIntDivision){
        addIntegerDivisionWithModuloAxioms(Theory::INT_PLUS, Theory::INT_UNARY_MINUS, Theory::INT_LESS,
                                 Theory::INT_MULTIPLY, Theory::INT_QUOTIENT_E, Theory::INT_DIVIDES,
                                 Theory::INT_REMAINDER_E, Theory::INT_ABS, zero,one);
      }
      else if(haveIntDivides){ 
        Stack<TermList>& ns = env.signature->getDividesNvalues(); 
        Stack<TermList>::Iterator nsit(ns);
        while(nsit.hasNext()){
          TermList n = nsit.next();
          addIntegerDividesAxioms(Theory::INT_DIVIDES,Theory::INT_MULTIPLY,zero,n); 
        }
      }
    }
    else {
      addAdditionAndOrderingAxioms(Theory::INT_PLUS, Theory::INT_UNARY_MINUS, zero, one,
				   Theory::INT_LESS);
    }
    addExtraIntegerOrderingAxiom(Theory::INT_PLUS, one, Theory::INT_LESS);
    modified = true;
  }
  bool haveRatPlus =
    prop->hasInterpretedOperation(Theory::RAT_PLUS) ||
    prop->hasInterpretedOperation(Theory::RAT_UNARY_MINUS) ||
    prop->hasInterpretedOperation(Theory::RAT_LESS) ||
    prop->hasInterpretedOperation(Theory::RAT_QUOTIENT) ||
    prop->hasInterpretedOperation(Theory::RAT_MULTIPLY);
  bool haveRatMultiply =
    prop->hasInterpretedOperation(Theory::RAT_MULTIPLY);
  bool haveRatQuotient =
    prop->hasInterpretedOperation(Theory::RAT_QUOTIENT);

  bool haveRatFloor = prop->hasInterpretedOperation(Theory::RAT_FLOOR);
  bool haveRatCeiling = prop->hasInterpretedOperation(Theory::RAT_CEILING);
  bool haveRatRound = prop->hasInterpretedOperation(Theory::RAT_ROUND);
  bool haveRatTruncate = prop->hasInterpretedOperation(Theory::RAT_TRUNCATE);
  bool haveRatUnaryRoundingFunction = haveRatFloor || haveRatCeiling || haveRatRound || haveRatTruncate;

  if (haveRatPlus || haveRatUnaryRoundingFunction) {
    TermList zero(theory->representConstant(RationalConstantType(0, 1)));
    TermList one(theory->representConstant(RationalConstantType(1, 1)));
    if(haveRatMultiply || haveRatRound || haveRatQuotient) {
      addAdditionOrderingAndMultiplicationAxioms(Theory::RAT_PLUS, Theory::RAT_UNARY_MINUS, zero, one,
						 Theory::RAT_LESS, Theory::RAT_MULTIPLY);

      if(haveRatQuotient){
        addQuotientAxioms(Theory::RAT_QUOTIENT,Theory::RAT_MULTIPLY,zero,one,Theory::RAT_LESS);
      }
    }
    else {
      addAdditionAndOrderingAxioms(Theory::RAT_PLUS, Theory::RAT_UNARY_MINUS, zero, one,
				   Theory::RAT_LESS);
    }
    if(haveRatFloor || haveRatRound){
      addFloorAxioms(Theory::RAT_FLOOR,Theory::RAT_LESS,Theory::RAT_UNARY_MINUS,Theory::RAT_PLUS,one);
    }
    if(haveRatCeiling || haveRatRound){
      addCeilingAxioms(Theory::RAT_CEILING,Theory::RAT_LESS,Theory::RAT_PLUS,one);
    }
    if(haveRatRound){
      //addRoundAxioms(Theory::INT_TRUNCATE,Theory::INT_FLOOR,Theory::INT_CEILING);
    }
    if(haveRatTruncate){
      addTruncateAxioms(Theory::RAT_TRUNCATE,Theory::RAT_LESS,Theory::RAT_UNARY_MINUS,
                        Theory::RAT_PLUS,zero,one);
    }
    modified = true;
  }
  bool haveRealPlus =
    prop->hasInterpretedOperation(Theory::REAL_PLUS) ||
    prop->hasInterpretedOperation(Theory::REAL_UNARY_MINUS) ||
    prop->hasInterpretedOperation(Theory::REAL_LESS) ||
    prop->hasInterpretedOperation(Theory::REAL_QUOTIENT) ||
    prop->hasInterpretedOperation(Theory::REAL_MULTIPLY);
  bool haveRealMultiply =
    prop->hasInterpretedOperation(Theory::REAL_MULTIPLY);
  bool haveRealQuotient =
    prop->hasInterpretedOperation(Theory::REAL_QUOTIENT);

  bool haveRealFloor = prop->hasInterpretedOperation(Theory::REAL_FLOOR);
  bool haveRealCeiling = prop->hasInterpretedOperation(Theory::REAL_CEILING);
  bool haveRealRound = prop->hasInterpretedOperation(Theory::REAL_ROUND);
  bool haveRealTruncate = prop->hasInterpretedOperation(Theory::REAL_TRUNCATE);
  bool haveRealUnaryRoundingFunction = haveRealFloor || haveRealCeiling || haveRealRound || haveRealTruncate;

  if (haveRealPlus || haveRealUnaryRoundingFunction) {
    TermList zero(theory->representConstant(RealConstantType(RationalConstantType(0, 1))));
    TermList one(theory->representConstant(RealConstantType(RationalConstantType(1, 1))));
    if(haveRealMultiply || haveRealQuotient) {
      addAdditionOrderingAndMultiplicationAxioms(Theory::REAL_PLUS, Theory::REAL_UNARY_MINUS, zero, one,
						 Theory::REAL_LESS, Theory::REAL_MULTIPLY);

      if(haveRealQuotient){
        addQuotientAxioms(Theory::REAL_QUOTIENT,Theory::REAL_MULTIPLY,zero,one,Theory::REAL_LESS);
      }
    }
    else {
      addAdditionAndOrderingAxioms(Theory::REAL_PLUS, Theory::REAL_UNARY_MINUS, zero, one,
				   Theory::REAL_LESS);
    }
    if(haveRealFloor || haveRealRound){
      addFloorAxioms(Theory::REAL_FLOOR,Theory::REAL_LESS,Theory::REAL_UNARY_MINUS,Theory::REAL_PLUS,one);
    }
    if(haveRealCeiling || haveRealRound){
      addCeilingAxioms(Theory::REAL_CEILING,Theory::REAL_LESS,Theory::REAL_PLUS,one);
    }
    if(haveRealRound){
      //addRoundAxioms(Theory::INT_TRUNCATE,Theory::INT_FLOOR,Theory::INT_CEILING);
    }
    if(haveRealTruncate){
      addTruncateAxioms(Theory::REAL_TRUNCATE,Theory::REAL_LESS,Theory::REAL_UNARY_MINUS,
                        Theory::REAL_PLUS,zero,one);
    }

    modified = true;
  }

  VirtualIterator<unsigned> arraySorts = env.sorts->getStructuredSorts(Sorts::StructuredSort::ARRAY);
  while(arraySorts.hasNext()){
    unsigned arraySort = arraySorts.next();

    bool isBool = (env.sorts->getArraySort(arraySort)->getInnerSort() == Sorts::SRT_BOOL);
    
    // Check if they are used
    Interpretation arraySelect = isBool ? Theory::ARRAY_BOOL_SELECT : Theory::ARRAY_SELECT;
    bool haveSelect = prop->hasInterpretedOperation(arraySelect,Theory::getArrayOperatorType(arraySort,arraySelect));
    bool haveStore = prop->hasInterpretedOperation(Theory::ARRAY_STORE,Theory::getArrayOperatorType(arraySort,Theory::ARRAY_STORE));

    if (haveSelect || haveStore) {
      unsigned sk = theory->getArrayExtSkolemFunction(arraySort);
      if (isBool) {
        addBooleanArrayExtensionalityAxioms(arraySort, sk);
      } else {
        addArrayExtensionalityAxioms(arraySort, sk);
      }
      if (haveStore) {
        if (isBool) {
          addBooleanArrayWriteAxioms(arraySort);
        } else {
          addArrayWriteAxioms(arraySort);
        }
      }
      modified = true;
    }
  }

  VirtualIterator<TermAlgebra*> tas = env.signature->termAlgebrasIterator();
  while (tas.hasNext()) {
    TermAlgebra* ta = tas.next();

    addExhaustivenessAxiom(ta);
    addDistinctnessAxiom(ta);
    addInjectivityAxiom(ta);
    addDiscriminationAxiom(ta);

    if (env.options->termAlgebraCyclicityCheck() == Options::TACyclicityCheck::AXIOM) {
      addAcyclicityAxiom(ta);
    }

    modified = true;
  }
  

  VirtualIterator<Theory::MonomorphisedInterpretation> it = env.property->getPolymorphicInterpretations();
  unsigned size;
  while(it.hasNext()){
	  Theory::MonomorphisedInterpretation entry = it.next();

	  Interpretation itp = entry.first;
	  /* ------------------------------------------
	         * Start function conditions
	  * ------------------------------------------
	  * */
      if (!(itp >= Theory::BVADD && itp <= Theory::CONCAT))
    	  	  continue;

      if (entry.second->isFunctionType())
    	  	  size = env.sorts->getBitVectorSort(entry.second->result())->getSize();
      else
    	  	  size = env.sorts->getBitVectorSort(entry.second->arg(0))->getSize();

      if (itp == Theory::BVNAND){
    	  	  addPolyMorphicBinaryFunctionEquivalentToUnaryFunctionAppliedToBinaryFunction(Theory::BVNAND,Theory::BVNOT,Theory::BVAND,size);
      }
      else if(itp == Theory::BVNOR){
    	  	addPolyMorphicBinaryFunctionEquivalentToUnaryFunctionAppliedToBinaryFunction(Theory::BVNOR,Theory::BVNOT,Theory::BVOR,size);
      }
      else if(itp == Theory::BVXOR){
    	  	  addBVXORAxiom1(Theory::BVXOR, Theory::BVOR , Theory::BVAND, Theory::BVNOT, size);
      }
      else if(itp == Theory::BVXNOR){
    	    addBVXNORAxiom1(Theory::BVXNOR, Theory::BVOR , Theory::BVAND, Theory::BVNOT, size);
      }

      else if (itp == Theory::BVADD){
    	    TermList zero(theory->representConstant(BitVectorOperations::getZeroBVCT(size)));
    	    unsigned srt0 = entry.second->arg(0);

    	    addBitVectorCommutativity(Theory::BVADD,size);
    	    addPolyMorphicBinaryFunctionEquivalentToBinaryFunctionAppliedToUnaryFunction(Theory::BVSUB, Theory::BVADD, Theory::BVNEG,size);
    	    addSomeAdditionAxiom(srt0);
    	    addAdditionByOneAxioms(srt0);
      }
      else if (itp == Theory::BVMUL){

    	  TermList one(theory->representConstant(BitVectorOperations::getOneBVCT(size)));
    	  TermList zero(theory->representConstant(BitVectorOperations::getZeroBVCT(size)));
    	  unsigned srt0 = entry.second->arg(0);

    	  // add that (bvmul (X 1)) = X
    	  addBitVectorRightIdentity(Theory::BVMUL,one,size);
    	  // add that (bvmul X 0) = 0
    	  addPolyMorphicSpecialConstantAxiom(Interpretation::BVMUL, zero, zero, size);
    	  addBitVectorCommutativity(Theory::BVMUL,size);


       }
      else if (itp == Theory::BVSUB){

    	  unsigned srt0 = entry.second->arg(0);

    	  addPolyMorphicBinaryFunctionEquivalentToBinaryFunctionAppliedToUnaryFunction(Theory::BVSUB, Theory::BVADD, Theory::BVNEG,size);

      }
      else if (itp== Theory::BVUDIV){

    	  TermList zero(theory->representConstant(BitVectorOperations::getZeroBVCT(size)));
          TermList allOnes(theory->representConstant(BitVectorOperations::getAllOnesBVCT(size)));
          TermList one(theory->representConstant(BitVectorOperations::getOneBVCT(size)));
          unsigned srt0 = entry.second->arg(0);

          // add that bvudiv ( x zero) = allOnes
          addPolyMorphicSpecialConstantAxiom(Theory::BVUDIV, zero, allOnes,size);

          addDivisionZeroAxiom(srt0);
          addDivisionOneAxiom(srt0);
          addAnotherDivisionAxiom(srt0);

          // x!=MAX -> bvugt(x+1 x)
          addMaxAxiom(Theory::BVUGT, srt0);

          // (t>s) -> (s/t = 0)
          addDivAxiomGT(srt0);
          //(s>t) -> (s/t > 0)
          addDivAxiomGT2(srt0);

          //t=s & t!=0 -> s/t = 1
          addDivONEAxiom(srt0);
          // (x / s = t) -> (t >> s) << s = t
          // addShiftingAxiom(srt0, Theory::BVUDIV, Theory::BVMUL);
          //s/x = t -> (s/(s/t)) = t
          addTempAxiom(srt0);


      }
      else if (itp== Theory::BVUREM){// bvurem returns its first operand if the second operand is 0

    	  unsigned srt0 = entry.second->arg(0);
    	  TermList zero(theory->representConstant(BitVectorOperations::getZeroBVCT(size)));
    	  TermList one(theory->representConstant(BitVectorOperations::getOneBVCT(size)));

    	  // add that ( bvurem x 0 ) = x
    	  addBitVectorRightIdentity(Theory::BVUREM,zero,size);
          // (bvurem x one) = zero
          addPolyMorphicSpecialConstantAxiom(itp, one, zero,  size);
          // add that bvule (x%y x)
          addBVUREMwithPredicateAxiom(itp, Theory::BVULE, srt0);
          // add that (bvurem x x) = zero
          addFunctionWithSameArgumentEqualsConstant(itp, zero, srt0);

         // addTempOrAxiom2(srt0, Theory::BVSGT, itp);

      }
      else if (itp == Theory::CONCAT){

    	  unsigned srt0 = entry.second->arg(0);
    	  unsigned srt1 = entry.second->arg(1);
    	  unsigned resultSrt = entry.second->result();

    	  addConcatArgumentsNotEqualEquivalentToConcatResultsNotEqual(srt0,srt1,resultSrt);

    	  addPredicateOnConcatArgsImpliesPredicateConcatFirstArg(srt0,srt1,resultSrt, Theory::BVUGE);
    	  addConcatArgsPredicateImpliesWholePredicate(Theory::BVUGE, srt0, srt1, resultSrt);

    	  addPredicateOnConcatArgsImpliesPredicateConcatFirstArg(srt0,srt1,resultSrt, Theory::BVSLE); // this one works
    	  addConcatArgsPredicateImpliesWholePredicateVariation(Theory::BVSLE, srt0, srt1, resultSrt);

      }
      else if (itp == Theory::BVNOT){

          unsigned srt0 = entry.second->arg(0);
          // add that !(!x) = x
          addUnaryFunctionAppliedTwiceEqualsArgument(itp,srt0);


      }
      else if (itp == Theory::BVNEG){

         unsigned srt0 = entry.second->arg(0);
         // add that -(-x) = x
         addUnaryFunctionAppliedTwiceEqualsArgument(itp,srt0);


      }
      else if (itp == Theory::BVASHR){

    	  unsigned srt0 = entry.second->arg(0);
    	  TermList zero(theory->representConstant(BitVectorOperations::getZeroBVCT(size)));
    	  TermList allOnes(theory->representConstant(BitVectorOperations::getAllOnesBVCT(size)));

    	  // add that (bvashr 0 x) = 0
    	  addPolyMorphicSpecialConstantAxiomVariation(Interpretation::BVASHR, zero, zero, size);
    	  // add that (bvashr x 0) = x
    	  addBitVectorRightIdentity(itp, zero, size);
    	  // add that bvashr (allones x) = allones
    	  addPolyMorphicSpecialConstantAxiomVariation(itp, allOnes, allOnes, size);

      }
      else if (itp == Theory::BVSHL){

    	  unsigned srt0 = entry.second->arg(0);
          TermList zero(theory->representConstant(BitVectorOperations::getZeroBVCT(size)));
          TermList allOnes(theory->representConstant(BitVectorOperations::getAllOnesBVCT(size)));

          // add that (bvshl 0 x) = 0
          addPolyMorphicSpecialConstantAxiomVariation(Interpretation::BVSHL, zero, zero, size);
          // add that (bvshl x 0) = x
          addBitVectorRightIdentity(itp, zero, size);
          // add that bvshl x allones = 0
          addPolyMorphicSpecialConstantAxiom(Interpretation::BVSHL, allOnes, zero, size);

          // add that ( bvule ( bvshl (allones  X) allones ) )
          addFunctionAppliedToConstantPredicateFirstArgVariation(itp, Theory::BVULE, allOnes,srt0);

          addShiftingAxiom(srt0, Theory::BVSHL, Theory::BVLSHR);
      }
      else if (itp == Theory::BVLSHR){

    	  unsigned srt0 = entry.second->arg(0);
          TermList zero(theory->representConstant(BitVectorOperations::getZeroBVCT(size)));
          TermList allOnes(theory->representConstant(BitVectorOperations::getAllOnesBVCT(size)));

          // add that (bvlshr 0 x) = 0
          addPolyMorphicSpecialConstantAxiomVariation(Theory::BVLSHR, zero, zero, size);
          // add that (bvlshr x allones) = 0
          addPolyMorphicSpecialConstantAxiom(Theory::BVLSHR, allOnes, zero, size);
          addBitVectorRightIdentity(itp,zero,size);

          addShiftingAxiom(srt0, Theory::BVLSHR, Theory::BVSHL);

      }
      else if (itp == Theory::BVOR){

    	  unsigned srt0 = entry.second->arg(0);
    	  TermList zero(theory->representConstant(BitVectorOperations::getZeroBVCT(size)));
    	  TermList allOnes(theory->representConstant(BitVectorOperations::getAllOnesBVCT(size)));

    	  addBitVectorRightIdentity(Theory::BVOR,zero,size);
    	  addBitVectorCommutativity(Theory::BVOR,size);

    	  // add that X || allOnes = allOnes
    	  addPolyMorphicSpecialConstantAxiom(itp, allOnes,allOnes, size);
    	  // add that X || X = X
    	  addFunctionWithSameArgumentEqualArgument(itp, srt0);

    	  addSpecialEqualAndAxiom(srt0, itp);

	  }
      else if (itp == Theory::BVAND){

    	  unsigned srt0 = entry.second->arg(0);
    	  TermList zero(theory->representConstant(BitVectorOperations::getZeroBVCT(size)));

    	  TermList allOnes(theory->representConstant(BitVectorOperations::getAllOnesBVCT(size)));
    	  // add that bvand(X,zero) = zero
    	  addPolyMorphicSpecialConstantAxiom(itp, zero,zero, size);
    	  // add that bvand(X,Max) = X
    	  addBitVectorRightIdentity(itp,allOnes,size);
    	  addBitVectorCommutativity(Theory::BVAND,size);
    	  addFunctionWithSameArgumentEqualArgument(itp, srt0);

    	  addSpecialEqualAndAxiom(srt0, itp);

    	  //addTempAndAxiom1(srt0, itp);

      }
      /* ------------------------------------------
       * End function conditions
       * ------------------------------------------
       * */

      /* ------------------------------------------
       * Start predicates conditions
       * ------------------------------------------
       * */
      else if (itp == Theory::BVSLE){

       /*rewrites*/

      }

      else if (itp == Theory::BVSGE){

    	  unsigned srt0 = entry.second->arg(0);
    	  TermList signedMin(theory->representConstant(BitVectorOperations::getSignedMinBVCT(size)));
    	  TermList signedMax(theory->representConstant(BitVectorOperations::getSignedMaxBVCT(size)));

    	  //x != y OR bvsge(x y)
    	  addEqualsImpliesBinaryPredicate(Theory::BVSGE, srt0);

    	  // add that (bvsge x signedmin)
    	  addSimplePolyMorphicPredicateWithConstantAxiom(srt0, itp, signedMin, false, true,false);
    	  // add that (bvsge signedMax x)
    	  addSimplePolyMorphicPredicateWithConstantAxiom(srt0, itp, signedMax, true, true,false);

    	  // !bvsge(x y) OR !bvsgt(y x)
    	  addPolyMorphicClauseAxiom(srt0, Theory::BVSGE , false, false, Theory::BVSGT, true, false);
    	 // !(bvsge x s) OR (bvsgt x s) OR (x = s)
    	  isPredicateWithEqualRemovedOrEqualAxiom(Theory::BVSGE,Theory::BVSGT,size);


      }

      else if (itp == Theory::BVUGE){

    	  unsigned srt0 = entry.second->arg(0);
    	  TermList zero(theory->representConstant(BitVectorOperations::getZeroBVCT(size)));
          TermList allOnes(theory->representConstant(BitVectorOperations::getAllOnesBVCT(size)));

          // x != y OR bvuge(x y)
          addEqualsImpliesBinaryPredicate(itp, srt0);

          // add that (bvuge x zero)
          addSimplePolyMorphicPredicateWithConstantAxiom(srt0, itp, zero, false, true,false);
          // add that (bvuge allones x)
          addSimplePolyMorphicPredicateWithConstantAxiom(srt0, itp, allOnes, true, true,false);

          // !bvuge(x y) OR !bvugt(y x)
          addPolyMorphicClauseAxiom(srt0, Theory::BVUGE , false, false, Theory::BVUGT, true, false);
          // !(bvuge x s) OR (bvugt x s) OR (x = s)
          isPredicateWithEqualRemovedOrEqualAxiom(Theory::BVUGE,Theory::BVUGT,size); // ??

          // bvuge(x bvand(x y)) & bvuge(y bvand(x,y))
          predicateTrueForArgumentsOfAFunction(srt0, Theory::BVAND, Theory::BVUGE);

          // bvuge (bvlshr(s x) t) -> bvuge(s t)
          addTempOrAxiom2(srt0, itp,  Interpretation::BVLSHR);

          // bvuge (bvand(s x) t) -> bvuge(s t)
          addTempOrAxiom2(srt0, itp, Interpretation::BVAND);


      }
      else if (itp == Theory::BVULE){
    	  // empty due to rewrites
      }

      else if (itp == Theory::BVSLT){
    	  // empty due to rewrites
      }
      else if(itp == Theory::BVULT) {
           // empty due to rewrites
      }
      else if(itp == Theory::BVUGT){

         unsigned srt0 = entry.second->arg(0);
         TermList allOnes(theory->representConstant(BitVectorOperations::getAllOnesBVCT(size)));
         TermList zero(theory->representConstant(BitVectorOperations::getZeroBVCT(size)));

         //bvugt (bvlshr(s x) t) -> bvugt(s t)
         // !bvugt (bvlshr (s x) t) OR bvugt(s t)
         addTempOrAxiom2(srt0, itp,  Interpretation::BVLSHR);

         //bvugt(bvand(s, x), t) -> bvugt(s,t)
          addTempOrAxiom2(srt0, Theory::BVUGT,  Theory::BVAND);

          // x!= umax -> bvugt(umax x)
         // x = umax OR bvugt(umax x)
         addXNEqualToConstantImpliesAxiom(srt0, Theory::BVUGT, allOnes,false);

         // x!=zero -> bvugt (x zero)
         // x=zero OR bvugt x zero
         addXNEqualToConstantImpliesAxiom(srt0, Theory::BVUGT, zero,true);

         //P(x,c)
         // !(x > umax)
         addPolyMorphicLiteralWithConstantAxiom(srt0, Theory::BVUGT, allOnes, false, false);

         //P(x,c)
         // !(zero > x)
          addPolyMorphicLiteralWithConstantAxiom(srt0, Theory::BVUGT, zero, true, false);

         //rewrite this
         //bvult(bvor(s, x), t) -> bvult(s,t)
         // addTempOrAxiom2(srt0, Theory::BVULT,  Theory::BVOR);
      }

      else if(itp == Theory::BVSGT) {
          unsigned srt0 = entry.second->arg(0);
          TermList signedMax(theory->representConstant(BitVectorOperations::getSignedMaxBVCT(size)));
          TermList signedMin(theory->representConstant(BitVectorOperations::getSignedMinBVCT(size)));
          TermList zero(theory->representConstant(BitVectorOperations::getZeroBVCT(size)));

          addORSignedOperatorWithConstantAxiom(srt0, Theory::BVSGT, Theory::BVOR,signedMax);

          // x = smax OR bvsgt(smax x)
          addXNEqualToConstantImpliesAxiom(srt0, Theory::BVSGT, signedMax,false);

          // x!=zero -> bvugt (x zero)
          // x=zero OR bvugt x zero
          addXNEqualToConstantImpliesAxiom(srt0, Theory::BVSGT, signedMin,true);

          //bvsgt(s t) -> bvsgt(bvand(s c) t)
          addOtherBVANDSignedPredicatesAxiom(srt0, Theory::BVSGT, Theory::BVAND,signedMax);


          //P(x,c)
          // !(x > smax)
          addPolyMorphicLiteralWithConstantAxiom(srt0, Theory::BVSGT, signedMax, false, false);

          //P(x,c)
           // !(smin > x)
          addPolyMorphicLiteralWithConstantAxiom(srt0, Theory::BVSGT, signedMin, true, false);

          // bvsgt (0 s) -> bvslt(s bvand(s & signedMax)) // TODO CNF REWRITE, check occurence
          // addBVANDSignedPredicatesAxiom(srt0, Theory::BVSGT, Theory::BVSLT, Theory::BVAND,zero, signedMax); // bvsgt (0 s) -> bvslt(s bvand(s & signedMax))
      }

      /* ------------------------------------------
       * End predicates conditions
       * ------------------------------------------
       * */

      modified = true;
  }
  
  if(modified) {
    _prb.reportEqualityAdded(false);
  }

} // TheoryAxioms::apply

void TheoryAxioms::applyFOOL() {
  CALL("TheoryAxioms::applyFOOL");

  TermList t(Term::foolTrue());
  TermList f(Term::foolFalse());

  Inference* foolAxiom = new Inference(Inference::FOOL_AXIOM);

  // Add "$$true != $$false"
  Clause* tneqfClause = new(1) Clause(1, Unit::AXIOM, foolAxiom);
  (*tneqfClause)[0] = Literal::createEquality(false, t, f, Sorts::SRT_BOOL);
  addAndOutputTheoryUnit(tneqfClause, CHEAP);

  // Do not add the finite domain axiom if --fool_paradomulation on
  if (env.options->FOOLParamodulation()) {
    return;
  }

  // Add "![X : $bool]: ((X = $$true) | (X = $$false))"
  Clause* boolVarClause = new(2) Clause(2, Unit::AXIOM, foolAxiom);
  (*boolVarClause)[0] = Literal::createEquality(true, TermList(0, false), t, Sorts::SRT_BOOL);
  (*boolVarClause)[1] = Literal::createEquality(true, TermList(0, false), f, Sorts::SRT_BOOL);
  addAndOutputTheoryUnit(boolVarClause, CHEAP);
} // TheoryAxioms::addBooleanDomainAxiom

void TheoryAxioms::addExhaustivenessAxiom(TermAlgebra* ta) {
  CALL("TheoryAxioms::addExhaustivenessAxiom");

  TermList x(0, false);
  Stack<TermList> argTerms;
  bool addsFOOL = false;

  FormulaList* l = FormulaList::empty();

  for (unsigned i = 0; i < ta->nConstructors(); i++) {
    TermAlgebraConstructor *c = ta->constructor(i);
    argTerms.reset();

    for (unsigned j = 0; j < c->arity(); j++) {
      if (c->argSort(j) == Sorts::SRT_BOOL) {
        addsFOOL = true;
        Literal* lit = Literal::create1(c->destructorFunctor(j), true, x);
        Term* t = Term::createFormula(new AtomicFormula(lit));
        argTerms.push(TermList(t));
      } else {
        Term* t = Term::create1(c->destructorFunctor(j), x);
        argTerms.push(TermList(t));
      }
    }

    TermList rhs(Term::create(c->functor(), argTerms.size(), argTerms.begin()));
    FormulaList::push(new AtomicFormula(Literal::createEquality(true, x, rhs, ta->sort())), l);
  }

  Formula::VarList* vars = Formula::VarList::cons(x.var(), Formula::VarList::empty());
  Formula::SortList* sorts = Formula::SortList::cons(ta->sort(), Formula::SortList::empty());

  Formula *axiom;
  switch (FormulaList::length(l)) {
    case 0:
      // the algebra cannot have 0 constructors
      ASSERTION_VIOLATION;
    case 1:
      axiom = new QuantifiedFormula(Connective::FORALL, vars, sorts, l->head());
      break;
    default:
      axiom = new QuantifiedFormula(Connective::FORALL, vars, sorts, new JunctionFormula(Connective::OR, l));
  }

  Unit* u = new FormulaUnit(axiom, new Inference(Inference::TERM_ALGEBRA_EXHAUSTIVENESS), Unit::AXIOM);
  addAndOutputTheoryUnit(u, CHEAP);
  if (addsFOOL) { _prb.reportFOOLAdded(); }
}

void TheoryAxioms::addDistinctnessAxiom(TermAlgebra* ta) {
  CALL("TermAlgebra::addDistinctnessAxiom");

  Array<TermList> terms(ta->nConstructors());

  unsigned var = 0;
  for (unsigned i = 0; i < ta->nConstructors(); i++) {
    TermAlgebraConstructor* c = ta->constructor(i);

    Stack<TermList> args;
    for (unsigned j = 0; j < c->arity(); j++) {
      args.push(TermList(var++, false));
    }
    TermList term(Term::create(c->functor(), (unsigned)args.size(), args.begin()));
    terms[i] = term;
  }

  for (unsigned i = 0; i < ta->nConstructors(); i++) {
    for (unsigned j = i + 1; j < ta->nConstructors(); j++) {
      Literal* ineq = Literal::createEquality(false, terms[i], terms[j], ta->sort());
      addTheoryUnitClause(ineq, new Inference(Inference::TERM_ALGEBRA_DISTINCTNESS),CHEAP);
    }
  }
}

void TheoryAxioms::addInjectivityAxiom(TermAlgebra* ta)
{
  CALL("TheoryAxioms::addInjectivityAxiom");

  for (unsigned i = 0; i < ta->nConstructors(); i++) {
    TermAlgebraConstructor* c = ta->constructor(i);

    Stack<TermList> lhsArgs(c->arity());
    Stack<TermList> rhsArgs(c->arity());

    for (unsigned j = 0; j < c->arity(); j++) {
      lhsArgs.push(TermList(j * 2, false));
      rhsArgs.push(TermList(j * 2 + 1, false));
    }

    TermList lhs(Term::create(c->functor(), (unsigned)lhsArgs.size(), lhsArgs.begin()));
    TermList rhs(Term::create(c->functor(), (unsigned)rhsArgs.size(), rhsArgs.begin()));
    Literal* eql = Literal::createEquality(false, lhs, rhs, ta->sort());

    for (unsigned j = 0; j < c->arity(); j++) {
      Literal* eqr = Literal::createEquality(true, TermList(j * 2, false), TermList(j * 2 + 1, false), c->argSort(j));

      Clause* injectivity = new(2) Clause(2, Unit::AXIOM, new Inference(Inference::TERM_ALGEBRA_INJECTIVITY));
      (*injectivity)[0] = eql;
      (*injectivity)[1] = eqr;
      addAndOutputTheoryUnit(injectivity,CHEAP);
    }
  }
}

void TheoryAxioms::addDiscriminationAxiom(TermAlgebra* ta) {
  CALL("addDiscriminationAxiom");

  Array<TermList> cases(ta->nConstructors());
  for (unsigned i = 0; i < ta->nConstructors(); i++) {
    TermAlgebraConstructor* c = ta->constructor(i);

    Stack<TermList> variables;
    for (unsigned var = 0; var < c->arity(); var++) {
      variables.push(TermList(var, false));
    }

    TermList term(Term::create(c->functor(), (unsigned)variables.size(), variables.begin()));
    cases[i] = term;
  }

  for (unsigned i = 0; i < ta->nConstructors(); i++) {
    TermAlgebraConstructor* constructor = ta->constructor(i);

    if (!constructor->hasDiscriminator()) continue;

    for (unsigned c = 0; c < cases.size(); c++) {
      Literal* lit = Literal::create1(constructor->discriminator(), c == i, cases[c]);
      addTheoryUnitClause(lit, new Inference(Inference::TERM_ALGEBRA_DISCRIMINATION),CHEAP);
    }
  }
}

void TheoryAxioms::addAcyclicityAxiom(TermAlgebra* ta)
{
  CALL("TheoryAxioms::addAcyclicityAxiom");

  unsigned pred = ta->getSubtermPredicate();

  if (ta->allowsCyclicTerms()) {
    return;
  }

  bool rec = false;

  for (unsigned i = 0; i < ta->nConstructors(); i++) {
    if (addSubtermDefinitions(pred, ta->constructor(i))) {
      rec = true;
    }
  }

  // rec <=> the subterm relation is non-empty
  if (!rec) {
    return;
  }

  static TermList x(0, false);

  Literal* sub = Literal::create2(pred, false, x, x);
  addTheoryUnitClause(sub, new Inference(Inference::TERM_ALGEBRA_ACYCLICITY),CHEAP);
}

bool TheoryAxioms::addSubtermDefinitions(unsigned subtermPredicate, TermAlgebraConstructor* c)
{
  CALL("TheoryAxioms::addSubtermDefinitions");

  TermList z(c->arity(), false);

  Stack<TermList> args;
  for (unsigned i = 0; i < c->arity(); i++) {
    args.push(TermList(i, false));
  }
  TermList right(Term::create(c->functor(), (unsigned)args.size(), args.begin()));

  bool added = false;
  for (unsigned i = 0; i < c->arity(); i++) {
    if (c->argSort(i) != c->rangeSort()) continue;

    TermList y(i, false);

    // Direct subterms are subterms: Sub(y, c(x1, ... y ..., xn))
    Literal* sub = Literal::create2(subtermPredicate, true, y, right);
    addTheoryUnitClause(sub, new Inference(Inference::TERM_ALGEBRA_ACYCLICITY),CHEAP);

    // Transitivity of the subterm relation: Sub(z, y) -> Sub(z, c(x1, ... y , xn))
    Clause* transitivity = new(2) Clause(2, Unit::AXIOM, new Inference(Inference::TERM_ALGEBRA_ACYCLICITY));
    (*transitivity)[0] = Literal::create2(subtermPredicate, false, z, y);
    (*transitivity)[1] = Literal::create2(subtermPredicate, true,  z, right);
    addAndOutputTheoryUnit(transitivity,CHEAP);

    added = true;
  }
  return added;
}
