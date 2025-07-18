/* mockturtle: C++ logic network library
 * Copyright (C) 2018-2022  EPFL
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
  \file xmg.hpp
  \brief XMG logic network implementation

  \author Alessandro Tempia Calvino
  \author Bruno Schmitt
  \author Hanyu Wang
  \author Heinz Riener
  \author Jinzheng Tu
  \author Mathias Soeken
  \author Max Austin
  \author Siang-Yun (Sonia) Lee
  \author Walter Lau Neto
*/

#pragma once

#include <map>
#include <memory>
#include <optional>
#include <stack>
#include <string>

#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operators.hpp>

#include "../traits.hpp"
#include "../utils/algorithm.hpp"
#include "detail/foreach.hpp"
#include "events.hpp"
#include "storage.hpp"

namespace mockturtle
{

/*! \brief XMG storage container

  XMGs have nodes with fan-in 3.  We split of one bit of the index pointer to
  store a complemented attribute.  Every node has 64-bit of additional data
  used for the following purposes:

  `data[0].h1`: Fan-out size (we use MSB to indicate whether a node is dead)
  `data[0].h2`: Application-specific value
  `data[1].h1`: Visited flag
  `data[1].h2`: Node phase
*/
using xmg_storage = storage<regular_node<3, 2, 1>>;

class xmg_network
{
public:
#pragma region Types and constructors
  static constexpr auto min_fanin_size = 3u;
  static constexpr auto max_fanin_size = 3u;

  using base_type = xmg_network;
  using storage = std::shared_ptr<xmg_storage>;
  using node = std::size_t;
  std::map<node, std::vector<node>> parentss;
  static constexpr node XMG_NULL{ 0x7FFFFFFFFFFFFFFFu };

  struct signal
  {
    signal() = default;

    signal( std::size_t index, std::size_t complement )
        : complement( complement ), index( index )
    {
    }

    signal( std::size_t data )
        : data( data )
    {
    }

    signal( xmg_storage::node_type::pointer_type const& p )
        : complement( p.weight ), index( p.index )
    {
    }

    union
    {
      struct
      {
        std::size_t complement : 1;
        std::size_t index : 63;
      };
      std::size_t data;
    };

    signal operator!() const
    {
      return signal( data ^ 1 );
    }

    signal operator+() const
    {
      return { index, 0 };
    }

    signal operator-() const
    {
      return { index, 1 };
    }

    signal operator^( bool complement ) const
    {
      return signal( data ^ ( complement ? 1 : 0 ) );
    }

    bool operator==( signal const& other ) const
    {
      return data == other.data;
    }

    bool operator!=( signal const& other ) const
    {
      return data != other.data;
    }

    bool operator<( signal const& other ) const
    {
      return data < other.data;
    }

    operator xmg_storage::node_type::pointer_type() const
    {
      return { index, complement };
    }

#if __cplusplus > 201703L
    bool operator==( xmg_storage::node_type::pointer_type const& other ) const
    {
      return data == other.data;
    }
#endif
  };
  enum FLAG_TYPE
  {
    F_PHASE = 0x1,
  };
  xmg_network()
      : _storage( std::make_shared<xmg_storage>() ),
        _events( std::make_shared<decltype( _events )::element_type>() )
  {
  }

  xmg_network( std::shared_ptr<xmg_storage> storage )
      : _storage( storage ),
        _events( std::make_shared<decltype( _events )::element_type>() )
  {
  }

  xmg_network clone() const
  {
    return { std::make_shared<xmg_storage>( *_storage ) };
  }
#pragma endregion

#pragma region Primary I / O and constants
  signal get_constant( bool value ) const
  {
    return { 0, static_cast<size_t>( value ? 1 : 0 ) };
  }

  signal create_pi()
  {
    const auto index = _storage->nodes.size();
    auto& node = _storage->nodes.emplace_back();
    node.children[0].data = node.children[1].data = node.children[2].data = _storage->inputs.size();
    // node.data[1].h2 = 1; // mark as PI
    phase( index, 0 );
    _storage->inputs.emplace_back( index );
    return { index, 0 };
  }

  uint32_t create_po( signal const& f )
  {
    /* increase ref-count to children */
    _storage->nodes[f.index].data[0].h1++;
    auto const po_index = static_cast<uint32_t>( _storage->outputs.size() );
    _storage->outputs.emplace_back( f.index, f.complement );
    return po_index;
  }

  bool is_combinational() const
  {
    return true;
  }

  bool is_constant( node const& n ) const
  {
    return n == 0;
  }

  bool is_ci( node const& n ) const
  {
    return _storage->nodes[n].children[0].data == _storage->nodes[n].children[1].data;
  }

  bool is_pi( node const& n ) const
  {
    return _storage->nodes[n].children[0].data == _storage->nodes[n].children[1].data && !is_constant( n );
  }

  bool constant_value( node const& n ) const
  {
    (void)n;
    return false;
  }
#pragma endregion

#pragma region Create unary functions
  signal create_buf( signal const& a )
  {
    return a;
  }

  signal create_not( signal const& a )
  {
    return !a;
  }
#pragma endregion

#pragma region Create binary / ternary functions
  signal create_maj( signal a, signal b, signal c )
  {
    /* order inputs */
    if ( a.index > b.index )
    {
      std::swap( a, b );
    }
    if ( b.index > c.index )
    {
      std::swap( b, c );
    }
    if ( a.index > b.index )
    {
      std::swap( a, b );
    }

    /* trivial cases */
    if ( a.index == b.index )
    {
      return ( a.complement == b.complement ) ? a : c;
    }
    else if ( b.index == c.index )
    {
      return ( b.complement == c.complement ) ? b : a;
    }
    else if ( ( a.index == b.index ) && ( b.index == c.index ) )
    {
      return ( a.complement == b.complement ) ? a : c;
    }

    /*  complemented edges minimization */
    auto node_complement = false;
    if ( static_cast<unsigned>( a.complement ) + static_cast<unsigned>( b.complement ) +
             static_cast<unsigned>( c.complement ) >=
         2u )
    {
      node_complement = true;
      a.complement = !a.complement;
      b.complement = !b.complement;
      c.complement = !c.complement;
    }

    storage::element_type::node_type node;
    node.children[0] = a;
    node.children[1] = b;
    node.children[2] = c;

    /* structural hashing */
    const auto it = _storage->hash.find( node );
    if ( it != _storage->hash.end() )
    {
      return { it->second, node_complement };
    }

    const auto index = _storage->nodes.size();

    if ( index >= .9 * _storage->nodes.capacity() )
    {
      _storage->nodes.reserve( static_cast<size_t>( 3.1415 * index ) );
      _storage->hash.reserve( static_cast<size_t>( 3.1415 * index ) );
    }

    _storage->nodes.push_back( node );

    _storage->hash[node] = index;

    // phase
    phase( index, ( phase( a.index ) ^ a.complement ) & ( phase( b.index ) ^ b.complement ) & ( phase( c.index ) ^ c.complement ) );

    /* increase ref-count to children */
    _storage->nodes[a.index].data[0].h1++;
    _storage->nodes[b.index].data[0].h1++;
    _storage->nodes[c.index].data[0].h1++;

    for ( auto const& fn : _events->on_add )
    {
      ( *fn )( index );
    }

    return { index, node_complement };
  }

