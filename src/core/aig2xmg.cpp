/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

#include <mockturtle/views/topo_view.hpp>
#include "aig2xmg.hpp"

using namespace mockturtle;

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
  
  class mig2xmg_manager
  {
    public:
      mig2xmg_manager( mig_network mig );
      xmg_network run();

    private:
      mig_network mig; //source graph
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
  
  mig2xmg_manager::mig2xmg_manager( mig_network mig )
    : mig( mig )
  {
  }

  xmg_network mig2xmg_manager::run()
  {
    xmg_network xmg;
    node_map<sig, mig_network> node2new( mig ); 
    
    node2new[mig.get_node( mig.get_constant( false ) )] = xmg.get_constant( false );
    
    /* create pis */
    mig.foreach_pi( [&]( auto n ) {
        node2new[n] = xmg.create_pi();
        });

    /* create xmg nodes */
    topo_view mig_topo{mig};
    mig_topo.foreach_node( [&]( auto n ) {
      if ( mig.is_constant( n ) || mig.is_pi( n ) )
        return;
      
      std::vector<sig> children;
      mig.foreach_fanin( n, [&]( auto const& f ) {
        children.push_back( mig.is_complemented( f ) ? xmg.create_not( node2new[f] ) : node2new[f] );
      } );

      assert( children.size() == 3u );
      node2new[n] = xmg.create_maj( children[0], children[1], children[0] );
        } );

    /* create pos */
    mig.foreach_po( [&]( auto const& f, auto index ) {
        auto const o = mig.is_complemented( f ) ? xmg.create_not( node2new[f] ) : node2new[f];
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
  
  xmg_network xmg_from_mig( const mig_network& mig )
  {
    mig2xmg_manager mgr( mig );
    return mgr.run();
  }

}
