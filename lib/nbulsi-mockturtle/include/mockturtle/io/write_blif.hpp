/* mockturtle: C++ logic network library
 * Copyright (C) 2018-2022  EPFL
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
  \file write_blif.hpp
  \brief Write networks to BLIF format

  \author Heinz Riener
  \author Mathias Soeken
  \author Max Austin
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "../networks/sequential.hpp"
#include "../traits.hpp"
#include "../views/topo_view.hpp"

#include <kitty/constructors.hpp>
#include <kitty/isop.hpp>
#include <kitty/operations.hpp>
#include <kitty/print.hpp>

#include <fmt/format.h>

#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace mockturtle
{

struct write_blif_params
{
  uint32_t rename_ri_using_node = 0u;
};

template<class Ntk>
void write_blif_optimized( Ntk const& ntk, std::ostream& os, write_blif_params const& ps = {} )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_fanin_size_v<Ntk>, "Ntk does not implement the fanin_size method" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_foreach_pi_v<Ntk>, "Ntk does not implement the foreach_pi method" );
  static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
  static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_num_pis_v<Ntk>, "Ntk does not implement the num_pis method" );
  static_assert( has_num_pos_v<Ntk>, "Ntk does not implement the num_pos method" );
  static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
  static_assert( has_node_function_v<Ntk>, "Ntk does not implement the node_function method" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );

  uint32_t num_latches{ 0 };
  if constexpr ( has_num_registers_v<Ntk> )
  {
    num_latches = ntk.num_registers();
  }

  topo_view topo_ntk{ ntk };

  std::unordered_set<std::string> defined_names;

  std::unordered_map<typename Ntk::node, std::string> node_name_map;

  /* PI names */
  topo_ntk.foreach_pi( [&]( auto const& n ) {
    std::string name;
    if constexpr ( has_has_name_v<Ntk> && has_get_name_v<Ntk> )
    {
      // Note: Use ntk.node_to_index to ensure getting the original index for querying the name in the original ntk
      signal<Ntk> const s = ntk.make_signal( ntk.node_to_index( n ) );
      name = ntk.has_name( s ) ? ntk.get_name( s ) : fmt::format( "pi{}", topo_ntk.node_to_index( n ) );
    }
    else
    {
      name = fmt::format( "pi{}", topo_ntk.node_to_index( n ) );
    }
    defined_names.insert( name );
    node_name_map[n] = name;
  } );

  /* 2. Preprocess PO names and attempt "Name Absorption" (direct connection optimization) */
  struct PoInfo
  {
    std::string name;
    typename Ntk::node driver_node;
    bool is_complemented;
  };
  std::vector<PoInfo> pos_info;

  topo_ntk.foreach_co( [&]( auto const& f, auto index ) {
    if ( index >= topo_ntk.num_cos() - num_latches )
      return; // Skip Latches here

    std::string output_name = fmt::format( "po{}", index );
    if constexpr ( has_has_output_name_v<Ntk> && has_get_output_name_v<Ntk> )
    {
      if ( ntk.has_output_name( index ) )
        output_name = ntk.get_output_name( index );
    }
    defined_names.insert( output_name );

    auto driver_node = topo_ntk.get_node( f );
    bool is_compl = topo_ntk.is_complemented( f );
    pos_info.push_back( { output_name, driver_node, is_compl } );

    // Conditions for direct naming: not PI, not constant, same polarity, and not yet named
    if ( !topo_ntk.is_pi( driver_node ) && !topo_ntk.is_constant( driver_node ) )
    {
      if ( !is_compl )
      {
        if ( node_name_map.find( driver_node ) == node_name_map.end() )
        {
          node_name_map[driver_node] = output_name;
        }
      }
    }
  } );

  /* 3. Determine names for constants */
  auto get_safe_name = [&]( std::string base ) {
    while ( defined_names.count( base ) )
      base += "_x";
    defined_names.insert( base );
    return base;
  };

  auto const_false_node = topo_ntk.get_node( topo_ntk.get_constant( false ) );
  std::string const_false_name = get_safe_name( "gnd" );
  node_name_map[const_false_node] = const_false_name;

  std::string const_true_name = "";
  if constexpr ( has_get_constant_v<Ntk> )
  {
    auto const_true_node = topo_ntk.get_node( topo_ntk.get_constant( true ) );
    if ( const_true_node != const_false_node )
    {
      const_true_name = get_safe_name( "vdd" );
      node_name_map[const_true_node] = const_true_name;
    }
  }

  /* write model */
  os << ".model top\n";

  /* write inputs */
  if ( topo_ntk.num_pis() > 0u )
  {
    os << ".inputs ";
    topo_ntk.foreach_pi( [&]( auto const& n ) {
      os << node_name_map.at( n ) << " ";
    } );
    os << "\n";
  }

  /* write outputs */
  if ( topo_ntk.num_pos() > 0u )
  {
    os << ".outputs ";
    for ( const auto& info : pos_info )
    {
      os << info.name << " ";
    }
    os << "\n";
  }

  /* write latches */
  if constexpr ( has_num_registers_v<Ntk> )
  {
    if ( num_latches > 0u )
    {
      uint32_t latch_idx = 0;
      topo_ntk.foreach_co( [&]( auto const& f, auto index ) {
        if ( index >= topo_ntk.num_cos() - num_latches )
        {
          os << ".latch ";

          auto const ro_signal = topo_ntk.make_signal( topo_ntk.ro_at( latch_idx ) );
          auto const ri_node = topo_ntk.get_node( f );

          std::string ri_name;
          bool ri_name_found = false;
          if constexpr ( has_has_output_name_v<Ntk> && has_get_output_name_v<Ntk> )
          {
            if ( ntk.has_output_name( index ) )
            {
              ri_name = ntk.get_output_name( index );
              ri_name_found = true;
            }
          }
          if ( !ri_name_found && ps.rename_ri_using_node )
          {
            if ( node_name_map.count( ri_node ) )
            {
              ri_name = node_name_map.at( ri_node );
              ri_name_found = true;
            }
            else
            {
              ri_name = fmt::format( "n{}", topo_ntk.node_to_index( ri_node ) );
              ri_name_found = true;
            }
          }

          if ( !ri_name_found )
          {
            ri_name = fmt::format( "li{}", latch_idx );
          }

          std::string ro_name;
          if constexpr ( has_has_name_v<Ntk> && has_get_name_v<Ntk> )
          {
            signal<Ntk> const s_orig = ntk.make_signal( ntk.node_to_index( topo_ntk.get_node( ro_signal ) ) );
            ro_name = ntk.has_name( s_orig ) ? ntk.get_name( s_orig )
                                             : fmt::format( "n{}", topo_ntk.node_to_index( topo_ntk.get_node( ro_signal ) ) );
          }
          else
          {
            ro_name = fmt::format( "n{}", topo_ntk.node_to_index( topo_ntk.get_node( ro_signal ) ) );
          }

          if ( defined_names.count( ro_name ) == 0 )
          {
            defined_names.insert( ro_name );
            node_name_map[topo_ntk.get_node( ro_signal )] = ro_name;
          }
          else
          {
            if ( node_name_map.count( topo_ntk.get_node( ro_signal ) ) )
              ro_name = node_name_map.at( topo_ntk.get_node( ro_signal ) );
          }

          register_t latch_info = topo_ntk.register_at( latch_idx );
          os << fmt::format( "{} {} {} {} {}\n", ri_name, ro_name, latch_info.type, latch_info.control, latch_info.init );

          latch_idx++;
        }
      } );
    }
  }

  /* write constants */
  os << ".names " << const_false_name << "\n0\n";
  if ( const_true_name != "" )
    os << ".names " << const_true_name << "\n1\n";

  /* write internal nodes */
  topo_ntk.foreach_node( [&]( auto const& n ) {
    if ( topo_ntk.is_constant( n ) || topo_ntk.is_ci( n ) )
      return;

    /* Determine the output name for the current node */
    std::string output_name;
    if ( node_name_map.count( n ) )
    {
      output_name = node_name_map.at( n );
    }
    else
    {
      // No name yet, generate a default name and ensure no conflicts
      std::string base = "";
      if constexpr ( has_has_name_v<Ntk> && has_get_name_v<Ntk> )
      {
        signal<Ntk> const s_orig = ntk.make_signal( ntk.node_to_index( n ) );
        if ( ntk.has_name( s_orig ) )
          base = ntk.get_name( s_orig );
      }

      if ( base.empty() )
        base = fmt::format( "n{}", topo_ntk.node_to_index( n ) );

      while ( defined_names.count( base ) )
        base += "_uniq";
      output_name = base;
      defined_names.insert( output_name );
      node_name_map[n] = output_name;
    }

    auto func = topo_ntk.node_function( n );
    if ( isop( func ).size() == 0 )
      return;

    os << ".names ";

    topo_ntk.foreach_fanin( n, [&]( auto const& f, auto index ) {
      auto f_node = topo_ntk.get_node( f );

      std::string fanin_name;
      if ( node_name_map.count( f_node ) )
      {
        fanin_name = node_name_map.at( f_node );
      }
      else
      {
        fanin_name = fmt::format( "n{}", topo_ntk.node_to_index( f_node ) );
      }

      if ( topo_ntk.is_complemented( f ) )
      {
        kitty::flip_inplace( func, index );
      }
      os << fanin_name << ' ';
    } );

    os << output_name << "\n";

    for ( auto cube : isop( func ) )
    {
      cube.print( topo_ntk.fanin_size( n ), os );
      os << " 1\n";
    }
  } );

  /* write PO connections (handle cases not directly absorbed) */
  for ( const auto& info : pos_info )
  {
    std::string driver_name = node_name_map.at( info.driver_node );

    if ( driver_name == info.name )
    {
      // Already directly absorbed, no additional action needed
    }
    else
    {
      os << fmt::format( ".names {} {}\n", driver_name, info.name );
      if ( info.is_complemented )
      {
        os << "0 1\n"; // Inverter
      }
      else
      {
        os << "1 1\n"; // Buffer
      }
    }
  }

  if constexpr ( has_num_registers_v<Ntk> )
  {
    if ( num_latches > 0u )
    {
      uint32_t latch_idx = 0;
      topo_ntk.foreach_co( [&]( auto const& f, auto index ) {
        if ( index >= topo_ntk.num_cos() - num_latches )
        {
          std::string ri_name;
          auto ri_node = topo_ntk.get_node( f );
          bool ri_name_found = false;

          if constexpr ( has_has_output_name_v<Ntk> && has_get_output_name_v<Ntk> )
          {
            if ( ntk.has_output_name( index ) )
            {
              ri_name = ntk.get_output_name( index );
              ri_name_found = true;
            }
          }

          if ( !ri_name_found && ps.rename_ri_using_node )
          {
            if ( node_name_map.count( ri_node ) )
              ri_name = node_name_map.at( ri_node );
            else
              ri_name = fmt::format( "n{}", topo_ntk.node_to_index( ri_node ) );
            ri_name_found = true;
          }

          if ( !ri_name_found )
          {
            ri_name = fmt::format( "li{}", latch_idx );
          }

          std::string driver_name = node_name_map.at( ri_node );

          if ( ri_name != driver_name )
          {
            bool is_compl = topo_ntk.is_complemented( f );
            os << fmt::format( ".names {} {}\n", driver_name, ri_name );
            os << ( is_compl ? "0 1\n" : "1 1\n" );
          }
          latch_idx++;
        }
      } );
    }
  }

  os << ".end\n";
  os << std::flush;
}

template<class Ntk>
void write_blif( Ntk const& ntk, std::ostream& os, write_blif_params const& ps = {} )
{
  write_blif_optimized( ntk, os, ps );
}

template<class Ntk>
void write_blif( Ntk const& ntk, std::string const& filename, write_blif_params const& ps = {} )
{
  std::ofstream os( filename.c_str(), std::ofstream::out );
  write_blif_optimized( ntk, os, ps );
  os.close();
}

} /* namespace mockturtle */