/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

#include <iostream>
#include <set>
#include <mockturtle/properties/migcost.hpp>
#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/views/fanout_view.hpp>
#include <mockturtle/views/topo_view.hpp>
#include <mockturtle/utils/node_map.hpp>
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/io/write_dot.hpp>
#include <mockturtle/io/write_bench.hpp>
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

        auto klut = ntk2klut();

        std::cout << fmt::format(
            " [i] klut #pis  = {}\n"
            " [i] klut #pos  = {}\n"
            " [i] klut size  = {}\n"
            " [i] klut #invs = {}\n",
            klut.num_pis(), klut.num_pos(), klut.num_gates(), num_inverters_in_klut( klut )
            );

        write_dot( klut, "test.dot" );
        write_bench( klut, "test.bench" );

        /* not gates are seprated nodes in the klut network */
        assert( ntk.num_gates() + num_inverters_in_klut( klut ) == klut.num_gates() );
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

                  if( has_constant( n ) && !ntk.is_complemented( children[0] ) )
                  {
                    num_xnor_opt++;
                    opt_inverted_nodes.insert( ntk.get_node( compl_fanins[0] ) );
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

      void orded_inputs( std::vector<signal<klut_network>>& array )
      {
        std::sort( array.begin(), array.end(), [this]( auto const& c1, auto const& c2 ) {
            return c1 < c2;
            } );
      }

      unsigned num_inverters_in_klut( klut_network const& klut )
      {
        unsigned count{0u};
        klut.foreach_gate( [&]( auto const& n ) {
            if( klut.fanin_size( n) == 1u )
            {
              count++;
            }
            } );

        return count;
      }

      signal<klut_network> create_nni( klut_network& klut, std::vector<signal<klut_network>> const& children )
      {
        static uint64_t _nni = 0x71;
        kitty::dynamic_truth_table tt_nni( 3 );
        kitty::create_from_words( tt_nni, &_nni, &_nni + 1 );

        return klut.create_node( children, tt_nni );
      }

      /* mapping the current ntk into a klut network */
      klut_network ntk2klut()
      {
        klut_network klut;

        node_map<signal<klut_network>, Ntk> node2new( ntk ); 

        node2new[ ntk.get_constant( false ) ] = klut.get_constant( false );
        //node2new[ ntk.get_constant( true )  ] = klut.get_constant( true );

        /* reserve constant 1 */
        /* create pis */
        ntk.foreach_pi( [&]( auto n ) {
            node2new[n] = klut.create_pi();
            });

        /* create klut nodes */
        topo_view ntk_topo{ntk};
        ntk_topo.foreach_node( [&]( auto n ) {
            if ( ntk.is_constant( n ) || ntk.is_pi( n ) )
            return;

            if( ntk.is_maj( n ) )
            {
              auto compl_fanins = get_invs_fanins( n );
              auto tmp = compl_fanins.size();
              
              /* nni(a, b, c) = <!a!bc> */
              if( tmp == 1 && has_constant( n ) && compl_fanins[0].index != 0 )
              {
                std::array<signal<klut_network>, 3u> replaced_children;
                
                ntk.foreach_fanin( n, [&]( auto const& f ) {
                    if( f.index == 0 )
                    {
                      replaced_children[0] = ntk.is_complemented( f ) ? klut.get_constant( false ) : klut.get_constant( true ); 
                    }
                    else
                    {
                      if( ntk.is_complemented( f ) )
                      {
                        replaced_children[1] = node2new[f];
                      }
                      else
                      {
                        replaced_children[2] = node2new[f];
                      }
                    }
                    } );


                std::cout << "create nni node: " << replaced_children[0] 
                          << " " << replaced_children[1] << " " << replaced_children[2] << std::endl;

                /* array to vector */
                std::vector<signal<klut_network>> v;
                v.push_back( replaced_children[0] );
                v.push_back( replaced_children[1] );
                v.push_back( replaced_children[2] );

                node2new[n] = create_nni( klut, v );
              }
              else
              {
                std::vector<signal<klut_network>> children;
                ntk.foreach_fanin( n, [&]( auto const& f ) {
                      children.push_back( ntk.is_complemented( f ) ? klut.create_not( node2new[f] ) : node2new[f] );
                    } );
                node2new[n] = klut.create_maj( children[0], children[1], children[2] );
              }
            }
            else if( ntk.is_xor3( n ) )
            {
              auto compl_fanins = get_invs_fanins( n );
              auto tmp = compl_fanins.size();

              if( tmp == 1 && has_constant( n ) && compl_fanins[0].index != 0 )
              {
                std::vector<signal<klut_network>> replaced_children;
                ntk.foreach_fanin( n, [&]( auto const& f ) {
                    if( f.index != 0 )
                    {
                    replaced_children.push_back( node2new[f] );
                    }
                    } );

                node2new[n] = klut.create_xor3( klut.get_constant( true ), replaced_children[0], replaced_children[1] );
              }
              else
              {
                std::vector<signal<klut_network>> children;
                ntk.foreach_fanin( n, [&]( auto const& f ) {
                    children.push_back( ntk.is_complemented( f ) ? klut.create_not( node2new[f] ) : node2new[f] );
                    } );
                node2new[n] = klut.create_xor3( children[0], children[1], children[2] );
              }
            }
            else
            {
              assert( false && "not allowed" );
            }
            } );

        /* create pos */
        ntk.foreach_po( [&]( auto const& f, auto index ) {
            auto const o = ntk.is_complemented( f ) ? klut.create_not( node2new[f] ) : node2new[f];
            klut.create_po( o );
            } );

        klut = cleanup_dangling( klut );
        return klut;
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
