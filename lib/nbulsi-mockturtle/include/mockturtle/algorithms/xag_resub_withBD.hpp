/* mockturtle: C++ logic network library
 * Copyright (C) 2018-2021  EPFL
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
  \file xag_resub_withBD.hpp
  \brief Resubstitution with free xor (works for XAGs, XOR gates are considered for free)

  \author Sen Liu
  \author Zhufei Chu

*/

#pragma once
#include "dont_cares.hpp"
#include <kitty/hash.hpp>
#include <kitty/operations.hpp>
#include <mockturtle/algorithms/resubstitution.hpp>
#include <mockturtle/networks/xag.hpp>
#include <unordered_map>

namespace mockturtle
{

struct xag_resub_stats_bd
{
  /*! \brief Accumulated runtime for const-resub */
  stopwatch<>::duration time_resubC{ 0 };

  /*! \brief Accumulated runtime for zero-resub */
  stopwatch<>::duration time_resub0{ 0 };

  /*! \brief Accumulated runtime for collect_xortt */
  stopwatch<>::duration time_collect_xortt_unate_divisors{ 0 };

  /*! \brief Accumulated runtime for collect_xortt */
  stopwatch<>::duration time_collect_xortt_binate_divisors{ 0 };

  /*! \brief Accumulated runtime for collect_xortt */
  stopwatch<>::duration time_collect_andtt_binate_divisors{ 0 };

  /*! \brief Accumulated runtime for collect_xortt */
  stopwatch<>::duration time_collect_ortt_binate_divisors{ 0 };

  /*! \brief Accumulated runtime for one-xor-resub */
  stopwatch<>::duration time_resub_xor{ 0 };

  /*! \brief Accumulated runtime for two-xor-resub. */
  stopwatch<>::duration time_resub2_xor{ 0 };

  /*! \brief Accumulated runtime for three-xor-resub. */
  stopwatch<>::duration time_resub3_xor{ 0 };

  /*! \brief Accumulated runtime for collecting unate divisors. */
  stopwatch<>::duration time_collect_unate_divisors{ 0 };

  /*! \brief Accumulated runtime for one-resub */
  stopwatch<>::duration time_resub_and{ 0 };

  /*! \brief Accumulated runtime for xa-resub */
  stopwatch<>::duration time_resub_xor_and{ 0 };

  /*! \brief Accumulated runtime for xa-resub */
  stopwatch<>::duration time_resub_xor_and_xor{ 0 };

  /*! \brief Accumulated runtime for collecting unate divisors. */
  stopwatch<>::duration time_collect_binate_divisors{ 0 };

  /*! \brief Accumulated runtime for aa-resub */
  stopwatch<>::duration time_resub_div_aa_oo{ 0 };

  /*! \brief Accumulated runtime for and-resub. */
  stopwatch<>::duration time_resub_div_ao_oa{ 0 };

  /*! \brief Accumulated runtime for and-resub. */
  stopwatch<>::duration time_resub_div_x2{ 0 };

  /*! \brief Accumulated runtime for and-resub. */
  stopwatch<>::duration time_resub_div_aaa{ 0 };

  /*! \brief Number of accepted constant resubsitutions */
  uint32_t num_const_accepts{ 0 };

  /*! \brief Number of accepted zero resubsitutions */
  uint32_t num_div0_accepts{ 0 };

  /*! \brief Number of accepted one resubsitutions */
  uint64_t num_div_xor_accepts{ 0 };

  /*! \brief Number of accepted one resubsitutions */
  uint64_t num_div_xor_first_accepts{ 0 };

  /*! \brief Number of accepted one resubsitutions */
  uint64_t num_div_xor_second_accepts{ 0 };

  /*! \brief Number of accepted two resubsitutions */
  uint64_t num_div_2xor_accepts{ 0 };

  /*! \brief Number of accepted three resubsitutions */
  uint64_t num_div_3xor_accepts{ 0 };

  /*! \brief Number of accepted one resub situtions for AND */
  uint64_t num_div_and_accepts{ 0 };

  /*! \brief Number of accepted one resubsitutions for XOR_AND */
  uint64_t num_div_xor_and_accepts{ 0 };

  /*! \brief Number of accepted one resubsitutions for XOR_AND */
  uint64_t num_div_xax_accepts{ 0 };

  /*! \brief Number of accepted two resubsitutions for XOR_2AND */
  uint64_t num_resub_aa_oo_accepts{ 0 };

  /*! \brief Number of accepted 3 resubsitutions using triples of unate divisors */
  uint64_t num_resub_ao_oa_accepts{ 0 };

  /*! \brief Number of accepted one resubsitutions for AND */
  uint64_t num_resub_xor2_accepts{ 0 };

  /*! \brief Number of accepted one resubsitutions for AND */
  uint64_t num_resub_aaa_accepts{ 0 };

  void report() const
  {
    std::cout << "[i] kernel: xag_resub_functor_bd\n";
    std::cout << fmt::format( "[i]     constant-resub {:6d}                                    ({:>5.2f} secs)\n",
                              num_const_accepts, to_seconds( time_resubC ) );
    std::cout << fmt::format( "[i]            0-resub {:6d}                                    ({:>5.2f} secs)\n",
                              num_div0_accepts, to_seconds( time_resub0 ) );
    std::cout << fmt::format( "[i]            xor-resub {:6d}                                  ({:>5.2f} secs)\n",
                              num_div_xor_accepts, to_seconds( time_resub_xor ) );
    std::cout << fmt::format( "[i]            2xor-resub  {:6d}                                ({:>5.2f} secs)\n",
                              num_div_2xor_accepts, to_seconds( time_resub2_xor ) );
    std::cout << fmt::format( "[i]            3xor-resub  {:6d}                                ({:>5.2f} secs)\n",
                              num_div_3xor_accepts, to_seconds( time_resub3_xor ) );
    std::cout << fmt::format( "[i]            And-resub   {:6d}                                ({:>5.2f} secs)\n",
                              num_div_and_accepts, to_seconds( time_resub_and ) );
    std::cout << fmt::format( "[i]            XA_XO-resub   {:6d}                              ({:>5.2f} secs)\n",
                              num_div_xor_and_accepts, to_seconds( time_resub_xor_and ) );
    std::cout << fmt::format( "[i]            XAX-resub   {:6d}                                ({:>5.2f} secs)\n",
                              num_div_xax_accepts, to_seconds( time_resub_xor_and_xor ) );
    std::cout << fmt::format( "[i]            AA_OO-resub {:6d}                                ({:>5.2f} secs)\n",
                              num_resub_aa_oo_accepts, to_seconds( time_resub_div_aa_oo ) );
    std::cout << fmt::format( "[i]            AO_OA-resub {:6d}                                ({:>5.2f} secs)\n",
                              num_resub_ao_oa_accepts, to_seconds( time_resub_div_ao_oa ) );
    std::cout << fmt::format( "[i]            X2-resub {:6d}                                   ({:>5.2f} secs)\n",
                              num_resub_xor2_accepts, to_seconds( time_resub_div_x2 ) );

    std::cout << fmt::format( "[i]            collect unate divisors                          ({:>5.2f} secs)\n", to_seconds( time_collect_unate_divisors ) );
    std::cout << fmt::format( "[i]            collect binate divisors                         ({:>5.2f} secs)\n", to_seconds( time_collect_binate_divisors ) );
    std::cout << fmt::format( "[i]            collect uxor divisors                           ({:>5.2f} secs)\n", to_seconds( time_collect_xortt_unate_divisors ) );
    std::cout << fmt::format( "[i]            collect bxor divisors                           ({:>5.2f} secs)\n", to_seconds( time_collect_xortt_binate_divisors ) );
    std::cout << fmt::format( "[i]            collect band divisors                           ({:>5.2f} secs)\n", to_seconds( time_collect_andtt_binate_divisors ) );
    std::cout << fmt::format( "[i]            collect bor  divisors                           ({:>5.2f} secs)\n", to_seconds( time_collect_ortt_binate_divisors ) );
    std::cout << fmt::format( "[i]            total   {:6d}\n",
                              ( num_const_accepts + num_div0_accepts + num_div_xor_accepts + num_div_2xor_accepts + num_div_3xor_accepts + num_div_and_accepts +
                                num_div_xor_and_accepts + num_div_xax_accepts + num_resub_aa_oo_accepts + num_resub_ao_oa_accepts + num_resub_xor2_accepts /* + num_resub_aaa_accepts */ ) );
  }
}; /* xag_resub_stats_bd */

namespace detail
{

template<typename Ntk>
class node_mffc_inside_xag_bd
{
public:
  using node = typename Ntk::node;

public:
  explicit node_mffc_inside_xag_bd( Ntk const& ntk )
      : ntk( ntk )
  {
  }

  std::pair<int32_t, int32_t> run( node const& n, std::vector<node> const& leaves, std::vector<node>& inside )
  {
    /* increment the fanout counters for the leaves */
    for ( const auto& l : leaves )
      ntk.incr_fanout_size( l );

    /* dereference the node */
    auto count1 = node_deref_rec( n );

    /* collect the nodes inside the MFFC */
    node_mffc_cone( n, inside );

    /* reference it back */
    auto count2 = node_ref_rec( n );
    (void)count2;

    assert( count1.first == count2.first );
    assert( count1.second == count2.second );

    for ( const auto& l : leaves )
      ntk.decr_fanout_size( l );

    return count1;
  }

private:
  /* ! \brief Dereference the node's MFFC */
  std::pair<int32_t, int32_t> node_deref_rec( node const& n )
  {

    if ( ntk.is_pi( n ) )
      return { 0, 0 };

    int32_t counter_and = 0;
    int32_t counter_xor = 0;

    if ( ntk.is_and( n ) )
    {
      counter_and = 1;
    }
    else if ( ntk.is_xor( n ) )
    {
      counter_xor = 1;
    }

    ntk.foreach_fanin( n, [&]( const auto& f )
                       {
      auto const& p = ntk.get_node( f );

      ntk.decr_fanout_size( p );
      if ( ntk.fanout_size( p ) == 0 )
      {
        auto counter = node_deref_rec( p );
        counter_and += counter.first;
        counter_xor += counter.second;
      } } );

    return { counter_and, counter_xor };
  }

  /* ! \brief Reference the node's MFFC */
  std::pair<int32_t, int32_t> node_ref_rec( node const& n )
  {
    if ( ntk.is_pi( n ) )
      return { 0, 0 };

    int32_t counter_and = 0;
    int32_t counter_xor = 0;

    if ( ntk.is_and( n ) )
    {
      counter_and = 1;
    }
    else if ( ntk.is_xor( n ) )
    {
      counter_xor = 1;
    }

    ntk.foreach_fanin( n, [&]( const auto& f )
                       {
      auto const& p = ntk.get_node( f );

      auto v = ntk.fanout_size( p );
      ntk.incr_fanout_size( p );
      if ( v == 0 )
      {
        auto counter = node_ref_rec( p );
        counter_and += counter.first;
        counter_xor += counter.second;
      } } );

    return { counter_and, counter_xor };
  }

  void node_mffc_cone_rec( node const& n, std::vector<node>& cone, bool top_most )
  {
    /* skip visited nodes */
    if ( ntk.visited( n ) == ntk.trav_id() )
      return;
    ntk.set_visited( n, ntk.trav_id() );

    if ( !top_most && ( ntk.is_pi( n ) || ntk.fanout_size( n ) > 0 ) )
      return;

    /* recurse on children */
    ntk.foreach_fanin( n, [&]( const auto& f )
                       { node_mffc_cone_rec( ntk.get_node( f ), cone, false ); } );

    /* collect the internal nodes */
    cone.emplace_back( n );
  }

