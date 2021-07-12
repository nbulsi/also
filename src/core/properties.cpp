/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019-2021 Ningbo University, Ningbo, China */

#include "properties.hpp"


namespace also
{
/******************************************************************************
 * Types                                                                      *
 ******************************************************************************/

/******************************************************************************
 * Private functions                                                          *
 ******************************************************************************/
  std::vector<uint32_t> split_xmg_critical_path( xmg_network const& xmg )
  {
    uint32_t xor3{ 0 }, xor2{ 0 }, maj{ 0 }, and_or{ 0 };
    depth_view dxmg{ xmg };

    node<xmg_network> cp_node;

    /* find the po on critical path */
    xmg.foreach_po( [&]( auto const& f ) {
        auto level = dxmg.level( xmg.get_node( f ) );

        if( level == dxmg.depth() )
        {
          cp_node = xmg.get_node( f );
          return false;
        }

        return true;
        } );

    /* recursive until reach the primary inputs */
    while( !xmg.is_constant( cp_node ) && !xmg.is_pi( cp_node ) )
    {
      bool has_constant_fanin = false;

      /* check if all the fanin nodes are not constant */
      xmg.foreach_fanin( cp_node, [&]( auto const& f ) {
          if( xmg.is_constant( xmg.get_node( f ) ) )
          {
            has_constant_fanin = true;
            return false;
          }
          return true;
          } );

      if( xmg.is_maj( cp_node ) )
      {
        if( has_constant_fanin )
        {
          ++and_or;
        }
        else
        {
          ++maj;
        }
      }
      else if( xmg.is_xor3( cp_node ) )
      {
        if( has_constant_fanin )
        {
          ++xor2;
        }
        else
        {
          ++xor3;
        }
      }
      else
      {
        assert( false && "UNKNOWN operator" );
      }

      /* continue process fanin node */
      xmg.foreach_fanin( cp_node, [&]( auto const& f ) { 
            auto level = dxmg.level( xmg.get_node( f ) );

            if( level + 1 == dxmg.level( cp_node ) )
            {
              cp_node = xmg.get_node( f );
              return false;
            }
            return true;
          } );
    }

    std::vector<uint32_t> v{ xor3, xor2, maj, and_or };
    return v;
  }

/******************************************************************************
 * Public functions                                                           *
 ******************************************************************************/
  void xmg_critical_path_profile_gates( xmg_network const& xmg, xmg_critical_path_stats& stats )
  {
    auto v = split_xmg_critical_path( xmg );
    stats.xor3   = v[0];
    stats.xor2   = v[1];
    stats.maj    = v[2];
    stats.and_or = v[3];
  }

}
