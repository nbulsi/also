#pragma once
#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <mockturtle/networks/aoig.hpp>
#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/utils/node_map.hpp>
#include <mockturtle/views/topo_view.hpp>
#include <optional>
#include <set>
#include <stack>
#include <string>
#include <vector>
#include<memory>

namespace mockturtle
{

class aoig2xmg_manager
{
  using sig = xmg_network::signal;

public:
  aoig2xmg_manager( aoig_network aoig ) : aoig( aoig )
  {
  }

  xmg_network run()
  {
    xmg_network xmg;
    node_map<sig, aoig_network> node2new( aoig );

    node2new[aoig.get_node( aoig.get_constant( false ) )] = xmg.get_constant( false );

    /* create pis */
    aoig.foreach_pi( [&]( auto n ) {
      node2new[n] = xmg.create_pi();
    } );

    /* create xmg nodes */
    topo_view aoig_topo{aoig};
    aoig_topo.foreach_node( [&]( auto n ) {
      if ( aoig.is_constant( n ) || aoig.is_pi( n ) )
        return;

      std::vector<sig> children;
      uint64_t _and = 0x8, _mux = 0xd8;
      kitty::dynamic_truth_table tt_and( 2 );
      kitty::dynamic_truth_table tt_mux( 3 );
      kitty::create_from_words( tt_and, &_and, &_and + 1 );
      kitty::create_from_words( tt_mux, &_mux, &_mux + 1 );

      aoig.foreach_fanin( n, [&]( auto const& f ) { children.push_back( node2new[f] ); } );

      if ( aoig.fanin_size( n ) == 1 )
      {
        node2new[n] = xmg.create_not( children[0] );
      }

      else if ( aoig.fanin_size( n ) == 2 )
      {
        if ( aoig.node_function( n ) == tt_and )
          node2new[n] = xmg.create_and( children[0], children[1] );
        else
          node2new[n] = xmg.create_or( children[0], children[1] );
      }

      else
      {
        if ( aoig.node_function( n ) == tt_mux )
          node2new[n] = xmg.create_ite( children[0], children[1], children[2] );
        else
          node2new[n] = xmg.create_xor3( children[0], children[1], children[2] );
      }
    } );

      /* create pos */
      aoig.foreach_po( [&]( auto const& f ) {
          xmg.create_po( node2new[f] );
      } );

      return xmg;
  }

private:
  aoig_network aoig; //source graph
  };

  /******************************************************************************
 * Public functions                                                           *
 ******************************************************************************/
  xmg_network xmg_from_aoig(  aoig_network const& aoig )
  {
    aoig2xmg_manager mgr( aoig );
    return mgr.run();
  }

} // namespace also
