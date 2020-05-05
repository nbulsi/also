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
  \file m5ig_npn.hpp
  \brief Replace with size-optimum m5igs from NPN

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

#include "m5ig.hpp"
#include "m5ig_cleanup.hpp"
#include "../core/misc.hpp"

namespace mockturtle
{

/*! \brief Resynthesis function based on pre-computed size-optimum m5igs.
 *
 * This resynthesis function can be passed to ``node_resynthesis``,
 * ``cut_rewriting``, and ``refactoring``.  It will produce an m5ig based on
 * pre-computed size-optimum m5igs with up to at most 4 variables.
 * Consequently, the nodes' fan-in sizes in the input network must not exceed
 * 4.
 *
   \verbatim embed:rst
  
   Example
   
   .. code-block:: c++
   
      const klut_network klut = ...;
      m5ig_npn_resynthesis resyn;
      const auto m5ig = node_resynthesis<m5ig_network>( klut, resyn );
   \endverbatim
 */
class m5ig_npn_resynthesis
{
public:
  /*! \brief Default constructor.
   *
   */
  m5ig_npn_resynthesis( )
  {
    build_db();
  }

  template<typename LeavesIterator, typename Fn>
  void operator()( m5ig_network& m5ig, 
                   kitty::dynamic_truth_table const& function, 
                   LeavesIterator begin, 
                   LeavesIterator end, 
                   Fn&& fn ) const
  {
    assert( function.num_vars() <= 4 );
    const auto fe = kitty::extend_to( function, 4 );
    const auto config = kitty::exact_npn_canonization( fe );

    auto func_str = "0x" + kitty::to_hex( std::get<0>( config ) ); 
    //std::cout << "func: " << func_str << std::endl;
    const auto it = class2signal.find( func_str );
    assert( it != class2signal.end() );

    std::vector<m5ig_network::signal> pis( 4, m5ig.get_constant( false ) );
    std::copy( begin, end, pis.begin() );

    std::vector<m5ig_network::signal> pis_perm( 4 );
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
      auto f = m5ig_cleanup_dangling( topo, m5ig, pis_perm.begin(), pis_perm.end() ).front();

      if ( !fn( ( ( phase >> 4 ) & 1 ) ? !f : f ) )
      {
        return; /* quit */
      }
    }
  }

private:
  std::unordered_map<std::string, std::vector<std::string>> opt_m5ig;
  
