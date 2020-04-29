/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file nniinv.hpp
 *
 * @brief inversion optimization of MIGs
 *
 * @author hmtian
 * @since  0.1
 */

#ifndef NNIINV_HPP
#define NNIINV_HPP

#include <mockturtle/mockturtle.hpp>
#include <mockturtle/utils/stopwatch.hpp>
#include "../core/nni_inv.hpp"

namespace alice
{

  class nniinv_command: public command
  {
    public:
      explicit nniinv_command( const environment::ptr& env ) 
               : command( env, "inversion optimization of mig" )
      {
        add_flag( "--verbose, -v", "print the information" );
      }

      rules validity_rules() const
      {
        return { has_store_element<mig_network>( env ) };
      }


    protected:
      void execute()
      {
        /* derive some MIG */
         mig_network mig = store<mig_network>().current();
         
         stopwatch<>::duration time{0};
         call_with_stopwatch( time, [&]() { also::mig_inv_optimization( mig ); } );

         std::cout << fmt::format( "[time]: {:5.2f} seconds\n", to_seconds( time ) );
      }

  };

  ALICE_ADD_COMMAND( nniinv, "Optimization" )

}

#endif
