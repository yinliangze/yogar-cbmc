/*******************************************************************\

Module: Memory model for partial order concurrency

Author: Michael Tautschnig, michael.tautschnig@cs.ox.ac.uk

\*******************************************************************/

#include <util/std_expr.h>
#include <util/i2string.h>

#include "memory_model.h"
#include <iostream>
#include <map>

/*******************************************************************\

Function: memory_model_baset::memory_model_baset

  Inputs: 

 Outputs:

 Purpose:

\*******************************************************************/

memory_model_baset::memory_model_baset(const namespacet &_ns):
  partial_order_concurrencyt(_ns),
  var_cnt(0)
{
}

/*******************************************************************\

Function: memory_model_baset::~memory_model_baset

  Inputs: 

 Outputs:

 Purpose:

\*******************************************************************/

memory_model_baset::~memory_model_baset()
{
}

/*******************************************************************\

Function: memory_model_baset::nondet_bool_symbol

  Inputs: 

 Outputs:

 Purpose:

\*******************************************************************/

symbol_exprt memory_model_baset::nondet_bool_symbol(
  const std::string &prefix)
{
  return symbol_exprt(
    "memory_model::choice_"+prefix+i2string(var_cnt++),
    bool_typet());
}

/*******************************************************************\

Function: memory_model_baset::po

  Inputs: 

 Outputs:

 Purpose:

\*******************************************************************/

bool memory_model_baset::po(event_it e1, event_it e2)
{
  // within same thread
  if(e1->source.thread_nr==e2->source.thread_nr)
    return numbering[e1]<numbering[e2];
  else
  {
    // in general un-ordered, with exception of thread-spawning
    return false;
  }
}

/*******************************************************************\

Function: memory_model_baset::read_from

  Inputs: 

 Outputs:

 Purpose:

\*******************************************************************/
void memory_model_baset::read_from_backup(symex_target_equationt &equation)
{
  // We iterate over all the reads, and
  // make them match at least one
  // (internal or external) write.

  for(address_mapt::const_iterator
      a_it=address_map.begin();
      a_it!=address_map.end();
      a_it++)
  {
    const a_rect &a_rec=a_it->second;

    for(event_listt::const_iterator
        r_it=a_rec.reads.begin();
        r_it!=a_rec.reads.end();
        r_it++)
    {
      const event_it r=*r_it;

      exprt::operandst rf_some_operands;
      rf_some_operands.reserve(a_rec.writes.size());

      // this is quadratic in #events per address
      for(event_listt::const_iterator
          w_it=a_rec.writes.begin();
          w_it!=a_rec.writes.end();
          ++w_it)
      {
        const event_it w=*w_it;

        // rf cannot contradict program order
        if(po(r, w))
          continue; // contradicts po

        bool is_rfi=
          w->source.thread_nr==r->source.thread_nr;

        symbol_exprt s=nondet_bool_symbol("rf");

        // record the symbol
        choice_symbols[
          std::make_pair(r, w)]=s;

        // We rely on the fact that there is at least
        // one write event that has guard 'true'.
        implies_exprt read_from(s,
            and_exprt(w->guard,
              equal_exprt(r->ssa_lhs, w->ssa_lhs)));

        // Uses only the write's guard as precondition, read's guard
        // follows from rf_some
        add_constraint(equation,
          read_from, is_rfi?"rfi":"rf", r->source);

        if(!is_rfi)
        {
          // if r reads from w, then w must have happened before r
          exprt cond=implies_exprt(s, before(w, r));
          add_constraint(equation,
            cond, "rf-order", r->source);
        }

        // added by ylz
		equation.choice_symbol_map[s] = new symex_target_equationt::eq_edge(&(*w), &(*r));

        rf_some_operands.push_back(s);
      }

      // value equals the one of some write
      exprt rf_some;

      // uninitialised global symbol like symex_dynamic::dynamic_object*
      // or *$object
      if(rf_some_operands.empty())
        continue;
      else if(rf_some_operands.size()==1)
        rf_some=rf_some_operands.front();
      else
      {
        rf_some=or_exprt();
        rf_some.operands().swap(rf_some_operands);
      }

      // Add the read's guard, each of the writes' guards is implied
      // by each entry in rf_some
      add_constraint(equation,
        implies_exprt(r->guard, rf_some), "rf-some", r->source);
    }
  }
}