  void node_mffc_cone( node const& n, std::vector<node>& cone )
  {
    cone.clear();
    ntk.incr_trav_id();
    node_mffc_cone_rec( n, cone, true );
  }

private:
  Ntk const& ntk;
};

} /* namespace detail */

template<typename Ntk, typename Simulator, typename TT>
struct xag_resub_functor_bd
{
public:
  using node = xag_network::node;
  using signal = xag_network::signal;
  using stats = xag_resub_stats_bd;

  struct xor_unate_ttdivs
  {
    using tt_hash = kitty::hash<TT>;
    std::unordered_map<TT, node, tt_hash> uxtts_positive;
    std::unordered_map<TT, node, tt_hash> uxtts_negative;

    void clear()
    {
      uxtts_positive.clear();
      uxtts_negative.clear();
    }
  };

  struct xor_binate_ttdivs
  {
    using divs_pair = std::pair<node, node>; // 把divs_pair重命名
    using tt_hash = kitty::hash<TT>;
    std::unordered_map<TT, divs_pair, tt_hash> bxtts_positive;
    std::unordered_map<TT, divs_pair, tt_hash> bxtts_negative;

    void clear()
    {
      bxtts_positive.clear();
      bxtts_negative.clear();
    }
  };

  struct and_binate_ttdivs
  {
    using divs_pair = std::pair<signal, signal>; // 把divs_pair重命名
    using tt_hash = kitty::hash<TT>;
    std::unordered_map<TT, divs_pair, tt_hash> batts_positive;
    std::unordered_map<TT, divs_pair, tt_hash> batts_negative;

    void clear()
    {
      batts_positive.clear();
      batts_negative.clear();
    }
  };

  struct or_binate_ttdivs
  {
    using divs_pair = std::pair<signal, signal>; // 把divs_pair重命名
    using tt_hash = kitty::hash<TT>;
    std::unordered_map<TT, divs_pair, tt_hash> botts_positive;
    std::unordered_map<TT, divs_pair, tt_hash> botts_negative;

    void clear()
    {
      botts_positive.clear();
      botts_negative.clear();
    }
  };

  struct unate_divisors
  {
    using signal = typename xag_network::signal;

    std::vector<signal> positive_divisors;
    std::vector<signal> negative_divisors;
    std::vector<signal> next_candidates;

    void clear()
    {
      positive_divisors.clear();
      negative_divisors.clear();
      next_candidates.clear();
    }
  };

  struct binate_divisors
  {
    using signal = typename xag_network::signal;

    std::vector<signal> positive_divisors0;
    std::vector<signal> positive_divisors1;
    std::vector<signal> negative_divisors0;
    std::vector<signal> negative_divisors1;

    void clear()
    {
      positive_divisors0.clear();
      positive_divisors1.clear();
      negative_divisors0.clear();
      negative_divisors1.clear();
    }
  };

public:
  explicit xag_resub_functor_bd( Ntk& ntk, Simulator const& sim, std::vector<node> const& divs, uint32_t num_divs, stats& st )
      : ntk( ntk ), sim( sim ), divs( divs ), num_divs( num_divs ), st( st )
  {
  }

  std::optional<signal> operator()( node const& root, TT care, uint32_t required, uint32_t max_inserts, std::pair<uint32_t, uint32_t> num_mffc, uint32_t& last_gain )
  {

    uint32_t num_and_mffc = num_mffc.first;
    uint32_t num_xor_mffc = num_mffc.second;
    /* consider constants */
    auto g = call_with_stopwatch( st.time_resubC, [&]()
                                  { return resub_const( root, care, required ); } );
    if ( g )
    {
      ++st.num_const_accepts;
      last_gain = num_and_mffc;
      return g; /* accepted resub */
    }

    /* consider equal nodes */
    g = call_with_stopwatch( st.time_resub0, [&]()
                             { return resub_div0( root, care, required ); } );
    if ( g )
    {
      ++st.num_div0_accepts;
      last_gain = num_and_mffc;
      return g; /* accepted resub */
    }


    call_with_stopwatch( st.time_collect_xortt_unate_divisors, [&]()
                         { collect_xor_unate_divstt( root, care, required ); } );

    call_with_stopwatch( st.time_collect_xortt_binate_divisors, [&]()
                         { collect_xor_binate_divstt( root, care, required ); } );

    if ( num_and_mffc == 0 )
    {
      return std::nullopt;

      if ( max_inserts == 0 || num_xor_mffc == 1 )
        return std::nullopt;

      g = call_with_stopwatch( st.time_resub_xor, [&]()
                               { return resub_div_xor( root, care, required ); } );
      if ( g )
      {

        ++st.num_div_xor_accepts;
        last_gain = 0;
        return g; /* accepted resub */
      }

      if ( max_inserts == 1 || num_xor_mffc == 2 )
        return std::nullopt;
      /* consider two nodes */
      g = call_with_stopwatch( st.time_resub2_xor, [&]()
                               { return resub_div_2xor( root, care, required ); } );
      if ( g )
      {

        ++st.num_div_2xor_accepts;
        last_gain = 0;
        return g; /* accepted resub */
      }
      if ( max_inserts == 2 || num_xor_mffc == 3 )
        return std::nullopt;

      /* consider three nodes */
      g = call_with_stopwatch( st.time_resub3_xor, [&]()
                               { return resub_div_3xor( root, care, required ); } );
      if ( g )
      {
        ++st.num_div_3xor_accepts;
        last_gain = 0;
        return g; /* accepted resub */
      }
    }
    else
    {
      g = call_with_stopwatch( st.time_resub_xor, [&]()
                               { return resub_div_xor( root, care, required ); } );
      if ( g )
      {
       
        ++st.num_div_xor_accepts;
        last_gain = num_and_mffc;
        return g; /* accepted resub */
      }

      g = call_with_stopwatch( st.time_resub2_xor, [&]()
                               { return resub_div_2xor( root, care, required ); } );
      if ( g )
      {
       
        ++st.num_div_2xor_accepts;
        last_gain = num_and_mffc;
        return g; /* accepted resub */
      }

      g = call_with_stopwatch( st.time_resub3_xor, [&]()
                               { return resub_div_3xor( root, care, required ); } );
      if ( g )
      {
        ++st.num_div_3xor_accepts;
        last_gain = num_and_mffc;
        return g; /* accepted resub */
      }

      if ( num_and_mffc < 2 ) /* it is worth trying also AND resub here */
        return std::nullopt;

      /* collect level one divisors */
      call_with_stopwatch( st.time_collect_unate_divisors, [&]()
                           { collect_unate_divisors( root, required ); } );

      call_with_stopwatch( st.time_collect_andtt_binate_divisors, [&]()
                           { collect_and_binate_divstt( root, care, required ); } );

      call_with_stopwatch( st.time_collect_ortt_binate_divisors, [&]()
                           { collect_or_binate_divstt( root, care, required ); } );

      g = call_with_stopwatch( st.time_resub_and, [&]()
                               { return resub_div_and( root, care, required ); } );
      if ( g )
      {
        ++st.num_div_and_accepts;
        last_gain = num_and_mffc - 1;
        return g; /*  accepted resub */
      }

      g = call_with_stopwatch( st.time_resub_xor_and, [&]()
                               { return resub_div_xa_xo( root, care, required ); } );
      if ( g )
      {
        ++st.num_div_xor_and_accepts;
        last_gain = num_and_mffc - 1;
        return g; /*  accepted resub */
      }

      g = call_with_stopwatch( st.time_resub_xor_and_xor, [&]()
                               { return resub_div_xax( root, care, required ); } );
      if ( g )
      {
        ++st.num_div_xax_accepts;
        last_gain = num_and_mffc - 1;
        return g; /*  accepted resub */
      }

      if ( num_and_mffc < 3 ) /* it is worth trying also AND-12 resub here */
        return std::nullopt;

      /* consider triples */
      g = call_with_stopwatch( st.time_resub_div_aa_oo, [&]()
                               { return resub_div_aa_oo( root, care, required ); } );
      if ( g )
      {

        ++st.num_resub_aa_oo_accepts;
        last_gain = num_and_mffc - 2;
        return g; /* accepted resub */
      }

      /* collect level two divisors */
      call_with_stopwatch( st.time_collect_binate_divisors, [&]()
                           { collect_binate_divisors( root, required ); } );

      /* consider two nodes */
      g = call_with_stopwatch( st.time_resub_div_ao_oa, [&]()
                               { return resub_div_ao_oa( root, care, required ); } );
      if ( g )
      {
        ++st.num_resub_ao_oa_accepts;
        last_gain = num_and_mffc - 2;
        return g; /* accepted resub */
      }

      g = call_with_stopwatch( st.time_resub_div_x2, [&]()
                               { return resub_div_x2( root, care, required ); } );
      if ( g )
      {
        ++st.num_resub_xor2_accepts;
        last_gain = num_and_mffc - 2;
        return g; /* accepted resub */
      }

    }
    return std::nullopt;
  }

  std::optional<signal> resub_const( node const& root, TT care, uint32_t required ) const
  {

    (void)required;
    auto tt = sim.get_tt( ntk.make_signal( root ) );
    if ( care.num_vars() > tt.num_vars() )
      care = kitty::shrink_to( care, tt.num_vars() );
    else
      care = kitty::extend_to( care, tt.num_vars() );

    if ( binary_and( tt, care ) == sim.get_tt( ntk.get_constant( false ) ) )
    {
      return sim.get_phase( root ) ? ntk.get_constant( true ) : ntk.get_constant( false );
    }
    return std::nullopt;
  }

  std::optional<signal> resub_div0( node const& root, TT care, uint32_t required ) const
  {
    
    (void)required;
    auto const tt = sim.get_tt( ntk.make_signal( root ) );
    if ( care.num_vars() > tt.num_vars() )
      care = kitty::shrink_to( care, tt.num_vars() );
    else
      care = kitty::extend_to( care, tt.num_vars() );
    for ( auto i = 0u; i < num_divs; ++i )
    {
      auto const d = divs.at( i );

      if ( binary_and( tt, care ) != binary_and( sim.get_tt( ntk.make_signal( d ) ), care ) )
        continue;
      return ( sim.get_phase( d ) ^ sim.get_phase( root ) ) ? !ntk.make_signal( d ) : ntk.make_signal( d );
    }
    return std::nullopt;
  }
  
  void collect_xor_unate_divstt( node const& root, TT care, uint32_t required )
  {

    uxdivs.clear();
    auto const& tt = sim.get_tt( ntk.make_signal( root ) );
    if ( care.num_vars() > tt.num_vars() )
      care = kitty::shrink_to( care, tt.num_vars() );
    else
      care = kitty::extend_to( care, tt.num_vars() );

    for ( auto i = 0u; i < num_divs; ++i )
    {
      auto const& g0 = divs.at( i );
      auto const& tt_g0 = sim.get_tt( ntk.make_signal( g0 ) );
      auto tt_diff_p = binary_and( ( tt_g0 ^ tt ), care );
      uxdivs.uxtts_positive.emplace( tt_diff_p, g0 );
      auto tt_diff_n = binary_and( ( tt_g0 ^ kitty::unary_not( tt ) ), care );
      uxdivs.uxtts_negative.emplace( tt_diff_n, g0 );
    }
  }
  
