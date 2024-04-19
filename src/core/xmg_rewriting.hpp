/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/*!
  \file xmg_algebraic_rewriting.hpp
  \brief xmg algebraric rewriting

  \author Zhufei Chu
*/

#pragma once

#include <iostream>
#include <optional>

#include <mockturtle/mockturtle.hpp>

namespace mockturtle
{

/*! \brief Parameters for xmg_depth_rewriting.
 *
 * The data structure `xmg_depth_rewriting_params` holds configurable
 * parameters with default arguments for `xmg_depth_rewriting`.
 */
struct xmg_depth_rewriting_params
{
  /*! \brief Rewriting strategy. */
  enum strategy_t
  {
    /*! \brief DFS rewriting strategy.
     *
     * Applies depth rewriting once to all output cones whose drivers have
     * maximum levels
     */
    dfs,
    /*! \brief Aggressive rewriting strategy.
     *
     * Applies depth reduction multiple times until the number of nodes, which
     * cannot be rewritten, matches the number of nodes, in the current
     * network; or the new network size is larger than the initial size w.r.t.
     * to an `overhead`.
     */
    aggressive,
    /*! \brief Selective rewriting strategy.
     *
     * Like `aggressive`, but only applies rewriting to nodes on critical paths
     * and without `overhead`.
     */
    selective,
    /*! \brief Selective rewriting strategy.
     *
     * Like `dfs`, rewrite XOR( a, XOR( b, c ) ) as XOR3(a, b, c )
     */
    qca
  } strategy = dfs;

  /*! \brief Overhead factor in aggressive rewriting strategy.
   *
   * When comparing to the initial size in aggressive depth rewriting, also the
   * number of dangling nodes are taken into account.
   */
  float overhead{2.0f};

  /*! \brief Allow area increase while optimizing depth. */
  bool allow_area_increase{true};

  /*! \brief xor3 to two xor2s that compatiable with old cirkit */
  bool apply_xor3_to_xor2{false};
};

namespace detail
{

template<class Ntk>
class xmg_depth_rewriting_impl
{
public:
  xmg_depth_rewriting_impl( Ntk& ntk, xmg_depth_rewriting_params const& ps )
      : ntk( ntk ), ps( ps )
  {
  }

  void run()
  {
    switch ( ps.strategy )
    {
    case xmg_depth_rewriting_params::dfs:
      run_dfs();
      break;
    case xmg_depth_rewriting_params::selective:
      run_selective();
      break;
    case xmg_depth_rewriting_params::aggressive:
      run_aggressive();
      break;
    case xmg_depth_rewriting_params::qca:
      run_qca();
      break;
    }

    if( ps.apply_xor3_to_xor2 )
    {
      run_xor3_flatten();
    }
  }

private:
  void run_qca()
  {
    /* reduce depth */
    ntk.foreach_po( [this]( auto po ) {
      const auto driver = ntk.get_node( po );
      if ( ntk.level( driver ) < ntk.depth() )
        return;
      topo_view topo{ntk, po};
      topo.foreach_node( [this]( auto n ) {
        reduce_depth_ultimate( n );
        return true;
      } );
    } );

    /* rewrite xor2 to xor3 */
    ntk.foreach_po( [this]( auto po ) {
      topo_view topo{ntk, po};
      topo.foreach_node( [this]( auto n ) {
        reduce_depth_xor2_to_xor3( n );
        return true;
      } );
    } );
  }

  void run_dfs()
  {
    ntk.foreach_po( [this]( auto po ) {
      const auto driver = ntk.get_node( po );
      if ( ntk.level( driver ) < ntk.depth() )
        return;
      topo_view topo{ntk, po};
      topo.foreach_node( [this]( auto n ) {
        reduce_depth_ultimate( n );
        return true;
      } );
    } );
  }

