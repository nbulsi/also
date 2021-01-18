/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file stp.hpp
 *
 * @brief Semi-tensor product
 *
 * @author Zhufei
 * @since  0.1
 */

#ifndef STP_HPP
#define STP_HPP

#include "../core/stp_calc.hpp"

namespace alice
{
  class stp_command: public command
  {
      public:
      explicit stp_command( const environment::ptr& env ) : command( env, " semi-tensor product " )
      {
      }
      
      protected:
      void execute()
      {
        std::cout << "Welcometo STP\n";
        matrix_construct();
      }
  };
  
  ALICE_ADD_COMMAND( stp, "Various" )

}

#endif
