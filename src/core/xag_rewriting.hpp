/* also: Advanced Logic Synthesis and Optimization tool*/
/* Copyright (C) 2020- Ningbo University, Ningbo, China */

/*!
  \file xag_algebraic_rewriting.hpp
  \brief xag algebraric rewriting
  \author huiming tian
*/

#pragma once

#include <iostream>
#include <optional>

#include <mockturtle/mockturtle.hpp>

namespace mockturtle
{

/*! \brief Parameters for xag_depth_rewriting.
 *
 * The data structure `xag_depth_rewriting_params` holds configurable
 * parameters with default arguments for `xag_depth_rewriting`.
 */
struct xag_depth_rewriting_params
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
  } strategy = dfs;

  /*! \brief Overhead factor in aggressive rewriting strategy.
   *
   * When comparing to the initial size in aggressive depth rewriting, also the
   * number of dangling nodes are taken into account.
   */
  float overhead{2.0f};

  bool verbose{false};
};

namespace detail
{

template<class Ntk>
class xag_depth_rewriting_impl
{
public:
  xag_depth_rewriting_impl( Ntk& ntk, xag_depth_rewriting_params const& ps )
      : ntk( ntk ), ps( ps )
  {
  }

  void run()
  {
    switch ( ps.strategy )
    {
      case xag_depth_rewriting_params::dfs:
        run_dfs();
        break;
      
      case xag_depth_rewriting_params::selective:
        run_selective();
        break;
      
      case xag_depth_rewriting_params::aggressive:
        run_aggressive();
        break;
    }
  }
/*****************************************Optimization strategies Begin********************************************************************/
private:
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
/**************************************************************************************************************/
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
/**************************************************************************************************************/
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

/*****************************************Optimization strategies End**********************************************************************/

/*****************************************Optimization Functions Begin*********************************************************************/
private:
  bool reduce_depth_ultimate( node<Ntk> const& n )
  {
    auto b1 = xor_inv_transfer( n );
    auto b2 = xor_inv_remove( n );
    auto b3 = xor_and_transfer( n );
    auto b4 = reduce_depth_and_common_fanin( n );
    auto b5 = reduce_depth_use_constant_fanin( n );
    auto b6 = reduce_depth_use_xor_or_convert( n );
    auto b7 = reduce_depth_use_xor_and_convert( n );
   // auto b8 = reduce_depth_use_xor_or_convert2( n );   
 
    if(!( b1 | b2 | b3 | b4 | b5 | b6 | b7  )){ return false; }

    return true;
  }
/**************************************************************************************************************/
  /* [!a!b]=[ab], ![!a!b] = ![ab]  */
  /* Remove the INV of the two children of an node, thie node's type must be XOR.  */	
  bool xor_inv_remove( node<Ntk> const& n )
  {
    if ( !ntk.is_xor( n ) )
      return false;
    
    if ( ntk.level( n ) == 0 )
      return false;
    
    /* get children of top node, ordered by node level (ascending) */
    auto ocs = ordered_children( n );

    /* The two children must both have INV. */
    if ( ntk.is_complemented(ocs[0]) && ntk.is_complemented(ocs[1]) )
    {   
      ocs[0] = !ocs[0];
      ocs[1] = !ocs[1];
            
      auto opt = ntk.create_xor( ocs[0], ocs[1] );
      ntk.substitute_node( n, opt );
      ntk.update_levels();
    }

    return true;
  }
/**************************************************************************************************************/
  /* ![!ab] = ![a!b] = [ab], [!ab] = [a!b] = ![ab]  */
  /* Transfer the INV of the input to the fanout.  */
  /* can be used for subsquent optimization such as (a[!ab]) = (a![ab]) = (ab) */
  bool xor_inv_transfer( node<Ntk> const& n )
  {
    if ( !ntk.is_xor( n ) )
      return false;
    
    if ( ntk.level( n ) == 0 )
      return false;

    /* get children of top node, ordered by node level (ascending). */
    auto ocs = ordered_children( n );
   
    if(!ntk.is_complemented(ocs[0]) && !ntk.is_complemented(ocs[1]))
      return false;

    if( ( ntk.is_complemented(ocs[0]) ^ ntk.is_complemented(ocs[1] ) )  == 1)
    {
      if(ntk.is_complemented(ocs[0]))
      {
        ocs[0] = !ocs[0];
      }
      else
      {
        ocs[1] = !ocs[1];
      }
    }
    
    auto opt = ntk.create_xor(ocs[0], ocs[1]) ^ true;
    ntk.substitute_node( n, opt );
    ntk.update_levels();
   
    return true;
  }
/**************************************************************************************************************/
  bool reduce_depth_use_constant_fanin( node<Ntk> const& n )
  { 
    if ( ntk.level( n ) == 0 )
      return false;

    /* get children of top node, ordered by node level (ascending). */
    auto ocs = ordered_children( n );
    
    if(ntk.fanout_size(ntk.get_node(ocs[0])) ==1 && ntk.fanout_size(ntk.get_node(ocs[1])) == 1)
    {
      if(ntk.is_xor(n))
      {
        if(ocs[0].index == 0 || ocs[1].index == 0)
        {
          if(ocs[0].index == 0 )
          {
            auto opt = ntk.is_complemented(ocs[0])? !ocs[1] : ocs[1];
            ntk.substitute_node(n,opt);
            ntk.update_levels();
          }

          if (ocs[1].index == 0 )
          {
            auto opt = ntk.is_complemented(ocs[1])? !ocs[0] : ocs[0];
            ntk.substitute_node(n,opt);
            ntk.update_levels();
          }
        }

        if(ocs[0].index == 1 || ocs[1].index == 1 )
        {
          if(ocs[0].index == 1 )
          {
            auto opt = ntk.is_complemented(ocs[0])? ocs[1] : !ocs[1];
            ntk.substitute_node(n, opt);
            ntk.update_levels();	  
          }

          if(ocs[1].index == 1 )
          {
            auto opt = ntk.is_complemented(ocs[1])? ocs[0] : !ocs[0];
            ntk.substitute_node(n, opt);
            ntk.update_levels(); 
          }
        }
      }

      if(ntk.is_and(n))
      {
        if(ocs[0].index == 1 || ocs[1].index == 1)
        {
          if(ocs[0].index == 1 && !ntk.is_complemented(ocs[0]) )
          {
            auto opt = ocs[1];
            ntk.substitute_node(n,opt);
            ntk.update_levels();
          }
          else if(ocs[1].index == 1 && ntk.is_complemented(ocs[1]))
          {
            auto opt = ocs[0];
            ntk.substitute_node(n,opt);
            ntk.update_levels();
          }	  
        }
      }
    }
    /* XOR: [a0] = a, [a1] = !a.*/
    /* AND: (a1) = a, (a0) = 0.   */
    return true;
  }
  /**************************************************************************************************************/
  /* Use the Boolean configuration between AND and XOR.
   * There are main two expression for reference.
   * (a[!ab]) = (ab),  (!a[ab]) = (!ab). */
  bool reduce_depth_use_xor_and_convert( node<Ntk> const& n )
  {
    if ( !ntk.is_and( n ) )
      return false;

    if ( ntk.level( n ) == 0 )
      return false;

    /* get children of top node, ordered by node level (ascending). */
    const auto ocs = ordered_children( n );

    /* depth of last child must be (significantly) higher than depth of second child */
    if ( ntk.level( ntk.get_node( ocs[1] ) ) <= ntk.level( ntk.get_node( ocs[0] ) ) + 1 )
      return false;

    if( !ntk.is_xor( ntk.get_node( ocs[1] ) ) )
      return false;

    const auto ocs2 = ordered_children( ntk.get_node( ocs[1] ) );   
    /* Judgement */
    if( ocs[0].index == ocs2[0].index && (ocs[0].complement ^ ocs2[0].complement) == 1)
    {   
      if(ntk.fanout_size(ntk.get_node(ocs[1])) == 1)
      {
        auto opt = ntk.create_and( ocs[0], ocs2[1] );
        ntk.substitute_node( n, opt );
        ntk.update_levels();
      }
      return true;
    }
    return true;
  }
  /**************************************************************************************************************/
  /* Refer to follow two expressions:
   * (a(ab)) = (ab), (!a(!a!b)) = (!a!b).
   */
  bool reduce_depth_and_common_fanin( node<Ntk> const& n )
  {
    if ( !ntk.is_and( n ) )
      return false;
    
    if ( ntk.level( n ) == 0 )
      return false;

    /* get children of top node, ordered by node level (ascending). */
    const auto ocs = ordered_children( n );

    /* depth of last child must be (significantly) higher than depth of second child */
    if ( ntk.level( ntk.get_node( ocs[1] ) ) <= ntk.level( ntk.get_node( ocs[0] ) ) + 1 )
      return false;
    
    if( !ntk.is_and( ntk.get_node( ocs[1] ) ) )
      return false;
    
    const auto ocs2 = ordered_children( ntk.get_node( ocs[1] ) );

    if(ocs[0] == ocs2[0]  && ntk.fanout_size(ntk.get_node(ocs[1])) == 1 && ntk.fanout_size(ntk.get_node(ocs[1])) == 1)
    {
      auto opt = ocs[1];
      ntk.substitute_node(n,opt);
      ntk.update_levels();
    }
    /*if( auto cand = find_common_grand_child_two( ocs, ocs2 ); cand)
    { 
      auto r = *cand;
      
      auto opt = ntk.create_and( r.y, r.a );
      ntk.substitute_node( n, opt );
      ntk.update_levels();
      return true;
    }*/

    return true;
  }
/**************************************************************************************************************/
 /*  (a[ab]) = [a(ab)] */
  bool xor_and_transfer( node<Ntk> const& n )
  {
    if ( ntk.level( n ) == 0 )
      return false;

    /* get children of top node, ordered by node level (ascending). */
    const auto ocs = ordered_children( n );

    /* depth of last child must be (significantly) higher than depth of second child */
    if ( ntk.level( ntk.get_node( ocs[1] ) ) <= ntk.level( ntk.get_node( ocs[0] ) ) + 1 )
      return false;

    if ( !ntk.is_and( n ) && !ntk.is_xor( ntk.get_node( ocs[1] ) ) )
      return false;

    const auto ocs2 = ordered_children( ntk.get_node( ocs[1] ) );
    
    /* refer to the expressions above. */
    if ( !ntk.is_complemented(ocs[0]) && !ntk.is_complemented(ocs[1]) && !ntk.is_complemented(ocs2[0]) && !ntk.is_complemented(ocs2[1]) )
      return false;
    
    if(ntk.fanout_size(ntk.get_node(ocs[1])) == 1)
    {
      if(ocs[0] == ocs2[0])
      {
        auto opt = ntk.create_xor(ocs[0], ntk.create_and(ocs2[0], ocs2[1]));
        ntk.substitute_node(n, opt);
        ntk.update_levels();
      }
    /*if ( auto cand = find_common_grand_child_two( ocs, ocs2 ); cand )
    {
      auto r = *cand;
      auto opt = ntk.create_xor( r.a, ntk.create_and( r.a, r.y ) );
      ntk.substitute_node( n, opt );
      ntk.update_levels();
      return true;
    }*/
    }
    return true; 
  }
/**************************************************************************************************************/
  /* ［a!(!ａ!ｂ)］＝　(!ab)　［!a!(!a!b)]= !(!ab)*/
  bool reduce_depth_use_xor_or_convert( node<Ntk> const& n )
  {    
    if ( ntk.level( n ) == 0 )
      return false;

    /* get children of top node, ordered by node level (ascending). */
    auto ocs = ordered_children( n );

    /* depth of last child must be (significantly) higher than depth of second child */
    if ( ntk.level( ntk.get_node( ocs[1] ) ) <= ntk.level( ntk.get_node( ocs[0] ) ) + 1 )
      return false;

    if ( !ntk.is_xor( n ) && !ntk.is_and( ntk.get_node( ocs[1] ) ) )
      return false;

    auto ocs2 = ordered_children(ntk.get_node(ocs[1]));    

    if (ocs[0].index == ocs2[0].index && ntk.is_complemented(ocs[1]) && ntk.is_complemented(ocs2[0]) && ntk.is_complemented(ocs2[1]) && ntk.fanout_size(ntk.get_node(ocs[1])) == 1 )
    {
      if(ntk.is_complemented(ocs[0]))
      {
        ocs2[1] = !ocs2[1];
        auto opt =  ntk.create_nand(ocs2[0], ocs2[1]);
        ntk.substitute_node(n, opt);
        ntk.update_levels();
      }
      else
      {
        ocs2[1] = !ocs2[1];
        auto opt = ntk.create_and(ocs2[0], ocs2[1]);
        ntk.substitute_node(n, opt);
        ntk.update_levels();
      }
    }
    return true;
  }
/**************************************************************************************************************/
  /*{a!b} = {a[!ab]} = !(!a![!ab]))；  !{a!b} = (!ab)*/
  bool reduce_depth_use_xor_or_convert2( node<Ntk> const& n )
  {
    if ( ntk.level( n ) == 0 )
      return false;

    /* get children of top node, ordered by node level (ascending). */
    const auto ocs = ordered_children( n );

    /* depth of last child must be (significantly) higher than depth of second child */
    if ( ntk.level( ntk.get_node( ocs[1] ) ) <= ntk.level( ntk.get_node( ocs[0] ) ) + 1 )
      return false;

    if ( !ntk.is_and( n ) && !ntk.is_xor( ntk.get_node( ocs[1] ) ) )
      return false;
    
    auto ocs2 = ordered_children( ntk.get_node( ocs[1] ) );

    if(ntk.is_complemented(ocs[0]) && ntk.is_complemented(ocs[1]) && ocs[0] == ocs2[0] )
    {
      if(ntk.fanout_size(ntk.get_node(ocs[1])) == 1 && !ntk.is_complemented(ocs2[1]))
      {
         auto opt = ntk.create_or(ocs2[0], ocs2[1]);
         ntk.substitute_node(n, opt);
         ntk.update_levels();
      }
      return true;
    }
    return true;
  }
/*****************************************Optimization Functions End***********************************************************************/

/*****************************************Other Necessary Parts Begin**********************************************************************/

