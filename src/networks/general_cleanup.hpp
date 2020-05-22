/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019-  Ningbo University, Ningbo, China
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*!
  \file general_cleanup.hpp
  \brief Cleans up m5ig/img networks etc.

  \author Zhufei Chu
*/

#pragma once

#include <iostream>
#include <type_traits>
#include <vector>

#include <kitty/operations.hpp>

#include <mockturtle/traits.hpp>
#include <mockturtle/utils/node_map.hpp>
#include <mockturtle/views/topo_view.hpp>

namespace mockturtle
{

template<typename NtkSource, typename NtkDest, typename LeavesIterator>
std::vector<signal<NtkDest>> general_cleanup_dangling( NtkSource const& ntk, NtkDest& dest, LeavesIterator begin, LeavesIterator end )
{
  (void)end;

  node_map<signal<NtkDest>, NtkSource> old_to_new( ntk );
  old_to_new[ntk.get_constant( false )] = dest.get_constant( false );

  /* create inputs in same order */
  auto it = begin;
  ntk.foreach_pi( [&]( auto node ) {
    old_to_new[node] = *it++;
  } );
  assert( it == end );

  /* foreach node in topological order */
  topo_view topo{ntk};
  topo.foreach_node( [&]( auto node ) {
    if ( ntk.is_constant( node ) || ntk.is_pi( node ) )
      return;

    /* collect children */
    std::vector<signal<NtkDest>> children;
    ntk.foreach_fanin( node, [&]( auto child, auto ) {
      const auto f = old_to_new[child];
      if ( ntk.is_complemented( child ) )
      {
        children.push_back( dest.create_not( f ) );
      }
      else
      {
        children.push_back( f );
      }
    } );
      
    old_to_new[node] = dest.clone_node( ntk, node, children );
  } );

  /* create outputs in same order */
  std::vector<signal<NtkDest>> fs;
  ntk.foreach_po( [&]( auto po ) {
    const auto f = old_to_new[po];
    if ( ntk.is_complemented( po ) )
    {
      fs.push_back( dest.create_not( f ) );
    }
    else
    {
      fs.push_back( f );
    }
  } );

  return fs;
}

template<class NtkSrc, class NtkDest = NtkSrc>
NtkDest general_cleanup_dangling( NtkSrc const& ntk )
{
  static_assert( is_network_type_v<NtkSrc>, "NtkSrc is not a network type" );
  static_assert( is_network_type_v<NtkDest>, "NtkDest is not a network type" );
  static_assert( has_get_node_v<NtkSrc>, "NtkSrc does not implement the get_node method" );
  static_assert( has_node_to_index_v<NtkSrc>, "NtkSrc does not implement the node_to_index method" );
  static_assert( has_get_constant_v<NtkSrc>, "NtkSrc does not implement the get_constant method" );
  static_assert( has_foreach_node_v<NtkSrc>, "NtkSrc does not implement the foreach_node method" );
  static_assert( has_foreach_pi_v<NtkSrc>, "NtkSrc does not implement the foreach_pi method" );
  static_assert( has_foreach_po_v<NtkSrc>, "NtkSrc does not implement the foreach_po method" );
  static_assert( has_is_pi_v<NtkSrc>, "NtkSrc does not implement the is_pi method" );
  static_assert( has_is_constant_v<NtkSrc>, "NtkSrc does not implement the is_constant method" );
  static_assert( has_clone_node_v<NtkDest>, "NtkDest does not implement the clone_node method" );
  static_assert( has_create_pi_v<NtkDest>, "NtkDest does not implement the create_pi method" );
  static_assert( has_create_po_v<NtkDest>, "NtkDest does not implement the create_po method" );
  static_assert( has_create_not_v<NtkDest>, "NtkDest does not implement the create_not method" );
  static_assert( has_is_complemented_v<NtkSrc>, "NtkDest does not implement the is_complemented method" );

  NtkDest dest;
  std::vector<signal<NtkDest>> pis;
  ntk.foreach_pi( [&]( auto ) {
    pis.push_back( dest.create_pi() );
  } );

  for ( auto f : general_cleanup_dangling( ntk, dest, pis.begin(), pis.end() ) )
  {
    dest.create_po( f );
  }

  return dest;
}

} // namespace mockturtle
