/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file xmgrw.hpp
 *
 * @brief XMG algebraic rewriting
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef XMGRW_COMMAND_HPP
#define XMGRW_COMMAND_HPP

#include <mockturtle/mockturtle.hpp>

#include "../core/xmg_rewriting.hpp"
#include "../core/misc.hpp"

namespace alice
{

  class xmgrw_command : public command
  {
    public:
      explicit xmgrw_command( const environment::ptr& env ) : command( env, "Performs algebraic MIG rewriting" )
      {
        add_option( "strategy, -s", strategy, "dfs = 0, aggressive = 1, selective = 2, qca = 3" );
        add_flag( "--area_aware", "do not increase area" );
        add_flag( "--only_maj", "apply mig_algebraic_depth_rewriting method" );
      }
      
      rules validity_rules() const
      {
        return {
          has_store_element<xmg_network>( env ),
            {
              [this]() { return ( strategy <= 3 && strategy >= 0 ) ; },
              "strategy must in [0,3] "
            }
          };
      }


      protected:
      void execute()
      {
        xmg_network xmg = store<xmg_network>().current();

        also::print_stats( xmg );
        
        if( is_set( "only_maj" ) )
        {
          /* parameters */
          ps_mig.allow_area_increase = !is_set( "area_aware" );

          if( strategy == 0 )
          {
            ps_mig.strategy = mig_algebraic_depth_rewriting_params::dfs;
          }
          else if( strategy == 1 )
          {
            ps_mig.strategy = mig_algebraic_depth_rewriting_params::aggressive;
          }
          else if( strategy == 2 )
          {
            ps_mig.strategy = mig_algebraic_depth_rewriting_params::selective;
          }
          else
          {
            assert( false );
          }

          depth_view depth_xmg( xmg );
          mig_algebraic_depth_rewriting( depth_xmg, ps_mig );
          xmg = cleanup_dangling( xmg );
          
          also::print_stats( xmg );
        }
        else
        {
          /* parameters */
          ps_xmg.allow_area_increase = !is_set( "area_aware" );

          if( strategy == 0 )
          {
            ps_xmg.strategy = xmg_depth_rewriting_params::dfs;
          }
          else if( strategy == 1 )
          {
            ps_xmg.strategy = xmg_depth_rewriting_params::aggressive;
          }
          else if( strategy == 2 )
          {
            ps_xmg.strategy = xmg_depth_rewriting_params::selective;
          }
          else
          {
            ps_xmg.strategy = xmg_depth_rewriting_params::qca;
          }

          depth_view depth_xmg( xmg );
          xmg_depth_rewriting( depth_xmg, ps_xmg );
          xmg = cleanup_dangling( xmg );
          
          also::print_stats( xmg );
        }

        store<xmg_network>().extend(); 
        store<xmg_network>().current() = xmg;
      }
    
    private:
      mig_algebraic_depth_rewriting_params ps_mig;
      xmg_depth_rewriting_params ps_xmg;
      int strategy = 0;
  };

  ALICE_ADD_COMMAND( xmgrw, "Rewriting" )
}

#endif