  void collect_xor_binate_divstt( node const& root, TT care, uint32_t required )
  {
    bxdivs.clear();
    auto const& tt = sim.get_tt( ntk.make_signal( root ) );

    if ( care.num_vars() > tt.num_vars() )
      care = kitty::shrink_to( care, tt.num_vars() );
    else
      care = kitty::extend_to( care, tt.num_vars() );

    for ( auto i = 0u; i < num_divs; ++i )
    {
      auto const& g0 = divs.at( i ); 
      for ( auto j = i + 1; j < num_divs; ++j )
      {
        auto const& g1 = divs.at( j );                           
        auto const& tt_g0 = sim.get_tt( ntk.make_signal( g0 ) ); 
        auto const& tt_g1 = sim.get_tt( ntk.make_signal( g1 ) );

        auto tt_diff_p = binary_and( ( ( tt_g0 ^ tt_g1 ) ^ tt ), care );
        bxdivs.bxtts_positive.emplace( tt_diff_p, std::make_pair( g0, g1 ) );
        auto tt_diff_n = binary_and( ( ( tt_g0 ^ tt_g1 ) ^ kitty::unary_not( tt ) ), care );
        bxdivs.bxtts_negative.emplace( tt_diff_n, std::make_pair( g0, g1 ) );
      }
    }
  }
  
  void collect_and_binate_divstt( node const& root, TT care, uint32_t required )
  {
    badivs.clear();

    auto const& tt = sim.get_tt( ntk.make_signal( root ) );

    if ( care.num_vars() > tt.num_vars() )
      care = kitty::shrink_to( care, tt.num_vars() );
    else
      care = kitty::extend_to( care, tt.num_vars() );
    for ( auto i = 0u; i < num_divs; ++i )
    {
      auto const& g0 = divs.at( i ); 
      for ( auto j = i + 1; j < num_divs; ++j )
      {
        auto const& g1 = divs.at( j );                           
        auto const& tt_g0 = sim.get_tt( ntk.make_signal( g0 ) ); 
        auto const& tt_g1 = sim.get_tt( ntk.make_signal( g1 ) );

        auto tt_diff_p0 = binary_and( ( ( tt_g0 & tt_g1 ) ^ tt ), care );
        badivs.batts_positive.emplace( tt_diff_p0, std::make_pair( ntk.make_signal( g0 ), ntk.make_signal( g1 ) ) );
        auto tt_diff_p1 = binary_and( ( ( ~tt_g0 & tt_g1 ) ^ tt ), care );
        badivs.batts_positive.emplace( tt_diff_p1, std::make_pair( !ntk.make_signal( g0 ), ntk.make_signal( g1 ) ) );
        auto tt_diff_p2 = binary_and( ( ( tt_g0 & ~tt_g1 ) ^ tt ), care );
        badivs.batts_positive.emplace( tt_diff_p2, std::make_pair( ntk.make_signal( g0 ), !ntk.make_signal( g1 ) ) );
        auto tt_diff_p3 = binary_and( ( ( ~tt_g0 & ~tt_g1 ) ^ tt ), care );
        badivs.batts_positive.emplace( tt_diff_p3, std::make_pair( !ntk.make_signal( g0 ), !ntk.make_signal( g1 ) ) );

        auto tt_diff_n0 = binary_and( ( ( tt_g0 & tt_g1 ) ^ kitty::unary_not( tt ) ), care );
        badivs.batts_negative.emplace( tt_diff_n0, std::make_pair( ntk.make_signal( g0 ), ntk.make_signal( g1 ) ) );
        auto tt_diff_n1 = binary_and( ( ( ~tt_g0 & tt_g1 ) ^ kitty::unary_not( tt ) ), care );
        badivs.batts_negative.emplace( tt_diff_n1, std::make_pair( !ntk.make_signal( g0 ), ntk.make_signal( g1 ) ) );
        auto tt_diff_n2 = binary_and( ( ( tt_g0 & ~tt_g1 ) ^ kitty::unary_not( tt ) ), care );
        badivs.batts_negative.emplace( tt_diff_n2, std::make_pair( ntk.make_signal( g0 ), !ntk.make_signal( g1 ) ) );
        auto tt_diff_n3 = binary_and( ( ( ~tt_g0 & ~tt_g1 ) ^ kitty::unary_not( tt ) ), care );
        badivs.batts_negative.emplace( tt_diff_n3, std::make_pair( !ntk.make_signal( g0 ), !ntk.make_signal( g1 ) ) );
      }
    }

  }
  
  void collect_or_binate_divstt( node const& root, TT care, uint32_t required )
  {
    bodivs.clear();

    auto const& tt = sim.get_tt( ntk.make_signal( root ) );

    if ( care.num_vars() > tt.num_vars() )
      care = kitty::shrink_to( care, tt.num_vars() );
    else
      care = kitty::extend_to( care, tt.num_vars() );

    for ( auto i = 0u; i < num_divs; ++i )
    {
      auto const& g0 = divs.at( i ); // 定位g(节点)遍历divs（g） 让每个节点与 tt 异或
      for ( auto j = i + 1; j < num_divs; ++j )
      {
        auto const& g1 = divs.at( j );                           // 定位g(节点)遍历divs（g） 让每个节点与 tt 异或
        auto const& tt_g0 = sim.get_tt( ntk.make_signal( g0 ) ); // 获取signal（信号 index he compl） 信号的真值表（节点的真值表包含了反相器）
        auto const& tt_g1 = sim.get_tt( ntk.make_signal( g1 ) );

        auto tt_diff_p0 = binary_and( ( ( tt_g0 | tt_g1 ) ^ tt ), care );
        bodivs.botts_positive.emplace( tt_diff_p0, std::make_pair( ntk.make_signal( g0 ), ntk.make_signal( g1 ) ) );
        auto tt_diff_p1 = binary_and( ( ( ~tt_g0 | tt_g1 ) ^ tt ), care );
        bodivs.botts_positive.emplace( tt_diff_p1, std::make_pair( !ntk.make_signal( g0 ), ntk.make_signal( g1 ) ) );
        auto tt_diff_p2 = binary_and( ( ( tt_g0 | ~tt_g1 ) ^ tt ), care );
        bodivs.botts_positive.emplace( tt_diff_p2, std::make_pair( ntk.make_signal( g0 ), !ntk.make_signal( g1 ) ) );
        auto tt_diff_p3 = binary_and( ( ( ~tt_g0 | ~tt_g1 ) ^ tt ), care );
        bodivs.botts_positive.emplace( tt_diff_p3, std::make_pair( !ntk.make_signal( g0 ), !ntk.make_signal( g1 ) ) );

        auto tt_diff_n0 = binary_and( ( ( tt_g0 | tt_g1 ) ^ kitty::unary_not( tt ) ), care );
        bodivs.botts_negative.emplace( tt_diff_n0, std::make_pair( ntk.make_signal( g0 ), ntk.make_signal( g1 ) ) );
        auto tt_diff_n1 = binary_and( ( ( ~tt_g0 | tt_g1 ) ^ kitty::unary_not( tt ) ), care );
        bodivs.botts_negative.emplace( tt_diff_n1, std::make_pair( !ntk.make_signal( g0 ), ntk.make_signal( g1 ) ) );
        auto tt_diff_n2 = binary_and( ( ( tt_g0 | ~tt_g1 ) ^ kitty::unary_not( tt ) ), care );
        bodivs.botts_negative.emplace( tt_diff_n2, std::make_pair( ntk.make_signal( g0 ), !ntk.make_signal( g1 ) ) );
        auto tt_diff_n3 = binary_and( ( ( ~tt_g0 | ~tt_g1 ) ^ kitty::unary_not( tt ) ), care );
        bodivs.botts_negative.emplace( tt_diff_n3, std::make_pair( !ntk.make_signal( g0 ), !ntk.make_signal( g1 ) ) );
      }
    }

  }
  
  std::optional<signal> resub_div_xor( node const& root, TT care, uint32_t required ) const
  {

    (void)required;
    auto const& tt = sim.get_tt( ntk.make_signal( root ) );
    if ( care.num_vars() > tt.num_vars() )
      care = kitty::shrink_to( care, tt.num_vars() );
    else
      care = kitty::extend_to( care, tt.num_vars() );


    for ( auto i = 0u; i < num_divs; ++i ) 
    {
      auto const& g0 = divs.at( i );
      auto const& tt_g0 = sim.get_tt( ntk.make_signal( g0 ) );
      auto iter0 = uxdivs.uxtts_positive.find( binary_and( tt_g0, care ) );
      auto iter1 = uxdivs.uxtts_negative.find( binary_and( tt_g0, care ) );

      if ( iter0 != uxdivs.uxtts_positive.end() )
      {
        auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
        auto const b = ( ( sim.get_phase( iter0->second ) ) ? !ntk.make_signal( iter0->second ) : ntk.make_signal( iter0->second ) );
        return sim.get_phase( root ) ? !ntk.create_xor( a, b ) : ntk.create_xor( a, b );
      }
      if ( iter1 != uxdivs.uxtts_negative.end() )
      {
        auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
        auto const b = ( ( sim.get_phase( iter1->second ) ) ? !ntk.make_signal( iter1->second ) : ntk.make_signal( iter1->second ) );
        return sim.get_phase( root ) ? ntk.create_xor( a, b ) : !ntk.create_xor( a, b );
      }
    }
    return std::nullopt;
  }

  std::optional<signal> resub_div_2xor( node const& root, TT care, uint32_t required ) const
  {
    (void)required;
    auto const& tt = sim.get_tt( ntk.make_signal( root ) );
    if ( care.num_vars() > tt.num_vars() )
      care = kitty::shrink_to( care, tt.num_vars() );
    else
      care = kitty::extend_to( care, tt.num_vars() );

    for ( auto i = 0u; i < num_divs; ++i )
    {
      auto const& g0 = divs.at( i );
      auto const& tt_g0 = sim.get_tt( ntk.make_signal( g0 ) );
      auto iter0 = bxdivs.bxtts_positive.find( binary_and( tt_g0, care ) );
      auto iter1 = bxdivs.bxtts_negative.find( binary_and( tt_g0, care ) );

      if ( iter0 != bxdivs.bxtts_positive.end() )
      {
        auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
        auto const b = ( ( sim.get_phase( iter0->second.first ) ) ? !ntk.make_signal( iter0->second.first ) : ntk.make_signal( iter0->second.first ) );
        auto const c = ( ( sim.get_phase( iter0->second.second ) ) ? !ntk.make_signal( iter0->second.second ) : ntk.make_signal( iter0->second.second ) );
        return sim.get_phase( root ) ? !ntk.create_xor( a, ntk.create_xor( b, c ) ) : ntk.create_xor( a, ntk.create_xor( b, c ) );
      }
      if ( iter1 != bxdivs.bxtts_negative.end() )
      {
        auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
        auto const b = ( ( sim.get_phase( iter1->second.first ) ) ? !ntk.make_signal( iter1->second.first ) : ntk.make_signal( iter1->second.first ) );
        auto const c = ( ( sim.get_phase( iter1->second.second ) ) ? !ntk.make_signal( iter1->second.second ) : ntk.make_signal( iter1->second.second ) );
        return sim.get_phase( root ) ? ntk.create_xor( a, ntk.create_xor( b, c ) ) : !ntk.create_xor( a, ntk.create_xor( b, c ) );
      }
    }

    return std::nullopt;
  }

