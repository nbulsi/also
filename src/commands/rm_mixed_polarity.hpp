/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2021- Ningbo University, Ningbo, China */
/**
  * @file rm_mixed_polarity.hpp
  *
  * @brief RM logic optimization
  *
  * @author hongwei zhou
  * 
 */

#ifndef RM_MIXED_POLARITY_HPP
#define RM_MIXED_POLARITY_HPP

#include"../core/rm_mixed_polarity.hpp"

namespace alice
{

  class rm_mixed_polarity_command : public command
  {
    public:
      explicit rm_mixed_polarity_command( const environment::ptr& env ) : command( env, "Create a command, try hard!" )
      {
         add_flag( "--xag, -g",  "RM logic optimization for xag network" );
         add_flag( "--xmg, -x",  "RM logic optimization for xmg network" );
      }
      
      void execute()
      {
      
        if( is_set( "xag" ) )
        {
      	    mockturtle::xag_network xag=store<xag_network>().current();                         
              
              
            depth_view depth_xag1( xag );
  	    rm_mixed_polarity(depth_xag1);             
            xag = cleanup_dangling( xag );
       
            store<xag_network>().extend(); 
            store<xag_network>().current() = xag;
         }
         else if( is_set( "xmg" ) )
         {
      	     mockturtle::xmg_network xmg=store<xmg_network>().current();                         
              
              
            depth_view depth_xmg1( xmg );
  	    rm_mixed_polarity(depth_xmg1);             
            xmg = cleanup_dangling( xmg );
       
            store<xmg_network>().extend(); 
            store<xmg_network>().current() = xmg;
         }
     	 
	    }


    private:
        int strategy = 0;
  };

  ALICE_ADD_COMMAND( rm_mixed_polarity, "Rewriting" )

}

#endif

