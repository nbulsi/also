#ifndef XAG_CHOICE_COMPUTE_HPP
#define XAG_CHOICE_COMPUTE_HPP
#pragma once

#include <kitty/partial_truth_table.hpp>
#include <mockturtle/mockturtle.hpp>
#include <optional>
#include <unordered_map>
#include <vector>

using namespace mockturtle;
using namespace mockturtle::detail;

namespace also
{
struct xag_choice_compute_params
{
  xag_choice_compute_params()
  {
    cut_enumeration_ps.cut_size = 4;
    cut_enumeration_ps.cut_limit = 10;
    cut_enumeration_ps.minimize_truth_table = true;
  }
  cut_enumeration_params cut_enumeration_ps{};

  uint32_t min_cand_cut_size{ 2u };
  uint32_t max_pis{ 12 };
  float ratio{ 0.8 };

  bool use_reconvergence_cut{ true };
  bool verbose{ false };
};
struct xag_choice_compute_stats
{
};
namespace detail
{
struct xag_cut_enumeration_choice_compute_cut
{
  int32_t gain{ -1 };
};
template<class Ntk, class RewritingFn, class RewritingFn2, class Library,
         class RefactoringFn, class RefactoringFn2, class NodeCostFn>
class xag_choice_compute_manager
{
public:
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;
  static constexpr node XAG_NULL = xag_network::XAG_NULL;
  static constexpr uint32_t num_vars = 4u;

#define XAG_SIGNAL_NULL ( signal( XAG_NULL, 0 ) )
  xag_choice_compute_manager( Ntk& ntk, RewritingFn&& rewriting_fn,
                              RewritingFn2&& rewriting_fn2, Library&& library,
                              RefactoringFn&& refactoring_fn,
                              RefactoringFn2&& refactoring_fn2,
                              xag_choice_compute_params const& ps,
                              xag_choice_compute_stats& st,
                              NodeCostFn const& cost_fn )
      : ntk( ntk ),
        rewriting_fn( rewriting_fn ),
        rewriting_fn2( rewriting_fn2 ),
        library( library ),
        refactoring_fn( refactoring_fn ),
        refactoring_fn2( refactoring_fn2 ),
        ps( ps ),
        st( st ),
        cost_fn( cost_fn ) {}
  Ntk run()
  {
    const auto cuts =
        cut_enumeration<Ntk, true, xag_cut_enumeration_choice_compute_cut>(
            ntk, ps.cut_enumeration_ps );
    ntk.clear_values();
    reconvergence_driven_cut_parameters rps;
    rps.max_leaves = ps.max_pis;
    reconvergence_driven_cut_statistics rst;
    mockturtle::detail::reconvergence_driven_cut_impl<Ntk, false, false>
        reconv_cuts( ntk, rps, rst );
    ntk.foreach_node(
        [&]( auto const& n )
        { ntk.set_value( n, ntk.fanout_size( n ) ); } );
    node_map<std::vector<signal>, Ntk> best_replacements( ntk );
    const auto size = ntk.size();
    auto max_total_gain = 0u;
    std::array<signal, num_vars> leaves;
    std::array<uint8_t, num_vars> permutation;
    auto& db = library.get_database();

    critical_path = get_critical_path();
    for ( node const& n : critical_path )
    {
      if ( ntk.is_constant( n ) || ntk.is_pi( n ) || mffc_size( ntk, n ) == 1 )
        continue;

      for ( auto& cut : cuts.cuts( ntk.node_to_index( n ) ) )
      {
        if ( cut->size() < ps.min_cand_cut_size )
          continue;
        const auto tt_cut = cuts.truth_table( *cut );
        const kitty::static_truth_table<4> fe = kitty::extend_to<4>( tt_cut );
        // exact library rewrite function
        auto config = kitty::exact_npn_canonization( fe );
        auto tt_npn = std::get<0>( config );
        auto neg = std::get<1>( config );
        auto perm = std::get<2>( config );

        auto const structures = library.get_supergates( tt_npn );
        if ( structures == nullptr )
        {
          continue;
        }
        uint32_t negation = 0;
        for ( auto j = 0u; j < num_vars; ++j )
        {
          permutation[perm[j]] = j;
          negation |= ( ( neg >> perm[j] ) & 1 ) << j;
        }

        {
          auto j = 0u;
          for ( auto const leaf : *cut )
          {
            leaves[permutation[j++]] = ntk.make_signal( ntk.index_to_node( leaf ) );
          }
          while ( j < num_vars )
            leaves[permutation[j++]] = ntk.get_constant( false );
        }

        for ( auto j = 0u; j < num_vars; ++j )
        {
          if ( ( negation >> j ) & 1 )
          {
            leaves[j] = !leaves[j];
          }
        }
        {
          for ( auto const& dag : *structures )
          {
            auto [nodes_added, level] =
                evaluate_entry( n, db.get_node( dag.root ), leaves );

            /* discard if dag.root and n are the same */
            if ( ntk.node_to_index( n ) == db.value( db.get_node( dag.root ) ) >> 1 )
              continue;

            /* restore contained MFFC */
            // measure_mffc_ref(n, cut);

            mockturtle::topo_view topo{ db, dag.root };

            auto new_f_rewrite =
                cleanup_dangling( topo, ntk, leaves.begin(), leaves.end() )
                    .front();
            node equiv0 = ntk.get_node( new_f_rewrite );
            if ( equiv0 != n && ntk.fanout_size( n ) > 0 )
            {
              ntk.add_choice( n, equiv0 );
            }
          }
        }
        // cut rewrite function
        std::vector<signal> children;
        for ( auto l : *cut )
        {
          children.push_back( ntk.make_signal( ntk.index_to_node( l ) ) );
        }
        int32_t value = recursive_deref<Ntk, NodeCostFn>( ntk, n );
        {
          int32_t best_gain{ -1 };
          const auto on_signal = [&]( auto const& f_new )
          {
            auto [v, contains] = recursive_ref_contains( ntk.get_node( f_new ), n );
            recursive_deref<Ntk, NodeCostFn>( ntk, ntk.get_node( f_new ) );
            int32_t gain = contains ? -1 : value - v;
            if ( best_gain == -1 )
            {
              ( *cut )->data.gain = best_gain = gain;
              best_replacements[n].push_back( f_new );
            }
            else if ( gain > best_gain )
            {
              ( *cut )->data.gain = best_gain = gain;
              best_replacements[n].back() = f_new;
            }
            return true;
          };
          rewriting_fn( ntk, cuts.truth_table( *cut ), children.begin(),
                        children.end(), on_signal );
        }
      }
      for ( auto s : best_replacements[n] )
      {
        node equiv1 = ntk.get_node( s );
        if ( equiv1 != n && ntk.fanout_size( n ) > 0 )
        {
          ntk.add_choice( n, equiv1 );
        }
      }
      continue;
    }
    std::unordered_set<node> critical_path_set( critical_path.begin(),
                                                critical_path.end() );
    ntk.foreach_node( [&]( node const& n, auto index )
                      {
      // rewriting function
      if (index >= size) return false;
      if (ntk.fanout_size(n) == 0u) {
        return true;
      }
      if (ntk.is_constant(n) || ntk.is_pi(n)) return true;
      if (mockturtle::detail::mffc_size(ntk, n) == 1) return true;
      if (critical_path_set.find(n) != critical_path_set.end()) {
        return true;  // 跳过 critical_path 中的节点
      }

      for (auto& cut : cuts.cuts(ntk.node_to_index(n))) {
        if (cut->size() < ps.min_cand_cut_size) continue;
        const auto tt_cut = cuts.truth_table(*cut);
        const kitty::static_truth_table<4> fe = kitty::extend_to<4>(tt_cut);
        std::vector<signal> children;
        for (auto l : *cut) {
          children.push_back(ntk.make_signal(ntk.index_to_node(l)));
        }
        int32_t value = recursive_deref<Ntk, NodeCostFn>(ntk, n);
        {
          int32_t best_gain{-1};
          const auto on_signal = [&](auto const& f_new) {
            auto [v, contains] = recursive_ref_contains(ntk.get_node(f_new), n);
            recursive_deref<Ntk, NodeCostFn>(ntk, ntk.get_node(f_new));
            int32_t gain = contains ? -1 : value - v;
            if (best_gain == -1) {
              (*cut)->data.gain = best_gain = gain;
              best_replacements[n].push_back(f_new);
            } else if (gain > best_gain) {
              (*cut)->data.gain = best_gain = gain;
              best_replacements[n].back() = f_new;
            }
            return true;
          };
          rewriting_fn2(ntk, cuts.truth_table(*cut), children.begin(),
                        children.end(), on_signal);
        }
      }
      for (auto s : best_replacements[n]) {
        node equiv3 = ntk.get_node(s);
        if (equiv3 != n && ntk.fanout_size(n) > 0) {
          ntk.add_choice(n, equiv3);
        }
      }

      // refactor function
      ntk.set_value(n, ntk.fanout_size(n));
      const auto mffc = mffc_view<Ntk>(ntk, n);
      if (mffc.num_pos() == 0 ||
          (!ps.use_reconvergence_cut && mffc.num_pis() > ps.max_pis) ||
          mffc.size() < 4) {
        return true;
      }

      kitty::dynamic_truth_table tt;
      std::vector<signal> leaves(ps.max_pis);
      uint32_t num_leaves = 0;

      if (mffc.num_pis() <= ps.max_pis) {
        mffc.foreach_pi(
            [&](auto const& m, auto j) { leaves[j] = ntk.make_signal(m); });

        num_leaves = mffc.num_pis();

        default_simulator<kitty::dynamic_truth_table> sim(mffc.num_pis());
        tt = simulate<kitty::dynamic_truth_table>(mffc, sim)[0];
      } else {
        std::vector<node> roots = {n};
        auto const extended_leaves = reconv_cuts.run(roots).first;

        num_leaves = extended_leaves.size();
        assert(num_leaves <= ps.max_pis);

        for (auto j = 0u; j < num_leaves; ++j) {
          leaves[j] = ntk.make_signal(extended_leaves[j]);
        }
        cut_view<Ntk> cut(ntk, extended_leaves, ntk.make_signal(n));
        default_simulator<kitty::dynamic_truth_table> sim(num_leaves);
        tt = simulate<kitty::dynamic_truth_table>(cut, sim)[0];
      }
      signal new_f;
      bool resynthesized{false};

      ntk.incr_trav_id();
      // int32_t gain = recursive_deref_mark(n);
      // if constexpr (has_refactoring_with_dont_cares_v<
      //                   Ntk, RefactoringFn, decltype(leaves.begin())>) {
      //   std::vector<node> pivots;
      //   for (auto const& c : leaves) {
      //     pivots.push_back(ntk.get_node(c));
      //   }
      //   refactoring_fn(ntk, tt, satisfiability_dont_cares(ntk, pivots, 16u),
      //                  leaves.begin(), leaves.begin() + num_leaves,
      //                  [&](auto const& f) {
      //                    new_f = f;
      //                    resynthesized = true;
      //                    return false;
      //                  });
      // } else {
      refactoring_fn(ntk, tt, leaves.begin(), leaves.begin() + num_leaves,
                     [&](auto const& f) {
                       new_f = f;
                       resynthesized = true;
                       return false;
                     });
      //}

      // refactoring_fn(ntk, tt, leaves.begin(), leaves.begin() + num_leaves,
      //                [&](auto const& f) {
      //                  new_f = f;
      //                  resynthesized = true;
      //                  return false;
      //                });
      node equiv4 = ntk.get_node(new_f);
      // node root = n;
      if (equiv4 != n && ntk.fanout_size(n) > 0) {
        ntk.add_choice(n, equiv4);
      }

      
        refactoring_fn2(ntk, tt, leaves.begin(), leaves.begin() + num_leaves,
                        [&](auto const& f) {
                          new_f = f;
                          resynthesized = true;
                          return false;
                        });
        node equiv5 = ntk.get_node(new_f);

        if (equiv5 != n && ntk.fanout_size(n) > 0) {
          ntk.add_choice(n, equiv5);
        }
      
      return true; } );
    auto xwc = derive_choice_xag();
    return xwc;
  }

private:
  std::pair<int32_t, bool> recursive_ref_contains( node const& n,
                                                   node const& repl )
  {
    if ( ntk.is_constant( n ) || ntk.is_pi( n ) )
      return { 0, false };

    int32_t value = cost_fn( ntk, n );
    bool contains = ( n == repl );
    ntk.foreach_fanin( n, [&]( auto const& s )
                       {
      contains = contains || (ntk.get_node(s) == repl);
      if (ntk.incr_value(ntk.get_node(s)) == 0) {
        const auto [v, c] = recursive_ref_contains(ntk.get_node(s), repl);
        value += v;
        contains = contains || c;
      } } );
    return { value, contains };
  }

