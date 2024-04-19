/* mockturtle: C++ logic network library
 * Copyright (C) 2018-2019  EPFL
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

/*!
  \file xag_lut_npn.hpp
  \brief Replace with size-optimum XAGs from NPN

  \author Zhufei Chu
*/

#pragma once

#include <iostream>
#include <sstream>
#include <stack>
#include <unordered_map>
#include <vector>

#include <kitty/dynamic_truth_table.hpp>
#include <kitty/npn.hpp>
#include <kitty/print.hpp>

#include "build_xag_db.hpp"

namespace mockturtle
{

/*! \brief Resynthesis function based on pre-computed size-optimum xags.
 *
 * This resynthesis function can be passed to ``node_resynthesis``,
 * ``cut_rewriting``, and ``refactoring``.  It will produce an xag based on
 * pre-computed size-optimum xags with up to at most 4 variables.
 * Consequently, the nodes' fan-in sizes in the input network must not exceed
 * 4.
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      const klut_network klut = ...;
      xag_npn_lut_resynthesis resyn;
      const auto xag = node_resynthesis<xag_network>( klut, resyn );
   \endverbatim
 */
class xag_npn_lut_resynthesis
{
public:
  /*! \brief Default constructor.
   *
   */
  xag_npn_lut_resynthesis()
  {
    class2signal = also::get_xag_db( db );
  }

  template<typename LeavesIterator, typename Fn>
  void operator()( xag_network& xag, kitty::dynamic_truth_table const& function, LeavesIterator begin, LeavesIterator end, Fn&& fn ) const
  {
    assert( function.num_vars() <= 4 );
    const auto fe = kitty::extend_to( function, 4 );
    const auto config = kitty::exact_npn_canonization( fe );

    auto func_str = "0x" + kitty::to_hex( std::get<0>( config ) );
    const auto it = class2signal.find( func_str );
    assert( it != class2signal.end() );

    //const auto it = class2signal.find( static_cast<uint16_t>( std::get<0>( config )._bits[0] ) );

    std::vector<xag_network::signal> pis( 4, xag.get_constant( false ) );
    std::copy( begin, end, pis.begin() );

    std::vector<xag_network::signal> pis_perm( 4 );
    auto perm = std::get<2>( config );
    for ( auto i = 0; i < 4; ++i )
    {
      pis_perm[i] = pis[perm[i]];
    }

    const auto& phase = std::get<1>( config );
    for ( auto i = 0; i < 4; ++i )
    {
      if ( ( phase >> perm[i] ) & 1 )
      {
        pis_perm[i] = !pis_perm[i];
      }
    }

    for ( auto const& po : it->second )
    {
      topo_view topo{db, po};
      auto f = cleanup_dangling( topo, xag, pis_perm.begin(), pis_perm.end() ).front();

      if ( !fn( ( ( phase >> 4 ) & 1 ) ? !f : f ) )
      {
        return; /* quit */
      }
    }
  }

private:
  xag_network db;
  std::unordered_map<std::string, std::vector<xag_network::signal>> class2signal;
};

} /* namespace mockturtle */
