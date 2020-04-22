/**
 * @file ApplicativeHelper.hpp
 * Defines class ApplicativeHelper.
 */

#ifndef __ApplicativeHelper__
#define __ApplicativeHelper__

#include "Forwards.hpp"
#include "Signature.hpp"
#include "Lib/Deque.hpp"
#include "Lib/BiMap.hpp"

using namespace Kernel;
using namespace Shell;

/**
 * A class with function @b elimLambda() that eliminates a lambda expressions
 * It does this by applying the well known rewrite rules for SKIBC combinators.
 *
 * These can be found:
 * https://en.wikipedia.org/wiki/Combinatory_logic
 */
class ApplicativeHelper {
public:

  struct HigherOrderTermInfo
  {
  public:
    HigherOrderTermInfo(TermList h, TermList hs, unsigned an){
      head = h;
      headSort = hs;
      argNum = an;
    }

    TermList head;
    TermList headSort;
    unsigned argNum;
  };

  ApplicativeHelper() {};

  static TermList createAppTerm(TermList sort, TermList arg1, TermList arg2);
  static TermList createAppTerm(TermList s1, TermList s2, TermList arg1, TermList arg2, bool shared = true);
  static TermList createAppTerm3(TermList sort, TermList arg1, TermList arg2, TermList arg3);
  static TermList createAppTerm(TermList sort, TermList arg1, TermList arg2, TermList arg3, TermList arg4); 
  static TermList createAppTerm(TermList sort, TermList head, TermStack& terms); 
  static TermList createAppTerm(TermList sort, TermList head, TermList* args, unsigned arity, bool shared = true); 
  static TermList getNthArg(TermList arrowSort, unsigned argNum);
  static TermList getResultApplieadToNArgs(TermList arrowSort, unsigned argNum);
  static TermList getResultSort(TermList sort);
  static unsigned getArity(TermList sort);
  static void getHeadAndAllArgs(TermList term, TermList& head, TermStack& args); 
  static void getHeadAndArgs(TermList term, TermList& head, TermStack& args); 
  static void getHeadAndArgs(Term* term, TermList& head, TermStack& args);  
  static void getHeadAndArgs(const Term* term, TermList& head, Deque<TermList>& args); 
  static void getHeadSortAndArgs(TermList term, TermList& head, TermList& headSort, TermStack& args); 
  static bool isComb(const TermList t);
  static Signature::Combinator getComb(const TermList t);
  static TermList getHead(TermList t);
  static bool isApp(const Term* t); 
  static bool isType(const Term* t);
  static bool isArrowType(const Term* t);
  static bool isApp(const TermList* tl);
  static bool isUnderApplied(TermList head, unsigned argNum);
  static bool isExactApplied(TermList head, unsigned argNum);
  static bool isOverApplied(TermList head, unsigned argNum);
  static bool isSafe(TermStack& args);
  static TermList replaceFunctionalAndBooleanSubterms(Term* term, FuncSubtermMap* fsm);

private:

  static TermList getVSpecVar(Term* funcTerm, FuncSubtermMap* fsm)
  {
    if(fsm->find2(funcTerm)){
      unsigned vNum = fsm->get2(funcTerm);
      ASS(vNum > TermList::SPEC_UPPER_BOUND);
      return TermList(vNum, true);
    } else {
      unsigned vNum = TermList::SPEC_UPPER_BOUND + fsm->size() + 1;
      fsm->insert(vNum, funcTerm);
      return TermList(vNum, true);
    }
  }
  
};

#endif // __ApplicativeHelper__