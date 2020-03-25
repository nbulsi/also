/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file img_utils.hpp
 *
 * @brief img utilization functions
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef IMG_UTILS_HPP
#define IMG_UTILS_HPP

#include <fmt/format.h>
#include <fmt/ostream.h>
#include "img.hpp"

namespace also
{

  std::array<signal<img_network>, 2> img_get_children( img_network const& img, node<img_network> const& n ) 
  {
    std::array<signal<img_network>, 2> children;
    img.foreach_fanin( n, [&children]( auto const& f, auto i ) { children[i] = f; } );
    return children;
  }

  unsigned img_num_inverters( img_network const& img )
  {
    unsigned num = 0u;

    topo_view topo{img};

    topo.foreach_node( [&]( auto n ){

        const auto& cs = img_get_children( img, n );

        if( img.get_node( cs[1] ) == 0 )
        {
        num++;
        }
        } );

    return num;
  }

  
  std::map<unsigned, std::set<node<img_network>>> img_fc_node_map( img_network const& img )
  {
    std::map<unsigned, std::set<node<img_network>>> m;
    
    unsigned num    = 0u;
    unsigned num_fc = 0u; //2 or more fanout are connecting the working memristor

    topo_view topo{img};
    fanout_view fanout_img{topo};

    fanout_img.foreach_node( [&]( auto n ) {
        if( fanout_img.fanout_size( n ) >= 2 && n != 0 )
        {
        num++;
        
        std::set<node<img_network>> nodes;
        std::set<node<img_network>> working_mem_nodes;
        
        fanout_img.foreach_fanout( n, [&]( const auto& p )
            { nodes.insert( p ); } );

        unsigned tmp = 0u;
        for( const auto& pc : nodes )
        {
        const auto& cs = img_get_children( img, pc );

        if( img.get_node( cs[1] ) == n )
        {
        tmp++;
        working_mem_nodes.insert( pc );
        }
        }

        if( tmp >= 2u )
        {
          num_fc++;
          m.insert( std::pair<unsigned, std::set<node<img_network>>>( n, working_mem_nodes ) );
        }
        }
    } );
    
    return m;
  }

  bool is_interlock_pairs( img_network const& img, node<img_network> const& a, node<img_network> const& b )
  {
    if( img.is_pi( a ) || img.is_constant( a ) ) return false;
    if( img.is_pi( b ) || img.is_constant( b ) ) return false;

    const auto ca = img_get_children( img, a );
    const auto cb = img_get_children( img, b );

    if( ( img.get_node( ca[0] ) == img.get_node( cb[1] ) ) && ( img.get_node( ca[1] ) == img.get_node( cb[0] ) ) )
    {
      return true;
    }
    else
    {
      //std::cout << fmt::format( "node {} children : {} and {}\n", a, img.get_node( ca[0] ), img.get_node( ca[1] ) );
      //std::cout << fmt::format( "node {} children : {} and {}\n", b, img.get_node( cb[0] ), img.get_node( cb[1] ) );
      return false;
    }
  }

  std::set<node<img_network>> get_fanout_set( img_network const& img, node<img_network> const& n )
  {
    assert( n != 0 );
    topo_view topo{img};
    fanout_view fanout_img{topo};
        
    std::set<node<img_network>> nodes;
        
    fanout_img.foreach_fanout( n, [&]( const auto& p )
            { nodes.insert( p ); } );

    return nodes;
  }

  std::set<std::array<node<img_network>, 2>> get_interlock_pairs( img_network const& img )
  {
    topo_view topo{img};
    fanout_view fanout_img{topo};

    std::set<std::array<node<img_network>, 2>> pairs;

    fanout_img.foreach_node( [&]( auto n ) {
        if( fanout_img.fanout_size( n ) >= 2 && n != 0 )
        {
         const auto& sets = get_fanout_set( img, n );

         for( const auto& e1 : sets )
         {
          for( const auto& e2 : sets )
          {
            if( ( e1 != e2 ) && ( e1 < e2 ) && is_interlock_pairs( img, e1, e2 ) )
            {
              std::array<node<img_network>, 2> array;
              array[0] = e1;
              array[1] = e2;
              pairs.insert( array );
            }
          }
         }
        }
    } );

    return pairs;
  }

  void init_img_refs( img_network& img )
  {
    img.clear_values();
    img.foreach_node( [&]( auto const& n ) {
        img.set_value( n, img.fanout_size( n ) );
        } );
  }


}

#endif
