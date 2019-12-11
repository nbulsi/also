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
  \file  xmg_cut.hpp
  \brief Cut enumeration using the exact XMG representation as the cost function 

  \author Zhufei Chu
*/

#pragma once

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>

#include <mockturtle/mockturtle.hpp>
#include "../../core/misc.hpp"

namespace mockturtle
{

class xmg_p4_synthesis
{
  public:
  xmg_p4_synthesis()
  {
  }

  xmg_network create_xmg_from_str( const std::string&  xmg_expr )
  {
    xmg_network xmg;
    std::string str = xmg_expr;

    std::vector<xmg_network::signal> signals;
    signals.push_back( xmg.get_constant( false ) );

    //create three primary inputs
    for ( auto i = 0u; i < 4u; ++i )
    {
      signals.push_back( xmg.create_pi() );
    }
    
    std::stack<int> polar;
    std::stack<xmg_network::signal> inputs;

    for ( auto i = 0ul; i < str.size(); i++ )
    {
      // operators polarity
      if ( str[i] == '[' || str[i] == '<' )
      {
        polar.push( i > 0 && str[i - 1] == '!' ? 1 : 0 );
      }

      //input signals
      if ( str[i] >= 'a' && str[i] <= 'd' )
      {
        inputs.push( signals[str[i] - 'a' + 1] );

        polar.push( i > 0 && str[i - 1] == '!' ? 1 : 0 );
      }
      else if ( str[i] == '0' )
      {
        inputs.push( signals[0] );

        polar.push( i > 0 && str[i - 1] == '!' ? 1 : 0 );
      }

      //create signals
      if ( str[i] == '>' )
      {
        assert( inputs.size() >= 3u );
        auto x1 = inputs.top();
        inputs.pop();
        auto x2 = inputs.top();
        inputs.pop();
        auto x3 = inputs.top();
        inputs.pop();

        assert( polar.size() >= 4u );
        auto p1 = polar.top();
        polar.pop();
        auto p2 = polar.top();
        polar.pop();
        auto p3 = polar.top();
        polar.pop();

        auto p4 = polar.top();
        polar.pop();

        inputs.push( xmg.create_maj( x1 ^ p1, x2 ^ p2, x3 ^ p3 ) ^ p4 );
        polar.push( 0 );
      }

      if ( str[i] == ']' )
      {
        assert( inputs.size() >= 2u );
        auto x1 = inputs.top();
        inputs.pop();
        auto x2 = inputs.top();
        inputs.pop();

        assert( polar.size() >= 3u );
        auto p1 = polar.top();
        polar.pop();
        auto p2 = polar.top();
        polar.pop();

        auto p3 = polar.top();
        polar.pop();

        inputs.push( xmg.create_xor( x1 ^ p1, x2 ^ p2 ) ^ p3 );
        polar.push( 0 );
      }
    }

    assert( !polar.empty() );
    auto po = polar.top();
    polar.pop();
    xmg.create_po( inputs.top() ^ po );

    return xmg;
  }

  std::pair<unsigned, unsigned> xmg_from_expr( const std::string& query_tt )
  {
      load_xmgs();
      auto xmg = create_xmg_from_str( opt_xmgs[query_tt] );
      depth_view depth_xmg( xmg );

      auto size  = xmg.num_gates();
      auto depth = depth_xmg.depth();
      return std::make_pair( size, depth );
  }

  private:
    std::unordered_map<std::string, std::string> opt_xmgs;
    