  std::vector<node> get_critical_path()
  {
    int depth = depth_ntk.depth();
    depth_ntk.clear_visited();
    depth_ntk.foreach_po( [&]( auto const& po, auto i )
                          {
      node out_put = depth_ntk.get_node(po);
      if (depth_ntk.level(out_put) < depth * ps.ratio) return;
      recursive(out_put, critical_path, i); } );
    return critical_path;
  }

  std::vector<node> recursive( node& n, std::vector<node>& critical, int i )
  {
    if ( depth_ntk.visited( n ) != 0 )
      return critical;
    depth_ntk.set_visited( n, i + 1 );
    critical.emplace_back( n );
    depth_ntk.foreach_fanin( n, [&]( auto fi )
                             {
      node in_put = depth_ntk.get_node(fi);
      int level = depth_ntk.level(n);
      if (depth_ntk.level(in_put) == level - 1) {
        recursive(in_put, critical, i);
      } } );
    return critical;
  }
  inline std::pair<int32_t, uint32_t> evaluate_entry(
      node const& current_root, node const& n,
      std::array<signal, num_vars> const& leaves )
  {
    auto& db = library.get_database();
    db.incr_trav_id();

    return evaluate_entry_rec( current_root, n, leaves );
  }

  std::pair<int32_t, uint32_t> evaluate_entry_rec(
      node const& current_root, node const& n,
      std::array<signal, num_vars> const& leaves )
  {
    auto& db = library.get_database();
    if ( db.is_pi( n ) || db.is_constant( n ) )
      return { 0, 0 };
    if ( db.visited( n ) == db.trav_id() )
      return { 0, 0 };

    db.set_visited( n, db.trav_id() );

    int32_t area = 0;
    uint32_t level = 0;
    bool hashed = true;

    std::array<signal, Ntk::max_fanin_size> node_data;
    db.foreach_fanin( n, [&]( auto const& f, auto i )
                      {
      node g = db.get_node(f);
      if (db.is_constant(g)) {
        node_data[i] = f; /* ntk.get_costant( db.is_complemented( f ) ) */
        return;
      }
      if (db.is_pi(g)) {
        node_data[i] = leaves[db.node_to_index(g) - 1] ^ db.is_complemented(f);
        if constexpr (has_level_v<Ntk>) {
          level = std::max(
              level, ntk.level(ntk.get_node(leaves[db.node_to_index(g) - 1])));
        }
        return;
      }

      auto [area_rec, level_rec] = evaluate_entry_rec(current_root, g, leaves);
      area += area_rec;
      level = std::max(level, level_rec);

      /* check value */
      if (db.value(g) < UINT32_MAX) {
        signal s;
        s.data = static_cast<uint64_t>(db.value(g));
        node_data[i] = s ^ db.is_complemented(f);
      } else {
        hashed = false;
      } } );

    if ( hashed )
    {
      /* try hash */
      /* AIG, XAG, MIG, and XAG are supported now */
      std::optional<signal> val;
      do
      {
        /* XAG */
        if constexpr ( has_has_and_v<Ntk> && has_has_xor_v<Ntk> )
        {
          if ( db.is_and( n ) )
            val = ntk.has_and( node_data[0], node_data[1] );
          else
            val = ntk.has_xor( node_data[0], node_data[1] );
          break;
        }

        /* AIG */
        if constexpr ( has_has_and_v<Ntk> )
        {
          val = ntk.has_and( node_data[0], node_data[1] );
          break;
        }

        /* XMG */
        if constexpr ( has_has_maj_v<Ntk> && has_has_xor3_v<Ntk> )
        {
          if ( db.is_maj( n ) )
            val = ntk.has_maj( node_data[0], node_data[1], node_data[2] );
          else
            val = ntk.has_xor3( node_data[0], node_data[1], node_data[2] );
          break;
        }

        /* MAJ */
        if constexpr ( has_has_maj_v<Ntk> )
        {
          val = ntk.has_maj( node_data[0], node_data[1], node_data[2] );
          break;
        }

      } while ( false );

      if ( val.has_value() )
      {
        /* bad condition (current root is contained in the DAG): return a
        very
         * high cost */
        if ( db.get_node( *val ) == current_root )
          return { UINT32_MAX / 2, level + 1 };

        /* annotate hashing info */
        db.set_value( n, val->data );
        return {
            area +
                ( ntk.fanout_size( ntk.get_node( *val ) ) > 0 ? 0 : cost_fn( ntk, n ) ),
            level + 1 };
      }
    }

    db.set_value( n, UINT32_MAX );
    return { area + cost_fn( ntk, n ), level + 1 };
  }