  signal create_xor3( signal a, signal b, signal c )
  {
    /* order inputs */
    if ( a.index < b.index )
    {
      std::swap( a, b );
    }
    if ( b.index < c.index )
    {
      std::swap( b, c );
    }
    if ( a.index < b.index )
    {
      std::swap( a, b );
    }

    /* propagate complement edges */
    bool fcompl = ( a.complement != b.complement ) != c.complement;
    a.complement = b.complement = c.complement = false;

    /* trivial cases */
    if ( a.index == b.index )
    {
      return c ^ fcompl;
    }
    else if ( b.index == c.index )
    {
      return a ^ fcompl;
    }

    storage::element_type::node_type node;
    node.children[0] = a;
    node.children[1] = b;
    node.children[2] = c;

    /* structural hashing */
    const auto it = _storage->hash.find( node );
    if ( it != _storage->hash.end() )
    {
      return { it->second, fcompl };
    }

    const auto index = _storage->nodes.size();

    if ( index >= .9 * _storage->nodes.capacity() )
    {
      _storage->nodes.reserve( static_cast<size_t>( 3.1415 * index ) );
      _storage->hash.reserve( static_cast<size_t>( 3.1415 * index ) );
    }

    _storage->nodes.push_back( node );

    _storage->hash[node] = index;

    // phase
    phase( index, ( phase( a.index ) ^ a.complement ) & ( phase( b.index ) ^ b.complement ) & ( phase( c.index ) ^ c.complement ) );

    /* increase ref-count to children */
    _storage->nodes[a.index].data[0].h1++;
    _storage->nodes[b.index].data[0].h1++;
    _storage->nodes[c.index].data[0].h1++;

    for ( auto const& fn : _events->on_add )
    {
      ( *fn )( index );
    }

    return { index, fcompl };
  }

  /*
   * 2019/02/27, for inverters optimization, should turnoff the
   * automatic complemented edges minimization
   * */
  signal create_maj_without_complement_opt( signal a, signal b, signal c )
  {
    /* order inputs */
    if ( a.index > b.index )
    {
      std::swap( a, b );
    }
    if ( b.index > c.index )
    {
      std::swap( b, c );
    }
    if ( a.index > b.index )
    {
      std::swap( a, b );
    }

    /* trivial cases */
    if ( a.index == b.index )
    {
      return ( a.complement == b.complement ) ? a : c;
    }
    else if ( b.index == c.index )
    {
      return ( b.complement == c.complement ) ? b : a;
    }
    else if ( ( a.index == b.index ) && ( b.index == c.index ) )
    {
      return ( a.complement == b.complement ) ? a : c;
    }

    /*  complemented edges minimization */
    auto node_complement = false;

    storage::element_type::node_type node;
    node.children[0] = a;
    node.children[1] = b;
    node.children[2] = c;

    /* structural hashing */
    const auto it = _storage->hash.find( node );
    if ( it != _storage->hash.end() )
    {
      return { it->second, node_complement };
    }

    const auto index = _storage->nodes.size();

    if ( index >= .9 * _storage->nodes.capacity() )
    {
      _storage->nodes.reserve( static_cast<size_t>( 3.1415 * index ) );
      _storage->hash.reserve( static_cast<size_t>( 3.1415 * index ) );
    }

    _storage->nodes.push_back( node );

    _storage->hash[node] = index;

    /* increase ref-count to children */
    _storage->nodes[a.index].data[0].h1++;
    _storage->nodes[b.index].data[0].h1++;
    _storage->nodes[c.index].data[0].h1++;

    for ( auto const& fn : _events->on_add )
    {
      ( *fn )( index );
    }

    return { index, node_complement };
  }

  signal create_xor3_without_complement_opt( signal a, signal b, signal c )
  {
    /* order inputs */
    if ( a.index < b.index )
    {
      std::swap( a, b );
    }
    if ( b.index < c.index )
    {
      std::swap( b, c );
    }
    if ( a.index < b.index )
    {
      std::swap( a, b );
    }

    /* propagate complement edges */
    bool fcompl = ( a.complement != b.complement ) != c.complement;
    // a.complement = b.complement = c.complement = false;

    /* trivial cases */
    if ( a.index == b.index )
    {
      return c ^ fcompl;
    }
    else if ( b.index == c.index )
    {
      return a ^ fcompl;
    }
    else if ( ( a.index == b.index ) && ( b.index == c.index ) )
    {
      return a ^ fcompl;
    }

    storage::element_type::node_type node;
    node.children[0] = a;
    node.children[1] = b;
    node.children[2] = c;

    /* 2019/02/17 disable inverters propagation */
    fcompl = false;

    /* structural hashing */
    const auto it = _storage->hash.find( node );
    if ( it != _storage->hash.end() )
    {
      return { it->second, fcompl };
    }

    const auto index = _storage->nodes.size();

    if ( index >= .9 * _storage->nodes.capacity() )
    {
      _storage->nodes.reserve( static_cast<size_t>( 3.1415 * index ) );
      _storage->hash.reserve( static_cast<size_t>( 3.1415 * index ) );
    }

    _storage->nodes.push_back( node );

    _storage->hash[node] = index;

    /* increase ref-count to children */
    _storage->nodes[a.index].data[0].h1++;
    _storage->nodes[b.index].data[0].h1++;
    _storage->nodes[c.index].data[0].h1++;

    for ( auto const& fn : _events->on_add )
    {
      ( *fn )( index );
    }

    return { index, fcompl };
  }

  signal create_ite( signal cond, signal f_then, signal f_else )
  {
    bool f_compl{ false };
    if ( f_then.index < f_else.index )
    {
      std::swap( f_then, f_else );
      cond.complement ^= 1;
    }
    if ( f_then.complement )
    {
      f_then.complement = 0;
      f_else.complement ^= 1;
      f_compl = true;
    }

    return create_and( !create_and( !cond, f_else ), !create_and( cond, f_then ) ) ^ !f_compl;
  }

