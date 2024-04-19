/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file xmginv.hpp
 *
 * @brief inversion optimization of XMGs
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef XMGINV_HPP
#define XMGINV_HPP

#include <mockturtle/mockturtle.hpp>
#include <mockturtle/utils/stopwatch.hpp>
#include "../core/xmg_inv.hpp"

namespace alice
{

  class xmginv_command: public command
  {
    public:
      explicit xmginv_command( const environment::ptr& env )
               : command( env, "inversion optimization of xmg" )
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
        /* derive some XMG */
         xmg_network xmg = store<xmg_network>().current();
         xmg_network xmg_opt;

         stopwatch<>::duration time{0};
         call_with_stopwatch( time, [&]() { xmg_opt = also::xmg_inv_optimization( xmg );
             } );

         store<xmg_network>().extend();
         store<xmg_network>().current() = xmg_opt;
         std::cout << fmt::format( "[time]: {:5.2f} seconds\n", to_seconds( time ) );
      }

  };

  ALICE_ADD_COMMAND( xmginv, "Optimization" )

}

#endif
