/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

#include <mockturtle/properties/migcost.hpp>
#include <mockturtle/algorithms/cleanup.hpp>
#include "xmg_inv.hpp"

using namespace mockturtle;

namespace also
{

/******************************************************************************
 * XMG utilizations, public functions                                         *
 ******************************************************************************/
  std::array<xmg_network::signal, 3> get_children( xmg_network const& xmg, xmg_network::node const& n )
  {
    std::array<xmg_network::signal, 3> children;
    xmg.foreach_fanin( n, [&children]( auto const& f, auto i ) { children[i] = f; } );
    return children;
  }


  /* use substitue method for inveters propagation */
  xmg_network complement_node( xmg_network & xmg, xmg_network::node const& n )
  {
    xmg_network::signal opt;
    auto children = get_children( xmg, n );

    if( xmg.is_maj( n ) )
    {
      children[0] = !children[0];
      children[1] = !children[1];
      children[2] = !children[2];

      opt = xmg.create_maj_without_complement_opt( children[0], children[1], children[2] ) ^ true ;
    }
    else
    {
      if( xmg.is_complemented( children[2] ) )
      {
        children[2] = !children[2];
      }
      else
      {
        children[1] = !children[1];
      }

      opt = xmg.create_xor_without_complement_opt( children[1], children[2] ) ^ true ;
    }

    xmg.substitute_node_without_complement_opt( n, opt );

    return xmg;
  }

  /* print information */
  void print_node( xmg_network const& xmg, xmg_network::node const& n )
  {
    std::cout << " node " << n << " inverters infro: ";
    xmg.foreach_fanin(n, [&]( auto s ) { std::cout << " { " << s.index << " , " << s.complement << " } "; } );
    std::cout << std::endl;
  }

  void print_network( xmg_network const& xmg )
  {
    xmg.foreach_gate( [&]( auto n )
        {
          print_node( xmg, n );
        } );
  }
/******************************************************************************
 * Types                                                                      *
 ******************************************************************************/
  class inv_manager
  {
    public:
    inv_manager( xmg_network xmg );

    void compute_parents();

    std::vector<xmg_network::node> get_parents( xmg_network::node const& n );
    int num_invs_fanins( xmg_network::node const& n );
    int num_invs_fanouts( xmg_network::node const& n );
    int num_in_edges( xmg_network::node const& n );
    int num_out_edges( xmg_network::node const& n );
    int num_edges(  xmg_network::node const& n );
    int num_invs(  xmg_network::node const& n );
    int one_level_savings( xmg_network::node const& n );
    int one_level_savings_nni( xmg_network::node const& n );
    int two_level_savings( xmg_network::node const& n );
    void xor_jump();

    void one_level_optimization( const int& thres1, const int& thres2 );
    void two_level_optimization( const int& thres1, const int& thres2 );
    xmg_network run();

    private:
    xmg_network xmg;
    std::map<xmg_network::node, std::vector<xmg_network::node>> pmap;

    unsigned num_inv_origin{0u}, num_inv_opt{0u};
  };

  inv_manager::inv_manager( xmg_network xmg )
    : xmg( xmg )
  {
    compute_parents();
  }

  /* compute the node parents information and save it */
  void inv_manager::compute_parents()
  {
    xmg.foreach_gate( [&]( auto n )
        {
          xmg.foreach_fanin( n, [&]( auto c )
              {
                auto it = pmap.find( xmg.get_node( c ) );

                if( it == pmap.end() )
                {
                  std::vector<xmg_network::node> fout;
                  fout.push_back( n );
                  pmap[ xmg.get_node( c ) ] = fout;
                }
                else
                {
                  auto& f = it->second;
                  if( std::find( f.begin(), f.end(), n ) == f.end() )
                  {
                    f.push_back( n );
                  }
                }
              });
        });
  }

  std::vector<xmg_network::node> inv_manager::get_parents( xmg_network::node const& n )
  {
    return pmap[n];
  }

  int inv_manager::num_invs_fanins( xmg_network::node const& n )
  {
    int cost = 0;

    xmg.foreach_fanin( n, [&]( auto s ) { if( xmg.is_complemented( s ) ) { cost++; } } );
    return cost;
  }

  int inv_manager::num_invs_fanouts( xmg_network::node const& n )
  {
    int cost = 0u;

    /* ordinary fanouts */
    auto parents = get_parents( n );
    for( const auto pn : parents )
    {
      xmg.foreach_fanin( pn, [&]( auto s )
          {
            if( xmg.get_node( s ) == n && xmg.is_complemented( s ) )
            {
              cost++;
            }
          }
          );
    }

    /* POs */
    xmg.foreach_po( [&]( auto const& f )
    {
      if( xmg.get_node( f ) == n && xmg.is_complemented( f ) )
      {
        cost++;
      }
    });
    return cost;
  }

  int inv_manager::num_in_edges( xmg_network::node const& n )
  {
    auto num_in = ( xmg.is_maj( n ) ? 3 : 2 );
    return num_in;
  }