  Ntk derive_choice_xag()
  {
    ntk.clear_values();
    ntk.init_choices( ntk.size() );
    Ntk choice_xag;
    choice_xag.clear_values();
    choice_xag.init_choices( ntk.size() );
    std::vector<signal> old2new( ntk.size(), XAG_SIGNAL_NULL );
    old2new[0] = ntk.get_constant( false );
    ntk.foreach_pi( [&]( node const& i )
                    {
      choice_xag.create_pi();
      old2new[i] = signal(i, 0); } );
    auto get_repr_signal = [&]( signal const& s )
    {
      auto repr = choice_xag.get_repr( s.index );
      return signal( repr, choice_xag.phase( repr ) ^ choice_xag.phase( s.index ) ^
                               s.complement );
    };
    ntk.foreach_gate( [&]( node const& n )
                      {
      node repr = ntk.get_choice_representative(n);
      // for const and PI
      if (ntk.is_constant(repr) || ntk.is_ci(repr)) {
        old2new[n] = old2new[repr] ^ (ntk.phase(n) ^ ntk.phase(repr));
        return;
      }
      // get the new node
      auto old_child0 = ntk.get_child0(n);
      auto old_child1 = ntk.get_child1(n);
      auto new_child0 = old2new[old_child0.index] ^ old_child0.complement;
      auto new_child1 = old2new[old_child1.index] ^ old_child1.complement;
      if (ntk.is_and(n)) {
        auto new_node = choice_xag.create_and(get_repr_signal(new_child0),
                                              get_repr_signal(new_child1));
        while (true) {
          auto new2 = new_node;
          new_node = get_repr_signal(new2);
          if (new_node == new2) break;
        }
        assert(old2new[n] == XAG_SIGNAL_NULL);
        old2new[n] = new_node;
        // skip those without reprs
        if (repr == n) {
          return;
        }
        assert(repr < n);
        // get the corresponding new nodes
        auto new_repr = old2new[repr];
        // skip earlier nodes
        if (new_repr.index >= new_node.index) return;

        if (choice_xag.fanout_size(new_node.index) != 0) return;
        choice_xag.set_choice(new_node.index, new_repr.index);

      } else {
        auto new_node = choice_xag.create_xor(get_repr_signal(new_child0),
                                              get_repr_signal(new_child1));
        while (true) {
          auto new2 = new_node;
          new_node = get_repr_signal(new2);
          if (new_node == new2) break;
        }
        assert(old2new[n] == XAG_SIGNAL_NULL);
        old2new[n] = new_node;
        // skip those without reprs
        if (repr == n) {
          return;
        }
        assert(repr < n);
        // get the corresponding new nodes
        auto new_repr = old2new[repr];
        // skip earlier nodes
        if (new_repr.index >= new_node.index) return;

        if (choice_xag.fanout_size(new_node.index) != 0) return;
        choice_xag.set_choice(new_node.index, new_repr.index);
      } } );
    ntk.foreach_co( [&]( signal const& o )
                    {
      auto new_signal = old2new[o.index] ^ o.complement;
      auto new_repr = choice_xag.get_repr(new_signal.index);
      choice_xag.create_po(signal(new_repr, choice_xag.phase(new_signal.index) ^
                                                choice_xag.phase(new_repr) ^
                                                new_signal.complement)); } );
    return xag_man_dup_dfs( choice_xag );
  }