  signal create_and( signal const& a, signal const& b )
  {
    return create_maj( get_constant( false ), a, b );
  }

  signal create_nand( signal const& a, signal const& b )
  {
    return !create_and( a, b );
  }

  signal create_or( signal const& a, signal const& b )
  {
    return create_maj( get_constant( true ), a, b );
  }

  signal create_nor( signal const& a, signal const& b )
  {
    return !create_or( a, b );
  }

  signal create_lt( signal const& a, signal const& b )
  {
    return create_and( !a, b );
  }

  signal create_le( signal const& a, signal const& b )
  {
    return !create_and( a, !b );
  }

  signal create_xor( signal const& a, signal const& b )
  {
    return create_xor3( get_constant( false ), a, b );
  }

  signal create_xor_without_complement_opt( signal const& a, signal const& b )
  {
    return create_xor3_without_complement_opt( get_constant( false ), a, b );
  }

  signal create_xnor( signal const& a, signal const& b )
  {
    return create_xor3( get_constant( true ), a, b );
  }

  signal create_xnor_without_complement_opt( signal const& a, signal const& b )
  {
    return create_xor3_without_complement_opt( get_constant( true ), a, b );
  }
#pragma endregion

#pragma region Create nary functions
  signal create_nary_and( std::vector<signal> const& fs )
  {
    return tree_reduce( fs.begin(), fs.end(), get_constant( true ), [this]( auto const& a, auto const& b ) { return create_and( a, b ); } );
  }

  signal create_nary_or( std::vector<signal> const& fs )
  {
    return tree_reduce( fs.begin(), fs.end(), get_constant( false ), [this]( auto const& a, auto const& b ) { return create_or( a, b ); } );
  }

  signal create_nary_xor( std::vector<signal> const& fs )
  {
    return ternary_tree_reduce( fs.begin(), fs.end(), get_constant( false ), [this]( auto const& a, auto const& b, auto const& c ) { return create_xor3( a, b, c ); } );
  }
#pragma endregion

#pragma region Create arbitrary functions
  signal clone_node( xmg_network const& other, node const& source, std::vector<signal> const& children )
  {
    assert( children.size() == 3u );

    if ( other.is_maj( source ) )
    {
      return create_maj( children[0u], children[1u], children[2u] );
    }
    else
    {
      return create_xor3( children[0u], children[1u], children[2u] );
    }
  }
#pragma endregion

#pragma region Has node
  std::optional<signal> has_maj( signal a, signal b, signal c )
  {
    /* order inputs */
    if ( a.index > b.index )
    {
      std::swap( a, b );
    }
    if ( b.index > c.index )
    {
      std::swap( b, c );
    }
    if ( a.index > b.index )
    {
      std::swap( a, b );
    }

    /* trivial cases */
    if ( a.index == b.index )
    {
      return ( a.complement == b.complement ) ? a : c;
    }
    else if ( b.index == c.index )
    {
      return ( b.complement == c.complement ) ? b : a;
    }

    /*  complemented edges minimization */
    auto node_complement = false;
    if ( static_cast<unsigned>( a.complement ) + static_cast<unsigned>( b.complement ) +
             static_cast<unsigned>( c.complement ) >=
         2u )
    {
      node_complement = true;
      a.complement = !a.complement;
      b.complement = !b.complement;
      c.complement = !c.complement;
    }

    storage::element_type::node_type node;
    node.children[0] = a;
    node.children[1] = b;
    node.children[2] = c;

    /* structural hashing */
    const auto it = _storage->hash.find( node );
    if ( it != _storage->hash.end() )
    {
      assert( !is_dead( it->second ) );
      return signal( it->second, node_complement );
    }

    return {};
  }

  std::optional<signal> has_xor3( signal a, signal b, signal c )
  {
    /* order inputs */
    if ( a.index < b.index )
    {
      std::swap( a, b );
    }
    if ( b.index < c.index )
    {
      std::swap( b, c );
    }
    if ( a.index < b.index )
    {
      std::swap( a, b );
    }

    /* propagate complement edges */
    bool fcompl = ( a.complement != b.complement ) != c.complement;
    a.complement = b.complement = c.complement = false;

    /* trivial cases */
    if ( a.index == b.index )
    {
      return c ^ fcompl;
    }
    else if ( b.index == c.index )
    {
      return a ^ fcompl;
    }

    storage::element_type::node_type node;
    node.children[0] = a;
    node.children[1] = b;
    node.children[2] = c;

    /* structural hashing */
    const auto it = _storage->hash.find( node );
    if ( it != _storage->hash.end() )
    {
      assert( !is_dead( it->second ) );
      return signal( it->second, fcompl );
    }

    return {};
  }
#pragma endregion

#pragma region Restructuring
  std::optional<std::pair<node, signal>> replace_in_node( node const& n, node const& old_node, signal new_signal )
  {
    auto& node = _storage->nodes[n];

    uint32_t fanin = 0u;
    for ( auto i = 0u; i < 4u; ++i )
    {
      if ( i == 3u )
      {
        return std::nullopt;
      }

      if ( node.children[i].index == old_node )
      {
        fanin = i;
        new_signal.complement ^= node.children[i].weight;
        break;
      }
    }

    // determine potential new children of node n
    signal child2 = new_signal;
    signal child1 = node.children[( fanin + 1 ) % 3];
    signal child0 = node.children[( fanin + 2 ) % 3];

    auto _is_maj = is_maj( n );

    /* normalize order */
    if ( _is_maj )
    {
      if ( child0.index > child1.index )
      {
        std::swap( child0, child1 );
      }
      if ( child1.index > child2.index )
      {
        std::swap( child1, child2 );
      }
      if ( child0.index > child1.index )
      {
        std::swap( child0, child1 );
      }

      assert( child0.index <= child1.index );
      assert( child1.index <= child2.index );
    }
    else
    {
      if ( child0.index < child1.index )
      {
        std::swap( child0, child1 );
      }
      if ( child1.index < child2.index )
      {
        std::swap( child1, child2 );
      }
      if ( child0.index < child1.index )
      {
        std::swap( child0, child1 );
      }

      assert( child0.index >= child1.index );
      assert( child1.index >= child2.index );
    }

    // normalize complemented edges
    auto node_complement = false;
    if ( _is_maj )
    {
      if ( static_cast<unsigned>( child0.complement ) + static_cast<unsigned>( child1.complement ) +
               static_cast<unsigned>( child2.complement ) >=
           2u )
      {
        node_complement = true;
        child0.complement = !child0.complement;
        child1.complement = !child1.complement;
        child2.complement = !child2.complement;
      }
    }
    else
    {
      node_complement = ( child0.complement != child1.complement ) != child2.complement;
      child0.complement = child1.complement = child2.complement = false;
    }

    // check for trivial cases?
    if ( _is_maj )
    {
      if ( child0.index == child1.index )
      {
        const auto diff_pol = child0.complement != child1.complement;
        return std::make_pair( n, ( diff_pol ? child2 : child0 ) ^ node_complement );
      }
      else if ( child1.index == child2.index )
      {
        const auto diff_pol = child1.complement != child2.complement;
        return std::make_pair( n, ( diff_pol ? child0 : child1 ) ^ node_complement );
      }
    }
    else
    {
      if ( child0.index == child1.index )
      {
        return std::make_pair( n, child2 ^ node_complement );
      }
      else if ( child1.index == child2.index )
      {
        return std::make_pair( n, child0 ^ node_complement );
      }
    }

    // node already in hash table
    storage::element_type::node_type _hash_obj;
    _hash_obj.children[0] = child0;
    _hash_obj.children[1] = child1;
    _hash_obj.children[2] = child2;
    if ( const auto it = _storage->hash.find( _hash_obj ); it != _storage->hash.end() && it->second != old_node )
    {
      return std::make_pair( n, signal( it->second, 0 ) );
    }

    // remember before
    const auto old_child0 = signal{ node.children[0] };
    const auto old_child1 = signal{ node.children[1] };
    const auto old_child2 = signal{ node.children[2] };

    // erase old node in hash table
    _storage->hash.erase( node );

    // insert updated node into hash table
    node.children[0] = child0;
    node.children[1] = child1;
    node.children[2] = child2;
    _storage->hash[node] = n;

    // update the reference counter of the new signal
    _storage->nodes[new_signal.index].data[0].h1++;

    for ( auto const& fn : _events->on_modified )
    {
      ( *fn )( n, { old_child0, old_child1, old_child2 } );
    }

    return std::nullopt;
  }