    std::string npn4_sd = "0x3cc3 [b[!cd]]\n0x1bd8 [!<b!cd><a!b!c>]\n0x19e3 [<a!b!c><c[bd]<0a!c>>]\n0x19e1 [<!b!c[bd]><a[bd]!<cd<!b!c[bd]>>>]\n0x17e8 [d<abc>]\n0x179a [<ad!<0ad>><c<0ad>[!bd]>]\n0x178e <!c[ad]![!bd]>\n0x16e9 [d<<a!b!c><abc>!<0a!b>>]\n0x16bc [<0!b!c><!d!<abc>[ad]>]\n0x16ad [!<!b!<!0a!b>[ac]><0d<!0a!b>>]\n0x16ac [!<acd><!b!d<!a[cd]<acd>>>]\n0x16a9 <0<!0a<!b!cd>>[!<0!b!c>[!ad]]>\n0x169e <!d[!a[!bc]]!<c!d[!bc]>>\n0x169b <<a!b!<0!cd>>!<ab!<0!cd>>!<!bcd>>\n0x169a [a<![!bc]<0ac><!0!bd>>]\n0x1699 <b<!a!b<0!cd>>!<b<0!cd><!abd>>>\n0x1687 [[ac]<b!<a!bd>!<0bc>>]\n0x167e <<!b!c<!abc>>[a<!abc>]!<!ad!<!abc>>>\n0x166e [<ab!<0ab>><cd<0ab>>]\n0x03cf <!c!d[bd]>\n0x166b <!c[a<bcd>]<!bc!d>>\n0x01be <c!<bcd><!<!0!bd>[ad]!<bcd>>>\n0x07f2 <[cd]<0a!b>!<0ad>>\n0x07e9 [<ac<!0!ab>><!bd<!0!ab>>]\n0x6996 [[bc][ad]]\n0x01af <[!ac]<0a!d>!<bcd>>\n0x033c <<b!cd><0c!d>!<bcd>>\n0x07e3 [c<d!<abc>!<0b!c>>]\n0x1681 <0[!<b!cd>[ab]]!<!d[ab]<0!ac>>>\n0x01ae <!<bcd>![!ad]<0b!d>>\n0x07e2 [<!a!bd>!<!cd<0b!<!a!bd>>>]\n0x01ad <!<bcd><0b!d>[!ac]>\n0x07e1 [c<d<!0!bc>!<ab!d>>]\n0x001f <!a<a!b!d><0!c!d>>\n0x01ac [d<a<!a!cd>!<0!b!c>>]\n0x07e0 [d<ac<!abd>>]\n0x07bc [<bc<!0a!b>>!<0!d<!0a!b>>]\n0x03dc <b[d<!0bc>]!<ab<!0bc>>>\n0x06f6 <!d[ab][cd]>\n0x06bd <<b!c<0!bd>><a!b!d>!<a!c<0!bd>>>\n0x077e <<!0a![!cd]><0!ab>!<bcd>>\n0x06b9 <!<!0ac><a!b!d><b<a!b!d>![!cd]>>\n0x06b7 <!d[c[ab]]<!b!c[ab]>>\n0x06b6 [!c!<<!abc>!<0c!d>[ab]>]\n0x077a [!<0a!<!bcd>>![cd]]\n0x06b5 <!<a!b!c><a!b!d>![!c<!0!ad>]>\n0x0ff0 [cd]\n0x0779 <<ac!d><b!c!<ac!d>>!<ab![cd]>>\n0x06b4 [!c<!d<a!b!c>![ab]>]\n0x0778 [[cd]<0<b!c!d><0ab>>]\n0x007f <!d<!ab!c><0!b!d>>\n0x06b3 [![!bd]!<d<0bc><a!c!d>>]\n0x007e <0!d![a<a!b!c>]>\n0x06b2 [!<0!c!<a!bd>><bd!<ab!c>>]\n0x0776 <[ab][cd]!<abc>>\n0x06b1 [d<c<!0!bd>[!ab]>]\n0x06b0 [<!a!c<ab!d>><!b<ab!d>!<0cd>>]\n0x037c [[!bd]<d<!0b!c>!<acd>>]\n0x01ef <!c!d![!c<0!a!b>]>\n0x0696 <<ab!c><0c!d>!<abc>>\n0x035e [<!0ad>!<0!c!<bd!<!0ad>>>]\n0x0678 <<!ab!d><0a!b>[c<abd>]>\n0x0676 <!<abc>[ab]<0c!d>>\n0x0669 [<0!c!d><[cd]<0!c!d>[ab]>]\n0x01bc [<bd<!acd>>!<0!b!c>]\n0x0663 <0[b<a!c!d>]!<0cd>>\n0x07f0 <[cd]!<!0!cd>!<0ab>>\n0x01bf <!d<!0a!c><0!a!b>>\n0x0666 <0[ab]!<0cd>>\n0x0661 <<a!c!d>!<ab<a!c!d>>[<a!c!d><b!c!d>]>\n0x1689 [[d<abd>]<0!c<!a!bd>>]\n0x0660 [<b!c!d>!<!acd>]\n0x0182 [<!ab!c><b!d<!0!c<!ab!c>>>]\n0x07b6 [<!0!a<!bcd>>[!c<!0bd>]]\n0x03d7 <!d[!bc]!<abc>>\n0x06f1 [<!ac<!0!bc>><d<!0!bc><0!ad>>]\n0x03dd <[bd]<0!a!d>!<0cd>>\n0x0180 [<acd>!<b!d!<acd>>]\n0x07b4 [<bd<!0!ad>><c<!0!ad><abc>>]\n0x03db <[!bc]<a!b!d>!<acd>>\n0x07b0 <<0ac>[cd]!<abc>>\n0x03d4 [!d<!b!c<0a!d>>]\n0x03c7 <!<0bd><0!a!c>[!bc]>\n0x03c6 <0!<0cd>[!b<!ac!d>]>\n0x03c5 <[bd]<0!a!c>![bc]>\n0x03c3 <!b<0b!d>[!bc]>\n0x03c1 <!<b!cd>[b<b!cd>]<!a!c<b!cd>>>\n0x03c0 [d<bcd>]\n0x1798 [<ab<!a!bc>><d<!a!bc><!0cd>>]\n0x036f <!c!d[!b<0!ac>]>\n0x03fc [d!<0!b!c>]\n0x1698 [b<<!abd>[ac][!ad]>]\n0x177e [<0!b!d>!<ac![bd]>]\n0x066f <!c!d[ab]>\n0x035b <<ac!d><0!b!c>!<0ac>>\n0x1697 <!<abc><ab!c><!bc!d>>\n0x066b [<bc<!a!bd>><a<!a!bd>!<0!cd>>]\n0x07f8 [!d!<ac!<0a!b>>]\n0x035a [!<!0c<0bd>><0!a!d>]\n0x1696 <!a<!d<0!bd><a!bc>><b!c<a!bc>>>\n0x003f <0!d!<bcd>>\n0x0673 <!b![d<ab!c>]<!a!d<ab!c>>>\n0x0359 [<!0!bc><ac[!cd]>]\n0x0672 <[cd]<0!b!d>[ab]>\n0x0358 <<!acd><0a!c>[!d<0!b!c>]>\n0x003d <!<bcd>[bc]<0!a!d>>\n0x0357 <!0!<!0bc><0!a!d>>\n0x003c <0!d![!bc]>\n0x0356 [!<0!a!d>!<0!b!c>]\n0x07e6 [<!cd!<0!a!b>><abc>]\n0x033f <!b!c!d>\n0x033d <!<bcd><bc!d><0!b<!0!ad>>>\n0x0690 [!d<!c!d[ab]>]\n0x01e9 <!d<0!b![ac]>![b<!0ac>]>\n0x1686 [[bc]<a[bc]!<!abd>>]\n0x07f1 <!b<!ab!d>[cd]>\n0x01bd <!d[c<!ab!c>]<0!b<!ab!c>>>\n0x1683 [[bc]!<!ad<0bc>>]\n0x001e <0!d![!c<ab!d>]>\n0x01ab <!d![!ad]<0!b!c>>\n0x01aa <[ad]<0!b!c><0a!d>>\n0x01a9 [a<d<0!b!c><!0a!d>>]\n0x001b <0<a!b!d>!<acd>>\n0x01a8 [a<ad!<!0bc>>]\n0x000f <0!c!d>\n0x0199 <[!ab]<a!b!d><0!a!c>>\n0x0198 [!<!abd><!b!c<0ac>>]\n0x0197 <!d[!a<0bc>]<!b!c<0bc>>>\n0x06f9 [d<!0c![ab]>]\n0x0196 <<0!d!<a!b!c>>[ad]!<bc[ad]>>\n0x03d8 [<!cd!<a!b!c>><!0b!<a!b!c>>]\n0x06f2 <a<0!a[cd]><!d[cd]![!ab]>>\n0x03de <!d![c<!ab!d>][bd]>\n0x018f <!c!d[b<!ab!c>]>\n0x0691 <0<!d<!a!bd><ab!c>><!0c<!a!bd>>>\n0x01ea [d<!0a<bcd>>]\n0x07b1 <<a!b!d><0!a!c>[cd]>\n0x0189 <[!ab]!<!0ac><0a!d>>\n0x036b <!a!<!ad![!bc]>!<!abc>>\n0x03d5 [<bcd>!<!d<bcd><0a!d>>]\n0x0186 [<!cd<0ab>>!<!a!bc>]\n0x0119 <0[!ab]!<acd>>\n0x0368 [<bcd><c<b!cd><!0ad>>]\n0x0183 <[!bc]<0a!d><0!a!b>>\n0x07b5 <!<bcd>[cd]![ac]>\n0x0181 <<!ab!c>[b<!ab!c>]<0!d!<!ab!c>>>\n0x036e <!c<bc!d>[!b<0!a!d>]>\n0x011f <!c!d!<!0ab>>\n0x016f <<!ab!c><0a!d>!<abd>>\n0x016e <!d<0!c[bd]>[a[bd]]>\n0x013f <0!<0ad>!<bcd>>\n0x019f <!d<b!c[ad]><0!a!b>>\n0x013e [[!cd]<!bc!<a!d![!cd]>>]\n0x019e <!d!<!bc[!ad]><0!b<!acd>>>\n0x013d <<bc!d>!<!0ac>!<bcd>>\n0x016b <!d<0!b!c>![!b[!ac]]>\n0x01fe [d<!0c<ab!c>>]\n0x166a <a[a<bcd>]!<ab<!bcd>>>\n0x0019 <!<!abd><0!a!b><0b!c>>\n0x006b <!d<0b<!a!bc>>!<!abc>>\n0x069f <!d<abd>!<abc>>\n0x013c <<0!ad><b!d!<!0b!c>>!<bc<0!ad>>>\n0x037e [<!0ad>!<!b!c<0a!d>>]\n0x012f <!d<a!b!c>!<!0ac>>\n0x0693 [<acd><d<!0c!d>![bd]>]\n0x012d <<0ac><!ab!c>!<bcd>>\n0x01ee [!<!0ab>!<d<0!cd><!0ab>>]\n0x012c [<0!a!b><!d[bc]<0!a!b>>]\n0x017f <!b!d!<acd>>\n0x1796 <!b!<!a!bc><!a<!0cd>!<!0!bd>>>\n0x036d <<abc>!<bcd><!a!c[bd]>>\n0x011e <d!<cd!<0!a!b>>!<d<0!a!b>!<0c!d>>>\n0x0679 <!d<!a<abd><0!b!c>>[c<abd>]>\n0x035f <!c!<0bd>!<!0ad>>\n0x067b <!d<!b!c[ad]>[![ad]<0b!c>]>\n0x0118 <<0!a!b><ac!d><!cd<0b!d>>>\n0x067a [<!0ac><cd<ab!<!0ac>>>]\n0x0117 <!b<!a!d<a!cd>><0!c!<a!cd>>>\n0x1ee1 [[cd]<0!a!b>]\n0x016a <b!<b<!0!ad>!<b!cd>>!<ab<b!cd>>>\n0x0169 <!a<a!b!c><0!d<abc>>>\n0x037d <!a<a!b!c>[!d<0!b!c>]>\n0x0697 <<!a!c<0a!b>>!<!ab!c><!ab!d>>\n0x006f <!d<0!c!d>[ab]>\n0x17ac [<abc><!0d<0!ab>>]\n0x0069 <0!d[a[!bc]]>\n0x0667 <![!ab]<0!b[!ab]>!<cd[!ab]>>\n0x0168 [<bd<!0!bc>><ad<!0bc>>]\n0x067e <!d![c<0!a!d>][ab]>\n0x011b <a!<acd>!<!0ab>>\n0x036a [!<bcd><0!a!d>]\n0x018b <!<0ad><0!b!c><0ab>>\n0x0001 <0<0!a!b><0!c!d>>\n0x1669 <<!cd<!a!bc>><b!d[!ac]>!<bd[!ac]>>\n0x0018 <0!d[a<a!bc>]>\n0x168b [[!a<!ab!d>]<!b!c<0d<!ab!d>>>]\n0x0662 <!<!0bd>[ab][d<abc>]>\n0x00ff !d\n0x168e [!<a!c!<0!b!c>><!0<0!b!c><!ab!d>>]\n0x0007 <0!<!0cd>!<0ab>>\n0x19e6 [[ad]<0b!<0ac>>]\n0x0116 [<!b!d[!ac]><0[!ac]!<!abd>>]\n0x036c [[bd]!<!0!c<!ab![bd]>>]\n0x01e8 <!d<a[bd][cd]><0!ad>>\n0x06f0 <[cd]<a!bc>!<a!bd>>\n0x03d6 [<d!<0!a!d>!<0bc>><bc!<0bc>>]\n0x1be4 [d!<!b!c[ac]>]\n0x0187 <a!<!0ac>!<ad![!bc]>>\n0x0003 <0!d<0!b!c>>\n0x017e [[!bd]<<!ab!c>[!bd]<!0bd>>]\n0x01eb <!d[b<0!a!c>]!<!ab!<0!a!c>>>\n0x0006 <0[ab]<0!c!d>>\n0x011a <0[d[ac]]!<bcd>>\n0x0369 <!b<0!d<abc>><b!c!<0a!d>>>\n0x03d9 [d<<!0!ab><bcd><a!b!d>>]\n0x0000 0\n0x019b <!d<0!b!c>![ab]>\n0x1668 [<bcd><ab<!bcd>>]\n0x18e7 [[!cd]<abc>]\n0x0017 <0!d!<abc>>\n0x019a <!d![a<b!c!d>]!<!0c<b!c!d>>>\n0x0016 <0<a!d<bc!d>>!<abc>>";
    