  std::optional<signal> resub_div_3xor( node const& root, TT care, uint32_t required ) const
  {

    (void)required;
    auto const& tt = sim.get_tt( ntk.make_signal( root ) );
    if ( care.num_vars() > tt.num_vars() )
      care = kitty::shrink_to( care, tt.num_vars() );
    else
      care = kitty::extend_to( care, tt.num_vars() );
    for ( auto i = 0u; i < num_divs; ++i )
    {
      auto const& g0 = divs.at( i );
      for ( auto j = i + 1; j < num_divs; ++j )
      {
        auto const& g1 = divs.at( j );
        auto tt_g0 = sim.get_tt( ntk.make_signal( g0 ) );
        auto tt_g1 = sim.get_tt( ntk.make_signal( g1 ) );

        auto tt_tar = ( tt_g1 ^ tt_g0 );
        auto iter0 = bxdivs.bxtts_positive.find( binary_and( tt_tar, care ) );
        auto iter1 = bxdivs.bxtts_negative.find( binary_and( tt_tar, care ) );

        if ( iter0 != bxdivs.bxtts_positive.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( iter0->second.first ) ) ? !ntk.make_signal( iter0->second.first ) : ntk.make_signal( iter0->second.first ) );
          auto const d = ( ( sim.get_phase( iter0->second.second ) ) ? !ntk.make_signal( iter0->second.second ) : ntk.make_signal( iter0->second.second ) );
          return sim.get_phase( root ) ? !ntk.create_xor( ntk.create_xor( a, b ), ntk.create_xor( c, d ) ) : ntk.create_xor( ntk.create_xor( a, b ), ntk.create_xor( c, d ) );
        }
        if ( iter1 != bxdivs.bxtts_negative.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( iter1->second.first ) ) ? !ntk.make_signal( iter1->second.first ) : ntk.make_signal( iter1->second.first ) );
          auto const d = ( ( sim.get_phase( iter1->second.second ) ) ? !ntk.make_signal( iter1->second.second ) : ntk.make_signal( iter1->second.second ) );
          return sim.get_phase( root ) ? ntk.create_xor( ntk.create_xor( a, b ), ntk.create_xor( c, d ) ) : !ntk.create_xor( ntk.create_xor( a, b ), ntk.create_xor( c, d ) );
        }
      }
    }

    return std::nullopt;
  }

  void collect_unate_divisors( node const& root, uint32_t required )
  {
    udivs.clear();

    auto const& tt = sim.get_tt( ntk.make_signal( root ) );
    for ( auto i = 0u; i < num_divs; ++i )
    {
      auto const d = divs.at( i );

      auto const& tt_d = sim.get_tt( ntk.make_signal( d ) );

      /* check positive containment */
      if ( kitty::implies( tt_d, tt ) )
      {
        udivs.positive_divisors.emplace_back( ntk.make_signal( d ) );
        continue;
      }

      /* check negative containment */
      if ( kitty::implies( tt, tt_d ) )
      {
        udivs.negative_divisors.emplace_back( ntk.make_signal( d ) );
        continue;
      }

      udivs.next_candidates.emplace_back( ntk.make_signal( d ) );
    }
  }
  
  std::optional<signal> resub_div_and( node const& root, TT care, uint32_t required ) const
  {
    (void)care;
    (void)required;
    auto const& tt = sim.get_tt( ntk.make_signal( root ) );
    if ( care.num_vars() > tt.num_vars() )
      care = kitty::shrink_to( care, tt.num_vars() );
    else
      care = kitty::extend_to( care, tt.num_vars() );

    /* check for positive unate divisors */
    for ( auto i = 0u; i < udivs.positive_divisors.size(); ++i )
    {
      auto const& s0 = udivs.positive_divisors.at( i );

      for ( auto j = i + 1; j < udivs.positive_divisors.size(); ++j )
      {
        auto const& s1 = udivs.positive_divisors.at( j );

        auto const& tt_s0 = sim.get_tt( s0 );
        auto const& tt_s1 = sim.get_tt( s1 );

        if ( binary_and( ( tt_s0 | tt_s1 ), care ) == binary_and( tt, care ) )
        {
          auto const l = sim.get_phase( ntk.get_node( s0 ) ) ? !s0 : s0;
          auto const r = sim.get_phase( ntk.get_node( s1 ) ) ? !s1 : s1;
          return sim.get_phase( root ) ? !ntk.create_or( l, r ) : ntk.create_or( l, r );
        }
      }
    }
    /* check for negative unate divisors */
    for ( auto i = 0u; i < udivs.negative_divisors.size(); ++i )
    {
      auto const& s0 = udivs.negative_divisors.at( i );

      for ( auto j = i + 1; j < udivs.negative_divisors.size(); ++j )
      {
        auto const& s1 = udivs.negative_divisors.at( j );

        auto const& tt_s0 = sim.get_tt( s0 );
        auto const& tt_s1 = sim.get_tt( s1 );

        if ( binary_and( ( tt_s0 & tt_s1 ), care ) == binary_and( tt, care ) )
        {
          auto const l = sim.get_phase( ntk.get_node( s0 ) ) ? !s0 : s0;
          auto const r = sim.get_phase( ntk.get_node( s1 ) ) ? !s1 : s1;
          return sim.get_phase( root ) ? !ntk.create_and( l, r ) : ntk.create_and( l, r );
        }
      }
    }

    return std::nullopt;
  }

  std::optional<signal> resub_div_xa_xo( node const& root, TT care, uint32_t required ) const
  {
    (void)required;
    /* 考虑xa */
    auto const& tt = sim.get_tt( ntk.make_signal( root ) );
    if ( care.num_vars() > tt.num_vars() )
      care = kitty::shrink_to( care, tt.num_vars() );
    else
      care = kitty::extend_to( care, tt.num_vars() );

    for ( auto i = 0u; i < num_divs; ++i )
    {
      auto const& g0 = divs.at( i );
      auto const& tt_g0 = sim.get_tt( ntk.make_signal( g0 ) );
      auto iter0 = badivs.batts_positive.find( binary_and( tt_g0, care ) );
      auto iter1 = badivs.batts_negative.find( binary_and( tt_g0, care ) );

      if ( iter0 != badivs.batts_positive.end() )
      {
        auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
        auto const b = ( ( sim.get_phase( ntk.get_node( iter0->second.first ) ) ) ? !iter0->second.first : iter0->second.first );
        auto const c = ( ( sim.get_phase( ntk.get_node( iter0->second.second ) ) ) ? !iter0->second.second : iter0->second.second );
        return sim.get_phase( root ) ? !ntk.create_xor( a, ntk.create_and( b, c ) ) : ntk.create_xor( a, ntk.create_and( b, c ) );
      }
      if ( iter1 != badivs.batts_negative.end() )
      {
        auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
        auto const b = ( ( sim.get_phase( ntk.get_node( iter1->second.first ) ) ) ? !iter1->second.first : iter1->second.first );
        auto const c = ( ( sim.get_phase( ntk.get_node( iter1->second.second ) ) ) ? !iter1->second.second : iter1->second.second );
        return sim.get_phase( root ) ? ntk.create_xor( a, ntk.create_and( b, c ) ) : !ntk.create_xor( a, ntk.create_and( b, c ) );
      }
    }

    /* 考虑xo */

    for ( auto i = 0u; i < num_divs; ++i )
    {
      auto const& g0 = divs.at( i );
      auto const& tt_g0 = sim.get_tt( ntk.make_signal( g0 ) );
      auto iter0 = bodivs.botts_positive.find( binary_and( tt_g0, care ) );
      auto iter1 = bodivs.botts_negative.find( binary_and( tt_g0, care ) );

      if ( iter0 != bodivs.botts_positive.end() )
      {
        auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
        auto const b = ( ( sim.get_phase( ntk.get_node( iter0->second.first ) ) ) ? !iter0->second.first : iter0->second.first );
        auto const c = ( ( sim.get_phase( ntk.get_node( iter0->second.second ) ) ) ? !iter0->second.second : iter0->second.second );
        return sim.get_phase( root ) ? !ntk.create_xor( a, ntk.create_or( b, c ) ) : ntk.create_xor( a, ntk.create_or( b, c ) );
      }
      if ( iter1 != bodivs.botts_negative.end() )
      {
        auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
        auto const b = ( ( sim.get_phase( ntk.get_node( iter1->second.first ) ) ) ? !iter1->second.first : iter1->second.first );
        auto const c = ( ( sim.get_phase( ntk.get_node( iter1->second.second ) ) ) ? !iter1->second.second : iter1->second.second );
        return sim.get_phase( root ) ? ntk.create_xor( a, ntk.create_or( b, c ) ) : !ntk.create_xor( a, ntk.create_or( b, c ) );
      }
    }

    return std::nullopt;
  }
  
  std::optional<signal> resub_div_xax( node const& root, TT care, uint32_t required ) const
  {
    (void)required;

    auto const& tt = sim.get_tt( ntk.make_signal( root ) );
    if ( care.num_vars() > tt.num_vars() )
      care = kitty::shrink_to( care, tt.num_vars() );
    else
      care = kitty::extend_to( care, tt.num_vars() );
    /* 考虑xox */
    for ( auto i = 0u; i < num_divs; ++i )
    {
      auto const& g0 = divs.at( i );

      for ( auto j = i + 1; j < num_divs; ++j )
      {

        auto const& g1 = divs.at( j );
        auto const& tt_g0 = sim.get_tt( ntk.make_signal( g0 ) );
        auto const& tt_g1 = sim.get_tt( ntk.make_signal( g1 ) );
        auto tt_tar_ox_0 = ( tt_g0 | tt_g1 );
        auto tt_tar_ox_1 = ( ~tt_g0 | tt_g1 );
        auto tt_tar_ox_2 = ( tt_g0 | ~tt_g1 );
        auto tt_tar_ox_3 = ( ~tt_g0 | ~tt_g1 );

        auto iter0 = bxdivs.bxtts_positive.find( binary_and( tt_tar_ox_0, care ) );
        auto iter00 = bxdivs.bxtts_negative.find( binary_and( tt_tar_ox_0, care ) );
        auto iter1 = bxdivs.bxtts_positive.find( binary_and( tt_tar_ox_1, care ) );
        auto iter11 = bxdivs.bxtts_negative.find( binary_and( tt_tar_ox_1, care ) );
        auto iter2 = bxdivs.bxtts_positive.find( binary_and( tt_tar_ox_2, care ) );
        auto iter22 = bxdivs.bxtts_negative.find( binary_and( tt_tar_ox_2, care ) );
        auto iter3 = bxdivs.bxtts_positive.find( binary_and( tt_tar_ox_3, care ) );
        auto iter33 = bxdivs.bxtts_negative.find( binary_and( tt_tar_ox_3, care ) );

        if ( iter0 != bxdivs.bxtts_positive.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( iter0->second.first ) ) ? !ntk.make_signal( iter0->second.first ) : ntk.make_signal( iter0->second.first ) );
          auto const d = ( ( sim.get_phase( iter0->second.second ) ) ? !ntk.make_signal( iter0->second.second ) : ntk.make_signal( iter0->second.second ) );
          return sim.get_phase( root ) ? !ntk.create_xor( ntk.create_or( a, b ), ntk.create_xor( c, d ) ) : ntk.create_xor( ntk.create_or( a, b ), ntk.create_xor( c, d ) );
        }
        if ( iter00 != bxdivs.bxtts_negative.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( iter00->second.first ) ) ? !ntk.make_signal( iter00->second.first ) : ntk.make_signal( iter00->second.first ) );
          auto const d = ( ( sim.get_phase( iter00->second.second ) ) ? !ntk.make_signal( iter00->second.second ) : ntk.make_signal( iter00->second.second ) );
          return sim.get_phase( root ) ? ntk.create_xor( ntk.create_or( a, b ), ntk.create_xor( c, d ) ) : !ntk.create_xor( ntk.create_or( a, b ), ntk.create_xor( c, d ) );
        }
        if ( iter1 != bxdivs.bxtts_positive.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( iter1->second.first ) ) ? !ntk.make_signal( iter1->second.first ) : ntk.make_signal( iter1->second.first ) );
          auto const d = ( ( sim.get_phase( iter1->second.second ) ) ? !ntk.make_signal( iter1->second.second ) : ntk.make_signal( iter1->second.second ) );
          return sim.get_phase( root ) ? !ntk.create_xor( ntk.create_or( !a, b ), ntk.create_xor( c, d ) ) : ntk.create_xor( ntk.create_or( !a, b ), ntk.create_xor( c, d ) );
        }
        if ( iter11 != bxdivs.bxtts_negative.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( iter11->second.first ) ) ? !ntk.make_signal( iter11->second.first ) : ntk.make_signal( iter11->second.first ) );
          auto const d = ( ( sim.get_phase( iter11->second.second ) ) ? !ntk.make_signal( iter11->second.second ) : ntk.make_signal( iter11->second.second ) );
          return sim.get_phase( root ) ? ntk.create_xor( ntk.create_or( !a, b ), ntk.create_xor( c, d ) ) : !ntk.create_xor( ntk.create_or( !a, b ), ntk.create_xor( c, d ) );
        }
        if ( iter2 != bxdivs.bxtts_positive.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( iter2->second.first ) ) ? !ntk.make_signal( iter2->second.first ) : ntk.make_signal( iter2->second.first ) );
          auto const d = ( ( sim.get_phase( iter2->second.second ) ) ? !ntk.make_signal( iter2->second.second ) : ntk.make_signal( iter2->second.second ) );
          return sim.get_phase( root ) ? !ntk.create_xor( ntk.create_or( a, !b ), ntk.create_xor( c, d ) ) : ntk.create_xor( ntk.create_or( a, !b ), ntk.create_xor( c, d ) );
        }
        if ( iter22 != bxdivs.bxtts_negative.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( iter22->second.first ) ) ? !ntk.make_signal( iter22->second.first ) : ntk.make_signal( iter22->second.first ) );
          auto const d = ( ( sim.get_phase( iter22->second.second ) ) ? !ntk.make_signal( iter22->second.second ) : ntk.make_signal( iter22->second.second ) );
          return sim.get_phase( root ) ? ntk.create_xor( ntk.create_or( a, !b ), ntk.create_xor( c, d ) ) : !ntk.create_xor( ntk.create_or( a, !b ), ntk.create_xor( c, d ) );
        }
        if ( iter3 != bxdivs.bxtts_positive.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( iter3->second.first ) ) ? !ntk.make_signal( iter3->second.first ) : ntk.make_signal( iter3->second.first ) );
          auto const d = ( ( sim.get_phase( iter3->second.second ) ) ? !ntk.make_signal( iter3->second.second ) : ntk.make_signal( iter3->second.second ) );
          return sim.get_phase( root ) ? !ntk.create_xor( ntk.create_or( !a, !b ), ntk.create_xor( c, d ) ) : ntk.create_xor( ntk.create_or( !a, !b ), ntk.create_xor( c, d ) );
        }
        if ( iter33 != bxdivs.bxtts_negative.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( iter33->second.first ) ) ? !ntk.make_signal( iter33->second.first ) : ntk.make_signal( iter33->second.first ) );
          auto const d = ( ( sim.get_phase( iter33->second.second ) ) ? !ntk.make_signal( iter33->second.second ) : ntk.make_signal( iter33->second.second ) );
          return sim.get_phase( root ) ? ntk.create_xor( ntk.create_or( !a, !b ), ntk.create_xor( c, d ) ) : !ntk.create_xor( ntk.create_or( !a, !b ), ntk.create_xor( c, d ) );
        }
      }
    }
    /* 考虑xax */
    for ( auto i = 0u; i < num_divs; ++i )
    {
      auto const& g0 = divs.at( i );

      for ( auto j = i + 1; j < num_divs; ++j )
      {

        auto const& g1 = divs.at( j );
        auto const& tt_g0 = sim.get_tt( ntk.make_signal( g0 ) );
        auto const& tt_g1 = sim.get_tt( ntk.make_signal( g1 ) );
        auto tt_tar_ax_0 = ( tt_g0 & tt_g1 );
        auto tt_tar_ax_1 = ( ~tt_g0 & tt_g1 );
        auto tt_tar_ax_2 = ( tt_g0 & ~tt_g1 );
        auto tt_tar_ax_3 = ( ~tt_g0 & ~tt_g1 );

        auto iter0 = bxdivs.bxtts_positive.find( binary_and( tt_tar_ax_0, care ) );
        auto iter00 = bxdivs.bxtts_negative.find( binary_and( tt_tar_ax_0, care ) );
        auto iter1 = bxdivs.bxtts_positive.find( binary_and( tt_tar_ax_1, care ) );
        auto iter11 = bxdivs.bxtts_negative.find( binary_and( tt_tar_ax_1, care ) );
        auto iter2 = bxdivs.bxtts_positive.find( binary_and( tt_tar_ax_2, care ) );
        auto iter22 = bxdivs.bxtts_negative.find( binary_and( tt_tar_ax_2, care ) );
        auto iter3 = bxdivs.bxtts_positive.find( binary_and( tt_tar_ax_3, care ) );
        auto iter33 = bxdivs.bxtts_negative.find( binary_and( tt_tar_ax_3, care ) );

        if ( iter0 != bxdivs.bxtts_positive.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( iter0->second.first ) ) ? !ntk.make_signal( iter0->second.first ) : ntk.make_signal( iter0->second.first ) );
          auto const d = ( ( sim.get_phase( iter0->second.second ) ) ? !ntk.make_signal( iter0->second.second ) : ntk.make_signal( iter0->second.second ) );
          return sim.get_phase( root ) ? !ntk.create_xor( ntk.create_and( a, b ), ntk.create_xor( c, d ) ) : ntk.create_xor( ntk.create_and( a, b ), ntk.create_xor( c, d ) );
        }
        if ( iter00 != bxdivs.bxtts_negative.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( iter00->second.first ) ) ? !ntk.make_signal( iter00->second.first ) : ntk.make_signal( iter00->second.first ) );
          auto const d = ( ( sim.get_phase( iter00->second.second ) ) ? !ntk.make_signal( iter00->second.second ) : ntk.make_signal( iter00->second.second ) );
          return sim.get_phase( root ) ? ntk.create_xor( ntk.create_and( a, b ), ntk.create_xor( c, d ) ) : !ntk.create_xor( ntk.create_and( a, b ), ntk.create_xor( c, d ) );
        }
        if ( iter1 != bxdivs.bxtts_positive.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( iter1->second.first ) ) ? !ntk.make_signal( iter1->second.first ) : ntk.make_signal( iter1->second.first ) );
          auto const d = ( ( sim.get_phase( iter1->second.second ) ) ? !ntk.make_signal( iter1->second.second ) : ntk.make_signal( iter1->second.second ) );
          return sim.get_phase( root ) ? !ntk.create_xor( ntk.create_and( !a, b ), ntk.create_xor( c, d ) ) : ntk.create_xor( ntk.create_and( !a, b ), ntk.create_xor( c, d ) );
        }
        if ( iter11 != bxdivs.bxtts_negative.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( iter11->second.first ) ) ? !ntk.make_signal( iter11->second.first ) : ntk.make_signal( iter11->second.first ) );
          auto const d = ( ( sim.get_phase( iter11->second.second ) ) ? !ntk.make_signal( iter11->second.second ) : ntk.make_signal( iter11->second.second ) );
          return sim.get_phase( root ) ? ntk.create_xor( ntk.create_and( !a, b ), ntk.create_xor( c, d ) ) : !ntk.create_xor( ntk.create_and( !a, b ), ntk.create_xor( c, d ) );
        }
        if ( iter2 != bxdivs.bxtts_positive.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( iter2->second.first ) ) ? !ntk.make_signal( iter2->second.first ) : ntk.make_signal( iter2->second.first ) );
          auto const d = ( ( sim.get_phase( iter2->second.second ) ) ? !ntk.make_signal( iter2->second.second ) : ntk.make_signal( iter2->second.second ) );
          return sim.get_phase( root ) ? !ntk.create_xor( ntk.create_and( a, !b ), ntk.create_xor( c, d ) ) : ntk.create_xor( ntk.create_and( a, !b ), ntk.create_xor( c, d ) );
        }
        if ( iter22 != bxdivs.bxtts_negative.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( iter22->second.first ) ) ? !ntk.make_signal( iter22->second.first ) : ntk.make_signal( iter22->second.first ) );
          auto const d = ( ( sim.get_phase( iter22->second.second ) ) ? !ntk.make_signal( iter22->second.second ) : ntk.make_signal( iter22->second.second ) );
          return sim.get_phase( root ) ? ntk.create_xor( ntk.create_and( a, !b ), ntk.create_xor( c, d ) ) : !ntk.create_xor( ntk.create_and( a, !b ), ntk.create_xor( c, d ) );
        }
        if ( iter3 != bxdivs.bxtts_positive.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( iter3->second.first ) ) ? !ntk.make_signal( iter3->second.first ) : ntk.make_signal( iter3->second.first ) );
          auto const d = ( ( sim.get_phase( iter3->second.second ) ) ? !ntk.make_signal( iter3->second.second ) : ntk.make_signal( iter3->second.second ) );
          return sim.get_phase( root ) ? !ntk.create_xor( ntk.create_and( !a, !b ), ntk.create_xor( c, d ) ) : ntk.create_xor( ntk.create_and( !a, !b ), ntk.create_xor( c, d ) );
        }
        if ( iter33 != bxdivs.bxtts_negative.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( iter33->second.first ) ) ? !ntk.make_signal( iter33->second.first ) : ntk.make_signal( iter33->second.first ) );
          auto const d = ( ( sim.get_phase( iter33->second.second ) ) ? !ntk.make_signal( iter33->second.second ) : ntk.make_signal( iter33->second.second ) );
          return sim.get_phase( root ) ? ntk.create_xor( ntk.create_and( !a, !b ), ntk.create_xor( c, d ) ) : !ntk.create_xor( ntk.create_and( !a, !b ), ntk.create_xor( c, d ) );
        }
      }
    }
    /* 考虑xxa */
    for ( auto i = 0u; i < num_divs; ++i )
    {
      auto const& g0 = divs.at( i );

      for ( auto j = i + 1; j < num_divs; ++j )
      {
        auto const& g1 = divs.at( j );
        auto const& tt_g0 = sim.get_tt( ntk.make_signal( g0 ) );
        auto const& tt_g1 = sim.get_tt( ntk.make_signal( g1 ) );
        auto tt_tar_xa = ( tt_g0 ^ tt_g1 );

        auto iter0 = badivs.batts_positive.find( binary_and( tt_tar_xa, care ) );
        auto iter1 = badivs.batts_negative.find( binary_and( tt_tar_xa, care ) );

        if ( iter0 != badivs.batts_positive.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter0->second.first ) ) ) ? !iter0->second.first : iter0->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter0->second.second ) ) ) ? !iter0->second.second : iter0->second.second );
          return sim.get_phase( root ) ? !ntk.create_xor( ntk.create_xor( a, b ), ntk.create_and( c, d ) ) : ntk.create_xor( ntk.create_xor( a, b ), ntk.create_and( c, d ) );
        }
        if ( iter1 != badivs.batts_negative.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter1->second.first ) ) ) ? !iter1->second.first : iter1->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter1->second.second ) ) ) ? !iter1->second.second : iter1->second.second );
          return sim.get_phase( root ) ? ntk.create_xor( ntk.create_xor( a, b ), ntk.create_and( c, d ) ) : !ntk.create_xor( ntk.create_xor( a, b ), ntk.create_and( c, d ) );
        }
      }
    }
    /* 考虑xxo */
    for ( auto i = 0u; i < num_divs; ++i )
    {
      auto const& g0 = divs.at( i );

      for ( auto j = i + 1; j < num_divs; ++j )
      {

        auto const& g1 = divs.at( j );
        auto const& tt_g0 = sim.get_tt( ntk.make_signal( g0 ) );
        auto const& tt_g1 = sim.get_tt( ntk.make_signal( g1 ) );
        auto tt_tar_xo = ( tt_g0 ^ tt_g1 );

        auto iter0 = bodivs.botts_positive.find( binary_and( tt_tar_xo, care ) );
        auto iter1 = bodivs.botts_negative.find( binary_and( tt_tar_xo, care ) );

        if ( iter0 != bodivs.botts_positive.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter0->second.first ) ) ) ? !iter0->second.first : iter0->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter0->second.second ) ) ) ? !iter0->second.second : iter0->second.second );
          return sim.get_phase( root ) ? !ntk.create_xor( ntk.create_xor( a, b ), ntk.create_or( c, d ) ) : ntk.create_xor( ntk.create_xor( a, b ), ntk.create_or( c, d ) );
        }
        if ( iter1 != bodivs.botts_negative.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter1->second.first ) ) ) ? !iter1->second.first : iter1->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter1->second.second ) ) ) ? !iter1->second.second : iter1->second.second );
          return sim.get_phase( root ) ? ntk.create_xor( ntk.create_xor( a, b ), ntk.create_or( c, d ) ) : !ntk.create_xor( ntk.create_xor( a, b ), ntk.create_or( c, d ) );
        }
      }
    }
    return std::nullopt;
  }
  
  void collect_binate_divisors( node const& root, uint32_t required )
  {
    bdivs.clear();

    auto const& tt = sim.get_tt( ntk.make_signal( root ) );
    for ( auto i = 0u; i < udivs.next_candidates.size(); ++i )
    {
      auto const& s0 = udivs.next_candidates.at( i );
      if ( ntk.level( ntk.get_node( s0 ) ) > required - 2 )
        continue;

      for ( auto j = i + 1; j < udivs.next_candidates.size(); ++j )
      {
        auto const& s1 = udivs.next_candidates.at( j );
        if ( ntk.level( ntk.get_node( s1 ) ) > required - 2 )
          continue;

        if ( bdivs.positive_divisors0.size() < 500 ) // ps.max_divisors2
        {
          auto const& tt_s0 = sim.get_tt( s0 );
          auto const& tt_s1 = sim.get_tt( s1 );
          if ( kitty::implies( tt_s0 & tt_s1, tt ) )
          {
            bdivs.positive_divisors0.emplace_back( s0 );
            bdivs.positive_divisors1.emplace_back( s1 );
          }

          if ( kitty::implies( ~tt_s0 & tt_s1, tt ) )
          {
            bdivs.positive_divisors0.emplace_back( !s0 );
            bdivs.positive_divisors1.emplace_back( s1 );
          }

          if ( kitty::implies( tt_s0 & ~tt_s1, tt ) )
          {
            bdivs.positive_divisors0.emplace_back( s0 );
            bdivs.positive_divisors1.emplace_back( !s1 );
          }

          if ( kitty::implies( ~tt_s0 & ~tt_s1, tt ) )
          {
            bdivs.positive_divisors0.emplace_back( !s0 );
            bdivs.positive_divisors1.emplace_back( !s1 );
          }
        }

        if ( bdivs.negative_divisors0.size() < 500 ) // ps.max_divisors2
        {
          auto const& tt_s0 = sim.get_tt( s0 );
          auto const& tt_s1 = sim.get_tt( s1 );
          if ( kitty::implies( tt, tt_s0 & tt_s1 ) )
          {
            bdivs.negative_divisors0.emplace_back( s0 );
            bdivs.negative_divisors1.emplace_back( s1 );
          }

          if ( kitty::implies( tt, ~tt_s0 & tt_s1 ) )
          {
            bdivs.negative_divisors0.emplace_back( !s0 );
            bdivs.negative_divisors1.emplace_back( s1 );
          }

          if ( kitty::implies( tt, tt_s0 & ~tt_s1 ) )
          {
            bdivs.negative_divisors0.emplace_back( s0 );
            bdivs.negative_divisors1.emplace_back( !s1 );
          }

          if ( kitty::implies( tt, ~tt_s0 & ~tt_s1 ) )
          {
            bdivs.negative_divisors0.emplace_back( !s0 );
            bdivs.negative_divisors1.emplace_back( !s1 );
          }
        }
      }
    }
  }
  
  std::optional<signal> resub_div_aa_oo( node const& root, TT care, uint32_t required ) const
  {

    (void)required;
    auto const s = ntk.make_signal( root );
    auto const& tt = sim.get_tt( s );

    if ( care.num_vars() > tt.num_vars() )
      care = kitty::shrink_to( care, tt.num_vars() );
    else
      care = kitty::extend_to( care, tt.num_vars() );

    /* check positive unate divisors */
    for ( auto i = 0u; i < udivs.positive_divisors.size(); ++i )
    {
      auto const s0 = udivs.positive_divisors.at( i );

      for ( auto j = i + 1; j < udivs.positive_divisors.size(); ++j )
      {
        auto const s1 = udivs.positive_divisors.at( j );

        for ( auto k = j + 1; k < udivs.positive_divisors.size(); ++k )
        {
          auto const s2 = udivs.positive_divisors.at( k );

          auto const& tt_s0 = sim.get_tt( s0 );
          auto const& tt_s1 = sim.get_tt( s1 );
          auto const& tt_s2 = sim.get_tt( s2 );

          // if ( ( tt_s0 | tt_s1 | tt_s2 ) == tt )
          if ( binary_and( ( tt_s0 | tt_s1 | tt_s2 ), care ) == binary_and( tt, care ) )

          {

            auto const a = sim.get_phase( ntk.get_node( s0 ) ) ? !s0 : s0;
            auto const b = sim.get_phase( ntk.get_node( s1 ) ) ? !s1 : s1;
            auto const c = sim.get_phase( ntk.get_node( s2 ) ) ? !s2 : s2;

            return sim.get_phase( root ) ? !ntk.create_or( a, ntk.create_or( b, c ) ) : ntk.create_or( a, ntk.create_or( b, c ) );
          }
        }
      }
    }

    /* check negative unate divisors */
    for ( auto i = 0u; i < udivs.positive_divisors.size(); ++i )
    {
      auto const s0 = udivs.positive_divisors.at( i );

      for ( auto j = i + 1; j < udivs.positive_divisors.size(); ++j )
      {
        auto const s1 = udivs.positive_divisors.at( j );

        for ( auto k = j + 1; k < udivs.positive_divisors.size(); ++k )
        {
          auto const s2 = udivs.positive_divisors.at( k );

          auto const& tt_s0 = sim.get_tt( s0 );
          auto const& tt_s1 = sim.get_tt( s1 );
          auto const& tt_s2 = sim.get_tt( s2 );

          // if ( ( tt_s0 & tt_s1 & tt_s2 ) == tt )
          if ( binary_and( ( tt_s0 & tt_s1 & tt_s2 ), care ) == binary_and( tt, care ) )
          {
            auto const a = sim.get_phase( ntk.get_node( s0 ) ) ? !s0 : s0;
            auto const b = sim.get_phase( ntk.get_node( s1 ) ) ? !s1 : s1;
            auto const c = sim.get_phase( ntk.get_node( s2 ) ) ? !s2 : s2;

            return sim.get_phase( root ) ? !ntk.create_and( a, ntk.create_and( b, c ) ) : ntk.create_and( a, ntk.create_and( b, c ) );
          }
        }
      }
    }

    return std::nullopt;
  }
  
  std::optional<signal> resub_div_ao_oa( node const& root, TT care, uint32_t required ) const
  {

    (void)required;
    auto const s = ntk.make_signal( root );
    auto const& tt = sim.get_tt( s );
    if ( care.num_vars() > tt.num_vars() )
      care = kitty::shrink_to( care, tt.num_vars() );
    else
      care = kitty::extend_to( care, tt.num_vars() );

    /* check positive unate divisors */
    for ( const auto& s0 : udivs.positive_divisors )
    {
      auto const& tt_s0 = sim.get_tt( s0 );

      for ( auto j = 0u; j < bdivs.positive_divisors0.size(); ++j )
      {
        auto const s1 = bdivs.positive_divisors0.at( j );
        auto const s2 = bdivs.positive_divisors1.at( j );

        auto const& tt_s1 = sim.get_tt( s1 );
        auto const& tt_s2 = sim.get_tt( s2 );

        auto const a = sim.get_phase( ntk.get_node( s0 ) ) ? !s0 : s0;
        auto const b = sim.get_phase( ntk.get_node( s1 ) ) ? !s1 : s1;
        auto const c = sim.get_phase( ntk.get_node( s2 ) ) ? !s2 : s2;

        if ( binary_and( ( tt_s0 | ( tt_s1 & tt_s2 ) ), care ) == binary_and( tt, care ) )
        {
          return sim.get_phase( root ) ? !ntk.create_or( a, ntk.create_and( b, c ) ) : ntk.create_or( a, ntk.create_and( b, c ) );
        }
      }
    }

    /* check negative unate divisors */
    for ( const auto& s0 : udivs.negative_divisors )
    {
      auto const& tt_s0 = sim.get_tt( s0 );

      for ( auto j = 0u; j < bdivs.negative_divisors0.size(); ++j )
      {
        auto const s1 = bdivs.negative_divisors0.at( j );
        auto const s2 = bdivs.negative_divisors1.at( j );

        auto const& tt_s1 = sim.get_tt( s1 );
        auto const& tt_s2 = sim.get_tt( s2 );

        auto const a = sim.get_phase( ntk.get_node( s0 ) ) ? !s0 : s0;
        auto const b = sim.get_phase( ntk.get_node( s1 ) ) ? !s1 : s1;
        auto const c = sim.get_phase( ntk.get_node( s2 ) ) ? !s2 : s2;

        if ( binary_and( ( tt_s0 | ( tt_s1 & tt_s2 ) ), care ) == binary_and( tt, care ) )
        {
          return sim.get_phase( root ) ? !ntk.create_and( a, ntk.create_or( b, c ) ) : ntk.create_and( a, ntk.create_or( b, c ) );
        }
      }
    }

    return std::nullopt;
  }

  std::optional<signal> resub_div_x2( node const& root, TT care, uint32_t required ) const
  {
    (void)required;
    auto const& tt = sim.get_tt( ntk.make_signal( root ) );
    if ( care.num_vars() > tt.num_vars() )
      care = kitty::shrink_to( care, tt.num_vars() );
    else
      care = kitty::extend_to( care, tt.num_vars() );
    /* 考虑xaa */
    for ( auto i = 0u; i < num_divs; ++i )
    {
      auto const& g0 = divs.at( i );

      for ( auto j = i + 1; j < num_divs; ++j )
      {

        auto const& g1 = divs.at( j );
        auto const& tt_g0 = sim.get_tt( ntk.make_signal( g0 ) );
        auto const& tt_g1 = sim.get_tt( ntk.make_signal( g1 ) );
        auto tt_tar_aa0 = ( tt_g0 & tt_g1 );
        auto tt_tar_aa1 = ( ~tt_g0 & tt_g1 );
        auto tt_tar_aa2 = ( tt_g0 & ~tt_g1 );
        auto tt_tar_aa3 = ( ~tt_g0 & ~tt_g1 );

        auto iter0 = badivs.batts_positive.find( binary_and( tt_tar_aa0, care ) );
        auto iter00 = badivs.batts_negative.find( binary_and( tt_tar_aa0, care ) );
        auto iter1 = badivs.batts_positive.find( binary_and( tt_tar_aa1, care ) );
        auto iter11 = badivs.batts_negative.find( binary_and( tt_tar_aa1, care ) );
        auto iter2 = badivs.batts_positive.find( binary_and( tt_tar_aa2, care ) );
        auto iter22 = badivs.batts_negative.find( binary_and( tt_tar_aa2, care ) );
        auto iter3 = badivs.batts_positive.find( binary_and( tt_tar_aa3, care ) );
        auto iter33 = badivs.batts_negative.find( binary_and( tt_tar_aa3, care ) );

        if ( iter0 != badivs.batts_positive.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter0->second.first ) ) ) ? !iter0->second.first : iter0->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter0->second.second ) ) ) ? !iter0->second.second : iter0->second.second );
          return sim.get_phase( root ) ? !ntk.create_xor( ntk.create_and( a, b ), ntk.create_and( c, d ) ) : ntk.create_xor( ntk.create_and( a, b ), ntk.create_and( c, d ) );
        }
        if ( iter00 != badivs.batts_negative.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter00->second.first ) ) ) ? !iter00->second.first : iter00->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter00->second.second ) ) ) ? !iter00->second.second : iter00->second.second );
          return sim.get_phase( root ) ? ntk.create_xor( ntk.create_and( a, b ), ntk.create_and( c, d ) ) : !ntk.create_xor( ntk.create_and( a, b ), ntk.create_and( c, d ) );
        }

        if ( iter1 != badivs.batts_positive.end() )
        {

          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter1->second.first ) ) ) ? !iter1->second.first : iter1->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter1->second.second ) ) ) ? !iter1->second.second : iter1->second.second );
          return sim.get_phase( root ) ? !ntk.create_xor( ntk.create_and( !a, b ), ntk.create_and( c, d ) ) : ntk.create_xor( ntk.create_and( !a, b ), ntk.create_and( c, d ) );
        }
        if ( iter11 != badivs.batts_negative.end() )
        {

          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter11->second.first ) ) ) ? !iter11->second.first : iter11->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter11->second.second ) ) ) ? !iter11->second.second : iter11->second.second );
          return sim.get_phase( root ) ? ntk.create_xor( ntk.create_and( !a, b ), ntk.create_and( c, d ) ) : !ntk.create_xor( ntk.create_and( !a, b ), ntk.create_and( c, d ) );
        }

        if ( iter2 != badivs.batts_positive.end() )
        {

          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter2->second.first ) ) ) ? !iter2->second.first : iter2->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter2->second.second ) ) ) ? !iter2->second.second : iter2->second.second );
          return sim.get_phase( root ) ? !ntk.create_xor( ntk.create_and( a, !b ), ntk.create_and( c, d ) ) : ntk.create_xor( ntk.create_and( a, !b ), ntk.create_and( c, d ) );
        }
        if ( iter22 != badivs.batts_negative.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter22->second.first ) ) ) ? !iter22->second.first : iter22->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter22->second.second ) ) ) ? !iter22->second.second : iter22->second.second );
          return sim.get_phase( root ) ? ntk.create_xor( ntk.create_and( a, !b ), ntk.create_and( c, d ) ) : !ntk.create_xor( ntk.create_and( a, !b ), ntk.create_and( c, d ) );
        }

        if ( iter3 != badivs.batts_positive.end() )
        { 
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter3->second.first ) ) ) ? !iter3->second.first : iter3->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter3->second.second ) ) ) ? !iter3->second.second : iter3->second.second );
          return sim.get_phase( root ) ? !ntk.create_xor( ntk.create_and( !a, !b ), ntk.create_and( c, d ) ) : ntk.create_xor( ntk.create_and( !a, !b ), ntk.create_and( c, d ) );
        }
        if ( iter33 != badivs.batts_negative.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter33->second.first ) ) ) ? !iter33->second.first : iter33->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter33->second.second ) ) ) ? !iter33->second.second : iter33->second.second );
          return sim.get_phase( root ) ? ntk.create_xor( ntk.create_and( !a, !b ), ntk.create_and( c, d ) ) : !ntk.create_xor( ntk.create_and( !a, !b ), ntk.create_and( c, d ) );
        }
      }
    }

    /* 考虑xoo */
    for ( auto i = 0u; i < num_divs; ++i )
    {
      auto const& g0 = divs.at( i );

      for ( auto j = i + 1; j < num_divs; ++j )
      {

        auto const& g1 = divs.at( j );
        auto const& tt_g0 = sim.get_tt( ntk.make_signal( g0 ) );
        auto const& tt_g1 = sim.get_tt( ntk.make_signal( g1 ) );
        auto tt_tar_oo_0 = ( tt_g0 | tt_g1 );
        auto tt_tar_oo_1 = ( ~tt_g0 | tt_g1 );
        auto tt_tar_oo_2 = ( tt_g0 | ~tt_g1 );
        auto tt_tar_oo_3 = ( ~tt_g0 | ~tt_g1 );

        auto iter0 = bodivs.botts_positive.find( binary_and( tt_tar_oo_0, care ) );
        auto iter00 = bodivs.botts_negative.find( binary_and( tt_tar_oo_0, care ) );
        auto iter1 = bodivs.botts_positive.find( binary_and( tt_tar_oo_1, care ) );
        auto iter11 = bodivs.botts_negative.find( binary_and( tt_tar_oo_1, care ) );
        auto iter2 = bodivs.botts_positive.find( binary_and( tt_tar_oo_2, care ) );
        auto iter22 = bodivs.botts_negative.find( binary_and( tt_tar_oo_2, care ) );
        auto iter3 = bodivs.botts_positive.find( binary_and( tt_tar_oo_3, care ) );
        auto iter33 = bodivs.botts_negative.find( binary_and( tt_tar_oo_3, care ) );

        if ( iter0 != bodivs.botts_positive.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter0->second.first ) ) ) ? !iter0->second.first : iter0->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter0->second.second ) ) ) ? !iter0->second.second : iter0->second.second );
          return sim.get_phase( root ) ? !ntk.create_xor( ntk.create_or( a, b ), ntk.create_or( c, d ) ) : ntk.create_xor( ntk.create_or( a, b ), ntk.create_or( c, d ) );
        }
        if ( iter00 != bodivs.botts_negative.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter00->second.first ) ) ) ? !iter00->second.first : iter00->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter00->second.second ) ) ) ? !iter00->second.second : iter00->second.second );
          return sim.get_phase( root ) ? ntk.create_xor( ntk.create_or( a, b ), ntk.create_or( c, d ) ) : !ntk.create_xor( ntk.create_or( a, b ), ntk.create_or( c, d ) );
        }
        if ( iter1 != bodivs.botts_positive.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter1->second.first ) ) ) ? !iter1->second.first : iter1->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter1->second.second ) ) ) ? !iter1->second.second : iter1->second.second );
          return sim.get_phase( root ) ? !ntk.create_xor( ntk.create_or( !a, b ), ntk.create_or( c, d ) ) : ntk.create_xor( ntk.create_or( !a, b ), ntk.create_or( c, d ) );
        }
        if ( iter11 != bodivs.botts_negative.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter11->second.first ) ) ) ? !iter11->second.first : iter11->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter11->second.second ) ) ) ? !iter11->second.second : iter11->second.second );
          return sim.get_phase( root ) ? ntk.create_xor( ntk.create_or( !a, b ), ntk.create_or( c, d ) ) : !ntk.create_xor( ntk.create_or( !a, b ), ntk.create_or( c, d ) );
        }
        if ( iter2 != bodivs.botts_positive.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter2->second.first ) ) ) ? !iter2->second.first : iter2->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter2->second.second ) ) ) ? !iter2->second.second : iter2->second.second );
          return sim.get_phase( root ) ? !ntk.create_xor( ntk.create_or( a, !b ), ntk.create_or( c, d ) ) : ntk.create_xor( ntk.create_or( a, !b ), ntk.create_or( c, d ) );
        }
        if ( iter22 != bodivs.botts_negative.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter22->second.first ) ) ) ? !iter22->second.first : iter22->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter22->second.second ) ) ) ? !iter22->second.second : iter22->second.second );
          return sim.get_phase( root ) ? ntk.create_xor( ntk.create_or( a, !b ), ntk.create_or( c, d ) ) : !ntk.create_xor( ntk.create_or( a, !b ), ntk.create_or( c, d ) );
        }
        if ( iter3 != bodivs.botts_positive.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter3->second.first ) ) ) ? !iter3->second.first : iter3->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter3->second.second ) ) ) ? !iter3->second.second : iter3->second.second );
          return sim.get_phase( root ) ? !ntk.create_xor( ntk.create_or( !a, !b ), ntk.create_or( c, d ) ) : ntk.create_xor( ntk.create_or( !a, !b ), ntk.create_or( c, d ) );
        }
        if ( iter33 != bodivs.botts_negative.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter33->second.first ) ) ) ? !iter33->second.first : iter33->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter33->second.second ) ) ) ? !iter33->second.second : iter33->second.second );
          return sim.get_phase( root ) ? ntk.create_xor( ntk.create_or( !a, !b ), ntk.create_or( c, d ) ) : !ntk.create_xor( ntk.create_or( !a, !b ), ntk.create_or( c, d ) );
        }
      }
    }

    /* 考虑xao */
    for ( auto i = 0u; i < num_divs; ++i )
    {
      auto const& g0 = divs.at( i );

      for ( auto j = i + 1; j < num_divs; ++j )
      {

        auto const& g1 = divs.at( j );
        auto const& tt_g0 = sim.get_tt( ntk.make_signal( g0 ) );
        auto const& tt_g1 = sim.get_tt( ntk.make_signal( g1 ) );
        auto tt_tar_aa0 = ( tt_g0 & tt_g1 );
        auto tt_tar_aa1 = ( ~tt_g0 & tt_g1 );
        auto tt_tar_aa2 = ( tt_g0 & ~tt_g1 );
        auto tt_tar_aa3 = ( ~tt_g0 & ~tt_g1 );

        auto iter0 = bodivs.botts_positive.find( binary_and( tt_tar_aa0, care ) );
        auto iter00 = bodivs.botts_negative.find( binary_and( tt_tar_aa0, care ) );
        auto iter1 = bodivs.botts_positive.find( binary_and( tt_tar_aa1, care ) );
        auto iter11 = bodivs.botts_negative.find( binary_and( tt_tar_aa1, care ) );
        auto iter2 = bodivs.botts_positive.find( binary_and( tt_tar_aa2, care ) );
        auto iter22 = bodivs.botts_negative.find( binary_and( tt_tar_aa2, care ) );
        auto iter3 = bodivs.botts_positive.find( binary_and( tt_tar_aa3, care ) );
        auto iter33 = bodivs.botts_negative.find( binary_and( tt_tar_aa3, care ) );

        if ( iter0 != bodivs.botts_positive.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter0->second.first ) ) ) ? !iter0->second.first : iter0->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter0->second.second ) ) ) ? !iter0->second.second : iter0->second.second );
          return sim.get_phase( root ) ? !ntk.create_xor( ntk.create_and( a, b ), ntk.create_or( c, d ) ) : ntk.create_xor( ntk.create_and( a, b ), ntk.create_or( c, d ) );
        }
        if ( iter00 != bodivs.botts_negative.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter00->second.first ) ) ) ? !iter00->second.first : iter00->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter00->second.second ) ) ) ? !iter00->second.second : iter00->second.second );
          return sim.get_phase( root ) ? ntk.create_xor( ntk.create_and( a, b ), ntk.create_or( c, d ) ) : !ntk.create_xor( ntk.create_and( a, b ), ntk.create_or( c, d ) );
        }
        if ( iter1 != bodivs.botts_positive.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter1->second.first ) ) ) ? !iter1->second.first : iter1->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter1->second.second ) ) ) ? !iter1->second.second : iter1->second.second );
          return sim.get_phase( root ) ? !ntk.create_xor( ntk.create_and( !a, b ), ntk.create_or( c, d ) ) : ntk.create_xor( ntk.create_and( !a, b ), ntk.create_or( c, d ) );
        }
        if ( iter11 != bodivs.botts_negative.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter11->second.first ) ) ) ? !iter11->second.first : iter11->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter11->second.second ) ) ) ? !iter11->second.second : iter11->second.second );
          return sim.get_phase( root ) ? ntk.create_xor( ntk.create_and( !a, b ), ntk.create_or( c, d ) ) : !ntk.create_xor( ntk.create_and( !a, b ), ntk.create_or( c, d ) );
        }
        if ( iter2 != bodivs.botts_positive.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter2->second.first ) ) ) ? !iter2->second.first : iter2->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter2->second.second ) ) ) ? !iter2->second.second : iter2->second.second );
          return sim.get_phase( root ) ? !ntk.create_xor( ntk.create_and( a, !b ), ntk.create_or( c, d ) ) : ntk.create_xor( ntk.create_and( a, !b ), ntk.create_or( c, d ) );
        }
        if ( iter22 != bodivs.botts_negative.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter22->second.first ) ) ) ? !iter22->second.first : iter22->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter22->second.second ) ) ) ? !iter22->second.second : iter22->second.second );
          return sim.get_phase( root ) ? ntk.create_xor( ntk.create_and( a, !b ), ntk.create_or( c, d ) ) : !ntk.create_xor( ntk.create_and( a, !b ), ntk.create_or( c, d ) );
        }
        if ( iter3 != bodivs.botts_positive.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter3->second.first ) ) ) ? !iter3->second.first : iter3->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter3->second.second ) ) ) ? !iter3->second.second : iter3->second.second );
          return sim.get_phase( root ) ? !ntk.create_xor( ntk.create_and( !a, !b ), ntk.create_or( c, d ) ) : ntk.create_xor( ntk.create_and( !a, !b ), ntk.create_or( c, d ) );
        }
        if ( iter33 != bodivs.botts_negative.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter33->second.first ) ) ) ? !iter33->second.first : iter33->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter33->second.second ) ) ) ? !iter33->second.second : iter33->second.second );
          return sim.get_phase( root ) ? ntk.create_xor( ntk.create_and( !a, !b ), ntk.create_or( c, d ) ) : !ntk.create_xor( ntk.create_and( !a, !b ), ntk.create_or( c, d ) );
        }
      }
    }

    /* 考虑xoa */
    for ( auto i = 0u; i < num_divs; ++i )
    {
      auto const& g0 = divs.at( i );

      for ( auto j = i + 1; j < num_divs; ++j )
      {

        auto const& g1 = divs.at( j );
        auto const& tt_g0 = sim.get_tt( ntk.make_signal( g0 ) );
        auto const& tt_g1 = sim.get_tt( ntk.make_signal( g1 ) );
        auto tt_tar_oo_0 = ( tt_g0 | tt_g1 );
        auto tt_tar_oo_1 = ( ~tt_g0 | tt_g1 );
        auto tt_tar_oo_2 = ( tt_g0 | ~tt_g1 );
        auto tt_tar_oo_3 = ( ~tt_g0 | ~tt_g1 );

        auto iter0 = badivs.batts_positive.find( binary_and( tt_tar_oo_0, care ) );
        auto iter00 = badivs.batts_negative.find( binary_and( tt_tar_oo_0, care ) );
        auto iter1 = badivs.batts_positive.find( binary_and( tt_tar_oo_1, care ) );
        auto iter11 = badivs.batts_negative.find( binary_and( tt_tar_oo_1, care ) );
        auto iter2 = badivs.batts_positive.find( binary_and( tt_tar_oo_2, care ) );
        auto iter22 = badivs.batts_negative.find( binary_and( tt_tar_oo_2, care ) );
        auto iter3 = badivs.batts_positive.find( binary_and( tt_tar_oo_3, care ) );
        auto iter33 = badivs.batts_negative.find( binary_and( tt_tar_oo_3, care ) );

        if ( iter0 != badivs.batts_positive.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter0->second.first ) ) ) ? !iter0->second.first : iter0->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter0->second.second ) ) ) ? !iter0->second.second : iter0->second.second );
          return sim.get_phase( root ) ? !ntk.create_xor( ntk.create_or( a, b ), ntk.create_and( c, d ) ) : ntk.create_xor( ntk.create_or( a, b ), ntk.create_and( c, d ) );
        }
        if ( iter00 != badivs.batts_negative.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter00->second.first ) ) ) ? !iter00->second.first : iter00->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter00->second.second ) ) ) ? !iter00->second.second : iter00->second.second );
          return sim.get_phase( root ) ? ntk.create_xor( ntk.create_or( a, b ), ntk.create_and( c, d ) ) : !ntk.create_xor( ntk.create_or( a, b ), ntk.create_and( c, d ) );
        }
        if ( iter1 != badivs.batts_positive.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter1->second.first ) ) ) ? !iter1->second.first : iter1->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter1->second.second ) ) ) ? !iter1->second.second : iter1->second.second );
          return sim.get_phase( root ) ? !ntk.create_xor( ntk.create_or( !a, b ), ntk.create_and( c, d ) ) : ntk.create_xor( ntk.create_or( !a, b ), ntk.create_and( c, d ) );
        }
        if ( iter11 != badivs.batts_negative.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter11->second.first ) ) ) ? !iter11->second.first : iter11->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter11->second.second ) ) ) ? !iter11->second.second : iter11->second.second );
          return sim.get_phase( root ) ? ntk.create_xor( ntk.create_or( !a, b ), ntk.create_and( c, d ) ) : !ntk.create_xor( ntk.create_or( !a, b ), ntk.create_and( c, d ) );
        }
        if ( iter2 != badivs.batts_positive.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter2->second.first ) ) ) ? !iter2->second.first : iter2->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter2->second.second ) ) ) ? !iter2->second.second : iter2->second.second );
          return sim.get_phase( root ) ? !ntk.create_xor( ntk.create_or( a, !b ), ntk.create_and( c, d ) ) : ntk.create_xor( ntk.create_or( a, !b ), ntk.create_and( c, d ) );
        }
        if ( iter22 != badivs.batts_negative.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter22->second.first ) ) ) ? !iter22->second.first : iter22->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter22->second.second ) ) ) ? !iter22->second.second : iter22->second.second );
          return sim.get_phase( root ) ? ntk.create_xor( ntk.create_or( a, !b ), ntk.create_and( c, d ) ) : !ntk.create_xor( ntk.create_or( a, !b ), ntk.create_and( c, d ) );
        }
        if ( iter3 != badivs.batts_positive.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter3->second.first ) ) ) ? !iter3->second.first : iter3->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter3->second.second ) ) ) ? !iter3->second.second : iter3->second.second );
          return sim.get_phase( root ) ? !ntk.create_xor( ntk.create_or( !a, !b ), ntk.create_and( c, d ) ) : ntk.create_xor( ntk.create_or( !a, !b ), ntk.create_and( c, d ) );
        }
        if ( iter33 != badivs.batts_negative.end() )
        {
          auto const a = ( ( sim.get_phase( g0 ) ) ? !ntk.make_signal( g0 ) : ntk.make_signal( g0 ) );
          auto const b = ( ( sim.get_phase( g1 ) ) ? !ntk.make_signal( g1 ) : ntk.make_signal( g1 ) );
          auto const c = ( ( sim.get_phase( ntk.get_node( iter33->second.first ) ) ) ? !iter33->second.first : iter33->second.first );
          auto const d = ( ( sim.get_phase( ntk.get_node( iter33->second.second ) ) ) ? !iter33->second.second : iter33->second.second );
          return sim.get_phase( root ) ? ntk.create_xor( ntk.create_or( !a, !b ), ntk.create_and( c, d ) ) : !ntk.create_xor( ntk.create_or( !a, !b ), ntk.create_and( c, d ) );
        }
      }
    }

    return std::nullopt;
  }

