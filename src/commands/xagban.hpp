/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file xagban.hpp
 *
 * @brief TODO
 *
 * @author Zhufei
 * @since  0.1
 */

#ifndef XAGBAN_HPP
#define XAGBAN_HPP

#include <mockturtle/networks/xag.hpp>
#include <mockturtle/algorithms/balancing.hpp>
#include <mockturtle/algorithms/balancing/esop_balancing.hpp>

#include "../core/misc.hpp"

namespace alice
{

  class xagban_command : public command
  {
    public:
      explicit xagban_command( const environment::ptr& env ) : command( env, "Performs XAG balancing" )
      {
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

         xag = balancing( xag, {esop_rebalancing<xag_network>{}});
         
         xag = cleanup_dangling( xag );
         
         std::cout << "[xagban] "; 
         also::print_stats( xag ); 

         store<xag_network>().extend(); 
         store<xag_network>().current() = xag;
      }
  };

  ALICE_ADD_COMMAND( xagban, "Rewriting" )

}

#endif
