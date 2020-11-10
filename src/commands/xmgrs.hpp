/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file xmgrs.hpp
 *
 * @brief XMG resubstitution
 *
 * possible buggy, remove the command temporarily 20/10/28
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef XMGRS_HPP
#define XMGRS_HPP

#include <mockturtle/mockturtle.hpp>
#include <mockturtle/algorithms/xmg_resub.hpp>
#include <mockturtle/networks/xmg.hpp>

#include "../core/misc.hpp"

namespace alice
{

  class xmgrs_command : public command
  {
    public:
      explicit xmgrs_command( const environment::ptr& env ) : command( env, "Performs XMG resubsitution" )
      {
        add_flag( "-v,--verbose", ps.verbose, "show statistics" );
        add_flag( "--cec,-c", "apply equivalence checking in resubstitution" );
      }
      
      rules validity_rules() const
      {
        return { has_store_element<xmg_network>( env ) };
      }

    protected:
      void execute()
      {
        /* derive some XMG */
         xmg_network xmg = store<xmg_network>().current();

         xmg_network xmg1, xmg2;
         xmg1 = xmg;
         xmg2 = xmg;
        
         using view_t = depth_view<fanout_view<xmg_network>>;
         fanout_view<xmg_network> fanout_view{xmg1};
         view_t resub_view{fanout_view};
         xmg_resubstitution( resub_view );
         xmg2 = cleanup_dangling( xmg1 );
         
         //std::cout << "[xmgrs] "; 
         //auto xmg_copy = cleanup_dangling( xmg );
         //also::print_stats( xmg_copy ); 
         
         if( is_set( "cec" ) )
         {
           const auto miter_xmg = *miter<xmg_network>( xmg1, xmg2 ); 
           equivalence_checking_stats eq_st;
           const auto result = equivalence_checking( miter_xmg, {}, &eq_st );
           assert( result );
           assert( *result );
           std::cout << "Network is equivalent after resub\n";
         }

         store<xmg_network>().extend(); 
         store<xmg_network>().current() = xmg2;
      }
    
    private:
      resubstitution_params ps;
      resubstitution_stats st;
  };

  ALICE_ADD_COMMAND( xmgrs, "Rewriting" )

}

#endif