  void run_selective()
  {
    uint32_t counter{0};
    while ( true )
    {
      mark_critical_paths();

      topo_view topo{ntk};
      topo.foreach_node( [this, &counter]( auto n ) {
        if ( ntk.fanout_size( n ) == 0 || ntk.value( n ) == 0 )
          return;

        if ( reduce_depth_ultimate( n ) )
        {
          mark_critical_paths();
        }
        else
        {
          ++counter;
        }
      } );

      if ( counter > ntk.size() )
        break;
    }
  }

  void run_aggressive()
  {
    uint32_t counter{0}, init_size{ntk.size()};
    while ( true )
    {
      topo_view topo{ntk};
      topo.foreach_node( [this, &counter]( auto n ) {
        if ( ntk.fanout_size( n ) == 0 )
          return;

        if ( !reduce_depth_ultimate( n ) )
        {
          ++counter;
        }
      } );

      if ( ntk.size() > ps.overhead * init_size )
        break;
      if ( counter > ntk.size() )
        break;
    }
  }

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
  bool reduce_depth_ultimate( node<Ntk> const& n )
  {
    auto b1 = reduce_depth( n );
    auto b2 = reduce_depth_xor_associativity( n );
    auto b3 = reduce_depth_xor_complementary_associativity( n );
    auto b4 = reduce_depth_xor_distribute( n );

    if( !( b1 | b2 | b3 | b4 ) ) { return false; }

    return true;
  }

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

  bool reduce_depth_xor2_to_xor3( node<Ntk> const& n )
  {
    if ( !ntk.is_xor3( n ) )
      return false;

    if ( ntk.level( n ) == 0 )
      return false;

    /* get children of top node, ordered by node level (ascending) */
    const auto ocs = ordered_children( n );

    /* if the first child is not constant, return */
    if( ocs[0].index != 0 )
      return false;

    /* if there are no XOR children, return */
    if ( !ntk.is_xor3( ntk.get_node( ocs[2] ) ) && !ntk.is_xor3( ntk.get_node( ocs[1] ) ) )
      return false;

    if( ntk.is_xor3( ntk.get_node( ocs[2] ) ) )
    {
      /* size optimization, do not allow area increase */
      if( ntk.fanout_size( ntk.get_node( ocs[2] ) ) == 1 )
      {
        /* get children of last child */
        auto ocs2 = ordered_children( ntk.get_node( ocs[2] ) );

        /* propagate inverter if necessary */
        if ( ntk.is_complemented( ocs[2] ) )
        {
          /* complement the constants */
          ocs2[0] = !ocs2[0];
        }

        if( ocs2[0].index == 0 )
        {
          auto opt = ntk.create_xor3( ocs[1], ocs2[1], ocs2[2] );
          ntk.substitute_node( n, opt );
          ntk.update_levels();
        }

        return true;
      }

    }

    if( ntk.is_xor3( ntk.get_node( ocs[1] ) ) )
    {
      /* size optimization, do not allow area increase */
      if( ntk.fanout_size( ntk.get_node( ocs[1] ) ) == 1 )
      {
        /* get children of second child */
        auto ocs2 = ordered_children( ntk.get_node( ocs[1] ) );

        /* propagate inverter if necessary */
        if ( ntk.is_complemented( ocs[1] ) )
        {
          /* complement the constants */
          ocs2[0] = !ocs2[0];
        }

        if( ocs2[0].index == 0 )
        {
          auto opt = ntk.create_xor3( ocs[2], ocs2[1], ocs2[2] );
          ntk.substitute_node( n, opt );
          ntk.update_levels();
        }

        return true;
      }
    }

    return false;
  }

