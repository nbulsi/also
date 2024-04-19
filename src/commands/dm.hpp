/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file dm.hpp
 *
 * @brief Direct mapping of logic networks by one-to-one mapping
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef DM_HPP
#define DM_HPP

#include <mockturtle/mockturtle.hpp>
#include "../core/direct_mapping.hpp"

namespace alice
{
  class dm_command : public command
  {
    public:
      explicit dm_command( const environment::ptr& env ) : command( env, "Direct mapping of logic networks by one-to-one mapping" )
      {
        add_flag( "--aig_to_xmg",  "AIG to XMG" );
        add_flag( "--xmg_to_mig",  "XMG to MIG" );
        add_flag( "--mig_to_xmg",  "MIG to XMG" );
      }

    protected:
      void execute()
      {
          if( is_set( "aig_to_xmg" ) )
          {
              assert( store<aig_network>().size() > 0 );
              aig_network aig = store<aig_network>().current();

              store<xmg_network>().extend();
              store<xmg_network>().current() = also::xmg_from_aig( aig );
          }
          else if( is_set(  "xmg_to_mig" ) )
          {
              assert( store<xmg_network>().size() > 0 );
              xmg_network xmg = store<xmg_network>().current();

              store<mig_network>().extend();
              store<mig_network>().current() = also::mig_from_xmg( xmg );
          }
          else if( is_set(  "mig_to_xmg" ) )
          {
              assert( store<mig_network>().size() > 0 );
              mig_network mig = store<mig_network>().current();

              store<xmg_network>().extend();
              store<xmg_network>().current() = also::xmg_from_mig( mig );
          }
          else
          {
              std::cout << "You must specify the transformation parameter, press '-h' for help\n";
          }
      }
  };

  ALICE_ADD_COMMAND( dm, "Rewriting" )
}

#endif
