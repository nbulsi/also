/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file ax.hpp
 *
 * @brief AIG to XMG by one-to-one mapping
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef AX_HPP
#define AX_HPP

#include <mockturtle/mockturtle.hpp>
#include "../core/aig2xmg.hpp"

namespace alice
{
  class ax_command : public command
  {
    public:
      explicit ax_command( const environment::ptr& env ) : command( env, "AIG to XMG by one-to-one mapping" )
      {
        add_flag( "--dis_mig_opt, -d",  "Disable MIG algebraic rewriting" );
      }
    
    protected:
      void execute()
      {
          /* derive some aig */
          assert( store<aig_network>().size() > 0 );
          aig_network aig = store<aig_network>().current();

          auto xmg = also::xmg_from_aig( aig );
          also::print_stats( aig );
          also::print_stats( xmg );

          if( !is_set( "dis_mig_opt" ) )
          {
            depth_view depth_xmg{xmg};
            mig_algebraic_depth_rewriting( depth_xmg );
            also::print_stats( depth_xmg );
            
            store<xmg_network>().extend(); 
            store<xmg_network>().current() = depth_xmg;
          }
          else
          {
            store<xmg_network>().extend(); 
            store<xmg_network>().current() = xmg;
          }
      }
  };

  ALICE_ADD_COMMAND( ax, "Rewriting" )
}

#endif