  Ntk xag_man_dup_dfs( Ntk const& tmp )
  {
    Ntk new_ntk;
    new_ntk.clear_values();
    new_ntk.init_choices( tmp.size() );
    std::vector<signal> old2new( tmp.size(), XAG_SIGNAL_NULL );
    old2new[0] = tmp.get_constant( false );
    tmp.foreach_ci( [&]( node const& i )
                    {
      new_ntk.create_pi();
      old2new[i] = signal(i, 0); } );

    tmp.foreach_co( [&]( signal const& o )
                    {
      xag_man_dup_dfs_rec(tmp, new_ntk, old2new, o.index);
      new_ntk.create_po(old2new[o.index] ^ o.complement); } );
    return new_ntk;
  }

  static signal xag_man_dup_dfs_rec( Ntk const& tmp, Ntk& new_ntk,
                                     std::vector<signal>& old2new,
                                     node const& n )
  {
    if ( old2new[n] != XAG_SIGNAL_NULL )
      return old2new[n];
    signal new_equiv = signal( XAG_NULL, 0 );
    if ( tmp.get_equiv_node( n ) != XAG_NULL )
    {
      new_equiv =
          xag_man_dup_dfs_rec( tmp, new_ntk, old2new, tmp.get_equiv_node( n ) );
    }
    auto old_child0 = tmp.get_child0( n );
    auto old_child1 = tmp.get_child1( n );

    xag_man_dup_dfs_rec( tmp, new_ntk, old2new, old_child0.index );
    xag_man_dup_dfs_rec( tmp, new_ntk, old2new, old_child1.index );
    auto new_child0 = old2new[old_child0.index] ^ old_child0.complement;
    auto new_child1 = old2new[old_child1.index] ^ old_child1.complement;
    if ( tmp.is_and( n ) )
    {
      signal new_node = new_ntk.create_and( new_child0, new_child1 );
      if ( new_equiv != XAG_SIGNAL_NULL && new_equiv.index < new_node.index )
      {
        new_ntk.set_equiv( new_node.index, new_equiv.index );
      }
      old2new[n] = new_node;
      return new_node;
    }
    else
    {
      signal new_node = new_ntk.create_xor( new_child0, new_child1 );
      if ( new_equiv != XAG_SIGNAL_NULL && new_equiv.index < new_node.index )
      {
        new_ntk.set_equiv( new_node.index, new_equiv.index );
      }
      old2new[n] = new_node;
      return new_node;
    }
  }

private:
  Ntk& ntk;
  RewritingFn&& rewriting_fn;
  RewritingFn2&& rewriting_fn2;
  RefactoringFn&& refactoring_fn;
  RefactoringFn2&& refactoring_fn2;
  Library&& library;