  bool reduce_depth( node<Ntk> const& n )
  {
    if ( !ntk.is_maj( n ) )
      return false;

    if ( ntk.level( n ) == 0 )
      return false;

    /* get children of top node, ordered by node level (ascending) */
    const auto ocs = ordered_children( n );

    if ( !ntk.is_maj( ntk.get_node( ocs[2] ) ) )
      return false;

    /* depth of last child must be (significantly) higher than depth of second child */
    if ( ntk.level( ntk.get_node( ocs[2] ) ) <= ntk.level( ntk.get_node( ocs[1] ) ) + 1 )
      return false;

    /* child must have single fanout, if no area overhead is allowed */
    if ( !ps.allow_area_increase && ntk.fanout_size( ntk.get_node( ocs[2] ) ) != 1 )
      return false;

    /* get children of last child */
    auto ocs2 = ordered_children( ntk.get_node( ocs[2] ) );

    /* depth of last grand-child must be higher than depth of second grand-child */
    if ( ntk.level( ntk.get_node( ocs2[2] ) ) == ntk.level( ntk.get_node( ocs2[1] ) ) )
      return false;

    /* propagate inverter if necessary */
    if ( ntk.is_complemented( ocs[2] ) )
    {
      ocs2[0] = !ocs2[0];
      ocs2[1] = !ocs2[1];
      ocs2[2] = !ocs2[2];
    }

    if ( auto cand = associativity_candidate( ocs[0], ocs[1], ocs2[0], ocs2[1], ocs2[2] ); cand )
    {
      const auto& [x, y, z, u, assoc] = *cand;
      auto opt = ntk.create_maj( z, assoc ? u : x, ntk.create_maj( x, y, u ) );
      ntk.substitute_node( n, opt );
      ntk.update_levels();

      return true;
    }

    /* distributivity */
    if ( ps.allow_area_increase )
    {
      auto opt = ntk.create_maj( ocs2[2],
                                 ntk.create_maj( ocs[0], ocs[1], ocs2[0] ),
                                 ntk.create_maj( ocs[0], ocs[1], ocs2[1] ) );
      ntk.substitute_node( n, opt );
      ntk.update_levels();
      return true;
    }
    return false;
  }

  using candidate_t = std::tuple<signal<Ntk>, signal<Ntk>, signal<Ntk>, signal<Ntk>, bool>;
  std::optional<candidate_t> associativity_candidate( signal<Ntk> const& v, signal<Ntk> const& w, signal<Ntk> const& x, signal<Ntk> const& y, signal<Ntk> const& z ) const
  {
    if ( v.index == x.index )
    {
      return candidate_t{w, y, z, v, v.complement == x.complement};
    }
    if ( v.index == y.index )
    {
      return candidate_t{w, x, z, v, v.complement == y.complement};
    }
    if ( w.index == x.index )
    {
      return candidate_t{v, y, z, w, w.complement == x.complement};
    }
    if ( w.index == y.index )
    {
      return candidate_t{v, x, z, w, w.complement == y.complement};
    }

    return std::nullopt;
  }

  bool reduce_depth_xor_associativity( node<Ntk> const& n )
  {
    if ( !ntk.is_xor3( n ) )
      return false;

    if ( ntk.level( n ) == 0 )
      return false;

    /* get children of top node, ordered by node level (ascending) */
    const auto ocs = ordered_children( n );

    if ( !ntk.is_xor3( ntk.get_node( ocs[2] ) ) )
      return false;

    /* depth of last child must be (significantly) higher than depth of second child */
    if ( ntk.level( ntk.get_node( ocs[2] ) ) <= ntk.level( ntk.get_node( ocs[1] ) ) + 1 )
      return false;

    /* child must have single fanout, if no area overhead is allowed */
    if ( !ps.allow_area_increase && ntk.fanout_size( ntk.get_node( ocs[2] ) ) != 1 )
      return false;

    /* get children of last child */
    auto ocs2 = ordered_children( ntk.get_node( ocs[2] ) );

    /* depth of last grand-child must be higher than depth of second child */
    if ( ntk.level( ntk.get_node( ocs2[2] ) ) + 1 <= ntk.level( ntk.get_node( ocs[1] ) ) )
      return false;

    /* propagate inverter if necessary */
    if ( ntk.is_complemented( ocs[2] ) )
    {
      ocs2[2] = !ocs2[2];
    }

    if( ocs[0] == ocs2[0] && ocs[0].index == 0 && ntk.fanout_size( ntk.get_node( ocs[2] ) ) == 1)
    {
      auto opt = ntk.create_xor3( ocs[0], ocs2[2],
                                  ntk.create_xor3( ocs2[0], ocs2[1], ocs[1] ) );
      ntk.substitute_node( n, opt );
      ntk.update_levels();
      return true;
    }

    return false;
  }

