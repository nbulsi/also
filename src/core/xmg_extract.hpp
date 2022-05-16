/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019-2021 Ningbo University, Ningbo, China */

/**
 * @file xmg_extract.hpp
 *
 * @brief extract all XOR nodes to construct XMGs
 *
 * @author Zhufei
 * @since  0.1
 */

#ifndef XMG_EXTRACT_HPP
#define XMG_EXTRACT_HPP

#include <mockturtle/algorithms/circuit_validator.hpp>
#include <mockturtle/views/topo_view.hpp>

#include "utils.hpp"

using namespace mockturtle;

namespace also
{
  
  void xmg_extract( xmg_network const& xmg )
  {
    topo_view topo{ xmg };
    circuit_validator v( xmg );
    
    topo.foreach_node( [&]( auto n ) {
    if( xmg.is_xor3( n ) )
    {
    std::cout << "XOR" << std::endl;

    auto children = get_children( xmg, n );

    if( xmg.get_node( children[0] ) == 0 )
    { 
      auto result = v.validate( children[1], children[2] );
      if( result )
      {
        if( *result )
        {
          std::cout << "XOR can be removed\n";
        }
      }
    }

    }
        } );
  }
}

#endif
