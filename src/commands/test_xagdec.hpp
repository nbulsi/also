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
#include "../networks/aoig/build_xag_db.hpp"

namespace alice
{

  class test_xagdec_command: public command
  {
      public:
      explicit test_xagdec_command( const environment::ptr& env ) : command( env, " test the performance of combined tt decomposition" )
      {
        add_flag( "--random_test, -r", "enable random function test" );
        add_flag( "--disable_npn, -d", "disable NPN during decomposition" );
      }
      
      protected:
      void execute()
      {
        xag_network xag;
        auto opt_xags = also::load_xag_string_db( xag );
        ps.with_npn4 = is_set( "disable_npn" ) ? false : true;

        if( !is_set( "random_test" ) )
        {
          assert( store<optimum_network>().size() != 0u );
          auto& opt = store<optimum_network>().current();
          kitty::dynamic_truth_table table = opt.function;

          xag_network xag;
          std::vector<xag_network::signal> pis( table.num_vars() );
          std::generate( pis.begin(), pis.end(), [&]() { return xag.create_pi(); } );

          xag.create_po( also::xag_dec( xag, table, pis, opt_xags, ps ) );

          default_simulator<kitty::dynamic_truth_table> sim( table.num_vars() );
          auto res = simulate<kitty::dynamic_truth_table>( xag, sim )[0];

          std::cout << " table : " << kitty::to_binary( table ) << std::endl;
          std::cout << " sim   : " << kitty::to_binary( res ) << std::endl;
          assert( res == table && "Simulation results is not equal as the spec" );

          also::print_stats( xag );
          store<xag_network>().extend(); 
          store<xag_network>().current() = xag;
        }
        else
        {
          for ( uint32_t var = 4u; var <= 6u; ++var )
          {
            for ( auto i = 0u; i < 100u; ++i )
            {
              kitty::dynamic_truth_table func( var );
              kitty::create_random( func );

              xag_network xag;
              std::vector<xag_network::signal> pis( var );
              std::generate( pis.begin(), pis.end(), [&]() { return xag.create_pi(); } );
              xag.create_po( also::xag_dec( xag, func, pis, opt_xags, ps ) );

              default_simulator<kitty::dynamic_truth_table> sim( func.num_vars() );
              auto res = simulate<kitty::dynamic_truth_table>( xag, sim )[0];

              if( res != func )
              {
                std::cout << " table : " << kitty::to_binary( func ) << std::endl;
                std::cout << " sim   : " << kitty::to_binary( res ) << std::endl;
                assert( false );
              }
            }
          }

        }
      }

      private:
      also::xag_dec_params ps;

  };

  ALICE_ADD_COMMAND( test_xagdec, "Various" )

}

#endif