  /* XOR complementary associativity <xy[!yz]> = <xy[xz]> */
  bool reduce_depth_xor_complementary_associativity( node<Ntk> const& n )
  {
    if ( !ntk.is_maj( n ) )
      return false;

    if ( ntk.level( n ) == 0 )
      return false;

    /* get children of top node, ordered by node level (ascending) */
    const auto ocs = ordered_children( n );

    if ( !ntk.is_xor3( ntk.get_node( ocs[2] ) ) )
      return false;

    /* depth of last child must be (significantly) higher than depth of second child */
    /* depth of last child must be higher than depth of second child */
    if ( ntk.level( ntk.get_node( ocs[2] ) ) < ntk.level( ntk.get_node( ocs[1] ) ) + 1 )
      return false;

    /* multiple child fanout is allowable */
    if ( !ps.allow_area_increase && ntk.fanout_size( ntk.get_node( ocs[2] ) ) != 1 )
      return false;

    /* get children of last child */
    auto ocs2 = ordered_children( ntk.get_node( ocs[2] ) );

    /* depth of last grand-child must be higher than depth of second grand-child */
    if ( ntk.level( ntk.get_node( ocs2[2] ) ) == ntk.level( ntk.get_node( ocs2[1] ) ) )
      return false;

    /* propagate inverter if necessary */
    if ( ntk.is_complemented( ocs[2] ) )
    {
      if ( ntk.is_complemented( ocs2[0] ) )
      {
        ocs2[0] = !ocs2[0];
      }
      else if ( ntk.is_complemented( ocs2[1] ) )
      {
        ocs2[1] = !ocs2[1];
      }
      else if ( ntk.is_complemented( ocs2[2] ) )
      {
        ocs2[2] = !ocs2[2];
      }
      else
      {
        ocs2[0] = !ocs2[0];
      }
    }

    if ( auto cand = xor_compl_associativity_candidate( ocs[0], ocs[1], ocs2[0], ocs2[1], ocs2[2] ); cand )
    {
      const auto& [x, y, z, u, assoc] = *cand;
      auto opt = ntk.create_maj( x, u, ntk.create_xor3( assoc ? !x : x, y, z ) );
      ntk.substitute_node( n, opt );
      ntk.update_levels();

      return true;
    }

    return false;
  }

  std::optional<candidate_t> xor_compl_associativity_candidate( signal<Ntk> const& v, signal<Ntk> const& w, signal<Ntk> const& x, signal<Ntk> const& y, signal<Ntk> const& z ) const
  {
    if ( v.index == x.index )
    {
      return candidate_t{w, y, z, v, v.complement == x.complement};
    }
    if ( v.index == y.index )
    {
      return candidate_t{w, x, z, v, v.complement == y.complement};
    }
    if ( v.index == z.index )
    {
      return candidate_t{w, x, y, v, v.complement == z.complement};
    }
    if ( w.index == x.index )
    {
      return candidate_t{v, y, z, w, w.complement == x.complement};
    }
    if ( w.index == y.index )
    {
      return candidate_t{v, x, z, w, w.complement == y.complement};
    }
    if ( w.index == z.index )
    {
      return candidate_t{v, x, y, w, w.complement == z.complement};
    }

    return std::nullopt;
  }

