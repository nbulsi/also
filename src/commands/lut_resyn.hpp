/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file lut_resyn.hpp
 *
 * @brief TODO
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef LUT_RESYN_HPP
#define LUT_RESYN_HPP

#include <mockturtle/mockturtle.hpp>
#include <mockturtle/algorithms/node_resynthesis/xmg_npn.hpp>
#include <mockturtle/algorithms/node_resynthesis/xmg3_npn.hpp>
#include <mockturtle/algorithms/node_resynthesis/shannon.hpp>
#include <mockturtle/algorithms/node_resynthesis/dsd.hpp>

#include "../networks/m5ig/m5ig.hpp"
#include "../networks/m5ig/m5ig_npn.hpp"
#include "../networks/m5ig/exact_online_m3ig.hpp"
#include "../networks/m5ig/exact_online_m5ig.hpp"
#include "../networks/img/img.hpp"
#include "../networks/img/img_npn.hpp"
#include "../networks/aoig/xag_lut_npn.hpp"
#include "../networks/aoig/xag_lut_dec.hpp"
#include "../networks/aoig/build_xag_db.hpp"
#include "../networks/img/img_all.hpp"
#include "../core/aig2xmg.hpp"
#include "../core/misc.hpp"

namespace alice
{
  
  class lut_resyn_command: public command
  {
    public:
      explicit lut_resyn_command( const environment::ptr& env ) 
               : command( env, "lut resyn using optimal networks (default:mig)" )
      {
        add_option( "cut_size, -k", cut_size, "set the cut size from 2 to 8, default = 4" );
        add_flag( "--verbose, -v", "print the information" );
        add_flag( "--xmg, -x", "using xmg as target logic network" );
        add_flag( "--xmg3, -m", "using xmg3 as target logic network" );
        add_flag( "--test_m3ig", "using m3ig as target logic network, exact_m3ig online" );
        add_flag( "--test_m5ig", "using m5ig as target logic network, exact_m5ig online" );
        add_flag( "--m5ig, -r", "using m5ig as target logic network" );
        add_flag( "--img, -i",  "using img as target logic network" );
        add_flag( "--xag, -g",  "using xag as target logic network" );
        add_flag( "--new_entry, -n", "adds new store entry" );
        add_flag( "--enable_direct_mapping, -e", "enable aig to xmg by direct mapping for comparison" );
      }

      rules validity_rules() const
      {
        return { has_store_element<klut_network>( env ) };
      }


