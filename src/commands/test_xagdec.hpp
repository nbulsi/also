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

#include <mockturtle/mockturtle.hpp>
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

      protected:
      void execute()
      {
        kitty::dynamic_truth_table table( 5u );
        kitty::create_from_expression( table, "{a[(bc)(de)]}" );

        xag_network xag;
        std::vector<xag_network::signal> pis( 5u );
        std::generate( pis.begin(), pis.end(), [&]() { return xag.create_pi(); } );

        xag.create_po( also::xag_dec( xag, table, pis ) );

        also::print_stats( xag );
      }
  };

  ALICE_ADD_COMMAND( test_xagdec, "Various" )

}

#endif
