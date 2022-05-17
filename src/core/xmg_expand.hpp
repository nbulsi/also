/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/*!
  \file xmg_expand.hpp
  \brief xmg expand xor through maj

  \author Zhufei Chu
*/

#pragma once

#include <iostream>
#include <optional>

#include <mockturtle/mockturtle.hpp>

namespace mockturtle
{
  struct xmg_expand_rewriting_params
  {
    enum strategy_t
    {
      expand,
      constants
    } strategy = expand;

    std::vector<unsigned> xor_index;
  };

namespace detail
{
template<class Ntk>
class xmg_expand_rewriting_impl
{
public:
  xmg_expand_rewriting_impl( Ntk& ntk, xmg_expand_rewriting_params const& ps )
      : ntk( ntk ), ps( ps )
  {
  }

  void run()
  {
    switch( ps.strategy )
    {
      case xmg_expand_rewriting_params::expand:
        run_xor_maj_expand();
        break;
      case xmg_expand_rewriting_params::constants:
        run_xor_constants();
        break;
    }
  }

private:
  void run_xor_maj_expand()
  {
    ntk.foreach_po( [this]( auto po ) {
      topo_view topo{ntk, po};
      topo.foreach_node( [this]( auto n ) {
        xor_maj_expand( n );
        return true;
      } );
    } );
  }
  
  void run_xor_constants()
  {
    ntk.foreach_po( [this]( auto po ) {
      topo_view topo{ntk, po};
      topo.foreach_node( [this]( auto n ) {
        xor_constants( n, ps.xor_index );
        return true;
      } );
    } );
  }

private:
  void print_children( std::array<signal<Ntk>, 3u> const& children )
  {
    std::cout << "children 0 : " << ntk.get_node( children[0] ) << std::endl;
    std::cout << "children 1 : " << ntk.get_node( children[1] ) << std::endl;
    std::cout << "children 2 : " << ntk.get_node( children[2] ) << std::endl;
  }

  bool xor_constants( node<Ntk> const& n, std::vector<unsigned> const& xors )
  {
    if ( !ntk.is_xor3( n ) )
      return false;

    if( std::find( xors.begin(), xors.end(), n ) != xors.end() )
    {
      std::cout << "Please assign constants to node " << n << std::endl;
      auto opt = ntk.get_constant( false );
      ntk.substitute_node( n, opt );
      ntk.update_levels();

      return true;
    }
    else
    {
      return false;
    }
  }
 
  bool xor_maj_expand( node<Ntk> const& n )
  {
    if ( !ntk.is_xor3( n ) )
      return false;

    if ( ntk.level( n ) <= 1 )
      return false;

    /* get children of top node, ordered by node level (ascending) */
    const auto ocs = ordered_children( n );

    /* if the first child is not constant, return */
    if( ocs[0].index != 0 )
      return false;

    if( !ntk.is_maj( ntk.get_node( ocs[1] ) ) || !ntk.is_maj( ntk.get_node( ocs[2] ) ) )
      return false;

    const auto ogcs = ordered_children( ntk.get_node( ocs[1] ) ); 
    
    auto n1 = ntk.create_xor( ogcs[0], ocs[2] );
    auto n2 = ntk.create_xor( ogcs[1], ocs[2] );
    auto n3 = ntk.create_xor( ogcs[2], ocs[2] );

    auto opt = ntk.create_maj( n1, n2, n3 );
    ntk.substitute_node( n, opt );
    ntk.update_levels();

    return true;
  }

  std::array<signal<Ntk>, 3> ordered_children( node<Ntk> const& n ) const
  {
    std::array<signal<Ntk>, 3> children;
    ntk.foreach_fanin( n, [&children]( auto const& f, auto i ) { children[i] = f; } );
    std::sort( children.begin(), children.end(), [this]( auto const& c1, auto const& c2 ) {
        if( ntk.level( ntk.get_node( c1 ) ) == ntk.level( ntk.get_node( c2 ) ) )
        {
          return c1.index < c2.index;
        }
        else
        {
          return ntk.level( ntk.get_node( c1 ) ) < ntk.level( ntk.get_node( c2 ) );
        }
    } );
    return children;
  }

private:
  Ntk& ntk;
  xmg_expand_rewriting_params const& ps;
};

} // namespace detail

template<class Ntk>
void xmg_expand_rewriting( Ntk& ntk, xmg_expand_rewriting_params const& ps )
{
  detail::xmg_expand_rewriting_impl<Ntk> p( ntk, ps );
  p.run();
}

} /* namespace mockturtle */
