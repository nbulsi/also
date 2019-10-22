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
  \file  img_verilog_reader.hpp
  \brief Lorina reader for VERILOG files

  \author Zhufei Chu
*/

#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <lorina/verilog.hpp>
#include <mockturtle/mockturtle.hpp>

namespace mockturtle
{

/*! \brief Lorina reader callback for VERILOG files.
 *
 * **Required network functions:**
 * - `create_pi`
 * - `create_po`
 * - `get_constant`
 * - `create_not`
 * - `create_imp`
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      img_network img;
      lorina::read_verilog( "file.v", img_verilog_reader( img ) );
   \endverbatim
 */
template<typename Ntk>
class img_verilog_reader : public lorina::verilog_reader
{
public:
  explicit img_verilog_reader( Ntk& ntk ) : _ntk( ntk )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_create_pi_v<Ntk>, "Ntk does not implement the create_pi function" );
    static_assert( has_create_po_v<Ntk>, "Ntk does not implement the create_po function" );
    static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant function" );
    static_assert( has_create_not_v<Ntk>, "Ntk does not implement the create_not function" );

    signals["0"] = _ntk.get_constant( false );
    signals["1"] = _ntk.get_constant( true );
    signals["1'b0"] = _ntk.get_constant( false );
    signals["1'b1"] = _ntk.get_constant( true );
  }

  ~img_verilog_reader()
  {
    for ( auto const& o : outputs )
    {
      _ntk.create_po( signals[o], o );
    }
  }

  void on_inputs( const std::vector<std::string>& names ) const override
  {
    for ( const auto& name : names )
    {
      signals[name] = _ntk.create_pi( name );
    }
  }

  void on_outputs( const std::vector<std::string>& names ) const override
  {
    for ( const auto& name : names )
    {
      outputs.emplace_back( name );
    }
  }

  void on_assign( const std::string& lhs, const std::pair<std::string, bool>& rhs ) const override
  {
    if ( signals.find( rhs.first ) == signals.end()  )
      std::cerr << fmt::format( "[w] undefined signal {} assigned 0", rhs.first ) << std::endl;

    auto r = signals[rhs.first];
    signals[lhs] = rhs.second ? _ntk.create_not( r ) : r;
  }

  void on_or( const std::string& lhs, const std::pair<std::string, bool>& op1, const std::pair<std::string, bool>& op2 ) const override
  {
    if ( signals.find( op1.first ) == signals.end()  )
      std::cerr << fmt::format( "[w] undefined signal {} assigned 0", op1.first ) << std::endl;
    if ( signals.find( op2.first ) == signals.end()  )
      std::cerr << fmt::format( "[w] undefined signal {} assigned 0", op2.first ) << std::endl;

    auto a = signals[op1.first];
    auto b = signals[op2.first];

    if( !op1.second && op2.second )
    {
      signals[lhs] = _ntk.create_imp( b , a );
    }
    else if( op1.second && !op2.second )
    {
      signals[lhs] = _ntk.create_imp( a , b );
    }
    else
    {
      assert( false && "NOT a implication operation" );
    }

  }

private:
  Ntk& _ntk;

  mutable std::map<std::string, signal<Ntk>> signals;
  mutable std::vector<std::string> outputs;
};

} /* namespace mockturtle */