private:
  Ntk& ntk;
  Simulator const& sim;
  std::vector<node> const& divs;
  uint32_t const num_divs;
  stats& st;

  unate_divisors udivs;
  binate_divisors bdivs;
  xor_unate_ttdivs uxdivs;
  xor_binate_ttdivs bxdivs;
  and_binate_ttdivs badivs;
  or_binate_ttdivs bodivs;

}; /* xag_resub_functor_bd */

template<class Ntk>
void resubstitution_xag_bdiff( Ntk& ntk, resubstitution_params const& ps = {}, resubstitution_stats* pst = nullptr )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_clear_values_v<Ntk>, "Ntk does not implement the clear_values method" );
  static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );
  static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the ntk.get_node method" );
  static_assert( has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented method" );
  static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  static_assert( has_make_signal_v<Ntk>, "Ntk does not implement the make_signal method" );
  static_assert( has_set_value_v<Ntk>, "Ntk does not implement the set_value method" );
  static_assert( has_set_visited_v<Ntk>, "Ntk does not implement the set_visited method" );
  static_assert( has_size_v<Ntk>, "Ntk does not implement the has_size method" );
  static_assert( has_substitute_node_v<Ntk>, "Ntk does not implement the has substitute_node method" );
  static_assert( has_value_v<Ntk>, "Ntk does not implement the has_value method" );
  static_assert( has_visited_v<Ntk>, "Ntk does not implement the has_visited method" );

  using resub_view_t = fanout_view<depth_view<Ntk>>;
  depth_view<Ntk> depth_view{ ntk };
  resub_view_t resub_view{ depth_view };

  using truthtable_t = kitty::dynamic_truth_table;
  using mffc_result_t = std::pair<uint32_t, uint32_t>;
  using resub_impl_t = detail::resubstitution_impl<resub_view_t,
                                                   typename detail::window_based_resub_engine<resub_view_t, truthtable_t, truthtable_t,
                                                                                              xag_resub_functor_bd<resub_view_t, typename detail::window_simulator<resub_view_t, truthtable_t>, truthtable_t>, mffc_result_t>,
                                                   typename detail::default_divisor_collector<resub_view_t, typename detail::node_mffc_inside_xag_bd<resub_view_t>, mffc_result_t>>;

  resubstitution_stats st;
  typename resub_impl_t::engine_st_t engine_st;
  typename resub_impl_t::collector_st_t collector_st;

  resub_impl_t p( resub_view, ps, st, engine_st, collector_st );
  p.run();

  if ( ps.verbose )
  {
    st.report();
    collector_st.report();
    engine_st.report();
  }

  if ( pst )
  {
    *pst = st;
  }
}

} // namespace mockturtle
