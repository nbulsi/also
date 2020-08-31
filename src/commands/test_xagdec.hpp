/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file test_xagdec.hpp
 *
 * @brief test the combined decompostion method with target
 * network of XAG
 *
 * @author Zhufei
 * @since  0.1
 */

#ifndef TEST_XAGDEC_HPP
#define TEST_XAGDEC_HPP

#include <mockturtle/networks/xag.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <kitty/dynamic_truth_table.hpp>

#include "../networks/aoig/xag_dec.hpp"
#include "../core/misc.hpp"

namespace alice
{

  class test_xagdec_command: public command
  {
      public:
      explicit test_xagdec_command( const environment::ptr& env ) : command( env, " test the performance of combined tt decomposition" )
      {
      }
      
      rules validity_rules() const
      {
        return { has_store_element<optimum_network>( env ) };
      }

      protected:
      void execute()
      {
        //kitty::dynamic_truth_table table( 5u );
        //kitty::create_from_expression( table, "{a<bc(de)>}" );

        auto& opt = store<optimum_network>().current();
        kitty::dynamic_truth_table table = opt.function;

        xag_network xag;
        std::vector<xag_network::signal> pis( table.num_vars() );
        std::generate( pis.begin(), pis.end(), [&]() { return xag.create_pi(); } );

        xag.create_po( also::xag_dec( xag, table, pis ) );

        default_simulator<kitty::dynamic_truth_table> sim( table.num_vars() );
        auto res = simulate<kitty::dynamic_truth_table>( xag, sim )[0];

        std::cout << " table : " << kitty::to_binary( table ) << std::endl;
        std::cout << " sim   : " << kitty::to_binary( res ) << std::endl;
        assert( res == table && "Simulation results is not equal as the spec" );

        also::print_stats( xag );
        store<xag_network>().extend(); 
        store<xag_network>().current() = xag;
      }
  };

  ALICE_ADD_COMMAND( test_xagdec, "Various" )

}

#endif