  /* XOR distribute over MAJ */
  bool reduce_depth_xor_distribute( node<Ntk> const& n )
  {
    if ( !ntk.is_maj( n ) )
      return false;

    if ( ntk.level( n ) == 0 )
      return false;

    /* get children of top node, ordered by node level (ascending) */
    const auto ocs = ordered_children( n );

    std::vector<unsigned> xor_index;
    for( auto i = 0; i <= 2; i++ )
    {
      if( ntk.is_xor3( ntk.get_node( ocs[i] ) ) )
      {
        xor_index.push_back( i );
      }
    }

    /* consider at least two xor child nodes */
    if ( xor_index.size() < 2 )
      return false;

    /* depth of last child must be (significantly) higher than depth of second child */
    if ( ntk.level( ntk.get_node( ocs[2] ) ) <= ntk.level( ntk.get_node( ocs[1] ) ) + 1 )
        return false;

    /* child must have single fanout, if no area overhead is allowed */
    if( !ps.allow_area_increase )
    {
      for( const auto& index : xor_index )
      {
        if( ntk.fanout_size( ntk.get_node( ocs[index] ) ) != 1 )
        {
          return false;
        }
      }
    }

    /* propagate inverter if necessary */
    for( const auto& index : xor_index )
    {
      if( ntk.is_complemented( ntk.get_node( ocs[index] ) ) )
      {
        auto ocs_next = ordered_children( ntk.get_node( ocs[index] ) );
        ocs_next[2] = !ocs_next[2];
      }
    }

    if( xor_index.size() == 3u )
    {
      auto ocs1 = ordered_children( ntk.get_node( ocs[0] ) );
      auto ocs2 = ordered_children( ntk.get_node( ocs[1] ) );
      auto ocs3 = ordered_children( ntk.get_node( ocs[2] ) );

      if ( auto cand = find_common_grand_child_three( ocs1, ocs2, ocs3 ); cand )
      {
        auto r = *cand;
        auto opt = ntk.create_xor3( r.a, r.b, ntk.create_maj( r.x, r.y, r.z ) );
        ntk.substitute_node( n, opt );
        ntk.update_levels();

        return true;
      }

      return false;
    }
    else if( xor_index.size() == 2u )
    {
      auto ocs2 = ordered_children( ntk.get_node( ocs[1] ) );
      auto ocs3 = ordered_children( ntk.get_node( ocs[2] ) );

      if ( auto cand = find_common_grand_child_two( ocs[0], ocs2, ocs3 ); cand )
      {
        auto r = *cand;
        auto opt = ntk.create_xor3( r.a, r.b, ntk.create_maj( r.x, r.y, r.z ) );
        ntk.substitute_node( n, opt );
        ntk.update_levels();

        return true;
      }

      return false;
    }
    else
    {
      return false;
    }

    return false;
  }

  bool is_signal_equal( const signal<Ntk>& a, const signal<Ntk>& b )
  {
    return ( a == b ?  true : false );
  }

  bool is_three_signal_equal( const signal<Ntk>& a, const signal<Ntk>& b, const signal<Ntk>& c )
  {
    return ( a == b ?  ( b == c ? true : false ): false );
  }

  using children_t = std::array< signal<Ntk>, 3 >;
  std::bitset<9> get_pair_pattern( const children_t& c1, const children_t& c2 )
  {
    std::bitset<9> equals;

    equals.set( 0u, is_signal_equal( c1[0], c2[0] ) );
    equals.set( 1u, is_signal_equal( c1[0], c2[1] ) );
    equals.set( 2u, is_signal_equal( c1[0], c2[2] ) );
    equals.set( 3u, is_signal_equal( c1[1], c2[0] ) );
    equals.set( 4u, is_signal_equal( c1[1], c2[1] ) );
    equals.set( 5u, is_signal_equal( c1[1], c2[2] ) );
    equals.set( 6u, is_signal_equal( c1[2], c2[0] ) );
    equals.set( 7u, is_signal_equal( c1[2], c2[1] ) );
    equals.set( 8u, is_signal_equal( c1[2], c2[2] ) );

    return equals;
  }

