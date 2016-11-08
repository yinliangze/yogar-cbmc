/*******************************************************************\

Module: Bounded Model Checking for ANSI-C + HDL

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

#ifndef CPROVER_CBMC_BMC_H
#define CPROVER_CBMC_BMC_H

#include <list>
#include <map>

#include <util/hash_cont.h>
#include <util/options.h>

#include <solvers/prop/prop.h>
#include <solvers/prop/prop_conv.h>
#include <solvers/sat/cnf.h>
#include <solvers/sat/satcheck.h>
#include <solvers/smt1/smt1_dec.h>
#include <solvers/smt2/smt2_dec.h>
#include <langapi/language_ui.h>
#include <goto-symex/symex_target_equation.h>

#include "symex_bmc.h"
#include "eog.h"

class bmct:public messaget
{
	typedef std::vector<event_it> event_listt;
	typedef std::map<unsigned, event_listt> per_thread_mapt;

public:
  bmct(
    const optionst &_options,
    symbol_tablet &_symbol_table,
    message_handlert &_message_handler):
    messaget(_message_handler),
    options(_options),
    ns(_symbol_table, new_symbol_table),
    equation(ns),
    symex(ns, new_symbol_table, equation),
    gf_ptr(0),
    ui(ui_message_handlert::PLAIN)
  {
    symex.constant_propagation=options.get_bool_option("propagation");
  }
 
  virtual bool run(const goto_functionst &goto_functions);
  virtual ~bmct() { }

  // additional stuff   
  expr_listt bmc_constraints;  
  
  friend class cbmc_satt;
  friend class hw_cbmc_satt;
  friend class counterexample_beautification_greedyt;
  
  void set_ui(language_uit::uit _ui) { ui=_ui; }
  
protected:
  const optionst &options;  
  symbol_tablet new_symbol_table;
  namespacet ns;
  symex_target_equationt equation;
  symex_bmct symex;
  const goto_functionst *gf_ptr;
 
  // use gui format
  language_uit::uit ui;
  
  virtual decision_proceduret::resultt
    run_decision_procedure(prop_convt &prop_conv);

  virtual decision_proceduret::resultt incremental_solve(prop_convt &prop_conv, exprt& constraint);
    
  virtual bool decide(prop_convt &prop_conv);
    
  // the solvers we have
  virtual bool decide_default();
  virtual bool decide_bv_refinement();
  virtual bool decide_aig();
  virtual bool decide_smt1();
  virtual bool decide_smt2();
  smt1_dect::solvert get_smt1_solver_type() const;
  smt2_dect::solvert get_smt2_solver_type() const;
  virtual void smt1_convert(smt1_dect::solvert solver, std::ostream &out);
  virtual void smt2_convert(smt2_dect::solvert solver, std::ostream &out);
  virtual bool write_dimacs();
  virtual bool write_dimacs(std::ostream &out);
  
  // unwinding
  virtual void setup_unwind();

  virtual void do_unwind_module(
    decision_proceduret &decision_procedure);
  void do_conversion(prop_convt &solver);
  
  prop_convt *solver_factory();

  virtual void show_vcc();
  virtual bool all_properties(
    const goto_functionst &goto_functions,
    prop_convt &solver);
  virtual void show_vcc(std::ostream &out);
  virtual void show_program();
  virtual void report_success();
  virtual void report_failure();

  virtual void error_trace(
    const prop_convt &prop_conv);
  
  // vacuity checks
  void cover_assertions(
    const goto_functionst &goto_functions,
    prop_convt &solver);

  // for event-order-graphs, used for CGAR
  void build_eog(eog& graph, prop_convt &prop_conv);
  void add_nodes(eog& graph, prop_convt &prop_conv, bool trace_flag = true);
  void add_all_nodes(eog& graph);
  void add_program_order(eog& graph, prop_convt &prop_conv, bool trace_flag = true);
  void add_program_order_back1(eog& graph, prop_convt &prop_conv);
  void add_read_from(eog& graph, prop_convt &prop_conv);
  void thread_spawn(symex_target_equationt &equation,
    const per_thread_mapt &per_thread_map, prop_convt &prop_conv,eog& graph, bool trace_flag = true);
  bool is_true_counterexample(prop_convt &prop_conv, eog& graph);
  bool compute_refine_constraint(eog& graph, exprt& constraint);
  bool valid_mutex(symex_target_equationt &equation);
  void compute_init_constraint(prop_convt &prop_conv, exprt& constraint);
};

#endif
