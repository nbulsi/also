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
#include <mockturtle/mockturtle.hpp>
#include <mockturtle/io/write_verilog.hpp>
#include <fmt/format.h>
#include <kitty/kitty.hpp>

#include "networks/m5ig/m5ig.hpp"
#include "networks/img/img.hpp"

using namespace mockturtle;

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
  
  /* m5ig */
  ALICE_ADD_STORE( m5ig_network, "m5ig", "r", "m5ig", "m5igs" )

  ALICE_PRINT_STORE( m5ig_network, os, element )
  {
    os << fmt::format( " m5ig i/o = {}/{} gates = {} ", element.num_pis(), element.num_pos(), element.num_gates() );
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
    mockturtle::depth_view depth_lut{lut};
    os << fmt::format( "LUTs   i/o = {}/{}   gates = {}   level = {}", 
          lut.num_pis(), lut.num_pos(), lut.num_gates(), depth_lut.depth() );
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
      kitty::dynamic_truth_table function{0};
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

  /********************************************************************
   * Read and Write                                                   *
   ********************************************************************/
  ALICE_ADD_FILE_TYPE( aiger, "Aiger" );

  ALICE_READ_FILE( aig_network, aiger, filename, cmd )
  {
    aig_network aig;
    lorina::read_aiger( filename, mockturtle::aiger_reader( aig ) );
    return aig;
  }
  
  ALICE_PRINT_STORE_STATISTICS( aig_network, os, aig )
  {
    mockturtle::depth_view depth_aig{aig};
    os << fmt::format( "AIG   i/o = {}/{}   gates = {}   level = {}", 
          aig.num_pis(), aig.num_pos(), aig.num_gates(), depth_aig.depth() );
    os << "\n";
  }
  
  ALICE_ADD_FILE_TYPE( verilog, "Verilog" );

  ALICE_READ_FILE( xmg_network, verilog, filename, cmd )
  {
    xmg_network xmg;

    lorina::diagnostic_engine diag;
    if ( lorina::read_verilog( filename, mockturtle::verilog_reader( xmg ), &diag ) != lorina::return_code::success )
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
    mockturtle::depth_view depth_xmg{xmg};
    os << fmt::format( "XMG   i/o = {}/{}   gates = {}   level = {}", 
          xmg.num_pis(), xmg.num_pos(), xmg.num_gates(), depth_xmg.depth() );
    os << "\n";
  }
  
  ALICE_READ_FILE( mig_network, verilog, filename, cmd )
  {
    mig_network mig;
    lorina::read_verilog( filename, mockturtle::verilog_reader( mig ) );
    return mig;
  }
  
  ALICE_WRITE_FILE( mig_network, verilog, mig, filename, cmd )
  {
     mockturtle::write_verilog( mig, filename );
  }
  
  ALICE_PRINT_STORE_STATISTICS( mig_network, os, mig )
  {
    mockturtle::depth_view depth_mig{mig};
    os << fmt::format( "MIG   i/o = {}/{}   gates = {}   level = {}", 
          mig.num_pis(), mig.num_pos(), mig.num_gates(), depth_mig.depth() );
    os << "\n";
  }
  
  ALICE_ADD_FILE_TYPE( bench, "BENCH" );
  
  ALICE_WRITE_FILE( xmg_network, bench, xmg, filename, cmd )
  {
     mockturtle::write_bench( xmg, filename );
  }
  
  ALICE_WRITE_FILE( mig_network, bench, mig, filename, cmd )
  {
     mockturtle::write_bench( mig, filename );
  }
  
  ALICE_WRITE_FILE( m5ig_network, bench, m5ig, filename, cmd )
  {
     mockturtle::write_bench( m5ig, filename );
  }

  /********************************************************************
   * Convert from aig to mig                                          *
   ********************************************************************/
  ALICE_CONVERT( aig_network, element, mig_network )
  {
    aig_network aig = element;

    /* LUT mapping */
    mapping_view<aig_network, true> mapped_aig{aig};
    lut_mapping_params ps;
    ps.cut_enumeration_ps.cut_size = 4;
    lut_mapping<mapping_view<aig_network, true>, true>( mapped_aig, ps );

    /* collapse into k-LUT network */
    const auto klut = *collapse_mapped_network<klut_network>( mapped_aig );

    /* node resynthesis */
    mig_npn_resynthesis resyn;
    auto mig = node_resynthesis<mig_network>( klut, resyn );

    /* cut rewriting */
    cut_rewriting_params cut_ps;
    cut_ps.cut_enumeration_ps.cut_size = 4;
    cut_rewriting( mig, resyn, cut_ps );
    mig = cleanup_dangling( mig );

    return mig;
  }


}

#endif