    void load_xmgs()
    {
      std::vector<std::string> result;

      result = also::split( npn4_sd, "\n" );
      
      for ( auto record : result )
      {
        auto p = also::split( record, " " );
        assert( p.size() == 2u );
        opt_xmgs.insert( std::make_pair( p[0], p[1] ) );
      }
    }
};

std::pair<unsigned, unsigned> Evalxmg( const std::string& query_tt )
{
  xmg_p4_synthesis m;

  return m.xmg_from_expr( query_tt );
}


struct cut_enumeration_xmg_cut
{
  uint32_t delay{0};
  float flow{0};
  float cost{0};
};

template<bool ComputeTruth>
bool operator<( cut_type<ComputeTruth, cut_enumeration_xmg_cut> const& c1, cut_type<ComputeTruth, cut_enumeration_xmg_cut> const& c2 )
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
struct cut_enumeration_update_cut<cut_enumeration_xmg_cut>
{
  template<typename Cut, typename NetworkCuts, typename Ntk>
  static void apply( Cut& cut, NetworkCuts const& cuts, Ntk const& ntk, node<Ntk> const& n )
  {
    /******************************************************************
     * Zhufei Chu
     ******************************************************************/
    int xmg_size, xmg_depth;

    //std::cout << "AIG node: " << n << " ";
    auto tt = cuts.truth_table( cut );
    
    assert( tt.num_vars() <= 4 );
    const auto fe = kitty::extend_to( tt, 4 );
    const auto config = kitty::exact_npn_canonization( fe );

    auto tt_str = kitty::to_hex( std::get<0>( config ) );
    auto func_str = "0x" + tt_str;

    if( func_str == "0x0000" )
    {
      xmg_size = 0;
      xmg_depth = 0;
    }
    else
    {
      auto p = Evalxmg( func_str );
      xmg_size = p.first;
      xmg_depth = p.second;
    }
     
    /******************************************************************/

    uint32_t delay{0};
    float flow = cut->data.cost = cut.size() < 2 ? 0.0f : 1.0f;
    cut->data.cost = xmg_depth;

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
std::ostream& operator<<( std::ostream& os, cut<MaxLeaves, cut_data<false, cut_enumeration_xmg_cut>> const& c )
{
  os << "{ ";
  std::copy( c.begin(), c.end(), std::ostream_iterator<uint32_t>( os, " " ) );
  os << "}, D = " << std::setw( 3 ) << c->data.delay << " A = " << c->data.flow;
  return os;
}

} // namespace 
