/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file fanin_histogram.hpp
 *
 * @brief statistic fanin_histogram
 *
 * @author Chunliu Liao
 * @since  0.1
 */

#include <string>
#include <vector>
#include <alice/alice.hpp>
#include <mockturtle/mockturtle.hpp>
namespace also
{
using aig_names = mockturtle::names_view<mockturtle::aig_network>;
using mig_names = mockturtle::names_view<mockturtle::mig_network>;
using xag_names = mockturtle::names_view<mockturtle::xag_network>;
using xmg_names = mockturtle::names_view<mockturtle::xmg_network>;

struct function_counts
{
  int maj_num = 0;
  int xor_num = 0;
  int xnor_num = 0;
  int xor3_num = 0;
  int and_num = 0;
  int or_num = 0;
  int input_num = 0;
  int unknown_num = 0;
};
enum reconfig_function
{
  AND,
  OR,
  XOR,
  XNOR,
  XOR3,
  MAJ,
  INPUT,
  UNKNOWN
};
template<typename network>
reconfig_function node_function( const network& ntk, const typename network::node& node )
{

  if ( ntk.is_pi( node ) || ntk.is_constant( node ) )
  {
    return reconfig_function::INPUT;
  }
  else if ( ntk.is_and( node ) )
  {
    return reconfig_function::AND;
  }
  else if ( ntk.is_or( node ) )
  {
    return reconfig_function::OR;
  }
  else if ( ntk.is_xor( node ) )
  {
    return reconfig_function::XOR;
  }
  else if ( ntk.is_maj( node ) )
  {
    typename network::signal first_signal = ntk._storage->nodes[node].children[0];
    typename network::node first_fanin = ntk.get_node( first_signal );

    if ( ntk.is_constant( first_fanin ) )
    {
      if ( first_signal.complement )
      {
        return reconfig_function::OR;
      }
      else
      {
        return reconfig_function::AND;
      }
    }
    else
    {
      return reconfig_function::MAJ;
    }
  }
  else if ( ntk.is_xor3( node ) )
  {
    typename network::signal first_signal = ntk._storage->nodes[node].children[0];
    typename network::node first_fanin = ntk.get_node( first_signal );
    if ( ntk.is_constant( first_fanin ) )
    {
      if ( first_signal.complement )
      {
        return reconfig_function::XNOR;
      }
      else
      {
        return reconfig_function::XOR;
      }
    }
    else
    {
      return reconfig_function::XOR3;
    }
  }
  else
  {
    return reconfig_function::UNKNOWN;
  }
}
template<typename network>
void update_counts( function_counts& counts, const network& ntk, const typename network::node& node )
{
  reconfig_function func = node_function( ntk, node );
  switch ( func )
  {
  case AND:
    counts.and_num++;
    break;
  case OR:
    counts.or_num++;
    break;
  case XOR:
    counts.xor_num++;
    break;
  case XNOR:
    counts.xnor_num++;
    break;
  case XOR3:
    counts.xor3_num++;
    break;
  case MAJ:
    counts.maj_num++;
    break;
  case INPUT:
    counts.input_num++;
    break;
  case UNKNOWN:
  default:
    counts.unknown_num++;
  }
}
// template void update_counts<aig_names>( function_counts&, const aig_names&, const typename aig_names::node& );
// template void update_counts<mig_names>( function_counts&, const mig_names&, const typename mig_names::node& );
// template void update_counts<xmg_names>( function_counts&, const xmg_names&, const typename xmg_names::node& );
// template void update_counts<xag_names>( function_counts&, const xag_names&, const typename xag_names::node& );
template<typename network>
function_counts node_functions( const network& ntk )
{
  function_counts counts;
  ntk.foreach_node( [&]( auto node )
                    { update_counts( counts, ntk, node ); } );
  return counts;
}
// template function_counts node_functions<mig_names>( const mig_names& );
// template function_counts node_functions<xmg_names>( const xmg_names& );
// template function_counts node_functions<aig_names>( const aig_names& );
// template function_counts node_functions<xag_names>( const xag_names& );
template<typename Ntk>
class critical_node_view : public Ntk
{
public:
  using storage = typename Ntk::storage;
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

public:
  critical_node_view() {}
  explicit critical_node_view( Ntk const& ntk ) : Ntk( ntk ), ntk( ntk )
  {
    int depth = depth_ntk.depth();
    depth_ntk.clear_visited();
    nums_criti = 0;
    depth_ntk.foreach_po( [&]( auto const& po, auto i )
                          {
              node pon = depth_ntk.get_node(po);
              if (depth_ntk.level(pon) != depth)
                return;
              nums_criti++;
              recursive(pon,critical_path,i); } );
  }
  std::vector<node> recursive( node& n, std::vector<node>& critical, int i )
  {
    if ( depth_ntk.visited( n ) != 0 )
      return critical;
    depth_ntk.set_visited( n, i + 1 );
    critical.emplace_back( n );
    depth_ntk.foreach_fanin( n, [&]( auto fi )
                             {
            node fin = depth_ntk.get_node(fi);
            int level = depth_ntk.level(n);
            if (depth_ntk.level(fin) == level - 1){
              recursive(fin,critical,i);
            } } );
    return critical;
  }
  std::vector<node> get_critical_path()
  {
    return critical_path;
  }
  uint32_t get_critical_nums()
  {
    return nums_criti;
  }

private:
  uint32_t nums_criti;
  std::vector<node> critical_path;
  Ntk const& ntk;
  mockturtle::depth_view<Ntk> depth_ntk{ ntk };
};
static void compute_cone( mockturtle::aig_network aig, uint64_t index, std::unordered_map<int, int>& nodes, int outindex, std::unordered_map<int, int>& ins )
{
  if ( aig._storage->nodes[index].data[1].h1 == 0 )
  {

    // increment number of nodes in this cone
    std::unordered_map<int, int>::iterator it = nodes.find( outindex );

    if ( it != nodes.end() && index > aig.num_pis() )
    {
      // increment the number of nodes
      it->second++;
    }

    // set node as visited
    aig._storage->nodes[index].data[1].h1 = 1;

    // traverse one side to the PIs
    if ( !aig.is_pi( aig._storage->nodes[index].children[0].index ) && index > aig.num_pis() )
    {
      if ( aig._storage->nodes[index].children[0].data & 1 )
        aig._storage->nodes[index].children[0].data =
            aig._storage->nodes[index].children[0].data - 1;

      // calculate input node index
      auto inIndex = aig._storage->nodes[index].children[0].data >> 1;

      // im ignoring latches
      if ( inIndex > aig.num_pis() )
      {
        // call recursion
        compute_cone( aig, inIndex, nodes, outindex, ins );
      }
    }

    // traverse the other side to the PIs
    if ( !aig.is_pi( aig._storage->nodes[index].children[1].index ) && index > aig.num_pis() )
    {
      if ( aig._storage->nodes[index].children[1].data & 1 )
        aig._storage->nodes[index].children[1].data =
            aig._storage->nodes[index].children[1].data - 1;

      // calculate input node index
      auto inIndex = aig._storage->nodes[index].children[1].data >> 1;

      // im ignoring latches
      if ( inIndex > aig.num_pis() )
      {
        // call recursion
        compute_cone( aig, inIndex, nodes, outindex, ins );
      }
    }

    // if my child is PI and was not visited yet, I increase the input counter
    if ( aig.is_ci( aig._storage->nodes[index].children[0].index ) && aig._storage->nodes[aig._storage->nodes[index].children[0].index].data[1].h1 == 0 )
    {
      aig._storage->nodes[aig._storage->nodes[index].children[0].index].data[1].h1 =
          1;

      std::unordered_map<int, int>::iterator it = ins.find( outindex );
      if ( it != ins.end() )
      {
        // increment the number of inputs
        it->second++;
      }
    }

    // if my other child is PI and was not visited yet, I also increase the input counter
    if ( aig.is_ci( aig._storage->nodes[index].children[1].index ) && aig._storage->nodes[aig._storage->nodes[index].children[1].index].data[1].h1 == 0 )
    {
      aig._storage->nodes[aig._storage->nodes[index].children[1].index].data[1].h1 =
          1;

      std::unordered_map<int, int>::iterator it = ins.find( outindex );
      if ( it != ins.end() )
      {
        // increment the number of inputs
        it->second++;
      }
    }
  }
}
template<typename Ntk>
static int computeLevel( Ntk const& ntk, int index )
{
  // if node not visited
  if ( ntk._storage->nodes[index].data[1].h1 == 0 )
  {

    // set node as visited
    ntk._storage->nodes[index].data[1].h1 = 1;

    // if is input
    if ( ntk.is_ci( index ) )
    {
      return 0;
    }

    auto inIdx2 = ntk._storage->nodes[index].children[1].data;
    if ( inIdx2 & 1 )
      inIdx2 = inIdx2 - 1;

    // calculate input node index
    auto inNode1 = inIdx2 >> 1;
    int levelNode1 = computeLevel<Ntk>( ntk, inNode1 );

    auto inIdx = ntk._storage->nodes[index].children[0].data;
    if ( inIdx & 1 )
      inIdx = inIdx - 1;

    // calculate input node index
    auto inNode0 = inIdx >> 1;
    int levelNode0 = computeLevel<Ntk>( ntk, inNode0 );

    int level = 1 + std::max( levelNode0, levelNode1 );
    return level;
  }
  return 0;
}
} // namespace also