bool memory_model_baset::valid_mutex(symex_target_equationt &equation)
{
	int mutex_num = 0;
	for(eventst::const_iterator
		e_it=equation.SSA_steps.begin();
		e_it!=equation.SSA_steps.end();
		e_it++)
	{
		// concurreny-related?
		if(e_it->is_verify_atomic_begin())
			mutex_num++;
	}
	return (mutex_num != 1);
}

void memory_model_baset::read_from(symex_target_equationt &equation)
{
	per_thread_mapt per_thread_map;
	for(eventst::const_iterator e_it=equation.SSA_steps.begin();
	  e_it!=equation.SSA_steps.end(); e_it++)
	{
		if((is_shared_read(e_it) ||
			is_shared_write(e_it) ||
			e_it->is_verify_atomic_begin() ||
			e_it->is_verify_atomic_end() ||
			e_it->is_thread_create() ||
			e_it->is_thread_join()))
		{
//			if (address(e_it) == "c::__global_lock" || (equation.aux_enable && e_it->is_aux_var() && !equation.thread_malloc))
//				continue;
			per_thread_map[e_it->source.thread_nr].push_back(e_it);
		}
	}
	int thread_num = per_thread_map.size();

	// iterate over threads
	for(per_thread_mapt::const_iterator
	  t_it=per_thread_map.begin();
	  t_it!=per_thread_map.end();
	  t_it++)
	{
		const event_listt &events=t_it->second;
		std::map<irep_idt, event_it> event_value_map;
		bool atomic_flag = false;
		bool single_thread_flag = false;

		int curr_threads = (((*(events.begin()))->source.thread_nr == 0) ? 1 : 100);

		for(event_listt::const_iterator e_it=events.begin();
			e_it!=events.end(); e_it++)
		{
			event_it e = *e_it;

			if (e->is_thread_create() && e->source.thread_nr == 0) {
				curr_threads++;
				event_value_map.clear();
			}

			if (e->is_thread_join() && e->source.thread_nr == 0) {
				curr_threads--;
				event_value_map.clear();
			}

			single_thread_flag = (curr_threads == 1 ? true : false);

			if (e->is_verify_atomic_begin() && valid_mutex(equation)) {
				atomic_flag = true;
				event_value_map.clear();
			}
			else if (e->is_verify_atomic_end()) {
				atomic_flag = false;
				event_value_map.clear();
			}
			else if (atomic_flag || single_thread_flag) {
				if (is_shared_read(e)) {
					if (e->rely)
					{
						if (event_value_map.find(address(e)) == event_value_map.end()) {
							read_from_item(e, equation, thread_num);
							event_value_map[address(e)] = e;
						}
						else {
							add_constraint(equation, implies_exprt(e->guard, equal_exprt(e->ssa_lhs, event_value_map[address(e)]->ssa_lhs)), "rfi", e->source);
						}
					}
				}
				else if (is_shared_write(e)) {
					event_value_map[address(e)] = e;
				}
			}
			else {
				assert(!atomic_flag);
				if (is_shared_read(e) && e->rely) {
					read_from_item(e, equation, thread_num);
				}
			}
		}
	}

	if (!array_map.empty()) {
		for (array_mapt::iterator it = array_map.begin(); it != array_map.end(); it++) {
			choice_listt& c_list = it->second;
			for (choice_listt::iterator mt = c_list.begin(); mt != c_list.end(); mt++) {
				for (choice_listt::iterator nt = mt + 1; nt != c_list.end(); nt++)	{
					add_constraint(equation, implies_exprt((*mt), not_exprt(*nt)), "array_assign", symex_targett::sourcet());
				}
			}
		}
	}
}

