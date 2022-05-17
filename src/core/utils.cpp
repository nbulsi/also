/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

#include "utils.hpp"

using namespace mockturtle;

namespace also
{

/******************************************************************************
 * Types                                                                      *
 ******************************************************************************/

/******************************************************************************
 * Private functions                                                          *
 ******************************************************************************/

/******************************************************************************
 * Public functions                                                           *
 ******************************************************************************/
  std::array<xmg_network::signal, 3> get_children( xmg_network const& xmg, xmg_network::node const& n )
  {
    std::array<xmg_network::signal, 3> children;
    xmg.foreach_fanin( n, [&children]( auto const& f, auto i ) { children[i] = f; } );
    std::sort( children.begin(), children.end(), [&]( auto const& c1, auto const& c2 ) {
        return c1.index < c2.index;
        } );
    return children;
  }

  void print_children( std::array<xmg_network::signal, 3> const& children )
  {
    auto i = 0u;
    for( auto child : children )
    {
      std::cout << "children " << i << " is " << child.index << " complemented ? " << child.complement << std::endl;
      i++;
    }
  }
}
