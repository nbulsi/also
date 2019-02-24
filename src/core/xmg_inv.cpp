/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

#include <mockturtle/mockturtle.hpp>
#include <mockturtle/properties/migcost.hpp>

using namespace mockturtle;

namespace also
{

/******************************************************************************
 * Types                                                                      *
 ******************************************************************************/
  class inv_manager
  {
    public:
    inv_manager( xmg_network& xmg );
    
    unsigned count_current_invs();
    void run();
    void complement_node_maj( node<xmg_network>& n );
    void print( node<xmg_network>& n );

    private:
    xmg_network xmg;
  };

/******************************************************************************
 * Private functions                                                          *
 ******************************************************************************/
  inv_manager::inv_manager( xmg_network& xmg )
    : xmg( xmg )
  {
  }

  void inv_manager::run()
  {
    count_current_invs();
  }

  void inv_manager::complement_node_maj( node<xmg_network>& n )
  {
    topo_view xmg_topo{ xmg };
    
    xmg_topo.foreach_fanin( n, [&]( auto s ) 
        { 
          if( xmg.is_complemented( xmg.get_node( s ) ) )
          {
            s = !s;
          }
         } );

    /* compute the parent nodes */
    std::vector<node<xmg_network>> parent_nodes;
    fanout_view xmg_fanout( xmg );
    xmg_fanout.foreach_fanout( n, [&]( auto p ) { parent_nodes.push_back( p ); } );

    /* foreach parent node, find the signal that connect to node n */
    for( auto pn : parent_nodes )
    {
      xmg_topo.foreach_fanin( pn, [&]( auto s ) 
          { 
          if( xmg.get_node( s ) == n )
          {
            s = !s;
          }
          } );
    }
  }

  /* print information */
  void inv_manager::print( node<xmg_network>& n )
  {
    std::cout << " node " << n << " inverters infor: " << std::endl;
    xmg.foreach_fanin(n, [&]( auto s ) { std::cout << "fanin: " << s.index << " " << s.complement << std::endl; } ); 

    //fanout_view xmg_fanout{ xmg };
    //xmg_fanout.foreach_fanout( n, [&]( auto p ) { std::cout << "fanout: " << p.index << " " << p.complement << std::endl; } );
  }

  unsigned inv_manager::count_current_invs()
  {
    topo_view   xmg_topo{ xmg };
    fanout_view xmg_fanout{ xmg };

    std::vector<node<xmg_network>> nodes;
    xmg_topo.foreach_gate( [&nodes]( auto node ) { nodes.push_back( node ); } );

    for( const auto n : nodes )
    {
      std::cout << " \n node : " << n << std::endl;
      xmg_topo.foreach_fanin( n, [&]( auto s ) { std::cout << " fanin: " << xmg_topo.get_node( s ); } );
    }
    
    xmg_topo.foreach_po( [&]( auto s, auto i ) { std::cout << " PO: " << xmg_topo.get_node( s ); } );
    
    return nodes.size();
  }

/******************************************************************************
 * Public functions                                                           *
 ******************************************************************************/
  void xmg_inv_optimization( xmg_network& xmg )
  {
    std::cout << "num_invs: " << num_inverters( xmg ) << std::endl;
    inv_manager mgr( xmg );
    return mgr.run();
  }

}
