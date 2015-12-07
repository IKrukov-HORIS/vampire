/**
 * @file Skolem.cpp
 * Implementing Skolemisation.
 * @since 05/01/2003 Manchester
 * @since 08/07/2007 flight Manchester-Cork, changed to new datastructures
 */

#include "Kernel/Signature.hpp"
#include "Kernel/Term.hpp"
#include "Kernel/Inference.hpp"
#include "Kernel/InferenceStore.hpp"
#include "Kernel/Formula.hpp"
#include "Kernel/FormulaUnit.hpp"
#include "Kernel/SortHelper.hpp"
#include "Kernel/SubformulaIterator.hpp"

#include "Indexing/TermSharing.hpp"

#include "Options.hpp"
#include "Rectify.hpp"
#include "Refutation.hpp"
#include "Skolem.hpp"
#include "VarManager.hpp"

using namespace Kernel;
using namespace Shell;

Skolem::Skolem ()
    : _vars(16), _beingSkolemised(0) {}

/**
 * Skolemise the unit.
 *
 * @warning the unit must contain a closed formula in NNF
 * @since 05/01/2004 Manchester
 * @since 23/01/2004 Manchester, changed to use non-static functions
 * @since 31/01/2004 Manchester. Rectify inference has been added
 * (otherwise proof-checking had been very difficult).
 */
FormulaUnit* Skolem::skolemise (FormulaUnit* unit)
{
  CALL("Skolem::skolemise(Unit*)");
  ASS(! unit->isClause());

  unit = Rectify::rectify(unit);
  Formula* f = unit->formula();
  switch (f->connective()) {
  case FALSE:
  case TRUE:
    return unit;
  default:
    break;
  }

  static Skolem skol;
  skol.reset();
  return skol.skolemiseImpl(unit);
} // Skolem::skolemise

FormulaUnit* Skolem::skolemiseImpl (FormulaUnit* unit)
{
  CALL("Skolem::skolemiseImpl(FormulaUnit*)");

  ASS(_introducedSkolemFuns.isEmpty());
  _beingSkolemised=unit;

  _skolimizingDefinitions = UnitList::empty();

  ASS(UnitList::isEmpty(_skolimizingDefinitions));

  Formula* f = unit->formula();
  Formula* g = skolemise(f);

  _beingSkolemised = 0;

  if (f == g) { // not changed
    return unit;
  }

  UnitList* premiseList = new UnitList(unit,_skolimizingDefinitions);

  Inference* inf = new InferenceMany(Inference::SKOLEMIZE,premiseList);
  FormulaUnit* res = new FormulaUnit(g, inf, unit->inputType());

  ASS(_introducedSkolemFuns.isNonEmpty());
  while(_introducedSkolemFuns.isNonEmpty()) {
    unsigned fn = _introducedSkolemFuns.pop();
    InferenceStore::instance()->recordIntroducedSymbol(res,true,fn);
  }

  return res;
}


void Skolem::reset()
{
  CALL("Skolem::reset");

  _vars.reset();
  _varSorts.reset();
  _subst.reset();
}

unsigned Skolem::addSkolemFunction(unsigned arity, unsigned* domainSorts,
    unsigned rangeSort, unsigned var)
{
  CALL("Skolem::addSkolemFunction(unsigned,unsigned*,unsigned,unsigned)");

  if(VarManager::varNamePreserving()) {
    vstring varName=VarManager::getVarName(var);
    return addSkolemFunction(arity, domainSorts, rangeSort, varName.c_str());
  }
  else {
    return addSkolemFunction(arity, domainSorts, rangeSort);
  }
}

unsigned Skolem::addSkolemFunction(unsigned arity, unsigned* domainSorts,
    unsigned rangeSort, const char* suffix)
{
  CALL("Skolem::addSkolemFunction(unsigned,unsigned*,unsigned,const char*)");
  ASS(arity==0 || domainSorts!=0);

  unsigned fun = env.signature->addSkolemFunction(arity, suffix);
  Signature::Symbol* fnSym = env.signature->getFunction(fun);
  fnSym->setType(new FunctionType(arity, domainSorts, rangeSort));
  return fun;
}

void Skolem::ensureHavingVarSorts()
{
  CALL("Skolem::ensureHavingVarSorts");

  Formula* f = _beingSkolemised->formula();
  SortHelper::collectVariableSorts(f, _varSorts);
}

Term* Skolem::createSkolemTerm(unsigned var)
{
  CALL("Skolem::createSkolemFunction");

  int arity = _vars.length();

  ensureHavingVarSorts();
  unsigned rangeSort=_varSorts.get(var, Sorts::SRT_DEFAULT);
  static Stack<unsigned> domainSorts;
  static Stack<TermList> fnArgs;
  domainSorts.reset();
  fnArgs.reset();

  VarStack::TopFirstIterator vit(_vars);
  while(vit.hasNext()) {
    unsigned uvar = vit.next();
    domainSorts.push(_varSorts.get(uvar, Sorts::SRT_DEFAULT));
    fnArgs.push(TermList(uvar, false));
  }

  unsigned fun = addSkolemFunction(arity, domainSorts.begin(), rangeSort, var);
  _introducedSkolemFuns.push(fun);

  return Term::create(fun, arity, fnArgs.begin());
}