  void load_optimal_m5ig()
  {
    std::ifstream infile( "../src/networks/m5ig/opt_m5ig.txt" );
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
      opt_m5ig.insert( std::make_pair( strs[0],  v ) );
    }
  }

  std::vector<m5ig_network::signal> create_m5ig_from_str_vec( const std::vector<std::string> strs, 
                                                              const std::vector<m5ig_network::signal>& signals )
  {
    auto sig = signals;
    auto size = strs.size();
    auto pol = std::stoi( strs[0] );

    std::vector<m5ig_network::signal> result;
    
    int a, b, c, d, e;

    /*for( const auto& s : strs )
    {
      std::cout << s << std::endl;
    }*/

    for( auto i = 1; i < size; i++ )
    {
      const auto substrs = also::split_by_delim( strs[i], '-' );

      assert( substrs.size() == 3u );
      
      a = substrs[2][0] - '0';
      b = substrs[2][1] - '0';
      c = substrs[2][2] - '0';
      d = substrs[2][3] - '0';
      e = substrs[2][4] - '0';
#if 0
      std::cout << " a: " << a << " sig[a]: " << sig[a].index << std::endl
                << " b: " << b << " sig[b]: " << sig[b].index << std::endl
                << " c: " << c << " sig[c]: " << sig[c].index << std::endl
                << " d: " << d << " sig[d]: " << sig[d].index << std::endl
                << " e: " << e << " sig[e]: " << sig[e].index << std::endl;
#endif
      switch( std::stoi( substrs[1] ) )
      {
        case 0:
          sig.push_back( db.create_maj5( sig[a], sig[b], sig[c], sig[d], sig[e] ) );
          break;

        case 1:
          sig.push_back( db.create_maj5( sig[a] ^ true, sig[b] , sig[c], sig[d], sig[e] ) );
          break;
        
        case 2:
          sig.push_back( db.create_maj5( sig[a], sig[b] ^ true, sig[c], sig[d], sig[e] ) );
          break;
        
        case 3:
          sig.push_back( db.create_maj5( sig[a], sig[b], sig[c] ^ true, sig[d], sig[e] ) );
          break;
        
        case 4:
          sig.push_back( db.create_maj5( sig[a], sig[b], sig[c], sig[d] ^ true, sig[e] ) );
          break;
        
        case 5:
          sig.push_back( db.create_maj5( sig[a], sig[b], sig[c], sig[d], sig[e] ^ true ) );
          break;
        
        case 6:
          sig.push_back( db.create_maj5( sig[a] ^ true, sig[b] ^ true, sig[c], sig[d], sig[e] ) );
          break;
        
        case 7:
          sig.push_back( db.create_maj5( sig[a] ^ true, sig[b], sig[c] ^ true, sig[d], sig[e] ) );
          break;
        
        case 8:
          sig.push_back( db.create_maj5( sig[a] ^ true, sig[b], sig[c], sig[d] ^ true, sig[e] ) );
          break;
        
        case 9:
          sig.push_back( db.create_maj5( sig[a] ^ true, sig[b], sig[c], sig[d], sig[e] ^ true ) );
          break;
        
        case 10:
          sig.push_back( db.create_maj5( sig[a], sig[b] ^ true, sig[c] ^ true, sig[d], sig[e] ) );
          break;
        
        case 11:
          sig.push_back( db.create_maj5( sig[a], sig[b] ^ true, sig[c], sig[d] ^ true, sig[e] ) );
          break;
        
        case 12:
          sig.push_back( db.create_maj5( sig[a], sig[b] ^ true, sig[c], sig[d], sig[e] ^ true ) );
          break;
        
        case 13:
          sig.push_back( db.create_maj5( sig[a], sig[b], sig[c] ^ true, sig[d] ^ true, sig[e] ) );
          break;
        
        case 14:
          sig.push_back( db.create_maj5( sig[a], sig[b], sig[c] ^ true, sig[d], sig[e] ^ true ) );
          break;
        
        case 15:
          sig.push_back( db.create_maj5( sig[a], sig[b], sig[c], sig[d] ^ true, sig[e] ^ true ) );
          break;

        default: assert( false ); break;
      }
    }
    
    const auto driver = sig[ sig.size() - 1] ^ ( pol ? true : false );
    db.create_po( driver );
    result.push_back( driver ); 
    return result;
  }

  void build_db()
  {
    std::vector<m5ig_network::signal> signals;
    signals.push_back( db.get_constant( false ) );

    //note we do not use pi[5]
    for ( auto i = 0u; i < 4; ++i )
    {
      signals.push_back( db.create_pi() );
    }

    //void signal[5]
    signals.push_back( db.get_constant( false ) );
    
    load_optimal_m5ig();

    for( const auto e : opt_m5ig )
    {
      if( e.first == "0x0000" )
      {
        std::vector<m5ig_network::signal> tmp{ signals[0] };
        class2signal.insert( std::make_pair( e.first, tmp ) );
      }
      else if( e.first == "0x00ff" )
      {
        std::vector<m5ig_network::signal> tmp{ signals[4] ^ true };
        class2signal.insert( std::make_pair( e.first, tmp ) );
      }
      else
      {
        class2signal.insert( std::make_pair( e.first, create_m5ig_from_str_vec( e.second, signals ) ) );
      }
    }

  }

  m5ig_network db;
  std::unordered_map< std::string, std::vector<m5ig_network::signal>> class2signal;

};

} /* namespace mockturtle */
