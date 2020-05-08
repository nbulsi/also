/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

#include <iostream>
#include <set>
#include <mockturtle/properties/migcost.hpp>
#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/views/fanout_view.hpp>
#include <fmt/format.h>

using namespace mockturtle;

namespace also
{

/******************************************************************************
 * Types                                                                      *
 ******************************************************************************/
  template<class Ntk>
  class nni_manager
  {
    public:
      nni_manager( Ntk const& ntk )
        : ntk( ntk )
      {
      }

      void run()
      {
        compute_num_gates();
        report();
      }

      void report()
      {
        num_inv_opt     = opt_inverted_nodes.size();
        num_inv_remains = num_inv_origin - num_inv_opt;

        std::cout << fmt::format( 
            " [i] Gates             = {}\n"
            " [i] Num MAJs          = {}\n"
            " [i] Num XORs          = {}\n"
            " [i] Num NNIs          = {}\n"
            " [i] Num XNORs         = {}\n"
            " [i] Num INVs origin   = {}\n"
            " [i] Num INVs opt      = {}\n"
            " [i] Num INVs remains  = {}\n",
            ntk.num_gates(), num_maj, num_xor3, num_nni, num_xnor_opt, 
            num_inv_origin, num_inv_opt, num_inv_remains );

        assert( ntk.num_gates() == ( num_maj + num_xor3 + num_nni ) ); 
        assert( num_inv_origin  == ( num_inv_opt + num_inv_remains ) ); 
      }

    private:
      std::array<signal<Ntk>, 3> get_children( node<Ntk> const& n ) const
      {
        std::array<signal<Ntk>, 3> children;
        ntk.foreach_fanin( n, [&children]( auto const& f, auto i ) { children[i] = f; } );
        std::sort( children.begin(), children.end(), [this]( auto const& c1, auto const& c2 ) {
            return  ntk.get_node( c1 )  <  ntk.get_node( c2 ) ;
            } );
        return children;
      }

      std::vector<signal<Ntk>> get_invs_fanins( node<Ntk> const& n )
      {
        std::vector<signal<Ntk>> compl_fanin_signals;
        auto children = get_children( n );

        for( auto const& c : children )
        {
          if( ntk.is_complemented( c ) )
          {
            compl_fanin_signals.push_back( c );
          }
        }

        return compl_fanin_signals;
      }

      std::set<node<Ntk>> get_parent_nodes( node<Ntk> const& n )
      {
        fanout_view fanout_ntk{ntk};
        std::set<node<Ntk>> nodes;

        fanout_ntk.foreach_fanout( n, [&]( auto const& p ) 
            { nodes.insert( p ); }
            );

        return nodes;
      }

      std::vector<signal<Ntk>> get_parent_signals( node<Ntk> const& n )
      {
        std::vector<signal<Ntk>> signals;

        auto parents = get_parent_nodes( n );

        for( auto const& p : parents )
        {
          ntk.foreach_fanin( p, [&]( auto s) 
              {
                if( s.index == n )
                {
                  signals.push_back( s );
                }
              }
              );
        }

        return signals;
      }

      bool has_constant( node<Ntk> const& n )
      {
        if( ntk.is_pi( n ) || ntk.is_constant( n ) )
          return false;
        
        auto children = get_children( n );
        
        return ntk.get_node( children[0] ) == 0 ? true : false;
      }

      void compute_num_gates()
      {
        num_inv_origin = num_inverters( ntk );

        ntk.foreach_gate( [&]( auto n )
            {
              if( ntk.is_maj( n ) )
              {
                num_maj++;

                auto compl_fanins = get_invs_fanins( n );
                auto tmp = compl_fanins.size();
                
                if( tmp == 1 )
                {
                  auto children = get_children( n );

                  if( has_constant( n ) && !ntk.is_complemented( children[0] ) )
                  {
                    num_nni++;
                    num_maj--;
                    std::cout << " node " << n << " is replaced." << std::endl;
                    opt_inverted_nodes.insert( ntk.get_node( compl_fanins[0] ) );
                  }
                  else
                  {
                    auto parents = get_parent_signals( n );
                    if( parents.size() == 1 )
                    {
                      if( ntk.is_complemented( parents[0] ) )
                      {
                        num_nni++;
                        num_maj--;
                        opt_inverted_nodes.insert( n );
                        opt_inverted_nodes.insert( ntk.get_node( compl_fanins[0] ) );
                      }
                    }
                  }
                }
              }
              else if( ntk.is_xor3( n ) )
              {
                num_xor3++;
                
                auto compl_fanins = get_invs_fanins( n );
                auto tmp = compl_fanins.size();

                if( tmp == 1 )
                {
                  auto children = get_children( n );

                  std::cout << " child0: " << children[0].index
                            << " child1: " << children[1].index
                            << " child2: " << children[2].index << std::endl;

                  if( has_constant( n ) && !ntk.is_complemented( children[0] ) )
                  {
                    num_xnor_opt++;
                    opt_inverted_nodes.insert( ntk.get_node( compl_fanins[0] ) );
                    std::cout << " xor node " << n << " is replaced." << std::endl;
                  }
                }
              }
              else
              {
                /* support XMG and MIG now */
              }
            }
            );

      }

    private:
      Ntk ntk;
      std::unordered_set<node<Ntk>> opt_inverted_nodes;
      unsigned num_maj{0u}, num_nni{0u}, num_inv_origin{0u}, num_inv_remains{0u}, num_xor3{0u}, num_xnor_opt{0u}, num_inv_opt{0u};
  };

/******************************************************************************
 * Private functions                                                          *
 ******************************************************************************/

/******************************************************************************
 * Public functions                                                           *
 ******************************************************************************/
  void nni_opt( mockturtle::xmg_network const& ntk )
  {
    nni_manager<mockturtle::xmg_network> m( ntk );
    m.run();
  }

}
