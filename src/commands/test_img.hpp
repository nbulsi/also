/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file test_img.hpp
 *
 * @brief Test the exact_img synthesize versus exact_aig 
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef TEST_IMG_HPP
#define TEST_IMG_HPP

#include <mockturtle/mockturtle.hpp>
#include "../core/img_encoder.hpp"

namespace alice
{

  class test_img_command: public command
  {
      public:
      explicit test_img_command( const environment::ptr& env ) : command( env, " test the performance of exact_img" )
      {
      }

      protected:
      void execute()
      {
        kitty::dynamic_truth_table tt( 3 );
        int count = 0;
        do
        {
          std::cout << "*************************************" << std::endl;
          std::cout << "[" << count << "]" << " load function "<< kitty::to_hex( tt ) << std::endl;

          int bound;
          also::img_from_aig_syn( tt, false, bound );
          std::cout << bound << " nodes are required by aig synthesis " << kitty::to_hex( tt ) << std::endl;
                
          also::nbu_img_encoder_test( tt );

          kitty::next_inplace( tt );
          count++;

          std::cout << "*************************************" << std::endl << std::endl;
        } while( !kitty::is_const0( tt ) );
        
        std::cout << "[i] There are " << count << " functions enumerated. " << std::endl;
      }
    
      private:
  };

  ALICE_ADD_COMMAND( test_img, "Various" )


}

#endif
