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
#include "../core/approximate_encoder.hpp"
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

        spec.verbosity = 4;

        bsat_wrapper solver;
        also::mig_three_encoder encoder( solver );

        kitty::dynamic_truth_table x(3), y(3), z(3);

        kitty::create_nth_var( x, 0 );
        kitty::create_nth_var( y, 1 );
        kitty::create_nth_var( z, 2 );

        const auto sum = x ^ y ^ z;
        const auto carry = kitty::ternary_majority( x, y, z );

        spec[0] = sum;
        spec[1] = carry;
        
        auto result = also::mig_three_synthesize( spec, mig3, solver, encoder ); 

        assert( result == success );

        /*kitty::static_truth_table<1> tt_str;
        kitty::create_from_binary_string( tt_str, "00" );
        kitty::set_bit( tt_str, 1 );
        kitty::set_bit( tt_str, 0 );
        std::cout << "bit on 0 is " << kitty::get_bit( tt_str, 0 ) << std::endl; 
        std::cout << "bit on 1 is " << kitty::get_bit( tt_str, 1 ) << std::endl; 
        unsigned decimal = 0;
        if( kitty::get_bit( tt_str, 1 ) ) { decimal += 2; }
        if( kitty::get_bit( tt_str, 0 ) ) { decimal += 1; }
        std::cout << " decimal = " << decimal << std::endl;

        for( int i = 0; i < spec[2].num_bits(); i++ )
        {
          std::cout << " bit on " << i << " is " << kitty::get_bit( spec[2], i ) << std::endl;
        }

        std::vector<unsigned> vars{ 1, 2, 3, 4, 5 };
        auto v = also::get_all_combination_index( vars, 5, 3 );
        for( const auto sv : v )
        {
          also::show_array( sv );

          auto permu = get_all_permutation( sv );

          for( const auto e : permu )
          {
            std::cout << "permu: ";
            also::show_array( e );
          }
        }*/
        //auto res = mig_three_appro_synthesize( spec, 1 );
      }
    
      private:
  };

  ALICE_ADD_COMMAND( test_appro, "Various" )

}

#endif
