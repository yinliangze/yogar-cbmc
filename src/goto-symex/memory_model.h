/*******************************************************************\

Module: Memory models for partial order concurrency

Author: Michael Tautschnig, michael.tautschnig@cs.ox.ac.uk

\*******************************************************************/

#ifndef CPROVER_MEMORY_MODEL_H
#define CPROVER_MEMORY_MODEL_H

#include "partial_order_concurrency.h"
#include "../cbmc/eog.h"

class memory_model_baset:public partial_order_concurrencyt
{
public:
  explicit memory_model_baset(const namespacet &_ns);
  virtual ~memory_model_baset();

  virtual void operator()(symex_target_equationt &)=0;
  
protected:
  // program order
  bool po(event_it e1, event_it e2);

  // produce fresh symbols  
  unsigned var_cnt;
  symbol_exprt nondet_bool_symbol(const std::string &prefix);
  
  // This gives us the choice symbol for an R-W pair;
  // built by the method below.
  typedef std::map<
    std::pair<event_it, event_it>, symbol_exprt> choice_symbolst;
  choice_symbolst choice_symbols;

  // maps thread numbers to an event list
  typedef std::map<unsigned, event_listt> per_thread_mapt;

  bool valid_mutex(symex_target_equationt &equation);

  void read_from(symex_target_equationt &equation);
  
  void read_from_item(const event_it& r, symex_target_equationt &equation, int thread_num);

  void read_from_backup(symex_target_equationt &equation);
};

#endif