void memory_model_baset::read_from_item(const event_it& r, symex_target_equationt &equation, int thread_num) {
	const a_rect &a_rec=address_map[address(r)];

	event_listt rfwrites;

    exprt::operandst rf_some_operands;
    rf_some_operands.reserve(a_rec.writes.size());

    // this is quadratic in #events per address
    for(event_listt::const_iterator
        w_it=a_rec.writes.begin();
        w_it!=a_rec.writes.end();
        ++w_it)
    {
      const event_it w=*w_it;
      bool is_rfi = (r->source.thread_nr==w->source.thread_nr);
      if(po(r, w))
    	  continue; // contradicts po
      if (is_rfi && !(equation.aux_enable && w->is_aux_var()))
      {
    	  rfwrites.push_back(w);
      }
      else
      {
    	int symmetry_start = thread_num > 20 ? 2 : 1;
      	if (thread_num > 10 && r->source.thread_nr >= symmetry_start && r->source.thread_nr < w->source.thread_nr)
      		continue;

		symbol_exprt s=nondet_bool_symbol("rf");

		// record the symbol
		choice_symbols[std::make_pair(r, w)]=s;

		// We rely on the fact that there is at least
		// one write event that has guard 'true'.
		equal_exprt rw = equal_exprt(r->ssa_lhs, w->ssa_lhs);
		or_exprt read_from(not_exprt(s), and_exprt(rw, w->guard));

		if (r->array_assign || r->ssa_lhs.get_identifier() == "c::array#2") {
//		if (r->array_assign) {
			array_map[w->ssa_lhs.get_identifier()].push_back(s);
		}

		// add the rf relation to the amp
		equation.choice_symbol_map[s] = new symex_target_equationt::eq_edge(&(*w), &(*r));

		// Uses only the write's guard as precondition, read's guard
		// follows from rf_some
		add_constraint(equation,
		  read_from, "rf", r->source);

		rf_some_operands.push_back(s);
      }
    }

    event_listt::const_iterator w_it, wt_it;
    for(w_it=rfwrites.begin(); w_it!=rfwrites.end(); ++w_it)
    {
  	  for(wt_it=rfwrites.begin(); wt_it!=rfwrites.end(); ++wt_it)
  	  {
  		  if (&(*(*w_it)) != &(*(*wt_it)) && po(*w_it, *wt_it))
//  			if (&(*(*w_it)) != &(*(*wt_it)) && po(*w_it, *wt_it) && ((*w_it)->guard == (*wt_it)->guard))
  			  break;
  	  }
//  	  std::cout << r->ssa_lhs.get_identifier() << ", " << (*w_it)->ssa_lhs.get_identifier() << "\n";
  	  if ((*w_it)->original_lhs_object.get_identifier() != "c::array_index" || wt_it == rfwrites.end())
//  	  if (wt_it == rfwrites.end())
  	  {
  		  const event_it w=*w_it;
  		  symbol_exprt s=nondet_bool_symbol("rf");

  		  // record the symbol
  		  choice_symbols[std::make_pair(r, w)]=s;

  		  // We rely on the fact that there is at least
  		  // one write event that has guard 'true'.
  		  equal_exprt rw = equal_exprt(r->ssa_lhs, w->ssa_lhs);
  		  or_exprt read_from(not_exprt(s), and_exprt(rw, w->guard));

    	  if (r->array_assign || r->ssa_lhs.get_identifier() == "c::array#2") {
  //  	  if (r->array_assign) {
    		  array_map[w->ssa_lhs.get_identifier()].push_back(s);
    	  }

		  // add the rf relation to the amp
		  equation.choice_symbol_map[s] = new symex_target_equationt::eq_edge(&(*w), &(*r));

  		  // Uses only the write's guard as precondition, read's guard
  		  // follows from rf_some
  		  add_constraint(equation, read_from, "rfi", r->source);

  		  rf_some_operands.push_back(s);
  	  }
    }

    // value equals the one of some write
    exprt rf_some;

    // uninitialised global symbol like symex_dynamic::dynamic_object*
    // or *$object
    if(rf_some_operands.empty())
      return;
    else if(rf_some_operands.size()==1)
      rf_some=rf_some_operands.front();
    else
    {
      rf_some=or_exprt();
      rf_some.operands().swap(rf_some_operands);
    }

    // Add the read's guard, each of the writes' guards is implied
    // by each entry in rf_some
    add_constraint(equation,
      implies_exprt(r->guard, rf_some), "rf-some", r->source);
}
