/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file cutrw.hpp
 *
 * @brief cut rewriting
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef CUTRW_HPP
#define CUTRW_HPP

#include <mockturtle/mockturtle.hpp>
#include <mockturtle/algorithms/node_resynthesis/bidecomposition.hpp>

#include "../networks/m5ig/m5ig_npn.hpp"
#include "../networks/img/img_all.hpp"
#include "../networks/img/fc_cost.hpp"
#include "../networks/aoig/xag_lut_npn.hpp"
#include "../core/misc.hpp"

namespace alice
{

  class cutrw_command : public command
  {
    public:
      explicit cutrw_command( const environment::ptr& env ) : command( env, "Performs cut rewriting" )
      {
        add_flag( "--m5ig_npn,-r",    "cut rewriting based on m5ig_npn" );
        add_flag( "--m3ig_npn,-m",    "cut rewriting based on m3ig_npn" );
        add_flag( "--img_fc,-i",      "cut rewriting based on img without fanout conflicts" );
        add_flag( "--img_all,-a",     "cut rewriting based on img_all for size optimization" );
        add_flag( "--xag_npn_lut,-g", "cut rewriting based on xag_npn_lut" );
        add_flag( "--xag_npn,-p",     "cut rewriting based on xag_npn" );
      }
      
      template<class Ntk>
      void print_stats( const Ntk& ntk )
     {
       depth_view depth_ntk{ ntk };
       std::cout << fmt::format( "ntk   i/o = {}/{}   gates = {}   level = {}\n", 
                    ntk.num_pis(), ntk.num_pos(), ntk.num_gates(), depth_ntk.depth() );
     }

      void execute()
      {
        /* parameters */
        if( is_set( "m5ig_npn" ) )
        {
          m5ig_network m5ig = store<m5ig_network>().current();

          print_stats( m5ig );

          /******************************************************/
          /* for testing cut enumeration for m5ig               */
          /******************************************************/
          #if 0
          cut_enumeration_params ps_cut;
          ps_cut.cut_size = 4;
          const auto cuts = cut_enumeration<m5ig_network, true>( m5ig, ps_cut );

          std::vector<int> pos;
          m5ig.foreach_po( [&]( auto const& f ) {
              const auto i = m5ig.node_to_index( m5ig.get_node( f ) );
              pos.push_back( i );
              } );
          const auto to_vector = []( auto const& cut ) {
            return std::vector<uint32_t>( cut.begin(), cut.end() );
          };

          for( const auto& po : pos )
          {
            std::cout << "po: " << po << " cut size: " << cuts.cuts( po ).size() << std::endl; 
            
            also::show_array( to_vector( cuts.cuts( po )[0] ) );
            std::cout << std::hex << cuts.truth_table( cuts.cuts( po )[0] )._bits[0] << "\n";
            
            also::show_array( to_vector( cuts.cuts( po )[1] ) );
            std::cout << std::hex << cuts.truth_table( cuts.cuts( po )[1] )._bits[0] << "\n";
            
            also::show_array( to_vector( cuts.cuts( po )[2] ) );
            std::cout << std::hex << cuts.truth_table( cuts.cuts( po )[2] )._bits[0] << "\n";
          }
          #endif
          /******************************************************/
          m5ig_npn_resynthesis resyn;
          cut_rewriting_params ps;
          //ps.very_verbose = true;
          //ps.progress = true;
          ps.allow_zero_gain = false;
          ps.cut_enumeration_ps.cut_size = 4u;
          m5ig = cut_rewriting( m5ig, resyn, ps );
          m5ig = cleanup_dangling( m5ig );

          print_stats( m5ig );

          store<m5ig_network>().extend(); 
          store<m5ig_network>().current() = m5ig;
        }
        else if( is_set( "m3ig_npn" ) )
        {
          mig_network mig = store<mig_network>().current();

          print_stats( mig );

          mig_npn_resynthesis resyn;
          cut_rewriting_params ps;
          ps.cut_enumeration_ps.cut_size = 4u;
          mig = cut_rewriting( mig, resyn, ps );
          mig = cleanup_dangling( mig );

          print_stats( mig );

          store<mig_network>().extend(); 
          store<mig_network>().current() = mig;
        }
        else if( is_set( "xag_npn_lut" ) )
        {
          xag_network xag = store<xag_network>().current();

          print_stats( xag );

          xag_npn_lut_resynthesis resyn;
          cut_rewriting_params ps;
          ps.cut_enumeration_ps.cut_size = 4u;
          xag = cut_rewriting( xag, resyn, ps );
          
          xag = cleanup_dangling( xag );

          //bidecomposition refactoring
          bidecomposition_resynthesis<xag_network> resyn2;
          refactoring( xag, resyn2 );
          
          xag = cleanup_dangling( xag );

          print_stats( xag );

          store<xag_network>().extend(); 
          store<xag_network>().current() = xag;
        }
        else if( is_set( "xag_npn" ) )
        {
          xag_network xag = store<xag_network>().current();

          print_stats( xag );

          xag_npn_resynthesis<xag_network> resyn;
          cut_rewriting_params ps;
          ps.cut_enumeration_ps.cut_size = 4u;
          ps.min_cand_cut_size = 2;
          ps.min_cand_cut_size_override = 3;
          cut_rewriting( xag, resyn, ps );
          xag = cleanup_dangling( xag );

          print_stats( xag );

          store<xag_network>().extend(); 
          store<xag_network>().current() = xag;
        }
        else if( is_set( "img_fc" ) )
        {
          img_network img = store<img_network>().current();

          auto fcost1 = total_fc_cost( img );
          print_stats( img );

          cut_rewriting_params ps;
          ps.cut_enumeration_ps.cut_size = 3u;
          
          using cost_fn = fc_cost<fanout_view<img_network>>;
          using resyn_fn = img_all_resynthesis<fanout_view<img_network>>;

          resyn_fn resyn;
          fanout_view fanout_img{img};
          img = cut_rewriting<fanout_view<img_network>, resyn_fn, cost_fn>( fanout_img, resyn, ps );
          img = cleanup_dangling( img );

          auto fcost2 = total_fc_cost( img );
          print_stats( img );
          std::cout << "fcost: " << fcost1 << " to " << fcost2 << std::endl;

          store<img_network>().extend(); 
          store<img_network>().current() = img;
        }
        else if( is_set( "img_all" ) )
        {
          img_network img = store<img_network>().current();
          print_stats( img );
          
          cut_rewriting_params ps;
          ps.cut_enumeration_ps.cut_size = 3u;
          
          img_all_resynthesis<img_network> resyn;
          cut_rewriting( img, resyn, ps );
          img = cleanup_dangling( img );

          print_stats( img );
          
          store<img_network>().extend(); 
          store<img_network>().current() = img;
        }
        else
        {
          xmg_network xmg = store<xmg_network>().current();

          print_stats( xmg );

          xmg_npn_resynthesis resyn;
          cut_rewriting_params ps;
          ps.cut_enumeration_ps.cut_size = 4u;
          cut_rewriting( xmg, resyn, ps );
          xmg = cleanup_dangling( xmg );

          print_stats( xmg );

          store<xmg_network>().extend(); 
          store<xmg_network>().current() = xmg;
        }
      }
    
    private:
      bool verbose = false;
  };

  ALICE_ADD_COMMAND( cutrw, "Rewriting" )

}

#endif