/**
 * Skolemise a subformula.
 *
 * @param f the subformula
 *
 * @since 28/06/2002 Manchester
 * @since 04/09/2002 Bolzano, changed
 * @since 05/09/2002 Trento, changed
 * @since 19/01/2002 Manchester, information about 
 *        positions and inferences added.
 * @since 23/01/2004 Manchester, changed to use non-static functions
 * @since 31/01/2004 Manchester, simplified to work with rectified formulas
 * @since 11/12/2004 Manchester, true and false added
 * @since 12/12/2004 Manchester, optimised by quantifying only over
 *    variables actually occurring in the formula.
 * @since 28/12/2007 Manchester, changed to new datastructures
 */
Formula* Skolem::skolemise (Formula* f)
{
  CALL("Skolem::skolemise (Formula*)");

  switch (f->connective()) {
  case LITERAL: 
    {
      Literal* l = f->literal();
      Literal* ll = l->apply(_subst);
      if (l == ll) {
	return f;
      }
      return new AtomicFormula(ll);
    }

  case AND:
  case OR: 
    {
      FormulaList* fs = skolemise(f->args());
      if (fs == f->args()) {
	return f;
      }
      return new JunctionFormula(f->connective(),fs);
    }

  case FORALL: 
    {
      int ln = _vars.length();
      Formula::VarList::Iterator vs(f->vars());
      while (vs.hasNext()) {
	_vars.push(vs.next());
      }
      Formula* g = skolemise(f->qarg());
      _vars.truncate(ln);
      if (g == f->qarg()) {
	return f;
      }
      return new QuantifiedFormula(f->connective(),f->vars(),f->sorts(),g);
    }

  case EXISTS: 
    {
      int arity = _vars.length();

      Formula::VarList::Iterator vs(f->vars());
      while (vs.hasNext()) {
        int v = vs.next();
        Term* skolemTerm = createSkolemTerm(v);
        _subst.bind(v,skolemTerm);

        if (env.options->showSkolemisations()) {
          env.beginOutput();
          env.out() << "Skolemising: "<<skolemTerm->toString()<<" for X"<< v
              <<" in "<<f->toString()<<" in formula "<<_beingSkolemised->toString();
          env.endOutput();
        }

        if (env.options->showNonconstantSkolemFunctionTrace() && arity!=0) {
          env.beginOutput();
          ostream& out = env.out();
          out <<"Nonconstant skolem function introduced: "
              <<skolemTerm->toString()<<" for X"<<v<<" in "<<f->toString()
              <<" in formula "<<_beingSkolemised->toString()<<endl;

          Refutation ref(_beingSkolemised, true);
          ref.output(out);
          env.endOutput();
        }
      }

      {
        Formula* def = new BinaryFormula(IFF, f, SubstHelper::apply(f->qarg(), _subst));

        if (arity > 0) {
          Formula::VarList* args = Formula::VarList::empty();
          VarStack::Iterator vit(_vars);
          Formula::VarList::pushFromIterator(vit,args);
          def = new QuantifiedFormula(FORALL,args,nullptr,def);
        }

        Unit* defUnit = new FormulaUnit(def, new Inference(Inference::CHOICE_AXIOM), Unit::AXIOM);
        UnitList::push(defUnit,_skolimizingDefinitions);
      }
      
      Formula* g = skolemise(f->qarg());
      vs.reset(f->vars());
      while (vs.hasNext()) {
	_subst.unbind(vs.next());
      }
      _vars.truncate(arity);

      return g;
    }

  case BOOL_TERM:
    ASSERTION_VIOLATION;

  case TRUE:
  case FALSE:
    return f;

#if VDEBUG
  default:
    ASSERTION_VIOLATION;
#endif
  }
} // Skolem::skolemise


/**
 * Skolemise a list of formulas in NFF.
 *
 * @since 28/06/2002 Manchester
 * @since 04/09/2002 Bolzano, changed
 * @since 19/01/2002 Manchester, information about 
 *        positions and inferences added.
 * @since 23/01/2004 Manchester, changed to use non-static functions
 * @since 12/12/2004 Manchester, optimised by quantifying only over
 *    variables actually occurring in the formula.
 */
FormulaList* Skolem::skolemise (FormulaList* fs) 
{
  CALL("skolemise (FormulaList*)");

  if (FormulaList::isEmpty(fs)) {
    return fs;
  }

  Formula* g = fs->head();
  FormulaList* gs = fs->tail();
  Formula* h = skolemise(g);
  FormulaList* hs = skolemise(gs);

  if (gs == hs && g == h) {
    return fs;
  }
  return new FormulaList(h,hs);
} // Skolem::skolemise


