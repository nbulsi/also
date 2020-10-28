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

#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/views/depth_view.hpp>
#include <mockturtle/algorithms/equivalence_checking.hpp>
#include <mockturtle/algorithms/mig_algebraic_rewriting.hpp>

#include "../core/xmg_rewriting.hpp"
#include "../core/misc.hpp"

namespace alice
{

  class xmgrw_command : public command
  {
    public:
      explicit xmgrw_command( const environment::ptr& env ) : command( env, "Performs algebraic XMG rewriting" )
      {
        add_option( "strategy, -s", strategy, "qca = 0, aggressive = 1, selective = 2, dfs = 3" );
        add_flag( "--area_aware, -a", "do not increase area" );
        add_flag( "--xor3_flattan", "flattan xor3 to 2 xor2s" );
        add_flag( "--only_maj", "apply mig_algebraic_depth_rewriting method" );
        add_flag( "--cec,-c", "apply equivalence checking in rewriting" );
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

          depth_view depth_xmg{ xmg }; 
          mig_algebraic_depth_rewriting( depth_xmg, ps_mig );
          xmg = cleanup_dangling( xmg );
          
        }
        else
        {
          /* parameters */
          ps_xmg.allow_area_increase = !is_set( "area_aware" );
          ps_xmg.apply_xor3_to_xor2  =  is_set( "xor3_flattan" );
          ps_mig.allow_area_increase = !is_set( "area_aware" );

          if( strategy == 0 )
          {
            ps_xmg.strategy = xmg_depth_rewriting_params::qca;
            ps_mig.strategy = mig_algebraic_depth_rewriting_params::dfs;
          }
          else if( strategy == 1 )
          {
            ps_xmg.strategy = xmg_depth_rewriting_params::aggressive;
            ps_mig.strategy = mig_algebraic_depth_rewriting_params::aggressive;
          }
          else if( strategy == 2 )
          {
            ps_xmg.strategy = xmg_depth_rewriting_params::selective;
            ps_mig.strategy = mig_algebraic_depth_rewriting_params::selective;
          }
          else if( strategy == 3 )
          {
            ps_xmg.strategy = xmg_depth_rewriting_params::dfs;
            ps_mig.strategy = mig_algebraic_depth_rewriting_params::dfs;
          }
          else
          {
            assert( false );
          }

          xmg_network xmg1, xmg2;
          xmg1 = xmg;
          
          /* mig_algebraic_depth_rewriting is suitable for ntk that has majority nodes */
          depth_view depth_xmg1{ xmg };
          mig_algebraic_depth_rewriting( depth_xmg1, ps_mig );
          xmg = cleanup_dangling( xmg );

          depth_view depth_xmg2{ xmg }; 
          xmg_depth_rewriting( depth_xmg2, ps_xmg );
          xmg = cleanup_dangling( xmg );
          xmg2 = xmg;
         
          if( is_set( "cec" ) )
          {
            /* equivalence checking */
            const auto miter_xmg = *miter<xmg_network>( xmg1, xmg2 ); 
            equivalence_checking_stats eq_st;
            const auto result = equivalence_checking( miter_xmg, {}, &eq_st );
            assert( *result );
          }
        }

        std::cout << "[xmgrw] "; 
        auto xmg_copy = cleanup_dangling( xmg );
        also::print_stats( xmg_copy );
        
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
