/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file nniinv.hpp
 *
 * @brief inversion optimization of XMGs
 *
 * @author hmtian
 * @since  0.1
 */

#ifndef NNI_COST_HPP
#define NNI_COST_HPP

#include <mockturtle/mockturtle.hpp>
#include <mockturtle/utils/stopwatch.hpp>
#include "../core/nni_cost.hpp"

namespace alice
{

  class nnicost_command: public command
  {
    public:
      explicit nnicost_command( const environment::ptr& env ) 
               : command( env, "show the cost og XMG after optimizing using nni gate" )
      {
        add_flag( "--verbose, -v", "print the information" );
      }

      rules validity_rules() const
      {
        return { has_store_element<xmg_network>( env ) };
      }


    protected:
      void execute()
      {
        /* derive some MIG */
         xmg_network xmg = store<xmg_network>().current();
         
         stopwatch<>::duration time{0};
         call_with_stopwatch( time, [&]() { also::xmg2nni_inv_optimization( xmg ); } );

         std::cout << fmt::format( "[time]: {:5.2f} seconds\n", to_seconds( time ) );
      }

  };

  ALICE_ADD_COMMAND( nnicost, "Optimization" )

}

#endif
