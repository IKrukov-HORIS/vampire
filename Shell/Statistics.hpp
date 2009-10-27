/**
 * @file Statistics.hpp
 * Defines proof-search statistics
 *
 * @since 02/01/2008 Manchester
 */

#ifndef __Statistics__
#define __Statistics__

namespace Kernel {
  class Unit;
}

namespace Shell {

/**
 * Class Statistics
 * @since 02/01/2008 Manchester
 */
class Statistics
{
public:
  Statistics();

  void print();

  // Input
  /** number of input clauses */
  unsigned inputClauses;
  /** number of input formulas */
  unsigned inputFormulas;

  // Preprocessing
  /** number of formula names introduced during preprocessing */
  unsigned formulaNames;
  /** number of initial clauses */
  unsigned initialClauses;
  /** number of inequality splittings performed */
  unsigned splittedInequalities;
  /** number of pure predicates */
  unsigned purePredicates;
  /** number of unused predicate definitions */
  unsigned unusedPredicateDefinitions;
  /** number of eliminated function definitions */
  unsigned functionDefinitions;

  //Generating inferences
  /** number of clauses generated by factoring*/
  unsigned factoring;
  /** number of clauses generated by binary resolution*/
  unsigned resolution;
  /** number of clauses generated by forward superposition*/
  unsigned forwardSuperposition;
  /** number of clauses generated by backward superposition*/
  unsigned backwardSuperposition;
  /** number of clauses generated by self superposition*/
  unsigned selfSuperposition;
  /** number of clauses generated by equality factoring*/
  unsigned equalityFactoring;
  /** number of clauses generated by equality resolution*/
  unsigned equalityResolution;

  // Simplifying inferences
  /** number of duplicate literals deleted */
  unsigned duplicateLiterals;
  /** number of literals s |= s deleted */
  unsigned trivialInequalities;
  /** number of forward subsumption resolutions */
  unsigned forwardSubsumptionResolution;
  /** number of forward demodulations */
  unsigned forwardDemodulations;
  /** number of forward demodulations into equational tautologies */
  unsigned forwardDemodulationsToEqTaut;
  /** number of backward demodulations */
  unsigned backwardDemodulations;
  /** number of backward demodulations into equational tautologies */
  unsigned backwardDemodulationsToEqTaut;
  /** number of forward literal rewrites */
  unsigned forwardLiteralRewrites;
  /** number of condensations */
  unsigned condensations;
  /** number of evaluations */
  unsigned evaluations;

  // Deletion inferences
  /** number of tautologies A \/ ~A */
  unsigned simpleTautologies;
  /** number of equational tautologies s=s */
  unsigned equationalTautologies;
  /** number of forward subsumed clauses */
  unsigned forwardSubsumed;
  /** number of backward subsumed clauses */
  unsigned backwardSubsumed;
  /** number of subsumed empty clauses */
  unsigned subsumedEmptyClauses;
  /** number of empty clause subsumptions */
  unsigned emptyClauseSubsumptions;

  // Saturation
  /** all clauses ever occurring in the unprocessed queue */
  unsigned generatedClauses;
  /** all passive clauses */
  unsigned passiveClauses;
  /** all active clauses */
  unsigned activeClauses;

  unsigned discardedNonRedundantClauses;

  unsigned inferencesSkippedDueToColors;

  /** passive clauses at the end of the saturation algorithm run */
  unsigned finalPassiveClauses;
  /** active clauses at the end of the saturation algorithm run */
  unsigned finalActiveClauses;

  unsigned splittedClauses;
  unsigned splittedComponents;
  unsigned uniqueComponents;

  /** termination reason */
  enum TerminationReason {
    /** refutation found */
    REFUTATION,
    /** satisfiability detected (saturated set built) */
    SATISFIABLE,
    /** saturation terminated but an incomplete strategy was used */
    UNKNOWN,
    /** time limit reached */
    TIME_LIMIT,
    /** memory limit reached */
    MEMORY_LIMIT
  };
  /** termination reason */
  TerminationReason terminationReason;
  /** refutation, if any */
  Kernel::Unit* refutation;
}; // class Statistics

}

#endif