  std::optional<std::pair<node, signal>> replace_in_node_without_complement_opt( node const& n, node const& old_node, signal new_signal )
  {
    auto& node = _storage->nodes[n];

    uint32_t fanin = 0u;
    for ( auto i = 0u; i < 4u; ++i )
    {
      if ( i == 3u )
      {
        return std::nullopt;
      }

      if ( node.children[i].index == old_node )
      {
        fanin = i;
        new_signal.complement ^= node.children[i].weight;
        break;
      }
    }

    // determine potential new children of node n
    signal child2 = new_signal;
    signal child1 = node.children[( fanin + 1 ) % 3];
    signal child0 = node.children[( fanin + 2 ) % 3];

    auto _is_maj = is_maj( n );

    /* normalize order */
    if ( _is_maj )
    {
      if ( child0.index > child1.index )
      {
        std::swap( child0, child1 );
      }
      if ( child1.index > child2.index )
      {
        std::swap( child1, child2 );
      }
      if ( child0.index > child1.index )
      {
        std::swap( child0, child1 );
      }

      assert( child0.index <= child1.index );
      assert( child1.index <= child2.index );
    }
    else
    {
      if ( child0.index < child1.index )
      {
        std::swap( child0, child1 );
      }
      if ( child1.index < child2.index )
      {
        std::swap( child1, child2 );
      }
      if ( child0.index < child1.index )
      {
        std::swap( child0, child1 );
      }

      assert( child0.index >= child1.index );
      assert( child1.index >= child2.index );

      // check same fanins: transform the XOR3 into a MAJ3
      if ( child0.index == child1.index )
      {
        child0.index = child2.index;
        child1.index = child2.index;
        if ( child0.complement == child1.complement )
        {
          child0.complement = child2.complement;
          child1.complement = child2.complement;
        }
        else
        {
          child0.complement = !child2.complement;
          child1.complement = !child2.complement;
        }

        _is_maj = true;
      }
    }

    // normalize complemented edges
    auto node_complement = false;
    if ( _is_maj )
    {
      if ( static_cast<unsigned>( child0.complement ) + static_cast<unsigned>( child1.complement ) +
               static_cast<unsigned>( child2.complement ) >=
           2u )
      {
        node_complement = true;
        child0.complement = !child0.complement;
        child1.complement = !child1.complement;
        child2.complement = !child2.complement;
      }
    }
    else
    {
      if ( child0.index == child1.index )
      {
        return std::make_pair( n, child2 ^ node_complement );
      }
      else if ( child1.index == child2.index )
      {
        return std::make_pair( n, child0 ^ node_complement );
      }
      else if ( ( child0.index == child1.index ) && ( child1.index == child2.index ) )
      {
        return std::make_pair( n, child0 ^ node_complement );
      }
    }

    // node already in hash table
    storage::element_type::node_type _hash_obj;
    _hash_obj.children[0] = child0;
    _hash_obj.children[1] = child1;
    _hash_obj.children[2] = child2;
    if ( const auto it = _storage->hash.find( _hash_obj ); it != _storage->hash.end() )
    {
      return std::make_pair( n, signal( it->second, 0 ) );
    }

    // remember before
    const auto old_child0 = signal{ node.children[0] };
    const auto old_child1 = signal{ node.children[1] };
    const auto old_child2 = signal{ node.children[2] };

    // erase old node in hash table
    _storage->hash.erase( node );

    // insert updated node into hash table
    node.children[0] = child0;
    node.children[1] = child1;
    node.children[2] = child2;
    if ( _storage->hash.find( node ) == _storage->hash.end() )
    {
      _storage->hash[node] = n;
    }

    // update the reference counter of the new signal
    _storage->nodes[new_signal.index].data[0].h1++;

    for ( auto const& fn : _events->on_modified )
    {
      ( *fn )( n, { old_child0, old_child1, old_child2 } );
    }

    return std::nullopt;
  }
  void replace_in_node_no_restrash( node const& n, node const& old_node, signal new_signal )
  {
    auto& node = _storage->nodes[n];

    uint32_t fanin = 0u;
    for ( auto i = 0u; i < 4u; ++i )
    {
      if ( i == 3u )
      {
        return;
      }

      if ( node.children[i].index == old_node )
      {
        fanin = i;
        new_signal.complement ^= node.children[i].weight;
        break;
      }
    }

    // determine potential new children of node n
    signal child2 = new_signal;
    signal child1 = node.children[( fanin + 1 ) % 3];
    signal child0 = node.children[( fanin + 2 ) % 3];

    auto _is_maj = is_maj( n );

    /* normalize order */
    if ( _is_maj )
    {
      if ( child0.index > child1.index )
      {
        std::swap( child0, child1 );
      }
      if ( child1.index > child2.index )
      {
        std::swap( child1, child2 );
      }
      if ( child0.index > child1.index )
      {
        std::swap( child0, child1 );
      }

      assert( child0.index <= child1.index );
      assert( child1.index <= child2.index );
    }
    else
    {
      if ( child0.index < child1.index )
      {
        std::swap( child0, child1 );
      }
      if ( child1.index < child2.index )
      {
        std::swap( child1, child2 );
      }
      if ( child0.index < child1.index )
      {
        std::swap( child0, child1 );
      }

      assert( child0.index >= child1.index );
      assert( child1.index >= child2.index );

      // check same fanins: transform the XOR3 into a MAJ3
      if ( child0.index == child1.index )
      {
        child0.index = child2.index;
        child1.index = child2.index;
        if ( child0.complement == child1.complement )
        {
          child0.complement = child2.complement;
          child1.complement = child2.complement;
        }
        else
        {
          child0.complement = !child2.complement;
          child1.complement = !child2.complement;
        }

        _is_maj = true;
      }
    }

    // normalize complemented edges
    auto node_complement = false;
    if ( _is_maj )
    {
      if ( static_cast<unsigned>( child0.complement ) + static_cast<unsigned>( child1.complement ) +
               static_cast<unsigned>( child2.complement ) >=
           2u )
      {
        node_complement = true;
        child0.complement = !child0.complement;
        child1.complement = !child1.complement;
        child2.complement = !child2.complement;
      }
    }
    else
    {
      node_complement = ( child0.complement != child1.complement ) != child2.complement;
      child0.complement = child1.complement = child2.complement = false;
    }

    // don't check for trivial cases

    // remember before
    const auto old_child0 = signal{ node.children[0] };
    const auto old_child1 = signal{ node.children[1] };
    const auto old_child2 = signal{ node.children[2] };

    // erase old node in hash table
    _storage->hash.erase( node );

    // insert updated node into hash table
    node.children[0] = child0;
    node.children[1] = child1;
    node.children[2] = child2;
    if ( _storage->hash.find( node ) == _storage->hash.end() )
    {
      _storage->hash[node] = n;
    }

    // update the reference counter of the new signal
    _storage->nodes[new_signal.index].data[0].h1++;

    for ( auto const& fn : _events->on_modified )
    {
      ( *fn )( n, { old_child0, old_child1, old_child2 } );
    }
  }
  void replace_in_outputs( node const& old_node, signal const& new_signal )
  {
    if ( is_dead( old_node ) )
      return;

    for ( auto& output : _storage->outputs )
    {
      if ( output.index == old_node )
      {
        output.index = new_signal.index;
        output.weight ^= new_signal.complement;

        if ( old_node != new_signal.index )
        {
          /* increment fan-in of new node */
          _storage->nodes[new_signal.index].data[0].h1++;
        }
      }
    }
  }

