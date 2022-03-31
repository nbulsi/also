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

/*!
  \file img_npn.hpp
  \brief Replace with size-optimum imgs from NPN

  \author Zhufei Chu
*/

#pragma once

#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>

#include <kitty/dynamic_truth_table.hpp>
#include <kitty/npn.hpp>
#include <kitty/print.hpp>

#include <mockturtle/mockturtle.hpp>

#include "img.hpp"
#include "../../core/misc.hpp"

namespace mockturtle
{

/*! \brief Resynthesis function based on pre-computed size-optimum imgs.
 *
 * This resynthesis function can be passed to ``node_resynthesis``,
 * ``cut_rewriting``, and ``refactoring``.  It will produce an img based on
 * pre-computed size-optimum imgs with up to at most 3 variables.
 * Consequently, the nodes' fan-in sizes in the input network must not exceed
 * 3.
 *
   \verbatim embed:rst
  
   Example
   
   .. code-block:: c++
   
      const klut_network klut = ...;
      img_npn_resynthesis resyn;
      const auto img = node_resynthesis<img_network>( klut, resyn );
   \endverbatim
 */
class img_npn_resynthesis
{
public:
  /*! \brief Default constructor.
   *
   */
   img_npn_resynthesis( )
  {
    build_db();
  }

  template<typename LeavesIterator, typename Fn>
  void operator()( img_network& img, 
                   kitty::dynamic_truth_table const& function, 
                   LeavesIterator begin, 
                   LeavesIterator end, 
                   Fn&& fn )
  {
    assert( function.num_vars() <= 3 );
    const auto fe = kitty::extend_to( function, 3 );
    const auto config = kitty::exact_npn_canonization( fe );

    auto func_str = "0x" + kitty::to_hex( std::get<0>( config ) );
    const auto it = class2signal.find( func_str );
    assert( it != class2signal.end() );

    std::vector<img_network::signal> pis( 3, img.get_constant( false ) );
    std::copy( begin, end, pis.begin() );

    std::vector<img_network::signal> pis_perm( 3 );
    auto perm = std::get<2>( config );
    for ( auto i = 0; i < 3; ++i )
    {
      pis_perm[i] = pis[perm[i]];
    }

    const auto& phase = std::get<1>( config );
    for ( auto i = 0; i < 3; ++i )
    {
      if ( ( phase >> perm[i] ) & 1 )
      {
        pis_perm[i] = !pis_perm[i];
        //TODO: HERE we need to add new nodes to convert invtered
        //signals
      }
    }

    for ( auto const& po : it->second )
    {
      topo_view topo{db, po};
      auto f = cleanup_dangling( topo, img, pis_perm.begin(), pis_perm.end() ).front();

      if ( !fn( ( ( phase >> 3 ) & 1 ) ? !f : f ) ) 
      {
        //TODO: HERE we need to add new nodes to convert invtered
        //signals
        return; /* quit */
      }
    }
  }

private:
  std::unordered_map<std::string, std::vector<std::string>> opt_img;
  
  void load_optimal_img()
  {
    std::ifstream infile( "../src/networks/img/opt_img_npn3.txt" );
    if( !infile )
    {
      std::cout << " Cannot open file " << std::endl; 
      assert( false );
    }

    std::string line;
    std::vector<std::string> v;
    while (std::getline(infile, line) )
    {
      v.clear();
      auto strs = also::split_by_delim( line, ' ' );
      for( auto i = 1u; i < strs.size(); i++ )
      {
        v.push_back( strs[i] );
      }
      opt_img.insert( std::make_pair( strs[0],  v ) );
    }
  }

  std::vector<img_network::signal> create_img_from_str_vec( const std::vector<std::string> strs, 
                                                            const std::vector<img_network::signal>& signals )
  {
    auto sig  = signals;
    auto size = strs.size();

    std::vector<img_network::signal> result;
    
    int a, b;

    for( auto i = 0; i < size; i++ )
    {
      const auto substrs = also::split_by_delim( strs[i], '-' );

      assert( substrs.size() == 3u );

      a = std::stoi( substrs[1] );
      b = std::stoi( substrs[2] );

      auto f = db.create_imp( sig[a], sig[b] );
      sig.push_back( f );
    }
    
    const auto driver = sig[ sig.size() - 1]; 
    db.create_po( driver );
    result.push_back( driver ); 
    return result;
  }

  void build_db()
  {
    std::vector<img_network::signal> signals;
    signals.push_back( db.get_constant( false ) );

    for ( auto i = 0u; i < 3; ++i )
    {
      signals.push_back( db.create_pi() );
    }

    load_optimal_img();

    for( const auto e : opt_img )
    {
      if( e.first == "0x00" )
      {
        std::vector<img_network::signal> tmp{ signals[0] };
        class2signal.insert( std::make_pair( e.first, tmp ) );
      }
      else
      {
        class2signal.insert( std::make_pair( e.first, create_img_from_str_vec( e.second, signals ) ) );
      }
    }

  }

  img_network db;
  std::unordered_map< std::string, std::vector<img_network::signal>> class2signal;

};

} /* namespace mockturtle */
