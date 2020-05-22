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
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef FC_COST_HPP
#define FC_COST_HPP

namespace mockturtle
{
  template<class Ntk>
    struct fc_cost
    {
      uint32_t operator()( Ntk const& ntk, node<Ntk> const& node ) const
      {
        /* Given a node n, it has two parents (how to compute parent?) a and
         * b, the children of a are {c, n }, and the children of b are {d, n
         * }. This means node n has two fanouts and connecting to the
         * right-hand input of the implication node. We consider it as a
         * fanout conflict.
         * 
         * Now we want to find out all of these nodes and give them a cost of
         * 1, for other normal nodes without conflicts, just return 0.
         * */

        return ntk.fanout_size( node ); // as an example
      }
    };


}

#endif
