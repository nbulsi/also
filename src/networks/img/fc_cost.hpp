/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China 
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * @file fc_cost.hpp
 *
 * @brief cost function for conflict fanouts in img_network
 *
 * Thanks Heinz Riener @ EPFL for providing advice for cost functions
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef FC_COST_HPP
#define FC_COST_HPP

#include <mockturtle/traits.hpp>

namespace mockturtle
{

  template<class Ntk>
    struct fc_cost
    {
      uint32_t operator()( Ntk const& ntk, node<Ntk> const& n ) const
      {
        /* Given a node n, it has two parents a and
         * b, the children of a are {c, n }, and the children of b are {d, n
         * }. This means node n has two fanouts and connecting to the
         * right-hand input of the implication node. We consider it as a
         * fanout conflict.
         * 
         * Now we want to find out all of these nodes and give them a cost of
         * 1, for other normal nodes without conflicts, just return 0.
         * */
        if( ntk.fanout_size( n ) >= 2u )
        {
          /* get parents */
          std::vector<node<Ntk>> parents;
          ntk.foreach_fanout( n, [&]( auto const& p ) { parents.push_back( p ); } );

          /* compare the children of the parents */
          uint32_t tmp = 0u;
          std::vector<node<Ntk>> working_mem_nodes;
          for( auto const& p : parents )
          {
            std::array<signal<Ntk>, 2u> children;
            ntk.foreach_fanin( p, [&children]( auto const& f, auto i) { children[i] = f; } );
            
            if( ntk.get_node( children[1] ) == n )
            {
              tmp++;
              working_mem_nodes.push_back( p );
            }
          }


          /* if tmp == 1, should further check the interlock pairs */
          if( tmp == 1u )
          {
            auto p = working_mem_nodes[0u];
            std::array<signal<Ntk>, 2u> children_working;
            ntk.foreach_fanin( p, [&children_working]( auto const& f, auto i) { children_working[i] = f; } );
            
            /* consider the child on the left side */
            auto c = ntk.get_node( children_working[0u] );
            
            /* compute the parents of this child */
            std::vector<node<Ntk>> parents_current;
            ntk.foreach_fanout( c, [&]( auto const& pc ) { parents_current.push_back( pc ); } );

            /* since all the parents must have one child labeled 'c' 
             * need check whether it has child 0 labeled 'node' 
             * */
            for( auto const& pc : parents_current )
            {
              if( pc != p )
              {
                std::array<signal<Ntk>, 2u> children_current;
                ntk.foreach_fanin( pc, [&children_current]( auto const& f, auto i) { children_current[i] = f; } );

                if( ntk.get_node( children_current[0u] )  == n ) /* find the interlock pairs */
                {
                  return 1u;
                }
              }
            }

            return 0u;
          }
          else
          {
            return ( tmp >= 2u ) ? 1u : 0u;
          }
        }
        else
        {
          return 0u;
        }

        return 0u; 
      }
    };

  template<class Ntk>
  uint32_t total_fc_cost( Ntk const& ntk )
  {
    uint32_t total{0u};
    fc_cost<fanout_view<Ntk>> cost_fn{};

    fanout_view fanout_ntk{ntk};
    fanout_ntk.foreach_node( [&]( auto const& n )
        {
          if( n != 0u )
          {
            total += cost_fn( fanout_ntk, n );
          }
        } );

    return total;
  }

}

#endif
