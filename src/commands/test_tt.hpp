/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file test_tt.hpp
 *
 * @brief test the properities of truth tables
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef TEST_TT_HPP
#define TEST_TT_HPP

#include <mockturtle/mockturtle.hpp>

namespace alice
{

  class test_tt_command: public command
  {
      public:
      explicit test_tt_command( const environment::ptr& env ) : command( env, " test properities of truth tables" )
      {
      }

      protected:
      void execute()
      {
        auto& opt = store<optimum_network>().current();

        std::cout << kitty::to_hex( opt.function ) << " properities: \n"; 

        std::cout << " monotone   : " << ( kitty::is_monotone( opt.function ) ? " yes " : " no " ) << std::endl; 
        std::cout << " canalizing : " << ( kitty::is_canalizing( opt.function ) ? " yes " : " no ") << std::endl; 
        std::cout << " selfdual   : " << ( kitty::is_selfdual( opt.function ) ? " yes " : " no " ) << std::endl; 
        std::cout << " horn       : " << ( kitty::is_horn( opt.function ) ? " yes " : " no " ) << std::endl; 
        std::cout << " krom       : " << ( kitty::is_krom( opt.function ) ? " yes " : " no " ) << std::endl; 
        
      }
    
      private:
  };

  ALICE_ADD_COMMAND( test_tt, "Various" )
}

#endif
