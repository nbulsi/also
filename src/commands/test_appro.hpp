/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file test_appro.hpp
 *
 * @brief Approximate synthesis
 *
 * @author Zhufei
 * @since  0.1
 */

#ifndef TEST_APPRO_HPP
#define TEST_APPRO_HPP

#include "../core/exact_m3ig_encoder.hpp"
#include "../core/m3ig_helper.hpp"
#include <mockturtle/mockturtle.hpp>

namespace alice
{

  class test_appro_command: public command
  {
      public:
      explicit test_appro_command( const environment::ptr& env ) : command( env, " test exact synthesis of multi_output function" )
      {
      }

      protected:
      void execute()
      {
        mig3 mig3;
        spec spec( 2 );

        spec.verbosity = 3;

        bsat_wrapper solver;
        also::mig_three_encoder encoder( solver );

        kitty::dynamic_truth_table x(3), y(3), z(3);

        kitty::create_nth_var( x, 0 );
        kitty::create_nth_var( y, 1 );
        kitty::create_nth_var( z, 2 );

        const auto sum = x ^ y ^ z;
        const auto carry = kitty::ternary_majority( x, y, z );

        spec[0] = ~sum;
        spec[1] = ~carry;
        spec[2] = x;
        auto result = also::mig_three_synthesize( spec, mig3, solver, encoder ); 

        assert( result == success );
      }
    
      private:
  };

  ALICE_ADD_COMMAND( test_appro, "Various" )

}

#endif
