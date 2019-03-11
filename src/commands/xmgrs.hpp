/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file xmgrs.hpp
 *
 * @brief XMG resubstitution
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef XMGRS_HPP
#define XMGRS_HPP

#include <mockturtle/mockturtle.hpp>

#include "../core/xmg_resub.hpp"

namespace alice
{

  class xmgrs_command : public command
  {
    public:
      explicit xmgrs_command( const environment::ptr& env ) : command( env, "Performs XMG resubsitution" )
      {
        add_flag( "-v,--verbose", ps.verbose, "show statistics" );
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
        
         using view_t = depth_view<fanout_view<xmg_network>>;
         fanout_view<xmg_network> fanout_view{xmg};
         view_t resub_view{fanout_view};
         xmg_resubstitution( resub_view, ps, &st );
         xmg = cleanup_dangling( xmg );

         std::cout << "TODO: an improved XMG resubstitution version " << std::endl;
      }
    
    private:
      resubstitution_params ps;
      resubstitution_stats st;
  };

  ALICE_ADD_COMMAND( xmgrs, "Rewriting" )

}

#endif
