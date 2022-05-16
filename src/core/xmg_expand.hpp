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

namespace detail
{

template<class Ntk>
class xmg_expand_rewriting_impl
{
public:
  xmg_expand_rewriting_impl( Ntk& ntk )
      : ntk( ntk )
  {
  }

  void run()
  {
    run_xor3_flatten();
  }

private:

  void run_xor3_flatten()
  {
    /* rewrite xor3 to xor2 */
    ntk.foreach_po( [this]( auto po ) {
      topo_view topo{ntk, po};
      topo.foreach_node( [this]( auto n ) {
        xor3_to_xor2( n );
        return true;
      } );
    } );
  }

private:
  bool xor3_to_xor2( node<Ntk> const& n )
  {
    if ( !ntk.is_xor3( n ) )
      return false;

    if ( ntk.level( n ) == 0 )
      return false;

    /* get children of top node, ordered by node level (ascending) */
    const auto ocs = ordered_children( n );

    /* if the first child is constant, return */
    if( ocs[0].index == 0 )
      return false;

    auto opt = ntk.create_xor( ocs[2], ntk.create_xor( ocs[0], ocs[1] ) );
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
};

} // namespace detail

template<class Ntk>
void xmg_expand_rewriting( Ntk& ntk )
{
  detail::xmg_expand_rewriting_impl<Ntk> p( ntk );
  p.run();
}

} /* namespace mockturtle */
