/*******************************************************************\

Module:

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

#ifndef CPROVER_NAMESPACE_H
#define CPROVER_NAMESPACE_H

#include "irep.h"
#include <iostream>

class symbol_tablet;
class exprt;
class symbolt;
class typet;
class union_tag_typet;
class union_typet;
class struct_tag_typet;
class struct_typet;
class c_enum_tag_typet;

class namespace_baset
{
public:
  // service methods
  const symbolt &lookup(const irep_idt &name) const
  {
    const symbolt *symbol;
    if(lookup(name, symbol))
      throw "identifier "+id2string(name)+" not found";
    return *symbol;
  }
  
  const symbolt &lookup(const irept &irep) const
  {
    return lookup(irep.get(ID_identifier));
  }
  
  symbolt &get_symbol(const irep_idt &name) const
  {
    symbolt *symbol;
    if(get_symbol(name, symbol))
      throw "identifier "+id2string(name)+" not found";
    return *symbol;
  }

  virtual ~namespace_baset();
   
  void follow_symbol(irept &irep) const;
  void follow_macros(exprt &expr) const;
  const typet &follow(const typet &src) const;

  const union_typet &follow_tag(const union_tag_typet &src) const;
  const struct_typet &follow_tag(const struct_tag_typet &src) const;
  const typet &follow_tag(const c_enum_tag_typet &src) const;

  // these do the actual lookup
  virtual unsigned get_max(const std::string &prefix) const=0;
  virtual bool lookup(const irep_idt &name, const symbolt *&symbol) const=0;
  virtual bool get_symbol(const irep_idt &name, symbolt *&symbol) const=0;
};

/*! \brief TO_BE_DOCUMENTED
*/
class namespacet:public namespace_baset
{
public:
  // constructors
  explicit namespacet(symbol_tablet &_symbol_table)
  { symbol_table1=&_symbol_table; symbol_table2=NULL; }
   
  namespacet(symbol_tablet &_symbol_table1, symbol_tablet &_symbol_table2)
  { symbol_table1=&_symbol_table1; symbol_table2=&_symbol_table2; }
  
  namespacet(symbol_tablet *_symbol_table1, symbol_tablet *_symbol_table2)
  { symbol_table1=_symbol_table1; symbol_table2=_symbol_table2; }
 
  using namespace_baset::lookup;
  using namespace_baset::get_symbol;
   
  // these do the actual lookup
  virtual bool lookup(const irep_idt &name, const symbolt *&symbol) const;
  virtual bool get_symbol(const irep_idt &name, symbolt *&symbol) const;
  virtual unsigned get_max(const std::string &prefix) const;
  
  symbol_tablet &get_symbol_table() const
  {
    return *symbol_table1;
  }
  
protected:
  symbol_tablet *symbol_table1, *symbol_table2;
};

class multi_namespacet:public namespacet
{
public:
  // constructors
  multi_namespacet():namespacet(NULL, NULL)
  {
  }

  explicit multi_namespacet(const symbol_tablet &symbol_table):namespacet(NULL, NULL)
  {
    add(symbol_table);
  }

  // these do the actual lookup
  using namespace_baset::lookup;
  
  virtual bool lookup(const irep_idt &name, const symbolt *&symbol) const;
  virtual unsigned get_max(const std::string &prefix) const;
  
  inline void add(const symbol_tablet &symbol_table)
  {
    symbol_table_list.push_back(&symbol_table);
  }
  
protected:
  typedef std::vector<const symbol_tablet *> symbol_table_listt;
  symbol_table_listt symbol_table_list;
};
 
#endif
