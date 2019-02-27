/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

#include <mockturtle/mockturtle.hpp>
#include <mockturtle/properties/migcost.hpp>

using namespace mockturtle;

namespace also
{

/******************************************************************************
 * Types                                                                      *
 ******************************************************************************/
  using nd_t = node<xmg_network>;

  class inv_manager
  {
    public:
    inv_manager( xmg_network& xmg );
    
    unsigned count_current_invs();
    void run();
    void complement_node( nd_t const& n );
    std::array<signal<xmg_network>, 3> get_children( nd_t const& n ) const;
    std::vector<nd_t> get_parents( nd_t const& n ) const;
    void print_node( nd_t const& n ) const;
    void print_network() const;
    
    unsigned num_invs_fanins( nd_t const& n ) const;
    unsigned num_invs_fanouts( nd_t const& n ) const;
    unsigned num_invs_before( nd_t const& n ) const;
    
    unsigned num_in_edges( nd_t const& n ) const;
    unsigned num_out_edges( nd_t const& n ) const;
    unsigned num_edges( nd_t const& n ) const;
  
    int one_level_savings( nd_t const& n ) const;
    int two_level_savings( nd_t const& n ) const;

    private:
    xmg_network xmg;
  };

/******************************************************************************
 * Private functions                                                          *
 ******************************************************************************/
  inv_manager::inv_manager( xmg_network& xmg )
    : xmg( xmg )
  {
  }

  void inv_manager::run()
  {
    //complement_node( xmg.index_to_node( 9 ) );
    print_network();
    std::cout << "Optimized: " << num_inverters( xmg ) << std::endl;
  }

  unsigned inv_manager::num_invs_fanins( nd_t const& n ) const
  {
    unsigned cost = 0u;
    
    xmg.foreach_fanin( n, [&]( auto s ) { if( xmg.is_complemented( s ) ) { cost++; } } );
    return cost;
  }

  std::vector<nd_t> inv_manager::get_parents( nd_t const& n ) const
  {
    std::vector<nd_t> parents;
    fanout_view xmg_fanout{ xmg };
    xmg_fanout.foreach_fanout( n, [&]( const auto& p ){ parents.push_back( p ); } );
    return parents;
  }

  unsigned inv_manager::num_invs_fanouts( nd_t const& n ) const
  {
    unsigned cost = 0u;
    
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

  unsigned inv_manager::num_in_edges( nd_t const& n ) const
  {
    auto num_in = ( xmg.is_maj( n ) ? 3u : 2u );
    return num_in;
  }
  
  unsigned inv_manager::num_out_edges( nd_t const& n ) const
  {
    /* pos */
    auto num_po = 0u;
    xmg.foreach_po( [&]( auto const& f ) { if( xmg.get_node( f ) == n ) { num_po++; } } );
    
    return get_parents( n ).size() + num_po;
  }

  unsigned inv_manager::num_edges( nd_t const& n ) const
  {
    assert( xmg.is_maj( n ) || xmg.is_xor( n ) );
    return num_in_edges( n ) + num_out_edges( n );
  }

  unsigned inv_manager::num_invs_before( nd_t const& n ) const
  {
    return num_invs_fanins( n ) + num_invs_fanouts( n );
  }

  int inv_manager::one_level_savings( nd_t const& n ) const
  {
    int before, after;

    before = (int) num_invs_before( n );

    if( xmg.is_maj( n ) )
    {
      after = (int) num_edges( n ) - before;
    }
    else
    {
       int in, out;

       out = (int) num_out_edges( n ) - (int) num_invs_fanouts( n ); 
       
       auto tmp = (int) num_invs_fanins( n );
       
       if( tmp == 1 )
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
   
  int inv_manager::two_level_savings( nd_t const& n ) const
  {
    assert( !xmg.is_pi( n ) );

    auto parents = get_parents( n );
    int  total_savings = 0;

    /* no parents */
    if( parents.size() == 0u )
    {
      return one_level_savings( n );
    }
    else
    {
      auto child_savings = one_level_savings( n );

      for( const auto& p : parents )
      {
        total_savings += one_level_savings( p );

        xmg.foreach_fanin( n, [&]( auto s ) 
            {
              if( xmg.get_node( s ) == n && xmg.is_complemented( s ) )
              {
                total_savings--;
              }
              else
              {
                total_savings++;
              }
            }
            );
      }

      total_savings += child_savings;
    }

    return total_savings;
  }

  void inv_manager::complement_node( nd_t const& n )
  {
    auto children = get_children( n );

    if( xmg.is_maj( n ) )
    {
      children[0] = !children[0];
      children[1] = !children[1];
      children[2] = !children[2];

      auto opt = xmg.create_maj_without_complement_opt( children[0], children[1], children[2] ) ^ true ;
      xmg.substitute_node_without_complement_opt( n, opt );
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
      
      auto opt = xmg.create_xor_without_complement_opt( children[1], children[2] ) ^ true ;
      xmg.substitute_node_without_complement_opt( n, opt );
    }
  }
  
  std::array<signal<xmg_network>, 3> inv_manager::get_children( nd_t const& n ) const
  {
    std::array<signal<xmg_network>, 3> children;
    xmg.foreach_fanin( n, [&children]( auto const& f, auto i ) { children[i] = f; } );
    return children;
  }

  /* print information */
  void inv_manager::print_node( nd_t const& n ) const
  {
    std::cout << " node " << n << " inverters infor: ";
    xmg.foreach_fanin(n, [&]( auto s ) { std::cout << " { " << s.index << " , " << s.complement << " } "; } ); 
    std::cout << std::endl;
  }

  void inv_manager::print_network() const
  {
    xmg.foreach_gate( [&]( auto n ) { print_node( n ); std::cout << " cost= " << num_invs_before( n ) << " edges= " << num_edges( n ) << " one level savings: " << one_level_savings( n ) << " two level savings: " << two_level_savings( n ) << std::endl; } );
  }

/******************************************************************************
 * Public functions                                                           *
 ******************************************************************************/
  void xmg_inv_optimization( xmg_network& xmg )
  {
    std::cout << "num_invs: " << num_inverters( xmg ) << std::endl;
    inv_manager mgr( xmg );
    return mgr.run();
  }

}
