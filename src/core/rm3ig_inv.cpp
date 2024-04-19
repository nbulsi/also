/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

#include "utils.hpp"
#include "rm3ig_inv.hpp"
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/properties/migcost.hpp>

using namespace mockturtle;

namespace also
{
/******************************************************************************
 * RM3IG utilizations, public functions                                         *
 ******************************************************************************/
  /* use substitue method for inveters propagation */
  std::array<rm3_network::signal, 3> get_children3( rm3_network const& rm3ig, rm3_network::node const& n )
  {
    std::array<rm3_network::signal, 3> children;
    rm3ig.foreach_fanin( n, [&children]( auto const& f, auto i )
                         { children[i] = f; } );
    return children;
  }

  rm3_network complement_node( rm3_network & rm3ig, rm3_network::node const& n )
  {
    rm3_network::signal opt;
    auto children = get_children3( rm3ig, n );
    
    if( rm3ig.is_rm3( n ) )
    {
      children[0] = !children[0];
      children[1] = !children[1];
      children[2] = !children[2];

      opt = rm3ig.create_rm3_without_complement_opt( children[0], children[1], children[2] ) ^ true ;
    }

    rm3ig.substitute_node_without_complement_opt( n, opt );

    return rm3ig;
  }

  rm3_network node_complement( rm3_network& rm3ig, rm3_network::node const& n )
  {
    rm3_network::signal opt;
    auto children = get_children3( rm3ig, n );

    if ( rm3ig.is_rm3( n ) )
    {

      const auto& xx = children[0u].index;
      const auto& yy = children[1u].index;
      const auto& zz = children[2u].index;
      const auto cx = children[0u].complement;
      const auto cy = children[1u].complement;
      const auto cz = children[2u].complement;

      if ( cx && cy && !cz )
      {
        children[0] = !children[0];
        children[1] = !children[1];
        children[2] = children[2];

        opt = rm3ig.create_rm3_without_complement_opt( children[1], children[0], children[2] ) ;
      }
      else if ( cz && cy && !cx )
      {
        children[0] = children[0];
        children[1] = !children[1];
        children[2] = !children[2];

        opt = rm3ig.create_rm3_without_complement_opt( children[0], children[2], children[1] );
      }
      else
      {
        children[0] = children[0];
        children[1] = children[1];
        children[2] = children[2];

        opt = rm3ig.create_rm3_without_complement_opt( children[0], children[1], children[2] );
       }
        // children[0] = children[0];
        // children[1] = children[1];
        // children[2] = children[2];

       // opt = rm3ig.create_rm3_without_complement_opt( children[0], children[1], children[2] );
        rm3ig.substitute_node_without_complement_opt( n, opt );
    }



    return rm3ig;
  }

/******************************************************************************
 * Types                                                                      *
 ******************************************************************************/
  class rm3_inv_manager
  {
    public:
    rm3_inv_manager( rm3_network rm3ig );

    void compute_parents();

    std::vector<rm3_network::node> get_parents( rm3_network::node const& n );
    int num_invs_fanins( rm3_network::node const& n );
    int num_invs_fanouts( rm3_network::node const& n );
    int num_in_edges( rm3_network::node const& n );
    int num_out_edges( rm3_network::node const& n );
    int num_edges(  rm3_network::node const& n );
    int num_invs(  rm3_network::node const& n );
    int one_level_savings( rm3_network::node const& n );
    int one_level_savings_nni( rm3_network::node const& n );
    int two_level_savings( rm3_network::node const& n );
    void and_inv();

    void one_level_optimization( const int& thres1, const int& thres2 );
    void two_level_optimization( const int& thres1, const int& thres2 );
    rm3_network run();

    private:
    rm3_network rm3ig;
    std::map<rm3_network::node, std::vector<rm3_network::node>> pmap;

    unsigned num_inv_origin{0u}, num_inv_opt{0u};
  };

  rm3_inv_manager::rm3_inv_manager( rm3_network rm3ig )
      : rm3ig( rm3ig )
  {
    compute_parents();
  }

