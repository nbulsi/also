/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019-2021 Ningbo University, Ningbo, China */

/**
 * @file xmgban.hpp
 *
 * @brief balancing XMG
 *
 * @author Zhufei
 * @since  0.1
 */

#ifndef XMGBAN_HPP
#define XMGBAN_HPP

#include <mockturtle/algorithms/balancing.hpp>
#include <mockturtle/algorithms/balancing/sop_balancing.hpp>

#include "../core/misc.hpp"

namespace alice
{

  class xmgban_command : public command
  {
    public:
      explicit xmgban_command( const environment::ptr& env ) : command( env, "Performs XMG balancing" )
      {
      }
      
      rules validity_rules() const
      {
        return { has_store_element<xmg_network>( env ) };
      }

    protected:
      void execute()
      {
        /* derive some XAG */
         xmg_network xmg = store<xmg_network>().current();

         xmg = balancing( xmg, {sop_rebalancing<xmg_network>{}}); //TODO: we need maj-xor balancing
         
         xmg = cleanup_dangling( xmg );
         
         std::cout << "[xmgban] "; 
         auto xmg_copy = cleanup_dangling( xmg );
         also::print_stats( xmg_copy ); 

         store<xmg_network>().extend(); 
         store<xmg_network>().current() = xmg;
      }
  };

  ALICE_ADD_COMMAND( xmgban, "Rewriting" )


}

#endif