  /* struct for representing grand chiledren */
  struct grand_children_pair_t
  {
    signal<Ntk> x;
    signal<Ntk> y;
    signal<Ntk> z;
    signal<Ntk> a; //common grand child a
    signal<Ntk> b; //common grand child b
  };

  std::vector<size_t> equal_idx( const std::bitset<9> pattern )
  {
    std::vector<size_t> r;
    for( size_t idx = 0; idx < pattern.size(); idx++ )
    {
      if( pattern.test( idx ) )
      {
        r.push_back( idx );
      }
    }

    return r;
  }

  /* find common grand children in three children */
  std::optional<grand_children_pair_t> find_common_grand_child_three( const children_t& c1, const children_t& c2, const children_t& c3 )
  {
    grand_children_pair_t r;

    auto p1 = get_pair_pattern( c1, c2 );
    auto p2 = get_pair_pattern( c1, c3 );
    auto p3 = get_pair_pattern( c2, c3 );

    auto s1 = equal_idx( p1 );
    auto s2 = equal_idx( p2 );
    auto s3 = equal_idx( p3 );

    if( s1.size() != 2u || s2.size() != 2u || s3.size() != 2u )
      return std::nullopt;

    auto c11 = c1[ s1[0u] / 3 ];
    auto c12 = c1[ s1[1u] / 3 ];
    auto c21 = c2[ s3[0u] / 3 ];
    auto c22 = c2[ s3[1u] / 3 ];
    auto c31 = c3[ s2[0u] % 3 ];
    auto c32 = c3[ s2[1u] % 3 ];

    std::bitset<8> equals;

    equals.set( 0u, is_three_signal_equal( c11, c21, c31 ) );
    equals.set( 1u, is_three_signal_equal( c11, c21, c32 ) );
    equals.set( 2u, is_three_signal_equal( c11, c22, c31 ) );
    equals.set( 3u, is_three_signal_equal( c11, c22, c32 ) );
    equals.set( 4u, is_three_signal_equal( c12, c21, c31 ) );
    equals.set( 5u, is_three_signal_equal( c12, c21, c32 ) );
    equals.set( 6u, is_three_signal_equal( c12, c22, c31 ) );
    equals.set( 7u, is_three_signal_equal( c12, c22, c32 ) );

    std::vector<signal<Ntk>> common;
    for( size_t idx = 0; idx < 8; idx++ )
    {
      if( equals.test( idx ) )
      {
        if( idx < 4 )
        {
          common.push_back( c11 );
        }
        else
        {
          common.push_back( c12 );
        }
      }
    }

    if( common.size() != 2 )
      return std::nullopt;

    r.a = common[0u];
    r.b = common[1u];

    for( auto i = 0u; i < 3u; i++ )
    {
      if( c1[i] != common[0u] && c1[i] != common[1u] )
      {
        r.x = c1[i];
      }

      if( c2[i] != common[0u] && c2[i] != common[1u] )
      {
        r.y = c2[i];
      }

      if( c3[i] != common[0u] && c3[i] != common[1u] )
      {
        r.z = c3[i];
      }
    }

    return r;
  }

