/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

#include "direct_mapping.hpp"
#include <mockturtle/views/topo_view.hpp>

using namespace mockturtle;

namespace also
{

using sig = xmg_network::signal;
using sig2 = mig_network::signal;

/******************************************************************************
 * Types                                                                      *
 ******************************************************************************/
class aig2xmg_manager
{
public:
  aig2xmg_manager( aig_network aig );
  xmg_network run();

private:
  aig_network aig; // source graph
};

class mig2xmg_manager
{
public:
  mig2xmg_manager( mig_network mig );
  xmg_network run();

private:
  mig_network mig; // source graph
};

class xmg2mig_manager
{
public:
  xmg2mig_manager( xmg_network xmg );
  mig_network run();

private:
  xmg_network xmg; // source graph
};

class aig2xag_manager
{
public:
  aig2xag_manager( aig_network aig );
  xag_network run();

private:
  aig_network aig; // source graph
};

class aig2mig_manager
{
public:
  aig2mig_manager( aig_network aig );
  mig_network run();

private:
  aig_network aig; // source graph
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
  aig.foreach_pi( [&]( auto n )
                  { node2new[n] = xmg.create_pi(); } );

  /* create xmg nodes */
  topo_view aig_topo{ aig };
  aig_topo.foreach_node( [&]( auto n )
                         {
      if ( aig.is_constant( n ) || aig.is_pi( n ) )
        return;

      std::vector<sig> children;
      aig.foreach_fanin( n, [&]( auto const& f ) {
        children.push_back( aig.is_complemented( f ) ? xmg.create_not( node2new[f] ) : node2new[f] );
      } );

      assert( children.size() == 2u );
      node2new[n] = xmg.create_and( children[0], children[1] ); } );

  /* create pos */
  aig.foreach_po( [&]( auto const& f, auto index )
                  {
        auto const o = aig.is_complemented( f ) ? xmg.create_not( node2new[f] ) : node2new[f];
        xmg.create_po( o ); } );

  return xmg;
}

// mig to xmg
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
  mig.foreach_pi( [&]( auto n )
                  { node2new[n] = xmg.create_pi(); } );

  /* create xmg nodes */
  topo_view mig_topo{ mig };
  mig_topo.foreach_node( [&]( auto n )
                         {
      if ( mig.is_constant( n ) || mig.is_pi( n ) )
        return;

      std::vector<sig> children;
      mig.foreach_fanin( n, [&]( auto const& f ) {
        children.push_back( mig.is_complemented( f ) ? xmg.create_not( node2new[f] ) : node2new[f] );
      } );

      assert( children.size() == 3u );
      node2new[n] = xmg.create_maj( children[0], children[1], children[2] ); } );

  /* create pos */
  mig.foreach_po( [&]( auto const& f, auto index )
                  {
        auto const o = mig.is_complemented( f ) ? xmg.create_not( node2new[f] ) : node2new[f];
        xmg.create_po( o ); } );

  return xmg;
}

// xmg to mig
xmg2mig_manager::xmg2mig_manager( xmg_network xmg )
    : xmg( xmg )
{
}

mig_network xmg2mig_manager::run()
{
  mig_network mig;
  node_map<mig_network::signal, xmg_network> node2new( xmg );

  node2new[xmg.get_node( xmg.get_constant( false ) )] = mig.get_constant( false );

  /* create pis */
  xmg.foreach_pi( [&]( auto n )
                  { node2new[n] = mig.create_pi(); } );

  /* create xmg nodes */
  topo_view xmg_topo{ xmg };
  xmg_topo.foreach_node( [&]( auto n )
                         {
      if ( xmg.is_constant( n ) || xmg.is_pi( n ) )
        return;

      std::vector<mig_network::signal> children;
      xmg.foreach_fanin( n, [&]( auto const& f ) {
        children.push_back( xmg.is_complemented( f ) ? mig.create_not( node2new[f] ) : node2new[f] );
      } );

      assert( children.size() == 3u );
      if( xmg.is_maj( n  ) )
      {
        node2new[n] = mig.create_maj( children[0], children[1], children[2] );
      }
      else
      {
      //xor3
        auto minority = mig.create_not( mig.create_maj( children[0], children[1], children[2] ) );
        node2new[n] = mig.create_maj( children[2],
                                      minority,
                                      mig.create_maj( children[0], children[1],  minority ) );
      } } );

  /* create pos */
  xmg.foreach_po( [&]( auto const& f, auto index )
                  {
        auto const o = xmg.is_complemented( f ) ? mig.create_not( node2new[f] ) : node2new[f];
        mig.create_po( o ); } );

  return mig;
}

// aig to xag
aig2xag_manager::aig2xag_manager( aig_network aig ) : aig( aig ) {}

xag_network aig2xag_manager::run()
{
  xag_network xag;
  node_map<xag_network::signal, aig_network> node2new( aig );

  node2new[aig.get_node( aig.get_constant( false ) )] = xag.get_constant( false );

  /* create pis */
  aig.foreach_pi( [&]( auto n )
                  { node2new[n] = xag.create_pi(); } );

  /* create xag nodes */
  topo_view aig_topo{ aig };
  aig_topo.foreach_node( [&]( auto n )
                         {
    if (aig.is_constant(n) || aig.is_pi(n)) return;

    std::vector<xag_network::signal> children;
    aig.foreach_fanin(n, [&](auto const& f) {
      children.push_back(aig.is_complemented(f) ? xag.create_not(node2new[f])
                                                : node2new[f]);
    });

    assert(children.size() == 2u);
    node2new[n] = xag.create_and(children[0], children[1]); } );

  /* create pos */
  aig.foreach_po( [&]( auto const& f, auto index )
                  {
    auto const o =
        aig.is_complemented(f) ? xag.create_not(node2new[f]) : node2new[f];
    xag.create_po(o); } );

  return xag;
}

// aig to mig
aig2mig_manager::aig2mig_manager( aig_network aig ) : aig( aig ) {}

mig_network aig2mig_manager::run()
{
  mig_network mig;
  node_map<mig_network::signal, aig_network> node2new( aig );

  node2new[aig.get_node( aig.get_constant( false ) )] = mig.get_constant( false );

  /* create pis */
  aig.foreach_pi( [&]( auto n )
                  { node2new[n] = mig.create_pi(); } );

  /* create mig nodes */
  topo_view aig_topo{ aig };
  aig_topo.foreach_node( [&]( auto n )
                         {
    if (aig.is_constant(n) || aig.is_pi(n)) return;

    std::vector<mig_network::signal> children;
    aig.foreach_fanin( n, [&]( auto const& f )
                       { children.push_back( aig.is_complemented( f ) ? mig.create_not( node2new[f] )
                                                                      : node2new[f] ); } );

    assert(children.size() == 2u);
    node2new[n] = mig.create_and( children[0], children[1] ); } );

  /* create pos */
  aig.foreach_po( [&]( auto const& f, auto index )
                  {
    auto const o =
        aig.is_complemented(f) ? mig.create_not(node2new[f]) : node2new[f];
    mig.create_po( o ); } );

  return mig;
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

mig_network mig_from_xmg( const xmg_network& xmg )
{
  xmg2mig_manager mgr( xmg );
  return mgr.run();
}

xag_network xag_from_aig( const aig_network& aig )
{
  aig2xag_manager mgr( aig );
  return mgr.run();
}

mig_network mig_from_aig( const aig_network& aig )
{
  aig2mig_manager mgr( aig );
  return mgr.run();
}
} // namespace also
