/*******************************************************************\

Module: CBMC Main Module 

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

/*

  CBMC
  Bounded Model Checking for ANSI-C
  Copyright (C) 2001-2014 Daniel Kroening <kroening@kroening.com>

*/

#include <util/unicode.h>
#include <util/time_stopping.h>
#include <fstream>
#include <iostream>
#ifdef IREP_HASH_STATS
#include <iostream>
#endif

#include "cbmc_parseoptions.h"

/*******************************************************************\

Function: main / wmain

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

#ifdef IREP_HASH_STATS
extern unsigned long long irep_hash_cnt;
extern unsigned long long irep_cmp_cnt;
extern unsigned long long irep_cmp_ne_cnt;
#endif

#ifdef _MSC_VER
int wmain(int argc, const wchar_t **argv_wide)
{
  const char **argv=narrow_argv(argc, argv_wide);
#else
int main(int argc, const char **argv)
{
#endif

	std::cout << "This product includes software developed by Daniel Kroening, Edmund Clarke, \n";
    std::cout << "Computer Science Department, University of Oxford \n";
    std::cout << "Computer Science Department, Carnegie Mellon University\n";
  absolute_timet t=current_time();

  cbmc_parseoptionst parseoptions(argc, argv);

  int res=parseoptions.main();

  #ifdef IREP_HASH_STATS
  std::cout << "IREP_HASH_CNT=" << irep_hash_cnt << std::endl;
  std::cout << "IREP_CMP_CNT=" << irep_cmp_cnt << std::endl;
  std::cout << "IREP_CMP_NE_CNT=" << irep_cmp_ne_cnt << std::endl;
  #endif
//	std::ofstream out("cega.result", std::ios::app);
//	out << current_time() - t;
//	out.close();
  return res;
}