  void take_out_node( node const& n )
  {
    /* we cannot delete CIs or constants */
    if ( n == 0 || is_ci( n ) || is_dead( n ) )
      return;

    auto& nobj = _storage->nodes[n];
    nobj.data[0].h1 = UINT32_C( 0x80000000 ); /* fanout size 0, but dead */
    _storage->hash.erase( nobj );

    for ( auto const& fn : _events->on_delete )
    {
      ( *fn )( n );
    }

    for ( auto i = 0u; i < 3u; ++i )
    {
      if ( fanout_size( nobj.children[i].index ) == 0 )
      {
        continue;
      }
      if ( decr_fanout_size( nobj.children[i].index ) == 0 )
      {
        take_out_node( nobj.children[i].index );
      }
    }
  }

  void revive_node( node const& n )
  {
    if ( !is_dead( n ) )
      return;

    assert( n < _storage->nodes.size() );
    auto& nobj = _storage->nodes[n];
    nobj.data[0].h1 = UINT32_C( 0 ); /* fanout size 0, but not dead (like just created) */
    _storage->hash[nobj] = n;

    for ( auto const& fn : _events->on_add )
    {
      ( *fn )( n );
    }

    /* revive its children if dead, and increment their fanout_size */
    for ( auto i = 0u; i < 3u; ++i )
    {
      if ( is_dead( nobj.children[i].index ) )
      {
        revive_node( nobj.children[i].index );
      }
      incr_fanout_size( nobj.children[i].index );
    }
  }

  inline bool is_dead( node const& n ) const
  {
    return ( _storage->nodes[n].data[0].h1 >> 31 ) & 1;
  }

  void substitute_node( node const& old_node, signal const& new_signal )
  {
    std::unordered_map<node, signal> old_to_new;
    std::stack<std::pair<node, signal>> to_substitute;
    to_substitute.push( { old_node, new_signal } );

    while ( !to_substitute.empty() )
    {
      const auto [_old, _curr] = to_substitute.top();
      to_substitute.pop();

      signal _new = _curr;
      /* find the real new node */
      if ( is_dead( get_node( _new ) ) )
      {
        auto it = old_to_new.find( get_node( _new ) );
        while ( it != old_to_new.end() )
        {
          _new = is_complemented( _new ) ? create_not( it->second ) : it->second;
          it = old_to_new.find( get_node( _new ) );
        }
      }
      /* revive */
      if ( is_dead( get_node( _new ) ) )
      {
        revive_node( get_node( _new ) );
      }

      for ( auto idx = 1u; idx < _storage->nodes.size(); ++idx )
      {
        if ( is_ci( idx ) || is_dead( idx ) )
          continue; /* ignore CIs */

        if ( const auto repl = replace_in_node( idx, _old, _new ); repl )
        {
          to_substitute.push( *repl );
        }
      }

      /* check outputs */
      replace_in_outputs( _old, _new );

      // reset fan-in of old node
      if ( _old != _new.index )
      {
        old_to_new.insert( { _old, _new } );
        take_out_node( _old );
      }
    }
  }

  void substitute_node_without_complement_opt( node const& old_node, signal const& new_signal )
  {
    std::stack<std::pair<node, signal>> to_substitute;
    to_substitute.push( { old_node, new_signal } );

    while ( !to_substitute.empty() )
    {
      const auto [_old, _new] = to_substitute.top();
      to_substitute.pop();

      for ( auto idx = 1u; idx < _storage->nodes.size(); ++idx )
      {
        if ( is_pi( idx ) )
          continue; /* ignore PIs */

        if ( const auto repl = replace_in_node_without_complement_opt( idx, _old, _new ); repl )
        {
          to_substitute.push( *repl );
        }
      }

      /* check outputs */
      replace_in_outputs( _old, _new );

      // reset fan-in of old node
      take_out_node( _old );
    }
  }

