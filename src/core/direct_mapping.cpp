/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

#include "direct_mapping.hpp"
#include <mockturtle/views/topo_view.hpp>

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

class xmg2rm3_manager
{
public:
  xmg2rm3_manager( xmg_network xmg );
  rm3_network run();

private:
  xmg_network xmg; // source graph
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
        node2new[n] = mig.create_xor3( children[0], children[1], children[2] );
        // auto minority = mig.create_not( mig.create_maj( children[0], children[1], children[2] ) );
        // node2new[n] = mig.create_maj( children[2],
        //                               minority,
        //                               mig.create_maj( children[0], children[1],  minority ) );
      } } );

  /* create pos */
  xmg.foreach_po( [&]( auto const& f, auto index )
                  {
        auto const o = xmg.is_complemented( f ) ? mig.create_not( node2new[f] ) : node2new[f];
        mig.create_po( o ); } );

  return mig;
}

// xmg to rm3
xmg2rm3_manager::xmg2rm3_manager( xmg_network xmg )
    : xmg( xmg )
{
}

rm3_network xmg2rm3_manager::run()
{
  rm3_network rm3;
  node_map<rm3_network::signal, xmg_network> node2new( xmg );

  node2new[xmg.get_node( xmg.get_constant( false ) )] = rm3.get_constant( false );

  /* create pis */
  xmg.foreach_pi( [&]( auto n )
                  { node2new[n] = rm3.create_pi(); } );

  /* create xmg nodes */
  topo_view xmg_topo{ xmg };
  xmg_topo.foreach_node( [&]( auto n )
                         {
      if ( xmg.is_constant( n ) || xmg.is_pi( n ) )
        return;

      std::vector<rm3_network::signal> children;
      xmg.foreach_fanin( n, [&]( auto const& f ) {
        children.push_back( xmg.is_complemented( f ) ? rm3.create_not( node2new[f] ) : node2new[f] );
      } );

      //const auto children = xmg.node( n );

      assert( children.size() == 3u );

      /* no XOR for now */
      //assert( xmg.is_maj( n ) );
      if( xmg.is_maj( n  ) )
      {
        const auto& xx = children[0u].index;
        const auto& yy = children[1u].index;
        const auto& zz = children[2u].index;
        const auto cx = children[0u].complement;
        const auto cy = children[1u].complement;
        const auto cz = children[2u].complement;


        /* AND/OR */
        if ( xx == 0u )
        {
          if ( !cy && !cz )
          {
            node2new[n] = rm3.create_rm3( children[1u], cx ? rm3.get_constant( false ) : rm3.get_constant( true ), children[2u] );
          }
          else if ( !cy && cz )
          {
            node2new[n] = rm3.create_rm3( cx ? rm3.get_constant( true ) : rm3.get_constant( false ), rm3.create_not(children[2u]), children[1u] );
          }
          else if ( cy && !cz )
          {
            node2new[n] = rm3.create_rm3( cx ? rm3.get_constant( true ) : rm3.get_constant( false ), rm3.create_not(children[1u]), children[2u] );
          }
          else if ( cy && cz )
          {
            node2new[n] = rm3.create_not( rm3.create_rm3( children[1u], cx ? rm3.get_constant( true ) : rm3.get_constant( false ), children[2u] ) );
          }
        }
        /* MAJ */
        else
        {
          // 根据极性判断属于哪一种情况
          if ( cx )
          {
            assert( !cy && !cz );
            node2new[n] = rm3.create_rm3( children[1u], rm3.create_not(children[0u]), children[2u] );
          }
          else if ( cy )
          {
            assert( !cx && !cz );
            node2new[n] = rm3.create_rm3( children[0u], rm3.create_not(children[1u]), children[2u] );
          }
          else if ( cz )
          {
            assert( !cx && !cy );
            node2new[n] = rm3.create_rm3( children[0u], rm3.create_not(children[2u]), children[1u] );
          }
          else
          {
            node2new[n] = rm3.create_rm3( children[0u], rm3.create_not(children[1u]) , children[2u] );
          }
        }
      }
      else
      {
        //assert( xmg.is_maj( n ) );
        // xor3
        node2new[n] = rm3.create_xor3( children[0], children[1], children[2] );

        // auto minority = rm3.create_rm3( children[0], rm3.create_not( children[1] ), children[2] ) ;
        // node2new[n] = rm3.create_rm3( children[2],
        //                               rm3.create_not( minority ),
        //                               rm3.create_rm3( children[0], minority, children[1] ) );
      } } );

  /* create pos */
  xmg.foreach_po( [&]( auto const& f, auto index )
                  {
        auto const o = xmg.is_complemented( f ) ? rm3.create_not( node2new[f] ) : node2new[f];
        rm3.create_po( o ); } );

  return rm3;
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

rm3_network rm3_from_xmg( const xmg_network& xmg )
{
  xmg2rm3_manager mgr( xmg );
  return mgr.run();
}

} // namespace also
