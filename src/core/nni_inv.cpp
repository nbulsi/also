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
        : ntk( ntk ), fanout_ntk( fanout_view<Ntk> { ntk } )
      {
      }

      klut_network run()
      {
        //compute_num_gates();
        //report();

        auto klut = ntk2klut();

        std::cout << fmt::format(
            " [i] klut #pis  = {}\n"
            " [i] klut #pos  = {}\n"
            " [i] klut size  = {}\n"
            " [i] klut #invs = {}\n",
            klut.num_pis(), klut.num_pos(), klut.num_gates(), num_inverters_in_klut( klut )
            );

        write_bench( klut, "test.bench" );

        /* not gates are seprated nodes in the klut network */
        assert( ntk.num_gates() + num_inverters_in_klut( klut ) == klut.num_gates() );
        return klut;
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
      std::array<mockturtle::signal<Ntk>,3> get_children( node<Ntk> const& n ) const
      {
        std::array<mockturtle::signal<Ntk>, 3> children;
        ntk.foreach_fanin( n, [&children]( auto const& f, auto i ) { children[i] = f; } );
        std::sort( children.begin(), children.end(), [this]( auto const& c1, auto const& c2 ) {
            return  ntk.get_node( c1 )  <  ntk.get_node( c2 ) ;
            } );
        return children;
      }

      std::vector<mockturtle::signal<Ntk>> get_invs_fanins( node<Ntk> const& n )
      {
        std::vector<mockturtle::signal<Ntk>> compl_fanin_signals;
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
        std::set<node<Ntk>> nodes;

        fanout_ntk.foreach_fanout( n, [&]( auto const& p ) 
            { nodes.insert( p ); }
            );

        return nodes;
      }

      std::vector<mockturtle::signal<Ntk>> get_parent_signals( node<Ntk> const& n )
      {
        std::vector<mockturtle::signal<Ntk>> signals;

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

      void orded_inputs( std::vector<klut_network::signal>& array )
      {
        std::sort( array.begin(), array.end(), [this]( auto const& c1, auto const& c2 ) {
            return c1 < c2;
            } );
      }

      unsigned num_inverters_in_klut( klut_network const& klut )
      {
        unsigned count{0u};
        klut.foreach_gate( [&]( auto const& n ) {
            if( klut.fanin_size( n ) == 1u )
            {
              count++;
            }
            } );

        return count;
      }                

      bool is_complemented_po_driver( node<Ntk> const& n )
      {
        bool match = false;
        ntk.foreach_po( [&]( auto const& s)
            {
              if( ntk.get_node( s ) == n && ntk.is_complemented( s ) )
              {
                match = true;
              }
            } );
        return match;
      }


      klut_network::signal create_nni( klut_network& klut, std::vector<klut_network::signal> const& children )
      {
        static uint64_t _nni = 0x71;
        kitty::dynamic_truth_table tt_nni( 3 );
        kitty::create_from_words( tt_nni, &_nni, &_nni + 1 );

        return klut.create_node( children, tt_nni );
      }

      bool is_node_in_vector( node<Ntk> const& n )
      {
        return std::find( inv_po.begin(), inv_po.end(), n ) != inv_po.end();
      }

      /* mapping the current ntk into a klut network */
      klut_network ntk2klut()
      {
        klut_network klut;

        node_map<klut_network::signal, Ntk> node2new( ntk ); 

        node2new[ ntk.get_constant( false ) ] = klut.get_constant( false );

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
              auto parents = get_parent_signals( n );
              std::vector<klut_network::signal> replaced_children;
              
              /* nni(a, b, c) = <!a!bc> */
              if( tmp == 1 && has_constant( n ) )
              {
                /* normal and */
                if( compl_fanins[0].index == 0 ) 
                {
                  ntk.foreach_fanin( n, [&]( auto const& f ) {
                      if( f.index != 0 )
                      {
                        replaced_children.push_back( node2new[f] );
                      }
                      } );
                
                  node2new[n] = klut.create_maj( klut.get_constant( true ), replaced_children[0], replaced_children[1] );
                }
                else /* nni f = !bc, or f = !cb */
                {
                  replaced_children.push_back( klut.get_constant( true ) );
                  
                  klut_network::signal s1, s2;
                  ntk.foreach_fanin( n, [&]( auto const& f ) {
                      if( f.index != 0 )
                      {
                        if( ntk.is_complemented( f ) )
                        {
                          s1 = node2new[f];
                        }
                        else
                        {
                          s2 = node2new[f];
                        }
                      } } );

                  replaced_children.push_back( s1 );
                  replaced_children.push_back( s2 );
                  
                  node2new[n] = create_nni( klut, replaced_children );
                }
              }
              else if( tmp == 1 && is_complemented_po_driver( n ) && fanout_ntk.fanout_size( n ) == 0 )
              {
                klut_network::signal s2;
                ntk.foreach_fanin( n, [&]( auto const& f ) {
                    if( !ntk.is_complemented( f ) )
                    {
                    replaced_children.push_back( node2new[f] );
                    }
                    else
                    {
                      s2 = node2new[f];
                    }
                    } );
                replaced_children.push_back( s2 );
                node2new[n] = create_nni( klut, replaced_children );

                inv_po.push_back( n );
              }
              else if( tmp == 2 )
              {
                klut_network::signal s2;
                ntk.foreach_fanin( n, [&]( auto const& f ) {
                    if( ntk.is_complemented( f ) )
                    {
                    replaced_children.push_back( node2new[f] );
                    }
                    else
                    {
                      s2 = node2new[f];
                    }
                    } );
                replaced_children.push_back( s2 );
                node2new[n] = create_nni( klut, replaced_children );
              }
              else
              {
                std::vector<klut_network::signal> children;
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
                std::vector<klut_network::signal> replaced_children;
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
                std::vector<klut_network::signal> children;
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
            auto const o = ( ntk.is_complemented( f ) && !is_node_in_vector( ntk.get_node( f ) ) ) 
                           ? klut.create_not( node2new[f] ) : node2new[f];
            klut.create_po( o );
            } );

        klut = cleanup_dangling( klut );
        return klut;
      }

    private:
      Ntk ntk;
      fanout_view<Ntk> fanout_ntk;
      std::unordered_set<node<Ntk>> opt_inverted_nodes;
      std::vector<node<Ntk>> inv_po;
      unsigned num_maj{0u}, num_nni{0u}, num_inv_origin{0u}, num_inv_remains{0u}, num_xor3{0u}, num_xnor_opt{0u}, num_inv_opt{0u};
  };

/******************************************************************************
 * Private functions                                                          *
 ******************************************************************************/

/******************************************************************************
 * Public functions                                                           *
 ******************************************************************************/
  klut_network nni_opt( mockturtle::xmg_network const& ntk )
  {
    nni_manager<mockturtle::xmg_network> m( ntk );
    return m.run();
  }

}
