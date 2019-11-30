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
  \file  img_cut.hpp
  \brief Cut enumeration using the exact_img as the cost function 

  \author Zhufei Chu
*/

#pragma once

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>

#include <mockturtle/mockturtle.hpp>
#include "../../networks/img/img.hpp"
#include "../../core/misc.hpp"
#include "../../core/img_rewriting.hpp"

namespace mockturtle
{

class img_p3_synthesis
{
  public:
  img_p3_synthesis()
  {
  }

  img_network create_img_from_str( const std::string&  img_expr )
  {
    img_network img;
    also::img_depth_rewriting_params ps;
    std::string str = img_expr;

    std::vector<img_network::signal> signals;
    signals.push_back( img.get_constant( false ) );

    //create three primary inputs
    for ( auto i = 0u; i < 3u; ++i )
    {
      signals.push_back( img.create_pi() );
    }

    std::stack<img_network::signal> inputs;

    //string parsing
    for ( auto i = 0ul; i < str.size(); i++ )
    {
      if( str[i] == '(' )
      {
        continue;
      }
      else if( str[i] >= 'a' && str[i] <= 'c' ) 
      {
        inputs.push( signals[str[i] - 'a' + 1 ] );
      }
      else if( str[i] == '0' )
      {
        inputs.push( signals[0] );
      }
      else if( str[i] == ')' )
      {
        assert( inputs.size() >= 2u );

        auto x1 = inputs.top(); inputs.pop();
        auto x2 = inputs.top(); inputs.pop();
        
        //the expreesion a -> b, push in stack, pop out as b, a
        inputs.push( x1.index == 0u ? img.create_not( x2 ): img.create_imp( x2, x1 ) );

      }
      else
      {
        assert( false && "UNKNOWN char" );
      }
    }

    img.create_po( inputs.top() );

    /* rewriting */
    ps.allow_area_increase = false;
    depth_view depth_img( img );
    also::img_depth_rewriting( depth_img, ps );
    return cleanup_dangling( img );
  }

  std::pair<unsigned, unsigned> img_from_expr( const std::string& query_tt )
  {
      load_imgs();
      auto img = create_img_from_str( opt_imgs[query_tt] );
      depth_view depth_img( img );

      auto size  = img.num_gates();
      auto depth = depth_img.depth();
      return std::make_pair( size, depth );
  }

  private:
    std::unordered_map<std::string, std::string> opt_imgs;
    
    std::string p3_s = "0xfe ((((cb)b)a)a)\n0x6a ((ac)((ab)((c(ba))0)))\n0x6b ((b(ca))(((ca)b)((ac)0)))\n0x2f ((ab)(c0))\n0x69 (((ab)((ba)c))((c(ba))((c(ab))0)))\n0x3f (c(b0))\n0x28 ((b(ac))((a((ac)b))0))\n0x2d (((ab)c)((c(ab))0))\n0x19 (((ab)b)((a((ab)c))0))\n0x86 (((a(bc))(((bc)a)((cb)0)))0)\n0x3e ((((ac)c)b)((bc)0))\n0xa8 ((((cb)b)(a0))0)\n0x3c ((bc)((cb)0))\n0xad ((c(a0))(((ab)c)0))\n0x2b (((ba)c)((ab)0))\n0x1e ((c((ab)b))((((ab)b)c)0))\n0x68 ((((b(c0))0)a)((((c0)b)(a((b(c0))0)))0))\n0x8e (((cb)(((bc)a)0))0)\n0x07 (((a(bc))c)0)\n0x1f (c(((ab)b)0))\n0x3d (((ac)b)((bc)0))\n0x06 (((a(b0))(((b0)a)c))0)\n0x02 ((a((c0)b))0)\n0x2e ((bc)((ab)0))\n0x0a ((ac)0)\n0x2a ((ac)((ab)0))\n0x03 (((b0)c)0)\n0x08 ((a(bc))0)\n0x8b (((cb)((ba)0))0)\n0x17 (((c0)b)(((b(c0))a)0))\n0x01 (((b0)((ca)a))0)\n0x09 (((ba)((ab)c))0)\n0x18 (((ab)(cb))((a((ab)c))0))\n0x29 (((ba)((ab)c))((c(ab))0))\n0xbe ((bc)((cb)a))\n0x83 ((a(b(c0)))(((c0)b)0))\n0x0b (((ba)c)0)\n0xab (((c0)b)a)\n0x0e ((((ac)b)c)0)\n0x1a ((c((ab)b))((ac)0))\n0x6e ((bc)((ab)((ba)0)))\n0x7f (a(c(b0)))\n0xae ((bc)a)\n0x2c ((bc)((c(ab))0))\n0x0f (c0)\n0x81 (((cb)((ac)((ba)0)))0)\n0x8f ((a(b0))(c0))\n0x6f (c((ba)((ab)0)))\n0x80 ((c(a(b0)))0)\n0xa9 ((a(((c0)b)0))(((((c0)b)0)a)0))\n0x82 (((cb)((bc)(a0)))0)\n0xbd (((a0)b)((c(a0))((bc)0)))\n0x7e ((ac)((ba)((cb)0)))\n0xee ((b0)a)\n0x97 (((a(c0))b)((b(a(c0)))(((c0)a)0)))\n0xe8 ((((b0)c)(a0))((c(b0))0))\n0x16 ((((c0)a)((a(c0))b))((b((c0)a))0))\n0x88 ((a(b0))0)\n0x1b (((ba)c)(((ba)a)0))\n0x89 (((((b0)c)a)a)((a(b0))0))\n0x98 ((((cb)a)((ab)0))0)\n0x99 (((ab)((ba)0))0)\n0x9a ((((cb)a)((a(cb))0))0)\n0x9b (((a0)b)(((cb)(a0))0))\n0xaf (ca)\n0x9e (((ca)(b0))((ac)(((b0)(ca))0)))\n0x9f (c(((ab)((ba)0))0))\n0xac ((a(c0))((bc)0))\n0xe9 ((b(((ac)c)0))(((ac)((ca)b))0))\n0xbc (((ca)(b0))((cb)0))\n0xbf (b(ca))\n0x96 ((b((ac)((ca)0)))((((ac)((ca)0))b)0))\n0xea ((b(ca))a)\n0x8a ((a((cb)0))0)\n0xeb (((ca)b)((b(ca))a))\n0x87 ((c(a(b0)))(((a(b0))c)0))\n0xef ((b0)(ca))";

