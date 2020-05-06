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
    build_db();
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
  std::unordered_map<std::string, std::string> opt_xags;

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

  void load_optimal_xags()
  {
    std::vector<std::string> result;

    result = split( npn4, "\n" );

    for ( auto record : result )
    {
      auto p = split( record, " " );
      assert( p.size() == 2u );
      opt_xags.insert( std::make_pair( p[0], p[1] ) );
    }
  }

  std::vector<xag_network::signal> create_xag_from_str( const std::string& str, const std::vector<xag_network::signal>& signals )
  {
    auto sig = signals;
    std::vector<xag_network::signal> result;

    std::stack<int> polar;
    std::stack<xag_network::signal> inputs;

    for ( auto i = 0ul; i < str.size(); i++ )
    {
      // operators polarity
      if ( str[i] == '[' || str[i] == '(' || str[i] == '{' )
      {
        polar.push( i > 0 && str[i - 1] == '!' ? 1 : 0 );
      }

      //input signals
      if ( str[i] >= 'a' && str[i] <= 'd' )
      {
        inputs.push( sig[str[i] - 'a' + 1] );

        polar.push( i > 0 && str[i - 1] == '!' ? 1 : 0 );
      }

      //create signals
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

        inputs.push( db.create_xor( x1 ^ p1, x2 ^ p2 ) ^ p3 );
        polar.push( 0 );
      }
      
      if ( str[i] == ')' )
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

        inputs.push( db.create_and( x1 ^ p1, x2 ^ p2 ) ^ p3 );
        polar.push( 0 );
      }
      
      if ( str[i] == '}' )
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

        inputs.push( db.create_or( x1 ^ p1, x2 ^ p2 ) ^ p3 );
        polar.push( 0 );
      }
    }

    assert( !polar.empty() );
    auto po = polar.top();
    polar.pop();
    db.create_po( inputs.top() ^ po );
    result.push_back( inputs.top() ^ po );
    return result;
  }

  void build_db()
  {
    std::vector<xag_network::signal> signals;
    signals.push_back( db.get_constant( false ) );

    for ( auto i = 0u; i < 4; ++i )
    {
      signals.push_back( db.create_pi() );
    }

    load_optimal_xags(); //size optimization

    for ( const auto e : opt_xags )
    {
      if( e.first == "0x0000" )
      {
        std::vector<xag_network::signal> tmp{ signals[0] };
        class2signal.insert( std::make_pair( e.first, tmp ) );
      }
      else if( e.first == "0x00ff" )
      {
        std::vector<xag_network::signal> tmp{ signals[4] ^ true };
        class2signal.insert( std::make_pair( e.first, tmp ) );
      }
      else
      {
        class2signal.insert( std::make_pair( e.first, create_xag_from_str( e.second, signals ) ) );
      }
    }
  }

  xag_network db;
  std::unordered_map<std::string, std::vector<xag_network::signal>> class2signal;

  std::string npn4 = "0x0000 0\n0x00ff 0\n0x3cc3 ![d[bc]]\n0x1ee1 ![{ab}[cd]]\n0x1be4 [c[d(!a[bc])]]\n0x19e6 [[ad](b!(ac))]\n0x19e1 ![[a(c!d)](b!(a!(!cd)))]\n0x18e7 ![d([ac][bc])]\n0x1798 [d{(ab)(c![d[ab]])}]\n0x1796 [a{[bc](d![ab])}]\n0x177e [(ab){[cd][d[ab]]}]\n0x16e9 ![d(!(ab)[c{ab}])]\n0x169b ![[b[ad]]([c(a[b[ad]])][d(a[b[ad]])])]\n0x1699 ![b[a(d!(c!(ab)))]]\n0x1696 [a[b(c!(b(ad)))]]\n0x017e [d{[a(b!(ad))][c(b!(ad))]}]\n0x168e (!(c[d[ac]])[b([ac]!(b!d))])\n0x0666 ([ab]!(cd))\n0x1687 ![c([a(d!(a!c))][b(d!(a!c))])]\n0x016f !{(c![ab])(d{ab})}\n0x166e [(cd)[a(b!(a![cd]))]]\n0x1683 ![{bc}(![a(bc)][d(bc)])]\n0x1681 ![([ad][bd]){c[[ad]{b([ad][bd])}]}]\n0x17e8 [d({ab}!(!c[ab]))]\n0x166a [[a(!d[bc])]{[bc](c!(!ad))}]\n0x168b ![[c(b![ac])](d![b{a(b![ac])}])]\n0x1669 ![[b[ac]](d!(c(ab)))]\n0x16ac {([bc][ad])([bd][a[bc]])}\n0x07f8 [d{c(ab)}]\n0x07b5 !{(cd)([ac]!(!b[cd]))}\n0x07b4 [c{(!ab)(d!(b!c))}]\n0x001f !{d(c{ab})}\n0x03c7 !{(bd)({ac}[bc])}\n0x0ff0 [cd]\n0x03d7 !({ad}[b(c!(bd))])\n0x07b1 !{(b![ad])[d(!c{ad})]}\n0x077e (!(cd){[a[cd]][b[cd]]})\n0x0778 [d[c(b(a!(cd)))]]\n0x06f0 ([cd]!(d![ab]))\n0x06f9 ![d(!c[ab])]\n0x07b0 ([cd]!(b[ac]))\n0x06f6 [d{c[b[ad]]}]\n0x06b6 ([c[a{b(ac)}]]!(d![a{b(ac)}]))\n0x06b4 (!(cd)[[bc](a{bd})])\n0x019a [a{(ad)(!b[cd])}]\n0x06b0 ([cd]{(ac)[a[bc]]})\n0x07e3 ![(b!(!ad))(c!(a![bd]))]\n0x0697 !(!(!c[b(a!c)]){d[a[b(a!c)]]})\n0x06b5 !({d[ac]}!([cd][b[ac]]))\n0x0696 (!(cd)[b[ac]])\n0x178e [c{[a[cd]][b[cd]]}]\n0x0691 !{[b[ad]](![cd]{c[ad]})}\n0x169a (!(c(bd))[(!bc)[a(bd)]])\n0x077a (!(cd)[{cd}(a!(!b{cd}))])\n0x0679 ![d([b[ac]]!(c!(b!d)))]\n0x0678 [[c(ab)](d{[ab][c(ab)]})]\n0x0672 (!(cd)[{ac}(b{ad})])\n0x066b !({b{cd}}!(!(cd)[a(b{cd})]))\n0x0662 ([ab]![c(!d{bc})])\n0x6996 [d[c[ab]]]\n0x0661 !{({ac}![cd])[a[b[cd]]]}\n0x07e0 ([cd]!([ac][bc]))\n0x0690 ([cd][b[ac]])\n0x0660 ([ab][cd])\n0x011e [{cd}{b{a(cd)}}]\n0x03dd ![{bc}(!d{b[ac]})]\n0x1668 [[ac](![bd]{[ab][ac]})]\n0x0007 !{d{c(ab)}}\n0x06b1 ![(!c[b[ac]]){d(b[ac])}]\n0x03dc [d{b(c!(a!d))}]\n0x0667 !(!(!c[b{ac}]){d[a[b{ac}]]})\n0x01eb !{(d{ab})(!a[bc])}\n0x03d5 !({ad}!(![bc][cd]))\n0x066f ![(!c[b[ac]]){d[b[ac]]}]\n0x03d8 ([d[c(a[bc])]]!([bc]![c(a[bc])]))\n0x06b3 !{(cd)[(!ad)(b!(ac))]}\n0x03d6 [{bc}{d(a!(bc))}]\n0x0180 ([bd]([ad][cd]))\n0x03fc [d{bc}]\n0x03d4 [{bc}{d(a[bc])}]\n0x016a (!(d{bc})[(bc)[ad]])\n0x07e9 ![{c(ab)}(!d{ab})]\n0x06b9 ![d([ab]!(c!(b!d)))]\n0x03c6 [(!c{ad})(b!(d!(!c{ad})))]\n0x0669 !{(cd)[b[a{cd}]]}\n0x03c1 !{[bc]({ad}![cd])}\n0x036f ![{bc}(!d[b(ac)])]\n0x036e {(!c[bd])(!d[a[bd]])}\n0x07f2 [d{c(a![bd])}]\n0x03db !({bc}!(!d[c(a!(bc))]))\n0x036d ![{(b!c)(a!d)}[c(b!d)]]\n0x0189 ![{b(!ac)}(a!(d{b(!ac)}))]\n0x0369 !{[c[b(a!d)]](d[b(a!d)])}\n0x035e [c{(a!d)[d(b!c)]}]\n0x035b !({bc}!(!d[ac]))\n0x033f !{(bc)(d[bc])}\n0x01fe [d{c{ab}}]\n0x01ee [d{b{a(cd)}}]\n0x003d !{d[b(!c{ab})]}\n0x06bd ![d([ab]![c(b!d)])]\n0x035f !{(ac)(d{bc})}\n0x1bd8 [a[c(![bc][ad])]]\n0x0116 (!(c{ab})(!(ab)[c[d{ab}]]))\n0x03d9 ![b{(a!d)(c[bd])}]\n0x06f2 ([(a!c){bc}]{(a!c)[cd]})\n0x0358 [c({ad}{c[bd]})]\n0x033d !{(bc)[d(!b(!c{ad}))]}\n0x006f !{d(c![ab])}\n0x019e (!(c![b[ac]])[d{a[b[ac]]}])\n0x01e8 ([d{ab}]!(!(ab)[c{ab}]))\n0x07f0 [d{c(b(ad))}]\n0x01af !{(!ac)(d{ab})}\n0x0197 ![{d[ab]}(!c[d{ab}])]\n0x01bf !{(c(!a[bd]))(d!(!a[bd]))}\n0x03c5 ![{c(a!d)}(b[cd])]\n0x01bd ![{bc}({a[bc]}!(d{bc}))]\n0x011f !({cd}{b{a(cd)}})\n0x06b7 ![d([b(ad)][c{ad}])]\n0x013f !{(bc)(d{a[bc]})}\n0x01bc ([d{a[bc]}]!(![bc][c{a[bc]}]))\n0x036a [{bc}{d[a[bc]]}]\n0x036c [[b(ac)](d!(c![b(ac)]))]\n0x167e (!(d![c[ab]]){[ab][b[c[ab]]]})\n0x06b2 (!(!a[bd])[d{c[a[bd]]}])\n0x01ad ![a{(!ac)({bc}[ad])}]\n0x035a [{ad}{c(bd)}]\n0x01ab ![{bc}(a[d{bc}])]\n0x018b !{(ad)[c(b![ac])]}\n0x01ac ([a(!c[b{ad}])]!(d!(!c[b{ad}])))\n0x01be (!(bd)[d{a[bc]}])\n0x16ad ![(bd)[[cd](a!(b![cd]))]]\n0x0186 (![c(ab)][d{ab}])\n0x01aa ([ad]!(d{bc}))\n0x1686 ([b[ac]]!(c![ad]))\n0x01a9 ![a({bc}!(ad))]\n0x07b6 [{c(a![bd])}{d(!a[bd])}]\n0x01a8 (![a{bc}][d{bc}])\n0x007e (!d{[ab][ac]})\n0x019f !{(d!(!a[cd]))[(!a[cd])(!b[cd])]}\n0x0676 (!(cd)[a{b(!ac)}])\n0x0663 !{(cd)[b(!a[cd])]}\n0x013d ![(!d[bc]){b{a[bc]}}]\n0x019b !{(cd)[b(a{c[bd]})]}\n0x0119 ![{a(cd)}(b!({cd}{a(cd)}))]\n0x0168 (![c[ab]][d{ab}])\n0x16a9 ![{bc}[a(d!(!a(bc)))]]\n0x0368 [(a!d)(![bc][d{c(a!d)}])]\n0x0198 (![ab][d{ac}])\n0x018f !{(c!(ab))(d{ab})}\n0x06f1 ![d(!c[a(b!(a!d))])]\n0x0183 !{[bc][b(a[cd])]}\n0x0182 (!(bd)(![bc][ad]))\n0x0181 !{[a(b!(ad))][c(b!(ad))]}\n0x07bc [[cd](b![d(a[cd])])]\n0x03c3 ![b(c!(bd))]\n0x0199 !{[ab](d{bc})}\n0x19e3 ![{b(ac)}(!(!ad)[cd])]\n0x003c (!d[bc])\n0x0016 (!{d(ab)}[a[bc]])\n0x0779 !({c[b[a[cd]]]}!([cd]!(a[b[a[cd]]])))\n0x0359 ![{ad}(!c[bd])]\n0x0006 ([ab]!{cd})\n0x016e (!(c![ab])[d{ab}])\n0x1698 ({b[ac]}![[bd]{a(d[ac])}])\n0x01e9 ![{c(ab)}({ab}!(d{c(ab)}))]\n0x16bc [c[b(a[d(bc)])]]\n0x166b ![d([a[bc]]!(a![bd]))]\n0x0118 (![a(b!(ac))][c[d(b!(ac))]])\n0x017f ![d([cd]([ad][bd]))]\n0x0001 !{{ab}{cd}}\n0x0187 !{[c(ab)](d{ab})}\n0x07e1 ![c{(a!d)(b![d(a!c)])}]\n0x067e [d{[c(a!d)][a[bd]]}]\n0x0117 ![(!d(!(ab)[c{ab}])){{ab}(!(ab)[c{ab}])}]\n0x013e [{d(bc)}{a{bc}}]\n0x033c [b[c(d!(bc))]]\n0x07e6 [{c(ab)}{d[c{ab}]}]\n0x036b !({bc}!(!d[a(bc)]))\n0x012f !{(b[cd])[c(a[cd])]}\n0x0356 [{bc}{ad}]\n0x011b ![{b{a(cd)}}(!{cd}{a(cd)})]\n0x01ef ![d(!b(!a[cd]))]\n0x037c [{bc}{d(a(bc))}]\n0x012d !{(d{ab})[b[c{ab}]]}\n0x169e [c[a(b!(a![cd]))]]\n0x01ae (!(!ac)[d{ab}])\n0x01ea ([d{ab}]!(!a[bc]))\n0x0019 !{d[a(b!(ac))]}\n0x067a {(!d[a[cd]])([cd]![b[a[cd]]])}\n0x007f !{d(c(ab))}\n0x0776 (!(cd)[a{b(!a{cd})}])\n0x006b !{d[c[b(a{bc})]]}\n0x17ac [[d{bc}](!a[c(bd)])]\n0x011a ([d[ac]][c{a[bd]}])\n0x013c ([c[bd]]!(d!(!a[bd])))\n0x07f1 ![d(!c{[ad][bd]})]\n0x067b ![{d(a![bc])}(!c[a[bc]])]\n0x037d ![d({ad}!{[bc](cd)})]\n0x0003 !{d{bc}}\n0x0069 !{d[c[ab]]}\n0x003f !{d(bc)}\n0x012c ([b[cd]]![d(!a[cd])])\n0x0196 (!(d!(!b[ad]))[b[c[ad]]])\n0x001e (!d[c{ab}])\n0x0169 !{(d{ab})[b[ac]]}\n0x07e2 (!(cd)[{ad}(b[ac])])\n0x0693 ![(a[cd])[b(d!(bc))]]\n0x016b ![{bc}([a(bc)][d{bc}])]\n0x03c0 ([bd][cd])\n0x037e [d{[b(a!d)][c(a!d)]}]\n0x000f !{cd}\n0x001b !{d[b(a[bc])]}\n0x0357 !({bc}{ad})\n0x0018 (!d([ac][bc]))\n0x1689 ![[ad]{[b(!ac)](c![ad])}]\n0x0673 !({bd}!([ab][cd]))\n0x03cf ![c(b[cd])]\n0x069f ![c(![ab][cd])]\n0x1697 ![c(![ab]{[ad][cd]})]\n0x03de [d{b[c(a!d)]}]\n0x179a [a[{c[b(!ad)]}(b!(d[b(!ad)]))]]\n0x0017 !{(ab){d(c[ab])}}";

};

} /* namespace mockturtle */
