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
        fanout_img.foreach_fanout( n, [&]( const auto& p )
            { nodes.insert( p ); } );

        unsigned tmp = 0u;
        for( const auto& pc : nodes )
        {
        const auto& cs = img_get_children( img, pc );

        if( img.get_node( cs[1] ) == n )
        {
        tmp++;
        }
        }

        if( tmp >= 2u )
        {
          num_fc++;
          m.insert( std::pair<unsigned, std::set<node<img_network>>>( n, nodes ) );
        }
        }
    } );
    
    return m;
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
