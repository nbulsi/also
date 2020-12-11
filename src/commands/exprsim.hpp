/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file exprsim.hpp
 *
 * @brief simulate a expression and return a truth table
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef EXPRSIM_HPP
#define EXPRSIM_HPP

#include "../store.hpp"

namespace alice
{

  class exprsim_command: public command
  {
    public:
      explicit exprsim_command( const environment::ptr& env ) 
               : command( env, "expression simulation, return a truth table" )
      {
        add_flag( "--verbose, -v", "print the information" );    
        add_option( "-e,--expression", expression, "creates truth table from expression" );
        add_flag( "-n,--new", "adds new store entry" );
	add_option( "-m,--max_num_vars", max_num_vars, "set the maximum number of variables" );
      }

    protected:
      void execute()
      {
         auto& opt_ntks = store<optimum_network>();
         
         if ( opt_ntks.empty() || is_set( "new" ) )
         {
           opt_ntks.extend();
         }

         /* find max var */
         uint32_t num_vars{0u};
	 if( is_set( "max_num_vars" ) )
	 {
	   num_vars = max_num_vars;
	 }
	 else
	 {
	     for (auto c : expression)
	     {
		 if (c >= 'a' && c <= 'p')
		 {
		     num_vars = std::max<uint32_t>( num_vars, c - 'a' + 1u );
		 }
	     }
	 }
         
         
         kitty::dynamic_truth_table tt( num_vars );
         
         if ( kitty::create_from_expression( tt, expression ) )
         {
           optimum_network opt;
           opt.function = tt;
           opt.network  = expression;
           opt_ntks.current() = opt;
           std::cout << fmt::format( "tt: 0x{}", kitty::to_hex( opt.function ) ) << std::endl;

           store<optimum_network>().extend();
           store<optimum_network>().current() = opt;
         }
      }

    private:
      std::string expression = "";
      uint32_t max_num_vars = 0u;

  };

  ALICE_ADD_COMMAND( exprsim, "Loading" )

}

#endif