  /* find common grand children in two children */
  std::optional<grand_children_pair_t> find_common_grand_child_two( const signal<Ntk>& v, const children_t& c1, const children_t& c2 )
  {
    grand_children_pair_t r;
    auto p = get_pair_pattern( c1, c2 );

    std::vector<signal<Ntk>> common;

    for( size_t idx = 0; idx < p.size(); idx++ )
    {
      if( p.test(idx) )
      {
        assert( c1[idx / 3] == c2[ idx % 3 ] );
        common.push_back( c1[ idx / 3] );
      }
    }

    //std::cout << "common size: " << common.size() << std::endl;
    //std::cout << "common 0 index: " << common[0].index << std::endl;
    //std::cout << "common 1 index: " << common[1].index << std::endl;
    //std::cout << "v index: " << v.index << std::endl;
    /* for xor children, require at least two common children */
    if( common.size() != 2u )
      return std::nullopt;

    /* the signal v index must equal one of the common signal index, and one common signal is
     * constant 1 or 0 */
    if( v.index != common[0u].index && v.index != common[1u].index )
      return std::nullopt;

    if( common[0u].index != 0 && common[1u].index != 0 )
      return std::nullopt;

    /* save the records */
    r.a = common[0u];
    r.b = common[1u];

    for( auto i = 0u; i < 3u; i++ )
    {
      if( c1[i] != common[0u] && c1[i] != common[1u] )
      {
        r.x = c1[i];
      }

      if( c2[i] != common[0u] && c2[i] != common[1u] )
      {
        r.y = c2[i];
      }
    }

    if( v == !common[0u] || v == !common[1u] )
    {
      r.z = ntk.get_constant( true );
    }

    if( v == common[0u] || v == common[1u] )
    {
      r.z = ntk.get_constant( false );
    }

    return r;
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

  void mark_critical_path( node<Ntk> const& n )
  {
    if ( ntk.is_pi( n ) || ntk.is_constant( n ) || ntk.value( n ) )
      return;

    const auto level = ntk.level( n );
    ntk.set_value( n, 1 );
    ntk.foreach_fanin( n, [this, level]( auto const& f ) {
      if ( ntk.level( ntk.get_node( f ) ) == level - 1 )
      {
        mark_critical_path( ntk.get_node( f ) );
      }
    } );
  }

  void mark_critical_paths()
  {
    ntk.clear_values();
    ntk.foreach_po( [this]( auto const& f ) {
      if ( ntk.level( ntk.get_node( f ) ) == ntk.depth() )
      {
        mark_critical_path( ntk.get_node( f ) );
      }
    } );
  }

private:
  Ntk& ntk;
  xmg_depth_rewriting_params const& ps;
};

} // namespace detail

/*! \brief XMG algebraic depth rewriting.
 *
 * This algorithm tries to rewrite a network with majority gates for depth
 * optimization using the associativity and distributivity rule in
 * majority-of-3 logic.  It can be applied to networks other than XMGs, but
 * only considers pairs of nodes which both implement the majority-of-3
 * function and the XOR function.
 *
 * **Required network functions:**
 * - `get_node`
 * - `level`
 * - `update_levels`
 * - `create_maj`
 * - `substitute_node`
 * - `foreach_node`
 * - `foreach_po`
 * - `foreach_fanin`
 * - `is_maj`
 * - `clear_values`
 * - `set_value`
 * - `value`
 * - `fanout_size`
 *
   \verbatim embed:rst

  .. note::

      The implementation of this algorithm was heavily inspired by an
      implementation from Luca Amarù.
   \endverbatim
 */
template<class Ntk>
void xmg_depth_rewriting( Ntk& ntk, xmg_depth_rewriting_params const& ps = {} )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_level_v<Ntk>, "Ntk does not implement the level method" );
  static_assert( has_create_maj_v<Ntk>, "Ntk does not implement the create_maj method" );
  static_assert( has_create_xor_v<Ntk>, "Ntk does not implement the create_maj method" );
  static_assert( has_substitute_node_v<Ntk>, "Ntk does not implement the substitute_node method" );
  static_assert( has_update_levels_v<Ntk>, "Ntk does not implement the update_levels method" );
  static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
  static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_is_maj_v<Ntk>, "Ntk does not implement the is_maj method" );
  static_assert( has_is_xor3_v<Ntk>, "Ntk does not implement the is_maj method" );
  static_assert( has_clear_values_v<Ntk>, "Ntk does not implement the clear_values method" );
  static_assert( has_set_value_v<Ntk>, "Ntk does not implement the set_value method" );
  static_assert( has_value_v<Ntk>, "Ntk does not implement the value method" );
  static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );

  detail::xmg_depth_rewriting_impl<Ntk> p( ntk, ps );
  p.run();
}

} /* namespace mockturtle */
