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
    return children;
  }
}
