/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file xagrs.hpp
 *
 * @brief xag resubsitution
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef XAGRS_HPP
#define XAGRS_HPP

#include <mockturtle/mockturtle.hpp>
#include <mockturtle/algorithms/resubstitution.hpp>
#include <mockturtle/networks/xag.hpp>

#include "../core/misc.hpp"

namespace alice
{

  class xagrs_command : public command
  {
    public:
      explicit xagrs_command( const environment::ptr& env ) : command( env, "Performs XAG resubsitution" )
      {
        add_flag( "-v,--verbose", ps.verbose, "show statistics" );
      }
      
      rules validity_rules() const
      {
        return { has_store_element<xag_network>( env ) };
      }

    protected:
      void execute()
      {
        /* derive some XAG */
         xag_network xag = store<xag_network>().current();
         
         resubstitution( xag, ps, &st );
         xag = cleanup_dangling( xag );
         
         std::cout << "[xagrs] "; 
         also::print_stats( xag ); 

         store<xag_network>().extend(); 
         store<xag_network>().current() = xag;
      }
    
    private:
      resubstitution_params ps;
      resubstitution_stats  st;
  };
  
  ALICE_ADD_COMMAND( xagrs, "Rewriting" )


}

#endif