  void substitute_node_no_restrash( node const& old_node, signal const& new_signal )
  {
    if ( is_dead( get_node( new_signal ) ) )
    {
      revive_node( get_node( new_signal ) );
    }

    for ( auto idx = 1u; idx < _storage->nodes.size(); ++idx )
    {
      if ( is_ci( idx ) || is_dead( idx ) )
        continue; /* ignore CIs and dead nodes */

      replace_in_node_no_restrash( idx, old_node, new_signal );
    }

    /* check outputs */
    replace_in_outputs( old_node, new_signal );

    /* recursively reset old node */
    if ( old_node != new_signal.index )
    {
      take_out_node( old_node );
    }
  }
#pragma endregion

#pragma region Structural properties
  uint32_t size() const
  {
    return static_cast<uint32_t>( _storage->nodes.size() );
  }

  auto num_cis() const
  {
    return static_cast<uint32_t>( _storage->inputs.size() );
  }

  auto num_cos() const
  {
    return static_cast<uint32_t>( _storage->outputs.size() );
  }

  uint32_t num_pis() const
  {
    return static_cast<uint32_t>( _storage->inputs.size() );
  }

  uint32_t num_pos() const
  {
    return static_cast<uint32_t>( _storage->outputs.size() );
  }

  uint32_t num_gates() const
  {
    return static_cast<uint32_t>( _storage->hash.size() );
  }

  uint32_t fanin_size( node const& n ) const
  {
    if ( is_constant( n ) || is_ci( n ) )
      return 0;
    return 3;
  }

  uint32_t fanout_size( node const& n ) const
  {
    return _storage->nodes[n].data[0].h1 & UINT32_C( 0x7FFFFFFF );
  }

  uint32_t incr_fanout_size( node const& n ) const
  {
    return _storage->nodes[n].data[0].h1++ & UINT32_C( 0x7FFFFFFF );
  }

  uint32_t decr_fanout_size( node const& n ) const
  {
    return --_storage->nodes[n].data[0].h1 & UINT32_C( 0x7FFFFFFF );
  }