  int inv_manager::num_out_edges( xmg_network::node const& n )
  {
    /* pos */
    int num_po = 0;
    xmg.foreach_po( [&]( auto const& f ) { if( xmg.get_node( f ) == n ) { num_po++; } } );

    return get_parents( n ).size() + num_po;
  }

  int inv_manager::num_edges(  xmg_network::node const& n )
  {
    assert( xmg.is_maj( n ) || xmg.is_xor3( n ) );
    return num_in_edges( n ) + num_out_edges( n );
  }

  int inv_manager::num_invs( xmg_network::node const& n )
  {
    return num_invs_fanins( n ) + num_invs_fanouts( n );
  }

  int inv_manager::one_level_savings( xmg_network::node const& n )
  {
    int before, after;

    before = num_invs( n );

    if( xmg.is_maj( n ) )
    {
      after = num_edges( n ) - before;
    }
    else
    {
       int in, out;

       out = num_out_edges( n ) - num_invs_fanouts( n );

       auto tmp = num_invs_fanins( n );

       if( tmp == 1 || tmp == 3 )
       {
          in = 0;
       }
       else if( tmp == 2 || tmp == 0 )
       {
         in = 1;
       }
       else
       {
         assert( false );
       }

       after = in + out;
    }

    return before - after;
  }

  int inv_manager::one_level_savings_nni( xmg_network::node const& n )
  {
    int before, after, after_nni;

    before = num_invs( n );

    if( xmg.is_maj( n ) )
    {
      after = num_edges( n ) - before;

      if( num_invs_fanins( n ) == 2 )
      {

      }
    }
    else
    {
       int in, out;

       out = num_out_edges( n ) - num_invs_fanouts( n );

       auto tmp = num_invs_fanins( n );

       if( tmp == 1 || tmp == 3 )
       {
          in = 0;
       }
       else if( tmp == 2 || tmp == 0 )
       {
         in = 1;
       }
       else
       {
         assert( false );
       }

       after = in + out;
    }

    return before - after;
  }

  int inv_manager::two_level_savings( xmg_network::node const& n )
  {
    assert( !xmg.is_pi( n ) );

    auto parents = get_parents( n );
    int  total_savings = 0;

    /* no parents */
    if( parents.size() == 0 )
    {
      return one_level_savings( n );
    }
    else
    {
      auto child_savings = one_level_savings( n );

      for( const auto& p : parents )
      {
        total_savings += one_level_savings( p );

        xmg.foreach_fanin( p, [&]( auto s )
            {
              if( xmg.get_node( s ) == n && xmg.is_complemented( s ) )
              {
                if( xmg.is_complemented( s ) )
                {
                  total_savings -= 2;
                }
                else
                {
                  total_savings += 2;
                }
              }
            }
            );
      }

      total_savings += child_savings;
    }

    return total_savings;
  }

  void inv_manager::one_level_optimization( const int& thres1, const int& thres2 )
  {
    xmg.foreach_gate( [&]( auto n )
        {
          if( one_level_savings( n ) >= thres1 )
          {
            xmg.complement_node( n, pmap[n] );
          }
          else if( two_level_savings( n ) >= thres2 )
          {
           auto parents = pmap[n];
           xmg.complement_node( n, pmap[n] );

           for( const auto& p : parents )
           {
            xmg.complement_node( p, pmap[p] );
           }

          }
        }
        );
  }

  void inv_manager::two_level_optimization( const int& thres1, const int& thres2 )
  {
    xmg.foreach_gate( [&]( auto n )
        {
         auto parents = pmap[n];
         auto savings = two_level_savings( n );

         if( savings >= thres1 )
         {
           xmg.complement_node( n, pmap[n] );

           for( const auto& p : parents )
           {
            xmg.complement_node( p, pmap[p] );
           }
         }
         else if( one_level_savings( n ) >= thres2 )
         {
          xmg.complement_node( n, pmap[n] );
         }
        } );
  }

  void inv_manager::xor_jump()
  {
    xmg.foreach_gate( [&]( const auto& n )
        {
          if( xmg.is_xor3( n ) )
          {
            xmg.xor_inv_jump( n );
          }
        }
        );
  }

  xmg_network inv_manager::run()
  {
    num_inv_origin = num_inverters( xmg );

    one_level_optimization( 0, 1 );
    two_level_optimization( 1, 0 );

    one_level_optimization( 0, 1 );
    two_level_optimization( 1, 1 );

    num_inv_opt = num_inverters( xmg );

    xor_jump();

    std::cout << "[xmginv] " << " num_inv_origin: " << num_inv_origin << " num_opt_inv: " << num_inv_opt << std::endl;
    auto xmg_opt = mockturtle::cleanup_dangling( xmg );
    return xmg_opt;
  }

  /* public function */
  xmg_network xmg_inv_optimization( xmg_network& xmg )
  {
    inv_manager mgr( xmg );
    return mgr.run();
  }

}
