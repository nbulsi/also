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

#include "../networks/m5ig/m5ig_npn.hpp"
#include "../core/misc.hpp"

namespace alice
{

  class cutrw_command : public command
  {
    public:
      explicit cutrw_command( const environment::ptr& env ) : command( env, "Performs cut rewriting" )
      {
        add_flag( "--m5ig_npn,-r", "cut rewriting based on m5ig_npn" );
        add_flag( "--m3ig_npn,-m", "cut rewriting based on m3ig_npn" );
      }
      
      template<class Ntk>
      void print_stats( const Ntk& ntk )
     {
       depth_view depth_ntk( ntk );
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
          cut_rewriting( m5ig, resyn, ps );
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
          cut_rewriting( mig, resyn, ps );
          mig = cleanup_dangling( mig );

          print_stats( mig );

          store<mig_network>().extend(); 
          store<mig_network>().current() = mig;
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