  bool is_signal_equal( const signal<Ntk>& a, const signal<Ntk>& b )
  {
    return ( a == b ?  true : false );
  }
/**************************************************************************************************************/
  /* used for the subsquent judgement of the common child-node. */
  using children_t = std::array< signal<Ntk>, 2 >; 
  std::bitset<4> get_pair_pattern( const children_t& c1, const children_t& c2 )
  {
    std::bitset<4> equals;

    equals.set( 0u, is_signal_equal( c1[0], c2[0] ) );
    equals.set( 1u, is_signal_equal( c1[0], c2[1] ) );
    equals.set( 2u, is_signal_equal( c1[1], c2[0] ) );
    equals.set( 3u, is_signal_equal( c1[1], c2[1] ) );

    return equals;
  }
/**************************************************************************************************************/
  /* used to judge which two nodes are common. */
  std::vector<size_t> equal_idx( const std::bitset<4> pattern )
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
/**************************************************************************************************************/	
  struct grand_children_pair_t
  {
    signal<Ntk> x;
    signal<Ntk> y;
    signal<Ntk> a; //common grand child a
  };
/**************************************************************************************************************/
  std::optional<grand_children_pair_t> find_common_grand_child_two( const children_t& c1, const children_t& c2 )
  {
    grand_children_pair_t r;
    auto p = get_pair_pattern( c1, c2 );

    std::vector<signal<Ntk>> common;

    for( size_t idx = 0; idx < p.size(); idx++ )
    {
      if( p.test(idx) )
      {
        assert( c1[idx / 2] == c2[ idx % 2 ] );
        common.push_back( c1[ idx / 2] );
      }
    }

    /* for xor children, require at least one common children */
    if( common.size() != 1u )
      return std::nullopt;

    /* save the records */
    r.a = common[0u];

    for( auto i = 0u; i < 2u; i++ )
    {
      if( c1[i] != common[0u] && c1[i] != common[0u] )
      {
        r.x = c1[i];
      }

      if( c2[i] != common[0u] && c2[i] != common[0u] )
      {
        r.y = c2[i];
      }
    }

    return r;
  }
/**************************************************************************************************************/

