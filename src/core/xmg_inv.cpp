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
  class inv_manager
  {
    public:
    inv_manager( xmg_network& xmg );
    
    unsigned count_current_invs();
    void run();
    void complement_node( node<xmg_network> const& n );
    std::array<signal<xmg_network>, 3> get_children( node<xmg_network> const& n ) const;
    void print_node( node<xmg_network> const& n ) const;
    void print_network() const;

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
    complement_node( xmg.index_to_node( 9 ) );
    print_network();
    std::cout << "Optimized: " << num_inverters( xmg ) << std::endl;
  }

  void inv_manager::complement_node( node<xmg_network> const& n )
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
  
  std::array<signal<xmg_network>, 3> inv_manager::get_children( node<xmg_network> const& n ) const
  {
    std::array<signal<xmg_network>, 3> children;
    xmg.foreach_fanin( n, [&children]( auto const& f, auto i ) { children[i] = f; } );
    return children;
  }

  /* print information */
  void inv_manager::print_node( node<xmg_network> const& n ) const
  {
    std::cout << " node " << n << " inverters infor: ";
    xmg.foreach_fanin(n, [&]( auto s ) { std::cout << " { " << s.index << " , " << s.complement << " } "; } ); 
    std::cout << std::endl;
  }

  void inv_manager::print_network() const
  {
    xmg.foreach_gate( [&]( auto n ) { print_node( n ); } );
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