  xag_choice_compute_params const& ps;
  xag_choice_compute_stats& st;
  NodeCostFn cost_fn;
  mockturtle::depth_view<Ntk> depth_ntk{ ntk };
  std::vector<node> critical_path;
};
} // namespace detail
template<class Ntk, class RewritingFn, class RewritingFn2, class Library,
         class RefactoringFn, class RefactoringFn2,
         class NodeCostFn = unit_cost<Ntk>>

Ntk xag_choice_compute( Ntk& ntk, RewritingFn&& rewriting_fn,
                        RewritingFn2&& rewriting_fn2, Library&& library,
                        RefactoringFn&& refactoring_fn,
                        RefactoringFn2&& refactoring_fn2,
                        xag_choice_compute_params const& ps = {},
                        xag_choice_compute_stats* pst = nullptr,
                        NodeCostFn const& cost_fn = {} )
{
  xag_choice_compute_stats st;
  const auto res = [&]()
  {
    return also::detail::xag_choice_compute_manager<
               Ntk, RewritingFn, RewritingFn2, Library, RefactoringFn,
               RefactoringFn2, NodeCostFn>( ntk, rewriting_fn, rewriting_fn2,
                                            library, refactoring_fn,
                                            refactoring_fn2, ps, st, cost_fn )
        .run();
  }();
  return res;
}
} // namespace also
#endif