    inline std::vector<std::string> split( const std::string& str, const std::string& sep )
    {
      std::vector<std::string> result;

      size_t last = 0;
      size_t next = 0;
      while ( ( next = str.find( sep, last ) ) != std::string::npos )
      {
        result.push_back( str.substr( last, next - last ) );
        last = next + 1;
      }
      result.push_back( str.substr( last ) );

      return result;
    }

    void load_imgs()
    {
      std::vector<std::string> result;

      result = split( p3_s, "\n" );
      
      for ( auto record : result )
      {
        auto p = split( record, " " );
        assert( p.size() == 2u );
        opt_imgs.insert( std::make_pair( p[0], p[1] ) );
      }
    }
};

std::pair<unsigned, unsigned> EvalImg( const std::string& query_tt )
{
  img_p3_synthesis m;

  return m.img_from_expr( query_tt );
}


struct cut_enumeration_img_cut
{
  uint32_t delay{0};
  float flow{0};
  float cost{0};
};

template<bool ComputeTruth>
bool operator<( cut_type<ComputeTruth, cut_enumeration_img_cut> const& c1, cut_type<ComputeTruth, cut_enumeration_img_cut> const& c2 )
{
  constexpr auto eps{0.005f};
  if ( c1->data.flow < c2->data.flow - eps )
    return true;
  if ( c1->data.flow > c2->data.flow + eps )
    return false;
  if ( c1->data.delay < c2->data.delay )
    return true;
  if ( c1->data.delay > c2->data.delay )
    return false;
  return c1.size() < c2.size();
}

template<>
struct cut_enumeration_update_cut<cut_enumeration_img_cut>
{
  template<typename Cut, typename NetworkCuts, typename Ntk>
  static void apply( Cut& cut, NetworkCuts const& cuts, Ntk const& ntk, node<Ntk> const& n )
  {
    /******************************************************************
     * Zhufei Chu
     ******************************************************************/
    int img_size, img_depth;

    //std::cout << "AIG node: " << n << " ";
    auto tt = cuts.truth_table( cut );
    
    assert( tt.num_vars() <= 3 );
    const auto fe = kitty::extend_to( tt, 3 );
    const auto config = kitty::exact_p_canonization( fe );

    auto tt_str = kitty::to_hex( std::get<0>( config ) );
    auto func_str = "0x" + tt_str;

    if( func_str == "0xff" || func_str == "0x00" || func_str == "0xaa" )
    {
      img_size = 0;
      img_depth = 0;
    }
    else
    {
      auto p = EvalImg( func_str );
      img_size = p.first;
      img_depth = p.second;
    }
     
    /******************************************************************/

    uint32_t delay{0};
    float flow = cut->data.cost = cut.size() < 2 ? 0.0f : 1.0f;
    cut->data.cost = img_depth;

    for ( auto leaf : cut )
    {
      const auto& best_leaf_cut = cuts.cuts( leaf )[0];
      delay = std::max( delay, best_leaf_cut->data.delay );
      flow += best_leaf_cut->data.flow;
    }

    cut->data.delay = 1 + delay;
    cut->data.flow = flow / ntk.fanout_size( n );

    //std::cout << "delay: " << cut->data.delay << " area_flow: " << cut->data.flow << std::endl;
  }
};

template<int MaxLeaves>
std::ostream& operator<<( std::ostream& os, cut<MaxLeaves, cut_data<false, cut_enumeration_img_cut>> const& c )
{
  os << "{ ";
  std::copy( c.begin(), c.end(), std::ostream_iterator<uint32_t>( os, " " ) );
  os << "}, D = " << std::setw( 3 ) << c->data.delay << " A = " << c->data.flow;
  return os;
}

} // namespace 
