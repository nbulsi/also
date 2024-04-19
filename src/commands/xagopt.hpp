/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file xagopt.hpp
 *
 * @brief xag_optimization
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef XAGOPT_HPP
#define XAGOPT_HPP

#include <mockturtle/mockturtle.hpp>
#include <mockturtle/algorithms/xag_optimization.hpp>

#include "../core/misc.hpp"

namespace alice
{

  class xagopt_command : public command
  {
    public:
      explicit xagopt_command( const environment::ptr& env ) : command( env, "Performs XAG optimization" )
      {
        add_flag( "-v,--verbose", "show statistics" );
      }
      
      rules validity_rules() const
      {
        return { has_store_element<xag_network>( env ) };
      }

    protected:
      void execute()
      {
        /* derive some XAG */
         xag_network xag = store<xag_network>().current();
         
         xag = xag_constant_fanin_optimization( xag );

         std::cout << "[xagopt: constant_fanin] "; 
         also::print_stats( xag ); 
         
         xag = xag_dont_cares_optimization( xag );

         std::cout << "[xagopt: dc] "; 
         also::print_stats( xag ); 

         store<xag_network>().extend(); 
         store<xag_network>().current() = xag;
      }
  };

  ALICE_ADD_COMMAND( xagopt, "Rewriting" )

}

#endif