  /* compute the node parents information and save it */
  void rm3_inv_manager::compute_parents()
  {
    rm3ig.foreach_gate( [&]( auto n )
        {
          rm3ig.foreach_fanin( n, [&]( auto c )
              {
                auto it = pmap.find( rm3ig.get_node( c ) );

                if( it == pmap.end() )
                {
                  std::vector<rm3_network::node> fout;
                  fout.push_back( n );
                  pmap[rm3ig.get_node( c )] = fout;
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

  std::vector<rm3_network::node> rm3_inv_manager::get_parents( rm3_network::node const& n )
  {
    return pmap[n];
  }

  int rm3_inv_manager::num_invs_fanins( rm3_network::node const& n )
  {
    int cost = 0;

    rm3ig.foreach_fanin( n, [&]( auto s )
                       { if( rm3ig.is_complemented( s ) ) { cost++; } } );
    return cost;
  }

  int rm3_inv_manager::num_invs_fanouts( rm3_network::node const& n )
  {
    int cost = 0u;

    /* ordinary fanouts */
    auto parents = get_parents( n );
    for ( const auto pn : parents )
    {
      rm3ig.foreach_fanin( pn, [&]( auto s )
                         {
            if( rm3ig.get_node( s ) == n && rm3ig.is_complemented( s ) )
            {
              cost++;
            } } );
    }

    /* POs */
    rm3ig.foreach_po( [&]( auto const& f )
                    {
      if( rm3ig.get_node( f ) == n && rm3ig.is_complemented( f ) )
      {
        cost++;
      } } );
    return cost;
  }

  int rm3_inv_manager::num_in_edges( rm3_network::node const& n )
  {
    auto num_in = ( rm3ig.is_rm3( n ) ? 3 : 2 );
    return num_in;
  }

  int rm3_inv_manager::num_out_edges( rm3_network::node const& n )
  {
    /* pos */
    int num_po = 0;
    rm3ig.foreach_po( [&]( auto const& f )
                    { if( rm3ig.get_node( f ) == n ) { num_po++; } } );

    return get_parents( n ).size() + num_po;
  }

  int rm3_inv_manager::num_edges( rm3_network::node const& n )
  {
    assert( rm3ig.is_rm3( n ) );
    return num_in_edges( n ) + num_out_edges( n );
  }

  int rm3_inv_manager::num_invs( rm3_network::node const& n )
  {
    return num_invs_fanins( n ) + num_invs_fanouts( n );
  }

  int rm3_inv_manager::one_level_savings( rm3_network::node const& n )
  {
    int before, after;

    before = num_invs( n );

    if ( rm3ig.is_rm3( n ) )
    {
      after = num_edges( n ) - before;
    }
    else
    {//xor3
      int in, out;

      out = num_out_edges( n ) - num_invs_fanouts( n );

      auto tmp = num_invs_fanins( n );

      //这里好像有些问题
      if ( tmp == 1 || tmp == 3 )
      {
        in = 0;
      }
      else if ( tmp == 2 || tmp == 0 )
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

  int rm3_inv_manager::two_level_savings( rm3_network::node const& n )
  {
    assert( !rm3ig.is_pi( n ) );

    auto parents = get_parents( n );
    int total_savings = 0;

    /* no parents */
    if ( parents.size() == 0 )
    {
      return one_level_savings( n );
    }
    else
    {
      auto child_savings = one_level_savings( n );

      for ( const auto& p : parents )
      {
        total_savings += one_level_savings( p );

        rm3ig.foreach_fanin( p, [&]( auto s )
                           {
              if( rm3ig.get_node( s ) == n && rm3ig.is_complemented( s ) )
              {
                if( rm3ig.is_complemented( s ) )
                {
                  //为什么是2
                  total_savings -= 2;
                }
                else
                {
                  total_savings += 2;
                }
              } } );
      }

      total_savings += child_savings;
    }

    return total_savings;
  }

  void rm3_inv_manager::one_level_optimization( const int& thres1, const int& thres2 )
  {
    rm3ig.foreach_gate( [&]( auto n )
                        {
                          //std::cout << "one_level_savings( n )" << one_level_savings( n ) << std::endl;
                          //std::cout << "opt_one_level_savings( n )" << one_level_savings( n ) << std::endl;
                          //std::cout << "two_level_savings( n )" << two_level_savings( n ) << std::endl;
                          if ( one_level_savings( n ) >= thres1 )
                          {
                            rm3ig.node_complement( n, pmap[n] );
                            rm3ig.node_jump_complement( n, pmap[n] );
                            rm3ig.node_complement( n, pmap[n] );
                            rm3ig.complement_node( n, pmap[n] );
                            rm3ig.node_complement( n, pmap[n] );
                            rm3ig.node_jump_complement( n, pmap[n] );
                            rm3ig.node_complement( n, pmap[n] );
                            rm3ig.node_jump_complement( n, pmap[n] );
                            rm3ig.node_complement( n, pmap[n] );
                            rm3ig.node_jump_complement( n, pmap[n] );
                            rm3ig.node_complement( n, pmap[n] );
                          }
                          else if ( two_level_savings( n ) >= thres2 )
                          {
                            auto parents = pmap[n];
                            rm3ig.node_complement( n, pmap[n] );
                            rm3ig.node_jump_complement( n, pmap[n] );
                            rm3ig.node_complement( n, pmap[n] );
                            rm3ig.complement_node( n, pmap[n] );
                            rm3ig.node_complement( n, pmap[n] );
                            rm3ig.node_jump_complement( n, pmap[n] );
                            rm3ig.node_complement( n, pmap[n] );
                            rm3ig.node_jump_complement( n, pmap[n] );
                            rm3ig.node_complement( n, pmap[n] );
                            rm3ig.node_jump_complement( n, pmap[n] );
                            rm3ig.node_complement( n, pmap[n] );

                            for ( const auto& p : parents )
                            {
                              rm3ig.node_complement( n, pmap[n] );
                              rm3ig.node_jump_complement( n, pmap[n] );
                              rm3ig.node_complement( p, pmap[p] );
                              rm3ig.complement_node( p, pmap[p] );
                              rm3ig.node_complement( p, pmap[p] );
                              rm3ig.node_jump_complement( n, pmap[n] );
                              rm3ig.node_complement( n, pmap[n] );
                              rm3ig.node_jump_complement( n, pmap[n] );
                              rm3ig.node_complement( n, pmap[n] );
                              rm3ig.node_jump_complement( n, pmap[n] );
                              rm3ig.node_complement( n, pmap[n] );
                            }
                          }
                          // rm3ig = node_complement( rm3ig, n );
                        } );
  }

  void rm3_inv_manager::two_level_optimization( const int& thres1, const int& thres2 )
  {
    rm3ig.foreach_gate( [&]( auto n )
        {
         auto parents = pmap[n];
         auto savings = two_level_savings( n );

         if( savings >= thres1 )
         {
           rm3ig.node_complement( n, pmap[n] );
           rm3ig.node_jump_complement( n, pmap[n] );
           rm3ig.node_complement( n, pmap[n] );
           rm3ig.complement_node( n, pmap[n] );
           rm3ig.node_complement( n, pmap[n] );
           rm3ig.node_jump_complement( n, pmap[n] );
           rm3ig.node_complement( n, pmap[n] );
           rm3ig.node_jump_complement( n, pmap[n] );
           rm3ig.node_complement( n, pmap[n] );
           rm3ig.node_jump_complement( n, pmap[n] );
           rm3ig.node_complement( n, pmap[n] );

           for( const auto& p : parents )
           {
             rm3ig.node_complement( n, pmap[n] );
             rm3ig.node_jump_complement( n, pmap[n] );
             rm3ig.node_complement( p, pmap[p] );
             rm3ig.complement_node( p, pmap[p] );
             rm3ig.node_complement( p, pmap[p] );
             rm3ig.node_jump_complement( n, pmap[n] );
             rm3ig.node_complement( n, pmap[n] );
             rm3ig.node_jump_complement( n, pmap[n] );
             rm3ig.node_complement( n, pmap[n] );
             rm3ig.node_jump_complement( n, pmap[n] );
             rm3ig.node_complement( n, pmap[n] );
           }
         }
         else if( one_level_savings( n ) >= thres2 )
         {
           rm3ig.node_complement( n, pmap[n] );
           rm3ig.node_jump_complement( n, pmap[n] );
           rm3ig.node_complement( n, pmap[n] );
           rm3ig.complement_node( n, pmap[n] );
           rm3ig.node_complement( n, pmap[n] );
           rm3ig.node_jump_complement( n, pmap[n] );
           rm3ig.node_complement( n, pmap[n] );
           rm3ig.node_jump_complement( n, pmap[n] );
           rm3ig.node_complement( n, pmap[n] );
           rm3ig.node_jump_complement( n, pmap[n] );
           rm3ig.node_complement( n, pmap[n] );
         }
        } );
  }

  void rm3_inv_manager::and_inv()
  {
    rm3ig.foreach_gate( [&]( const auto& n )
                      {
          if( rm3ig.is_rm3( n ) )
          {
            rm3ig.and_inv( n );
          } } );
  }

  rm3_network rm3_inv_manager::run()
  {
    num_inv_origin = num_inverters( rm3ig );

    one_level_optimization( -20, -20 );
    two_level_optimization( -20, -20 );

    one_level_optimization( -20, -20 );
    two_level_optimization( -20, -20 );

    num_inv_opt = num_inverters( rm3ig );
    
    and_inv();

    std::cout << "[rm3iginv] "
              << " num_inv_origin: " << num_inv_origin << " num_opt_inv: " << num_inv_opt << std::endl;
    //auto rm3ig_opt = mockturtle::cleanup_dangling( rm3ig );
    return rm3ig;
  }

  /* public function */
  rm3_network rm3ig_inv_optimization( rm3_network& rm3ig )
  {
    rm3_inv_manager mgr( rm3ig );
    return mgr.run();
  }


}