    protected:
      void execute()
      {
        /* derive klut  */
        klut_network klut = store<klut_network>().current();

        /* lut resynthesis */
        if( is_set( "xmg" ) )
        {
          xmg_network xmg;
          xmg_npn_resynthesis resyn;
          xmg = node_resynthesis<xmg_network>( klut, resyn );
          
          /* add to store */
          if( is_set( "new_entry" ) )
          {
            store<xmg_network>().extend(); 
            store<xmg_network>().current() = xmg;
          }

          if( is_set( "enable_direct_mapping" ) )
          {
            /* make the xmg from aig one-to-one mapping as the
             * basiline xmg*/
            depth_view xmg_depth1{xmg};
            assert( store<aig_network>().size() > 0 );
            aig_network aig = store<aig_network>().current();
            auto xmg_baseline = also::xmg_from_aig( aig );

            depth_view xmg_depth2{xmg_baseline};

            std::cout << "depth 1: " << xmg_depth1.depth() 
                      << "depth 2: " << xmg_depth2.depth()
                      << std::endl;
            
            /*select the XMG that has fewer number of nodes 
             * naive method currently
             * */
            if( xmg_depth2.depth() < xmg_depth1.depth() )
            {
              xmg = also::xmg_from_aig( aig );
              /* add to store */
              if( is_set( "new_entry" ) )
              {
                store<xmg_network>().extend(); 
                store<xmg_network>().current() = xmg;
              }
            }
          }

        }
        else if( is_set( "xmg3" ) )
        {
          xmg_network xmg;
          xmg3_npn_resynthesis<xmg_network> resyn;
          xmg = node_resynthesis<xmg_network>( klut, resyn );

          /* add to store */
          if( is_set( "new_entry" ) )
          {
            store<xmg_network>().extend(); 
            store<xmg_network>().current() = xmg;
          }
          
          depth_view xmg_depth{xmg};
          std::cout << "[I/O:" << xmg.num_pis() << "/" << xmg.num_pos() << "] XMG3 gates: " 
                    << xmg.num_gates() << " XMG3 depth: " << xmg_depth.depth() << std::endl;
        }
        else if( is_set( "m5ig" ) )
        {
          m5ig_network m5ig;

          if( cut_size <= 4 )
          {
            m5ig_npn_resynthesis resyn;
            node_resynthesis( m5ig, klut, resyn );
            //m5ig = node_resynthesis<m5ig_network>( klut, resyn );

            depth_view m5ig_depth{m5ig};
            std::cout << "[I/O:" << m5ig.num_pis() << "/" << m5ig.num_pos() << "] M5IG gates: " 
              << m5ig.num_gates() << " M5IG depth: " << m5ig_depth.depth() << std::endl;
          }
          else
          {
            /*TODO: compute m5ig online */
          }

          /* add to store */
          if( is_set( "new_entry" ) )
          {
            store<m5ig_network>().extend(); 
            store<m5ig_network>().current() = m5ig;
          }
        }
        else if( is_set( "img" ) )
        {
          img_network img, tmp_img;

          img_all_resynthesis<img_network> resyn;
          img = node_resynthesis<img_network>( klut, resyn );

          /* add to store */
          if( is_set( "new_entry" ) )
          {
            store<img_network>().extend(); 
            store<img_network>().current() = img;
          }

          depth_view img_depth{img};
          std::cout << "[I/O:" << img.num_pis() << "/" << img.num_pos() << "] IMG gates: " 
            << img.num_gates() << " IMG depth: " << img_depth.depth() << std::endl;
        }
        else if( is_set( "test_m3ig" ) )
        {
          /***************************************************************/
          /* This is an example to generate an MIG by exact_m3ig online  */
          /***************************************************************/
          kitty::dynamic_truth_table maj( 5u );
          kitty::create_majority( maj );

          mig_network mig;
          const auto a = mig.get_constant( false );
          const auto b = mig.create_pi();
          const auto c = mig.create_pi();
          const auto d = mig.create_pi();
          const auto e = mig.create_pi();
          const auto f = mig.create_pi();

          std::vector<mig_network::signal> pis = {a, b, c, d, e, f};

          exact_mig_resynthesis<mig_network> resyn;
          resyn( mig, maj, pis.begin(), pis.end(), [&]( auto const& f ) { 
              mig.create_po( f );
              } );
          
          depth_view mig_depth{mig};
          std::cout << "[I/O:" << mig.num_pis() << "/" << mig.num_pos() << "] MIG gates: " 
                    << mig.num_gates() << " MIG depth: " << mig_depth.depth() << std::endl;
        }
        else if( is_set( "test_m5ig" ) )
        {
          /***************************************************************/
          /* This is an example to generate an M5IG by exact_m5ig online  */
          /***************************************************************/
          //kitty::dynamic_truth_table maj( 5u );
          //kitty::create_majority( maj );
          kitty::dynamic_truth_table maj( 5u );
          kitty::create_from_hex_string( maj, "fee8e88a" );

          m5ig_network m5ig;
          const auto a = m5ig.get_constant( false );
          const auto b = m5ig.create_pi();
          const auto c = m5ig.create_pi();
          const auto d = m5ig.create_pi();
          const auto e = m5ig.create_pi();
          const auto f = m5ig.create_pi();

          std::vector<m5ig_network::signal> pis = {a, b, c, d, e, f};

          exact_m5ig_resynthesis<m5ig_network> resyn;
          resyn( m5ig, maj, pis.begin(), pis.end(), [&]( auto const& f ) { 
              m5ig.create_po( f );
              } );
          
          depth_view m5ig_depth{m5ig};
          std::cout << "[I/O:" << m5ig.num_pis() << "/" << m5ig.num_pos() << "] M5IG gates: " 
                    << m5ig.num_gates() << " M5IG depth: " << m5ig_depth.depth() << std::endl;
        }
        else if( is_set( "xag" ) )
        {
          xag_network xag;
          if( cut_size <= 4 )
          {
            xag_npn_lut_resynthesis resyn;
            xag = node_resynthesis<xag_network>( klut, resyn );
          }
          else
          {
            //shannon_resynthesis<xag_network> fallback; // fallback
            //dsd_resynthesis<xag_network, decltype( fallback )> resyn( fallback );
            xag_network db;
            auto opt_xags = also::load_xag_string_db( db );
            xag_lut_dec_resynthesis<xag_network> resyn( opt_xags );
            
            xag = node_resynthesis<xag_network>( klut, resyn );
          }
          
          /* add to store */
          if( is_set( "new_entry" ) )
          {
            store<xag_network>().extend(); 
            store<xag_network>().current() = xag;
          }

          depth_view xag_depth{xag};
          std::cout << "[I/O:" << xag.num_pis() << "/" << xag.num_pos() << "] xag gates: " 
                    << xag.num_gates() << " xag depth: " << xag_depth.depth() << std::endl;
        }
        else
        {
          mig_npn_resynthesis resyn;
          const auto mig = node_resynthesis<mig_network>( klut, resyn );
          
          /* add to store */
          if( is_set( "new_entry" ) )
          {
            store<mig_network>().extend(); 
            store<mig_network>().current() = mig;
          }

          depth_view mig_depth{mig};
          std::cout << "[I/O:" << mig.num_pis() << "/" << mig.num_pos() << "] MIG gates: " 
                    << mig.num_gates() << " MIG depth: " << mig_depth.depth() << std::endl;
        }
      }

    private:
      int cut_size = 4;
  };

  ALICE_ADD_COMMAND( lut_resyn, "Exact synthesis" )


}

#endif