  std::array<signal<Ntk>, 2> ordered_children( node<Ntk> const& n ) const
  {
    std::array<signal<Ntk>, 2> children;
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
/**************************************************************************************************************/
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
/**************************************************************************************************************/
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
/*****************************************Other Necessary Parts End************************************************************************/
private:
  Ntk& ntk;
  xag_depth_rewriting_params const& ps;
};

} // namespace detail

/*! \brief xag algebraic depth rewriting.
 *
 * This algorithm tries to rewrite a network with majority gates for depth
 * optimization using the associativity and distributivity rule in
 * majority-of-3 logic.  It can be applied to networks other than xags, but
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

   \endverbatim
 */
template<class Ntk>
void xag_depth_rewriting( Ntk& ntk, xag_depth_rewriting_params const& ps = {} )
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
  static_assert( has_is_and_v<Ntk>, "Ntk does not implement the is_and method" );
  static_assert( has_is_xor_v<Ntk>, "Ntk does not implement the is_xor method" );
  static_assert( has_clear_values_v<Ntk>, "Ntk does not implement the clear_values method" );
  static_assert( has_set_value_v<Ntk>, "Ntk does not implement the set_value method" );
  static_assert( has_value_v<Ntk>, "Ntk does not implement the value method" );
  static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );

  detail::xag_depth_rewriting_impl<Ntk> p( ntk, ps );
  p.run();
}

} /* namespace mockturtle */
