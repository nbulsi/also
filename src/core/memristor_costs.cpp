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
 * @file memristor_costs.cpp
 *
 * @brief memristor costs
 *
 * @author YanMing
 * @since  0.1
 */

#include "memristor_costs.hpp"

#include "../networks/rm3/RM3.hpp"
#include <map>
//#include <mockturtle/properties/migcost.hpp>
#include <mockturtle/views/depth_view.hpp>
#include <mockturtle/views/topo_view.hpp>
#include <unordered_map>
#include <utility>
#include <vector>

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
std::pair<unsigned, unsigned> memristor_costs( rm3_network rm3 )
{
  // 创建 depth_view 实例
  depth_view<rm3_network> rm3_depth{ rm3 };

  // 获取网络的最大深度
  auto max_level = rm3_depth.depth();

  // 获取每个节点的级别
  std::map<rm3_network::node, unsigned> levels;
  rm3.foreach_node( [&]( auto const& n )
                    { levels[n] = rm3_depth.level( n ); } );

  // 将节点按级别分类
  std::map<unsigned, std::vector<rm3_network::node>> level_to_nodes;
  for ( const auto& p : levels )
  {
    level_to_nodes[p.second].push_back( p.first );
  }

  auto current_max_size = 0u;
  auto levels_with_complement = 0u;

  for ( auto l = 1u; l <= max_level; ++l )
  {
    auto size = 3u * level_to_nodes[l].size();
    if ( size > current_max_size )
    {
      current_max_size = size;
    }
  }

  auto flag = current_max_size;
  auto flag1 = current_max_size;
  int inv = 0;

  // 处理输出节点的补边情况
  rm3.foreach_po( [&]( const auto& f )
                  {
    if (rm3.is_complemented(f))
    {
      ++flag1;
      ++inv;
    } } );
  // std::cout << "po_inv" << inv << std::endl;
  if ( inv > current_max_size )
  {
    current_max_size = inv;
  }

  if ( flag1 > flag )
  {
    ++levels_with_complement;
  }

  std::cout << "[i] current_max_size:       " << current_max_size << std::endl
            << "[i] levels_with_complement: " << levels_with_complement << std::endl
            << "[i] max_level:              " << max_level << std::endl;

  auto operations = max_level * 3u + levels_with_complement;

  return { current_max_size, operations };
}
}
