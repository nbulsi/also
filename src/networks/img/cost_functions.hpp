/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file cost_functions.hpp
 *
 * @brief cost functions for implication logic network
 * fanout-conflict rewriting
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef COST_FUNCTIONS_HPP
#define COST_FUNCTIONS_HPP

#include "img_utils.hpp"
#include "img.hpp"

namespace also 
{
  using Ntk    = img_network; 
  using node_t = node<Ntk>;

  struct img_fc_cost
  {
    uint32_t operator()( Ntk const& ntk, node_t const& n ) const
    {
      fanout_view img{ntk};

      if( ntk.fanout_size( n ) >= 2 )
      {
        auto nodes = get_fanout_set( ntk, n );
        std::set<node_t> working_mem_nodes;
        
        /* record the number of fanouts that point to working
         * memristors */
        uint32_t tmp = 0u;
        for( const auto& pc : nodes )
        {
          const auto& cs = img_get_children( ntk, pc );

          if( ntk.get_node( cs[1] ) == n )
          {
            tmp++;
            working_mem_nodes.insert( pc );
          }
        }

        /* at least connecting 2 working memristors 
         * or nothing connet 
         * */
        if( tmp >= 2u || tmp == 0u )
        {
          return tmp;
        }
        else /* tmp == 1u, check for interlock pairs */
        {
          auto parent = *working_mem_nodes.begin();
          const auto& cs = img_get_children( ntk, parent );

          auto child = ntk.get_node( cs[0] );
          auto cf = get_fanout_set( ntk, child );

          for( auto const& p : cf )
          {
            auto c = img_get_children( ntk, p );

            if( ntk.get_node( c[0] ) == n )
            {
              return 1u;
            }
          }

          return 0u;
        }
      }

      return 0u;
    }
  };

  uint32_t fc_cost( Ntk const& ntk )
  {
    uint32_t total{0u};
    img_fc_cost cost_fn{};

    ntk.foreach_node( [&]( auto const& n )
        { 
        if( n != 0 )
        {
          total += cost_fn( ntk, n ); 
        }
        } );

    return total;
  }


}

#endif