  bool is_and( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_or( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_xor( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_maj( node const& n ) const
  {
    return n > 0 && !is_ci( n ) && _storage->nodes[n].children[0].index <= _storage->nodes[n].children[1].index;
  }

  bool is_ite( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_xor3( node const& n ) const
  {
    return n > 0 && !is_ci( n ) && _storage->nodes[n].children[0].index > _storage->nodes[n].children[1].index;
  }

  bool is_nary_and( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_nary_or( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_nary_xor( node const& n ) const
  {
    (void)n;
    return false;
  }
#pragma endregion

#pragma region Functional properties
  kitty::dynamic_truth_table node_function( const node& n ) const
  {
    kitty::dynamic_truth_table _tt( 3 );
    _tt._bits[0] = is_xor3( n ) ? 0x96 : 0xe8;
    return _tt;
  }
#pragma endregion

#pragma region Nodes and signals
  node get_node( signal const& f ) const
  {
    return f.index;
  }

  signal make_signal( node const& n ) const
  {
    return signal( n, 0 );
  }

  bool is_complemented( signal const& f ) const
  {
    return f.complement;
  }

  uint32_t node_to_index( node const& n ) const
  {
    return static_cast<uint32_t>( n );
  }

  node index_to_node( uint32_t index ) const
  {
    return index;
  }

  node ci_at( uint32_t index ) const
  {
    assert( index < _storage->inputs.size() );
    return *( _storage->inputs.begin() + index );
  }

  signal co_at( uint32_t index ) const
  {
    assert( index < _storage->outputs.size() );
    return *( _storage->outputs.begin() + index );
  }

  node pi_at( uint32_t index ) const
  {
    assert( index < _storage->inputs.size() );
    return *( _storage->inputs.begin() + index );
  }

  signal po_at( uint32_t index ) const
  {
    assert( index < _storage->outputs.size() );
    return *( _storage->outputs.begin() + index );
  }

  uint32_t ci_index( node const& n ) const
  {
    assert( _storage->nodes[n].children[0].data == _storage->nodes[n].children[1].data &&
            _storage->nodes[n].children[0].data == _storage->nodes[n].children[2].data );
    return static_cast<uint32_t>( _storage->nodes[n].children[0].data );
  }

  uint32_t co_index( signal const& s ) const
  {
    uint32_t i = -1;
    foreach_co( [&]( const auto& x, auto index ) {
      if ( x == s )
      {
        i = index;
        return false;
      }
      return true;
    } );
    return i;
  }

  uint32_t pi_index( node const& n ) const
  {
    assert( _storage->nodes[n].children[0].data == _storage->nodes[n].children[1].data &&
            _storage->nodes[n].children[0].data == _storage->nodes[n].children[2].data );
    return static_cast<uint32_t>( _storage->nodes[n].children[0].data );
  }

  uint32_t po_index( signal const& s ) const
  {
    uint32_t i = -1;
    foreach_po( [&]( const auto& x, auto index ) {
      if ( x == s )
      {
        i = index;
        return false;
      }
      return true;
    } );
    return i;
  }
#pragma endregion

#pragma region Node and signal iterators
  template<typename Fn>
  void foreach_node( Fn&& fn ) const
  {
    auto r = range<uint64_t>( _storage->nodes.size() );
    detail::foreach_element_if(
        r.begin(), r.end(),
        [this]( auto n ) { return !is_dead( n ); },
        fn );
  }

  template<typename Fn>
  void foreach_ci( Fn&& fn ) const
  {
    detail::foreach_element( _storage->inputs.begin(), _storage->inputs.end(), fn );
  }

  template<typename Fn>
  void foreach_co( Fn&& fn ) const
  {
    detail::foreach_element( _storage->outputs.begin(), _storage->outputs.end(), fn );
  }

  template<typename Fn>
  void foreach_pi( Fn&& fn ) const
  {
    detail::foreach_element( _storage->inputs.begin(), _storage->inputs.end(), fn );
  }

  template<typename Fn>
  void foreach_po( Fn&& fn ) const
  {
    detail::foreach_element( _storage->outputs.begin(), _storage->outputs.end(), fn );
  }

  template<typename Fn>
  void foreach_gate( Fn&& fn ) const
  {
    auto r = range<uint64_t>( 1u, _storage->nodes.size() ); // start from 1 to avoid constant
    detail::foreach_element_if(
        r.begin(), r.end(),
        [this]( auto n ) { return !is_ci( n ) && !is_dead( n ); },
        fn );
  }

  template<typename Fn>
  void foreach_fanin( node const& n, Fn&& fn ) const
  {
    if ( n == 0 || is_ci( n ) )
      return;

    static_assert( detail::is_callable_without_index_v<Fn, signal, bool> ||
                   detail::is_callable_with_index_v<Fn, signal, bool> ||
                   detail::is_callable_without_index_v<Fn, signal, void> ||
                   detail::is_callable_with_index_v<Fn, signal, void> );

    // we don't use foreach_element here to have better performance
    if constexpr ( detail::is_callable_without_index_v<Fn, signal, bool> )
    {
      if ( !fn( signal{ _storage->nodes[n].children[0] } ) )
        return;
      if ( !fn( signal{ _storage->nodes[n].children[1] } ) )
        return;
      fn( signal{ _storage->nodes[n].children[2] } );
    }
    else if constexpr ( detail::is_callable_with_index_v<Fn, signal, bool> )
    {
      if ( !fn( signal{ _storage->nodes[n].children[0] }, 0 ) )
        return;
      if ( !fn( signal{ _storage->nodes[n].children[1] }, 1 ) )
        return;
      fn( signal{ _storage->nodes[n].children[2] }, 2 );
    }
    else if constexpr ( detail::is_callable_without_index_v<Fn, signal, void> )
    {
      fn( signal{ _storage->nodes[n].children[0] } );
      fn( signal{ _storage->nodes[n].children[1] } );
      fn( signal{ _storage->nodes[n].children[2] } );
    }
    else if constexpr ( detail::is_callable_with_index_v<Fn, signal, void> )
    {
      fn( signal{ _storage->nodes[n].children[0] }, 0 );
      fn( signal{ _storage->nodes[n].children[1] }, 1 );
      fn( signal{ _storage->nodes[n].children[2] }, 2 );
    }
  }
  signal get_child0( node const& p ) const
  {
    return _storage->nodes[p].children[0];
  }
  signal get_child1( node const& p ) const
  {
    return _storage->nodes[p].children[1];
  }
  signal get_child2( node const& p ) const
  {
    return _storage->nodes[p].children[2];
  }
#pragma endregion

#pragma region Value simulation
  template<typename Iterator>
  iterates_over_t<Iterator, bool>
  compute( node const& n, Iterator begin, Iterator end ) const
  {
    (void)end;

    assert( n != 0 && !is_ci( n ) );

    auto const& c1 = _storage->nodes[n].children[0];
    auto const& c2 = _storage->nodes[n].children[1];
    auto const& c3 = _storage->nodes[n].children[2];

    auto v1 = *begin++;
    auto v2 = *begin++;
    auto v3 = *begin++;

    if ( is_xor3( n ) )
    {
      return ( ( v1 ^ c1.weight ) != ( v2 ^ c2.weight ) ) != ( v3 ^ c3.weight );
    }
    else
    {
      return ( ( v1 ^ c1.weight ) && ( v2 ^ c2.weight ) ) || ( ( v3 ^ c3.weight ) && ( v1 ^ c1.weight ) ) || ( ( v3 ^ c3.weight ) && ( v2 ^ c2.weight ) );
    }
  }

  template<typename Iterator>
  iterates_over_truth_table_t<Iterator>
  compute( node const& n, Iterator begin, Iterator end ) const
  {
    (void)end;

    assert( n != 0 && !is_ci( n ) );

    auto const& c1 = _storage->nodes[n].children[0];
    auto const& c2 = _storage->nodes[n].children[1];
    auto const& c3 = _storage->nodes[n].children[2];

    auto tt1 = *begin++;
    auto tt2 = *begin++;
    auto tt3 = *begin++;

    if ( is_xor3( n ) )
    {
      return ( c1.weight ? ~tt1 : tt1 ) ^ ( c2.weight ? ~tt2 : tt2 ) ^ ( c3.weight ? ~tt3 : tt3 );
    }
    else
    {
      return kitty::ternary_majority( c1.weight ? ~tt1 : tt1, c2.weight ? ~tt2 : tt2, c3.weight ? ~tt3 : tt3 );
    }
  }

  /*! \brief Re-compute the last block. */
  template<typename Iterator>
  void compute( node const& n, kitty::partial_truth_table& result, Iterator begin, Iterator end ) const
  {
    static_assert( iterates_over_v<Iterator, kitty::partial_truth_table>, "begin and end have to iterate over partial_truth_tables" );

    (void)end;
    assert( n != 0 && !is_ci( n ) );

    auto const& c1 = _storage->nodes[n].children[0];
    auto const& c2 = _storage->nodes[n].children[1];
    auto const& c3 = _storage->nodes[n].children[2];

    auto tt1 = *begin++;
    auto tt2 = *begin++;
    auto tt3 = *begin++;

    assert( tt1.num_bits() > 0 && "truth tables must not be empty" );
    assert( tt1.num_bits() == tt2.num_bits() );
    assert( tt1.num_bits() == tt3.num_bits() );
    assert( tt1.num_bits() >= result.num_bits() );
    assert( result.num_blocks() == tt1.num_blocks() || ( result.num_blocks() == tt1.num_blocks() - 1 && result.num_bits() % 64 == 0 ) );

    result.resize( tt1.num_bits() );
    if ( is_xor3( n ) )
    {
      result._bits.back() =
          ( c1.weight ? ~tt1._bits.back() : tt1._bits.back() ) ^
          ( c2.weight ? ~tt2._bits.back() : tt2._bits.back() ) ^
          ( c3.weight ? ~tt3._bits.back() : tt3._bits.back() );
    }
    else
    {
      result._bits.back() =
          ( ( c1.weight ? ~tt1._bits.back() : tt1._bits.back() ) & ( c2.weight ? ~tt2._bits.back() : tt2._bits.back() ) ) |
          ( ( c1.weight ? ~tt1._bits.back() : tt1._bits.back() ) & ( c3.weight ? ~tt3._bits.back() : tt3._bits.back() ) ) |
          ( ( c2.weight ? ~tt2._bits.back() : tt2._bits.back() ) & ( c3.weight ? ~tt3._bits.back() : tt3._bits.back() ) );
    }
    result.mask_bits();
  }
#pragma endregion

#pragma region Inverters propagations
  void complement_node( node const& n, std::vector<node> const& parents )
  {
    assert( n != 0 && !is_pi( n ) );

    auto& c1 = _storage->nodes[n].children[0];
    auto& c2 = _storage->nodes[n].children[1];
    auto& c3 = _storage->nodes[n].children[2];

    if ( is_maj( n ) )
    {
      c1.weight = !c1.weight;
      c2.weight = !c2.weight;
      c3.weight = !c3.weight;
    }
    else
    {
      if ( c3.weight )
      {
        c3.weight = !c3.weight;
      }
      else if ( c2.weight )
      {
        c2.weight = !c2.weight;
      }
      else
      {
        c1.weight = !c1.weight;
      }
    }

    for ( auto& p : parents )
    {
      for ( auto i = 0; i < 3; i++ )
      {
        auto& c = _storage->nodes[p].children[i];
        if ( c.index == n )
        {
          c.weight = !c.weight;
        }
      }
    }

    /* pos */
    for ( auto& output : _storage->outputs )
    {
      if ( output.index == n )
      {
        output.weight = !output.weight;
      }
    }
  }

  /*
   * f = !(0 xor a xor b) = ( 0 xor !a xor b) = ( 0 xor a xor !b) =
   * ( 1 xor a xor b)
   * */
  void xor_inv_jump( node const& n )
  {
    assert( n != 0 && !is_pi( n ) && is_xor3( n ) );

    auto& c1 = _storage->nodes[n].children[0];
    auto& c2 = _storage->nodes[n].children[1];
    auto& c3 = _storage->nodes[n].children[2];

    auto tmp = static_cast<int>( c1.weight ) + static_cast<int>( c2.weight ) + static_cast<int>( c3.weight );

    if ( tmp == 0 )
    {
      return;
    }
    else if ( tmp == 1 )
    {
      if ( c2.weight )
      {
        c1.weight = !c1.weight;
        c2.weight = !c2.weight;
      }
      if ( c3.weight )
      {
        c1.weight = !c1.weight;
        c3.weight = !c3.weight;
      }
    }
    else if ( tmp == 2 )
    {
      if ( c1.weight )
      {
        c1.weight = !c1.weight;
      }
      if ( c2.weight )
      {
        c2.weight = !c2.weight;
      }
      if ( c3.weight )
      {
        c3.weight = !c3.weight;
      }
    }
    else if ( tmp == 3 )
    {
      c2.weight = !c2.weight;
      c3.weight = !c3.weight;
    }
    else
    {
      assert( false );
    }
  }
#pragma endregion

#pragma region Custom node values
  void clear_values() const
  {
    std::for_each( _storage->nodes.begin(), _storage->nodes.end(), []( auto& n ) { n.data[0].h2 = 0; } );
  }

  auto value( node const& n ) const
  {
    return _storage->nodes[n].data[0].h2;
  }

  void set_value( node const& n, uint32_t v ) const
  {
    _storage->nodes[n].data[0].h2 = v;
  }

  auto incr_value( node const& n ) const
  {
    return _storage->nodes[n].data[0].h2++;
  }

  auto decr_value( node const& n ) const
  {
    return --_storage->nodes[n].data[0].h2;
  }
#pragma endregion

#pragma region Visited flags
  void clear_visited() const
  {
    std::for_each( _storage->nodes.begin(), _storage->nodes.end(), []( auto& n ) { n.data[1].h1 = 0; } );
  }

  auto visited( node const& n ) const
  {
    return _storage->nodes[n].data[1].h1;
  }

  void set_visited( node const& n, uint32_t v ) const
  {
    _storage->nodes[n].data[1].h1 = v;
  }

  uint32_t trav_id() const
  {
    return _storage->trav_id;
  }

  void incr_trav_id() const
  {
    ++_storage->trav_id;
  }
#pragma endregion

#pragma region phase flags
  bool phase( node const& n ) const
  {
    return _storage->nodes[n].data[1].h2 & F_PHASE;
  }

  void phase( node const& n, bool b )
  {
    if ( b )
    {
      _storage->nodes[n].data[1].h2 |= F_PHASE;
    }
    else
    {
      _storage->nodes[n].data[1].h2 &= ~F_PHASE;
    }
  }

#pragma endregion

#pragma region General methods
  auto& events() const
  {
    return *_events;
  }

#pragma endregion

#pragma region Choice nodes

  void init_choices( uint64_t size )
  {
    _equivs.assign( size, XMG_NULL );
    _reprs.clear();
    _reprs.reserve( size );
    for ( uint64_t n = 0; n < size; ++n )
    {
      _reprs.push_back( n );
    }
  }

  bool is_repr( node const& n ) const
  {
    return get_equiv_node( n ) != XMG_NULL && fanout_size( n ) > 0;
  }

  node get_equiv_node( node const& n ) const
  {
    return _equivs[n];
  }

  node get_repr( node const& n ) const
  {
    return _reprs[n];
  }

  bool set_choice( node const& n, node const& r )
  {

    if ( r >= n )
    {
      return false;
    }

    if ( check_tfi( n, r ) )
    {
      return false;
    }

    // set representative
    _reprs[n] = r;

    // add n to the tail of the linked list
    node tmp = r;
    while ( _equivs[tmp] != XMG_NULL )
    {
      tmp = _equivs[tmp];
    }

    assert( _equivs[tmp] == XMG_NULL );
    _equivs[tmp] = n;
    assert( _equivs[n] == XMG_NULL );

    return true;
  }

  void set_equiv( node const& n, node const& equiv ) { _equivs[n] = equiv; }

  bool check_tfi( node const& n, node const& r )
  {

    bool inTFI = false;

    for ( node tmp = r; tmp != XMG_NULL; tmp = _equivs[tmp] )
    {
      set_value( tmp, 1 ); // mark as 1
    }

    // will traverse the TFI of n
    incr_trav_id();
    inTFI = check_tfi_rec( n );

    for ( node tmp = r; tmp != XMG_NULL; tmp = _equivs[tmp] )
    {
      set_value( tmp, 0 ); // unmark
    }

    return inTFI;
  }

  bool check_tfi_rec( node const& n )
  {
    if ( n == XMG_NULL )

      return false;

    if ( is_ci( n ) )

      return false;

    if ( value( n ) == 1 )

      return true;

    if ( visited( n ) == trav_id() )

      return false;

    set_visited( n, trav_id() );

    if ( check_tfi_rec( get_child0( n ).index ) )
      return true;
    if ( check_tfi_rec( get_child1( n ).index ) )
      return true;
    if ( check_tfi_rec( get_child2( n ).index ) )
      return true;

    return check_tfi_rec( _equivs[n] );
  }

#pragma endregion

public:
  std::shared_ptr<xmg_storage> _storage;
  std::shared_ptr<network_events<base_type>> _events;

private:
  std::vector<node> _equivs;
  std::vector<node> _reprs;
};

} // namespace mockturtle

namespace std
{

template<>
struct hash<mockturtle::xmg_network::signal>
{
  uint64_t operator()( mockturtle::xmg_network::signal const& s ) const noexcept
  {
    uint64_t k = s.data;
    k ^= k >> 33;
    k *= 0xff51afd7ed558ccd;
    k ^= k >> 33;
    k *= 0xc4ceb9fe1a85ec53;
    k ^= k >> 33;
    return k;
  }
}; /* hash */

} // namespace std
