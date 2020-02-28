/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

#include "aig2xmg.hpp"


namespace also
{

  using sig = xmg_network::signal;

/******************************************************************************
 * Types                                                                      *
 ******************************************************************************/
  class aig2xmg_manager
  {
    public:
      aig2xmg_manager( aig_network aig );
      xmg_network run();

    private:
      aig_network aig; //source graph
  };

/******************************************************************************
 * Private functions                                                          *
 ******************************************************************************/
  aig2xmg_manager::aig2xmg_manager( aig_network aig )
    : aig( aig )
  {
  }

  xmg_network aig2xmg_manager::run()
  {
    xmg_network xmg;
    node_map<sig, aig_network> node2new( aig ); 
    
    node2new[aig.get_node( aig.get_constant( false ) )] = xmg.get_constant( false );
    
    /* create pis */
    aig.foreach_pi( [&]( auto n ) {
        node2new[n] = xmg.create_pi();
        });

    /* create xmg nodes */
    topo_view aig_topo{aig};
    aig_topo.foreach_node( [&]( auto n ) {
      if ( aig.is_constant( n ) || aig.is_pi( n ) )
        return;
      
      std::vector<sig> children;
      aig.foreach_fanin( n, [&]( auto const& f ) {
        children.push_back( aig.is_complemented( f ) ? xmg.create_not( node2new[f] ) : node2new[f] );
      } );

      assert( children.size() == 2u );
      node2new[n] = xmg.create_and( children[0], children[1] );
        } );

    /* create pos */
    aig.foreach_po( [&]( auto const& f, auto index ) {
        auto const o = aig.is_complemented( f ) ? xmg.create_not( node2new[f] ) : node2new[f];
        xmg.create_po( o );
        } );

    return xmg;
  }

/******************************************************************************
 * Public functions                                                           *
 ******************************************************************************/
  xmg_network xmg_from_aig( const aig_network& aig )
  {
    aig2xmg_manager mgr( aig );
    return mgr.run();
  }

}
