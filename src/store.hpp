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

#ifndef STORE_HPP
#define STORE_HPP

#include <alice/alice.hpp>
#include <fmt/format.h>
#include <kitty/kitty.hpp>
#include <mockturtle/mockturtle.hpp>

#include <lorina/diagnostics.hpp>
#include <lorina/genlib.hpp>
#include <mockturtle/io/blif_reader.hpp>
#include <mockturtle/io/genlib_reader.hpp>
#include <mockturtle/io/write_aiger.hpp>
#include <mockturtle/io/write_blif.hpp>
#include <mockturtle/io/write_verilog.hpp>
#include <mockturtle/networks/sequential.hpp>
#include <mockturtle/views/names_view.hpp>

#include "networks/img/img.hpp"
#include "networks/img/img_verilog_reader.hpp"
#include "networks/m5ig/m5ig.hpp"
#include "networks/mag/mag.hpp"
#include "networks/mag/mag_verilog_reader.hpp"

using namespace mockturtle;

// Define sequential network types for time-sequential circuit support
using seq_aig_network = sequential<aig_network>;
using seq_klut_network = sequential<klut_network>;

namespace alice
{

/********************************************************************
 * Genral stores                                                    *
 ********************************************************************/

/* aiger */
ALICE_ADD_STORE( aig_network, "aig", "a", "AIG", "AIGs" )

ALICE_PRINT_STORE( aig_network, os, element )
{
  os << "AIG PI/PO = " << element.num_pis() << "/" << element.num_pos() << "\n";
}

ALICE_DESCRIBE_STORE( aig_network, element )
{
  return fmt::format( "{} nodes", element.size() );
}

/* mig */
ALICE_ADD_STORE( mig_network, "mig", "m", "MIG", "MIGs" )

ALICE_PRINT_STORE( mig_network, os, element )
{
  os << "MIG PI/PO = " << element.num_pis() << "/" << element.num_pos() << "\n";
}

ALICE_DESCRIBE_STORE( mig_network, element )
{
  return fmt::format( "{} nodes", element.size() );
}

/* xmg */
ALICE_ADD_STORE( xmg_network, "xmg", "x", "xmg", "xmgs" )

ALICE_PRINT_STORE( xmg_network, os, element )
{
  os << fmt::format( " xmg i/o = {}/{} gates = {} ", element.num_pis(), element.num_pos(), element.num_gates() );
  os << "\n";
}

ALICE_DESCRIBE_STORE( xmg_network, element )
{
  return fmt::format( "{} nodes", element.size() );
}

/* xag */
ALICE_ADD_STORE( xag_network, "xag", "g", "xag", "xags" )

ALICE_PRINT_STORE( xag_network, os, element )
{
  os << fmt::format( " xag i/o = {}/{} gates = {} ", element.num_pis(), element.num_pos(), element.num_gates() );
  os << "\n";
}

ALICE_DESCRIBE_STORE( xag_network, element )
{
  return fmt::format( "{} nodes", element.size() );
}

/* m5ig */
ALICE_ADD_STORE( m5ig_network, "m5ig", "r", "m5ig", "m5igs" )

ALICE_PRINT_STORE( m5ig_network, os, element )
{
  os << fmt::format( " m5ig i/o = {}/{} gates = {} ", element.num_pis(), element.num_pos(), element.num_gates() );
  os << "\n";
}

ALICE_PRINT_STORE_STATISTICS( m5ig_network, os, m5ig )
{
  auto m5ig_copy = mockturtle::cleanup_dangling( m5ig );
  mockturtle::depth_view depth_m5ig{ m5ig_copy };
  os << fmt::format( "M5IG   i/o = {}/{}   gates = {}   level = {}",
                     m5ig.num_pis(), m5ig.num_pos(), m5ig.num_gates(), depth_m5ig.depth() );
  os << "\n";
}

ALICE_DESCRIBE_STORE( m5ig_network, element )
{
  return fmt::format( "{} nodes", element.size() );
}

/* img */
ALICE_ADD_STORE( img_network, "img", "i", "img", "imgs" )

ALICE_PRINT_STORE( img_network, os, element )
{
  os << fmt::format( " img i/o = {}/{} gates = {} ", element.num_pis(), element.num_pos(), element.num_gates() );
  os << "\n";
}

ALICE_DESCRIBE_STORE( img_network, element )
{
  return fmt::format( "{} nodes", element.size() );
}

/* mag */
ALICE_ADD_STORE( mag_network, "mag", "b", "mag", "mags" )

ALICE_PRINT_STORE( mag_network, os, element )
{
  os << fmt::format( " MAG i/o = {}/{} gates = {} ", element.num_pis(), element.num_pos(), element.num_gates() );
  os << "\n";
}

ALICE_DESCRIBE_STORE( mag_network, element )
{
  return fmt::format( "{} nodes", element.size() );
}

/*klut network*/
ALICE_ADD_STORE( klut_network, "lut", "l", "LUT network", "LUT networks" )

ALICE_PRINT_STORE( klut_network, os, element )
{
  os << fmt::format( " klut i/o = {}/{} gates = {} ", element.num_pis(), element.num_pos(), element.num_gates() );
  os << "\n";
}

ALICE_DESCRIBE_STORE( klut_network, element )
{
  return fmt::format( "{} nodes", element.size() );
}

ALICE_PRINT_STORE_STATISTICS( klut_network, os, lut )
{
  mockturtle::depth_view depth_lut{ lut };
  os << fmt::format( "LUTs   i/o = {}/{}   gates = {}   level = {}",
                     lut.num_pis(), lut.num_pos(), lut.num_gates(), depth_lut.depth() );
  os << "\n";
}

/********************************************************************
 * Sequential network stores (for sequential circuit support)       *
 ********************************************************************/

/* Sequential AIG network */
ALICE_ADD_STORE( seq_aig_network, "seq_aig", "s", "SeqAIG", "SeqAIGs" )

ALICE_PRINT_STORE( seq_aig_network, os, element )
{
  os << fmt::format( "SeqAIG PI/PO = {}/{}  RO/RI = {}/{}  gates = {}",
                     element.num_pis(), element.num_pos(),
                     element.num_registers(), element.num_registers(),
                     element.num_gates() );
  os << "\n";
}

ALICE_DESCRIBE_STORE( seq_aig_network, element )
{
  return fmt::format( "{} nodes, {} regs", element.size(), element.num_registers() );
}

ALICE_PRINT_STORE_STATISTICS( seq_aig_network, os, seq_aig )
{
  auto seq_aig_copy = mockturtle::cleanup_dangling( seq_aig );
  mockturtle::depth_view depth_seq_aig{ seq_aig_copy };
  os << fmt::format( "SeqAIG   i/o = {}/{}   regs = {}   gates = {}   level = {}",
                     seq_aig.num_pis(), seq_aig.num_pos(), seq_aig.num_registers(),
                     seq_aig.num_gates(), depth_seq_aig.depth() );
  os << "\n";
}

/* Sequential KLUT network */
ALICE_ADD_STORE( seq_klut_network, "seq_lut", "q", "SeqLUT", "SeqLUTs" )

ALICE_PRINT_STORE( seq_klut_network, os, element )
{
  os << fmt::format( "SeqLUT PI/PO = {}/{}  RO/RI = {}/{}  gates = {}",
                     element.num_pis(), element.num_pos(),
                     element.num_registers(), element.num_registers(),
                     element.num_gates() );
  os << "\n";
}

ALICE_DESCRIBE_STORE( seq_klut_network, element )
{
  return fmt::format( "{} nodes, {} regs", element.size(), element.num_registers() );
}

ALICE_PRINT_STORE_STATISTICS( seq_klut_network, os, seq_lut )
{
  mockturtle::depth_view depth_seq_lut{ seq_lut };
  os << fmt::format( "SeqLUT   i/o = {}/{}   regs = {}   gates = {}   level = {}",
                     seq_lut.num_pis(), seq_lut.num_pos(), seq_lut.num_registers(),
                     seq_lut.num_gates(), depth_seq_lut.depth() );
  os << "\n";
}

/* opt_network */
class optimum_network
{
public:
  optimum_network() = default;

