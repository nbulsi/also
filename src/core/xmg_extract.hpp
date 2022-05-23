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
  
  std::vector<unsigned> xmg_extract( xmg_network const& xmg )
  {
    std::vector<unsigned> xors;
    topo_view topo{ xmg };
    circuit_validator v( xmg );

    int count = 0;
    
    topo.foreach_node( [&]( auto n ) {
    if( xmg.is_xor3( n ) )
    {
      auto children = get_children( xmg, n );

      //print_children( children );

      if( xmg.get_node( children[0] ) == 0 )
      { 
        auto result1 = v.validate( children[1], children[2] );
        auto result2 = v.validate( children[1], !children[2] );

        if( result1 || result2 )
        {
          if( *result1 )
          {
          //std::cout << "XOR node " << n  << " can be removed as 0\n";
          xors.push_back( n );
          count++;
          }
          else if( *result2 )
          {
            //std::cout << "XOR can be removed as 1\n";
            count++;
          }
        }
      }
    }
    } );

    std::cout << "[xmgrw -u] Find " << count << " XOR constant nodes\n";
    return xors;
  }
}

#endif
