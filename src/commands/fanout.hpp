#ifndef FANOUT_HPP
#define FANOUT_HPP
#include <iostream>
#include <map>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/traits.hpp>
#include <mockturtle/views/depth_view.hpp>
#include <mockturtle/views/fanout_view.hpp>
#include <set>
#include <unordered_map>
#include <vector>

using namespace mockturtle;

namespace alice
{
class fanout_command : public command
{
public:
  explicit fanout_command( const environment::ptr& env ) : command( env, "memristor_costs" )
  {
    add_flag( "-c, --costs", "Show memristor costs" );
  }

private:
  template<typename Ntk>
  void precompute_ingoing_edges_and_degrees( const Ntk& ntk )
  {
    fanout_view<Ntk> fanout_ntk( ntk );

    std::unordered_map<typename Ntk::node, std::vector<typename Ntk::node>> fanouts;
    fanout_ntk.foreach_node( [&]( auto const& n )
                             {
    std::vector<typename Ntk::node> ingoing_edges;
    fanout_ntk.foreach_fanout(n, [&](auto const& fanout_node) {
      ingoing_edges.push_back(fanout_node);
      return true; 
    });
    fanouts[n] = ingoing_edges; } );

    std::unordered_map<typename Ntk::node, size_t> fanout_count;
    for ( const auto& [node, edges] : fanouts )
    {
      fanout_count[node] = edges.size();
    }

    std::cout << "Node ingoing edges:" << std::endl;
    for ( const auto& [node, edges] : fanouts )
    {
      std::cout << "Node " << node << ": ";
      for ( const auto& edge : edges )
      {
        std::cout << edge << " ";
      }
      std::cout << std::endl;
    }

    //std::cout << "Node ingoing degrees:" << std::endl;
    for ( const auto& [node, count] : fanout_count )
    {
      std::cout << "Node " << node << ": " << count << std::endl;
    }
  }
  template<typename Ntk>
  std::unordered_map<typename Ntk::node, size_t> precompute_out_degrees( Ntk& ntk )
  {
    fanout_view<Ntk> fanout_ntk( ntk );

    std::unordered_map<typename Ntk::node, size_t> _fanout_count;
    fanout_ntk.foreach_node( [&]( auto const& n )
                      {
    size_t out_degree = 0;
    fanout_ntk.foreach_fanout( n, [&]( auto const& fanout )
                               {
      (void)fanout;
      out_degree++; } );
    _fanout_count[n] = out_degree;

    std::cout << "Node: " << n << ", Fanout Count: " << out_degree << std::endl;
    } );
    return _fanout_count;
  }

protected:
  void execute()
  {
    mig_network mig = store<mig_network>().current();
    if ( is_set( "costs" ) )
    {
      precompute_ingoing_edges_and_degrees( mig );
      precompute_out_degrees( mig );
    }
    else
    {
      std::cout << "help";
    }
  }

};

ALICE_ADD_COMMAND( fanout, "Rewriting" )

} // namespace alice

#endif