  optimum_network( const kitty::dynamic_truth_table& function )
      : function( function ) {}

  optimum_network( kitty::dynamic_truth_table&& function )
      : function( std::move( function ) ) {}

  bool exists() const
  {
    static std::vector<std::unordered_set<kitty::dynamic_truth_table, kitty::hash<kitty::dynamic_truth_table>>> hash;

    if ( function.num_vars() >= hash.size() )
    {
      hash.resize( function.num_vars() + 1 );
    }

    return !hash[function.num_vars()].insert( function ).second;
  }

public: /* field access */
  kitty::dynamic_truth_table function{ 0 };
  std::string network;
};

ALICE_ADD_STORE( optimum_network, "opt", "o", "network", "networks" )

ALICE_DESCRIBE_STORE( optimum_network, opt )
{
  if ( opt.network.empty() )
  {
    return fmt::format( "{}", kitty::to_hex( opt.function ) );
  }
  else
  {
    return fmt::format( "{}, optimum network computed", kitty::to_hex( opt.function ) );
  }
}

ALICE_PRINT_STORE( optimum_network, os, opt )
{
  os << fmt::format( "function (hex): {}\nfunction (bin): {}\n", kitty::to_hex( opt.function ), kitty::to_binary( opt.function ) );

  if ( opt.network.empty() )
  {
    os << "no optimum network computed\n";
  }
  else
  {
    os << fmt::format( "optimum network: {}\n", opt.network );
  }
}

/* genlib */
ALICE_ADD_STORE( std::vector<mockturtle::gate>, "genlib", "f", "GENLIB", "GENLIBs" )

ALICE_PRINT_STORE( std::vector<mockturtle::gate>, os, element )
{
  os << "GENLIB gate size = " << element.size() << "\n";
}

ALICE_DESCRIBE_STORE( std::vector<mockturtle::gate>, element )
{
  return fmt::format( "{} gates", element.size() );
}

ALICE_ADD_FILE_TYPE( genlib, "Genlib" );

ALICE_READ_FILE( std::vector<mockturtle::gate>, genlib, filename, cmd )
{
  std::vector<mockturtle::gate> gates;
  if ( lorina::read_genlib( filename, mockturtle::genlib_reader( gates ) ) != lorina::return_code::success )
  {
    std::cout << "[w] parse error\n";
  }
  return gates;
}

ALICE_WRITE_FILE( std::vector<mockturtle::gate>, genlib, gates, filename, cmd )
{
  std::cout << "[e] not supported" << std::endl;
}

ALICE_PRINT_STORE_STATISTICS( std::vector<mockturtle::gate>, os, gates )
{
  os << fmt::format( "Entered genlib library with {} gates", gates.size() );
  os << "\n";
}

/********************************************************************
 * Read and Write                                                   *
 ********************************************************************/
ALICE_ADD_FILE_TYPE( aiger, "Aiger" );

ALICE_READ_FILE( aig_network, aiger, filename, cmd )
{
  aig_network aig;
  if ( lorina::read_aiger( filename, mockturtle::aiger_reader( aig ) ) != lorina::return_code::success )
  {
    std::cout << "[w] parse error\n";
  }
  return aig;
}

ALICE_PRINT_STORE_STATISTICS( aig_network, os, aig )
{
  auto aig_copy = mockturtle::cleanup_dangling( aig );
  mockturtle::depth_view depth_aig{ aig_copy };
  os << fmt::format( "AIG   i/o = {}/{}   gates = {}   level = {}",
                     aig.num_pis(), aig.num_pos(), aig.num_gates(), depth_aig.depth() );
  os << "\n";
}

ALICE_ADD_FILE_TYPE( verilog, "Verilog" );

ALICE_READ_FILE( xmg_network, verilog, filename, cmd )
{
  xmg_network xmg;

  if ( lorina::read_verilog( filename, mockturtle::verilog_reader( xmg ) ) != lorina::return_code::success )
  {
    std::cout << "[w] parse error\n";
  }
  return xmg;
}

ALICE_WRITE_FILE( xmg_network, verilog, xmg, filename, cmd )
{
  mockturtle::write_verilog( xmg, filename );
}

ALICE_PRINT_STORE_STATISTICS( xmg_network, os, xmg )
{
  auto xmg_copy = mockturtle::cleanup_dangling( xmg );
  mockturtle::depth_view depth_xmg{ xmg_copy };
  os << fmt::format( "XMG   i/o = {}/{}   gates = {}   level = {}",
                     xmg.num_pis(), xmg.num_pos(), xmg.num_gates(), depth_xmg.depth() );
  os << "\n";
}

ALICE_READ_FILE( mig_network, verilog, filename, cmd )
{
  mig_network mig;
  if ( lorina::read_verilog( filename, mockturtle::verilog_reader( mig ) ) != lorina::return_code::success )
  {
    std::cout << "[w] parse error\n";
  }
  return mig;
}

ALICE_WRITE_FILE( mig_network, verilog, mig, filename, cmd )
{
  mockturtle::write_verilog( mig, filename );
}

ALICE_PRINT_STORE_STATISTICS( mig_network, os, mig )
{
  auto mig_copy = mockturtle::cleanup_dangling( mig );
  mockturtle::depth_view depth_mig{ mig_copy };
  os << fmt::format( "MIG   i/o = {}/{}   gates = {}   level = {}",
                     mig.num_pis(), mig.num_pos(), mig.num_gates(), depth_mig.depth() );
  os << "\n";
}

ALICE_READ_FILE( xag_network, verilog, filename, cmd )
{
  xag_network xag;
  if ( lorina::read_verilog( filename, mockturtle::verilog_reader( xag ) ) != lorina::return_code::success )
  {
    std::cout << "[w] parse error\n";
  }

  return xag;
}

ALICE_WRITE_FILE( xag_network, verilog, xag, filename, cmd )
{
  mockturtle::write_verilog( xag, filename );
}

ALICE_PRINT_STORE_STATISTICS( xag_network, os, xag )
{
  auto xag_copy = mockturtle::cleanup_dangling( xag );
  mockturtle::depth_view depth_xag{ xag_copy };
  os << fmt::format( "XAG   i/o = {}/{}   gates = {}   level = {}",
                     xag.num_pis(), xag.num_pos(), xag.num_gates(), depth_xag.depth() );
  os << "\n";
}

ALICE_READ_FILE( img_network, verilog, filename, cmd )
{
  img_network img;

  if ( lorina::read_verilog( filename, img_verilog_reader( img ) ) != lorina::return_code::success )
  {
    std::cout << "[w] parse error\n";
  }
  return img;
}

ALICE_PRINT_STORE_STATISTICS( img_network, os, img )
{
  auto img_copy = mockturtle::cleanup_dangling( img );
  mockturtle::depth_view depth_img{ img_copy };
  os << fmt::format( "IMG   i/o = {}/{}   gates = {}   level = {}",
                     img.num_pis(), img.num_pos(), img.num_gates(), depth_img.depth() );
  os << "\n";
}

ALICE_READ_FILE( mag_network, verilog, filename, cmd )
{
  mag_network mag;

  lorina::text_diagnostics td;
  lorina::diagnostic_engine diag( &td );

  if ( lorina::read_verilog( filename, mag_verilog_reader( mag ), &diag ) != lorina::return_code::success )
  {
    std::cout << "[w] parse error\n";
  }
  return mag;
}

ALICE_WRITE_FILE( mag_network, verilog, mag, filename, cmd )
{
  mockturtle::write_verilog( mag, filename );
}

ALICE_PRINT_STORE_STATISTICS( mag_network, os, mag )
{
  auto mag_copy = mockturtle::cleanup_dangling( mag );
  mockturtle::depth_view depth_mag{ mag_copy };
  os << fmt::format( "mag   i/o = {}/{}   gates = {}   level = {}",
                     mag.num_pis(), mag.num_pos(), mag.num_gates(), depth_mag.depth() );
  os << "\n";
}

ALICE_ADD_FILE_TYPE( bench, "BENCH" );

ALICE_READ_FILE( klut_network, bench, filename, cmd )
{
  klut_network klut;
  if ( lorina::read_bench( filename, mockturtle::bench_reader( klut ) ) != lorina::return_code::success )
  {
    std::cout << "[w] parse error\n";
  }
  return klut;
}

ALICE_WRITE_FILE( xmg_network, bench, xmg, filename, cmd )
{
  mockturtle::write_bench( xmg, filename );
}

ALICE_WRITE_FILE( mig_network, bench, mig, filename, cmd )
{
  mockturtle::write_bench( mig, filename );
}

ALICE_WRITE_FILE( aig_network, bench, aig, filename, cmd )
{
  mockturtle::write_bench( aig, filename );
}

ALICE_WRITE_FILE( m5ig_network, bench, m5ig, filename, cmd )
{
  mockturtle::write_bench( m5ig, filename );
}

ALICE_WRITE_FILE( img_network, bench, img, filename, cmd )
{
  mockturtle::write_bench( img, filename );
}

ALICE_WRITE_FILE( mag_network, bench, mag, filename, cmd )
{
  mockturtle::write_bench( mag, filename );
}

ALICE_WRITE_FILE( xag_network, bench, xag, filename, cmd )
{
  mockturtle::write_bench( xag, filename );
}

ALICE_WRITE_FILE( klut_network, bench, klut, filename, cmd )
{
  mockturtle::write_bench( klut, filename );
}

ALICE_WRITE_FILE( aig_network, aiger, aig, filename, cmd )
{
  mockturtle::write_aiger( aig, filename );
}

ALICE_ADD_FILE_TYPE( blif, "Blif" );

ALICE_READ_FILE( klut_network, blif, filename, cmd )
{
  klut_network klut;

  if ( lorina::read_blif( filename, mockturtle::blif_reader( klut ) ) != lorina::return_code::success )
  {
    std::cout << "[w] parse error\n";
  }

  return klut;
}

/* Read BLIF into AIG network (for AND-INV BLIF files, via resynthesis) */
ALICE_READ_FILE( aig_network, blif, filename, cmd )
{
  // First read into klut network
  klut_network klut;

  if ( lorina::read_blif( filename, mockturtle::blif_reader( klut ) ) != lorina::return_code::success )
  {
    std::cout << "[w] parse error\n";
  }

  std::cout << fmt::format( "[i] read BLIF: {} PIs, {} POs, {} gates\n",
                            klut.num_pis(), klut.num_pos(), klut.num_gates() );

  // Convert klut to AIG using node resynthesis
  mockturtle::xag_npn_resynthesis<aig_network> resyn;
  aig_network aig = mockturtle::node_resynthesis<aig_network>( klut, resyn );
  aig = mockturtle::cleanup_dangling( aig );

  std::cout << fmt::format( "[i] converted to AIG: {} PIs, {} POs, {} gates\n",
                            aig.num_pis(), aig.num_pos(), aig.num_gates() );

  return aig;
}

/* Read BLIF into sequential klut network (supports .latch) */
ALICE_READ_FILE( seq_klut_network, blif, filename, cmd )
{
  seq_klut_network seq_klut;

  if ( lorina::read_blif( filename, mockturtle::blif_reader( seq_klut ) ) != lorina::return_code::success )
  {
    std::cout << "[w] parse error\n";
  }

  std::cout << fmt::format( "[i] read sequential BLIF: {} PIs, {} POs, {} registers, {} gates\n",
                            seq_klut.num_pis(), seq_klut.num_pos(), seq_klut.num_registers(), seq_klut.num_gates() );

  return seq_klut;
}

/* Read BLIF into sequential AIG network (via resynthesis, supports .latch) */
ALICE_READ_FILE( seq_aig_network, blif, filename, cmd )
{
  // First read into sequential klut
  seq_klut_network seq_klut;

  if ( lorina::read_blif( filename, mockturtle::blif_reader( seq_klut ) ) != lorina::return_code::success )
  {
    std::cout << "[w] parse error\n";
  }

  std::cout << fmt::format( "[i] read BLIF: {} PIs, {} POs, {} registers, {} LUTs\n",
                            seq_klut.num_pis(), seq_klut.num_pos(), seq_klut.num_registers(), seq_klut.num_gates() );

  // Convert sequential klut to sequential AIG using node resynthesis
  // This correctly handles RO/RI (registers) during the conversion
  mockturtle::xag_npn_resynthesis<seq_aig_network> resyn;
  seq_aig_network seq_aig = mockturtle::node_resynthesis<seq_aig_network>( seq_klut, resyn );

  std::cout << fmt::format( "[i] converted to SeqAIG: {} PIs, {} POs, {} registers, {} gates\n",
                            seq_aig.num_pis(), seq_aig.num_pos(), seq_aig.num_registers(), seq_aig.num_gates() );

  return seq_aig;
}

ALICE_WRITE_FILE( xmg_network, blif, xmg, filename, cmd )
{
  mockturtle::write_blif( xmg, filename );
}

ALICE_WRITE_FILE( klut_network, blif, klut, filename, cmd )
{
  mockturtle::write_blif( klut, filename );
}

/* Write AIG to BLIF (combinational) */
ALICE_WRITE_FILE( aig_network, blif, aig, filename, cmd )
{
  mockturtle::write_blif( aig, filename );
  std::cout << fmt::format( "[i] wrote AIG BLIF: {} PIs, {} POs, {} gates\n",
                            aig.num_pis(), aig.num_pos(), aig.num_gates() );
}

/* Write sequential klut to BLIF (with .latch support) */
ALICE_WRITE_FILE( seq_klut_network, blif, seq_klut, filename, cmd )
{
  mockturtle::write_blif( seq_klut, filename );
  std::cout << fmt::format( "[i] wrote sequential BLIF: {} PIs, {} POs, {} registers\n",
                            seq_klut.num_pis(), seq_klut.num_pos(), seq_klut.num_registers() );
}

/* Write sequential AIG to BLIF (with .latch support) */
ALICE_WRITE_FILE( seq_aig_network, blif, seq_aig, filename, cmd )
{
  mockturtle::write_blif( seq_aig, filename );
  std::cout << fmt::format( "[i] wrote sequential BLIF: {} PIs, {} POs, {} registers\n",
                            seq_aig.num_pis(), seq_aig.num_pos(), seq_aig.num_registers() );
}

/********************************************************************
 * Convert from aig to mig                                          *
 ********************************************************************/
ALICE_CONVERT( aig_network, element, mig_network )
{
  aig_network aig = element;

  /* LUT mapping */
  mapping_view<aig_network, true> mapped_aig{ aig };
  lut_mapping_params ps;
  ps.cut_enumeration_ps.cut_size = 4;
  lut_mapping<mapping_view<aig_network, true>, true>( mapped_aig, ps );

  /* collapse into k-LUT network */
  const auto klut = *collapse_mapped_network<klut_network>( mapped_aig );

  /* node resynthesis */
  mig_npn_resynthesis resyn;
  auto mig = node_resynthesis<mig_network>( klut, resyn );

  return mig;
}

/* show */
template<>
bool can_show<aig_network>( std::string& extension, command& cmd )
{
  extension = "dot";

  return true;
}

template<>
void show<aig_network>( std::ostream& os, const aig_network& element, const command& cmd )
{
  gate_dot_drawer<aig_network> drawer;
  write_dot( element, os, drawer );
}

template<>
bool can_show<mig_network>( std::string& extension, command& cmd )
{
  extension = "dot";

  return true;
}

template<>
void show<mig_network>( std::ostream& os, const mig_network& element, const command& cmd )
{
  gate_dot_drawer<mig_network> drawer;
  write_dot( element, os, drawer );
}

template<>
bool can_show<xmg_network>( std::string& extension, command& cmd )
{
  extension = "dot";

  return true;
}

template<>
void show<xmg_network>( std::ostream& os, const xmg_network& element, const command& cmd )
{
  gate_dot_drawer<xmg_network> drawer;
  write_dot( element, os, drawer );
}

template<>
bool can_show<klut_network>( std::string& extension, command& cmd )
{
  extension = "dot";

  return true;
}

template<>
void show<klut_network>( std::ostream& os, const klut_network& element, const command& cmd )
{
  gate_dot_drawer<klut_network> drawer;
  write_dot( element, os, drawer );
}

template<>
bool can_show<xag_network>( std::string& extension, command& cmd )
{
  extension = "dot";

  return true;
}

template<>
void show<xag_network>( std::ostream& os, const xag_network& element, const command& cmd )
{
  gate_dot_drawer<xag_network> drawer;
  write_dot( element, os, drawer );
}

/********************************************************************
 * Convert from aig to xmg                                          *
 ********************************************************************/
ALICE_CONVERT( aig_network, element, xmg_network )
{
  aig_network aig = element;

  /* LUT mapping */
  mapping_view<aig_network, true> mapped_aig{ aig };
  lut_mapping_params ps;
  ps.cut_enumeration_ps.cut_size = 4;
  lut_mapping<mapping_view<aig_network, true>, true>( mapped_aig, ps );

  /* collapse into k-LUT network */
  const auto klut = *collapse_mapped_network<klut_network>( mapped_aig );

  /* node resynthesis */
  xmg_npn_resynthesis resyn;
  auto xmg = node_resynthesis<xmg_network>( klut, resyn );

  return xmg;
}

ALICE_CONVERT( mig_network, element, xmg_network )
{
  mig_network mig = element;

  /* LUT mapping */
  mapping_view<mig_network, true> mapped_mig{ mig };
  lut_mapping_params ps;
  ps.cut_enumeration_ps.cut_size = 4;
  lut_mapping<mapping_view<mig_network, true>, true>( mapped_mig, ps );

  /* collapse into k-LUT network */
  const auto klut = *collapse_mapped_network<klut_network>( mapped_mig );

  /* node resynthesis */
  xmg_npn_resynthesis resyn;
  auto xmg = node_resynthesis<xmg_network>( klut, resyn );
  return xmg;
}

ALICE_CONVERT( xmg_network, element, mig_network )
{
  xmg_network xmg = element;

  /* LUT mapping */
  mapping_view<xmg_network, true> mapped_xmg{ xmg };
  lut_mapping_params ps;
  ps.cut_enumeration_ps.cut_size = 4;
  lut_mapping<mapping_view<xmg_network, true>, true>( mapped_xmg, ps );

  /* collapse into k-LUT network */
  const auto klut = *collapse_mapped_network<klut_network>( mapped_xmg );

  /* node resynthesis */
  mig_npn_resynthesis resyn;
  auto mig = node_resynthesis<mig_network>( klut, resyn );
  return mig;
}

/********************************************************************
 * Sequential network conversions                                   *
 ********************************************************************/

/* Convert sequential AIG to sequential KLUT (LUT mapping) */
ALICE_CONVERT( seq_aig_network, element, seq_klut_network )
{
  seq_aig_network seq_aig = element;

  /* LUT mapping */
  mapping_view<seq_aig_network, true> mapped_seq_aig{ seq_aig };
  lut_mapping_params ps;
  ps.cut_enumeration_ps.cut_size = 4;
  lut_mapping<mapping_view<seq_aig_network, true>, true>( mapped_seq_aig, ps );

  /* collapse into k-LUT network */
  auto seq_klut_opt = collapse_mapped_network<seq_klut_network>( mapped_seq_aig );
  if ( !seq_klut_opt )
  {
    std::cout << "[e] failed to collapse mapped network\n";
    return seq_klut_network{};
  }

  std::cout << fmt::format( "[i] converted SeqAIG to SeqLUT: {} PIs, {} POs, {} regs, {} LUTs\n",
                            seq_klut_opt->num_pis(), seq_klut_opt->num_pos(),
                            seq_klut_opt->num_registers(), seq_klut_opt->num_gates() );

  return *seq_klut_opt;
}

/* Convert sequential KLUT to sequential AIG (resynthesis) */
ALICE_CONVERT( seq_klut_network, element, seq_aig_network )
{
  seq_klut_network seq_klut = element;

  /* node resynthesis */
  mockturtle::xag_npn_resynthesis<seq_aig_network> resyn;
  auto seq_aig = mockturtle::node_resynthesis<seq_aig_network>( seq_klut, resyn );

  std::cout << fmt::format( "[i] converted SeqLUT to SeqAIG: {} PIs, {} POs, {} regs, {} gates\n",
                            seq_aig.num_pis(), seq_aig.num_pos(),
                            seq_aig.num_registers(), seq_aig.num_gates() );

  return seq_aig;
}

/* Convert combinational AIG to sequential AIG (for compatibility) */
ALICE_CONVERT( aig_network, element, seq_aig_network )
{
  aig_network aig = element;

  // Create sequential AIG with same structure but no registers
  seq_aig_network seq_aig;

  // Copy nodes using cleanup_dangling which preserves structure
  std::vector<seq_aig_network::signal> pi_signals;

  aig.foreach_pi( [&]( auto const& )
                  { pi_signals.push_back( seq_aig.create_pi() ); } );

  // Map old signals to new signals
  std::unordered_map<aig_network::signal, seq_aig_network::signal> signal_map;
  signal_map[aig.get_constant( false )] = seq_aig.get_constant( false );

  uint32_t pi_idx = 0;
  aig.foreach_pi( [&]( auto const& n )
                  { signal_map[aig.make_signal( n )] = pi_signals[pi_idx++]; } );

  // Process nodes in topological order
  aig.foreach_node( [&]( auto const& n )
                    {
      if ( aig.is_constant( n ) || aig.is_pi( n ) )
        return;

      std::vector<seq_aig_network::signal> children;
      aig.foreach_fanin( n, [&]( auto const& f ) {
        auto it = signal_map.find( aig.make_signal( aig.get_node( f ) ) );
        if ( it != signal_map.end() )
        {
          auto child = it->second;
          if ( aig.is_complemented( f ) )
            child = seq_aig.create_not( child );
          children.push_back( child );
        }
      });

      if ( children.size() == 2 )
      {
        signal_map[aig.make_signal( n )] = seq_aig.create_and( children[0], children[1] );
      } } );

  // Create POs
  aig.foreach_po( [&]( auto const& f )
                  {
      auto it = signal_map.find( aig.make_signal( aig.get_node( f ) ) );
      if ( it != signal_map.end() )
      {
        auto po_signal = it->second;
        if ( aig.is_complemented( f ) )
          po_signal = seq_aig.create_not( po_signal );
        seq_aig.create_po( po_signal );
      } } );

  std::cout << fmt::format( "[i] converted AIG to SeqAIG: {} PIs, {} POs, {} gates (no registers)\n",
                            seq_aig.num_pis(), seq_aig.num_pos(), seq_aig.num_gates() );

  return seq_aig;
}

} // namespace alice

#endif
