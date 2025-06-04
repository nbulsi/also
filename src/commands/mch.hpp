/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file mch.hpp
 *
 * @brief Technology map and logic optimization using mixed structural choice.
 * @brief Zhang Hu, Hongyang Pan, Yinshui Xia, Lunyao Wang and Zhufei Chu, "Mixed Structural Choice Operator: Enhancing Technology Mapping with Heterogeneous Representations", DAC' 25, ACM/IEEE Design Automation Conference, San Francisco, USA, June 2025.
 *
 * @author Zhang Hu
 * @since  0.1
 */

#ifndef MCH_HPP
#define MCH_HPP

#include "../core/direct_mapping.hpp"
#include "../core/structural_choice/aig_choice_compute.hpp"
#include "../core/structural_choice/choice_exact_map.hpp"
#include "../core/structural_choice/choice_lut_mapper.hpp"
#include "../core/structural_choice/mig_choice_compute.hpp"
#include "../core/structural_choice/xag_choice_compute.hpp"
#include "../core/structural_choice/xmg_choice_compute.hpp"
#include "../store.hpp"
#include <mockturtle/algorithms/node_resynthesis/sop_factoring.hpp>
#include <mockturtle/mockturtle.hpp>

using namespace mockturtle;
using namespace also;
using namespace std;

namespace alice
{
class mch_command : public command
{
public:
  explicit mch_command( const environment::ptr& env )
      : command( env, "tech map and logic optimization using mixed structural choice" )
  {
    add_option( "cut_limit, -l", cut_limit,
                "set the cut size from 2 to 49, default = 49" );
    add_option( "cut_size, -k", cut_size,
                "set the lut size from 2 to 8, default = 6" );
    add_option( "--output, -o", filename, "the verilog filename" );
    add_flag( "--Standard, -S", "Standard Cell Mapping" );
    add_flag( "--LUT, -L", "LUT Mapping" );
    add_flag( "--aig, -a", "Mapping and Optimization of AIG" );
    add_flag( "--xag, -g", "Mapping and Optimization of XAG" );
    add_flag( "--mig, -m", "Mapping and Optimization of MIG" );
    add_flag( "--xmg, -x", "Mapping and Optimization of XMG" );
    add_option( "--MCH, -M", mch_mode,
                "Mapping and Optimization using MCH\n"
                "  0 : AIG + XMG (default)\n"
                "  1 : AIG + XAG\n"
                "  2 : AIG + MIG\n"
                "  3 : MIG + XMG\n"
                "  4 : Reserved" );
    add_flag( "--area, -A", "Area-oriented" );
    add_option( "--ratio, -r", ratio,
                "Critical path node ratio, default = 0.8" );
    add_flag( "--verbose, -v", "print the information" );
  }

  // rules validity_rules() const
  // {
  //   return { has_store_element<std::vector<mockturtle::gate>>( env ) };
  // }

private:
  std::string filename = "choice_map.v";
  uint32_t mch_mode{ 2u };
  int cut_size = 6;
  int cut_limit = 49;
  float ratio = 0.8;

protected:
  void execute()
  {
    clock_t begin, end;
    double totalTime;

    choice_map_params tech_ps;
    choice_map_stats tech_st;

    choice_lut_map_params lut_ps;
    choice_lut_map_stats lut_st;
    lut_ps.cut_enumeration_ps.cut_size = cut_size;
    lut_ps.cut_enumeration_ps.cut_limit = cut_limit;

    if ( is_set( "area" ) )
    {
      tech_ps.skip_delay_round = true;
      lut_ps.area_oriented_mapping = true;
    }

    aig_choice_compute_params aig_ps;
    aig_ps.ratio = ratio;
    xmg_choice_compute_params xmg_ps;
    xmg_ps.ratio = ratio;
    mig_choice_compute_params mig_ps;
    mig_ps.ratio = ratio;
    xag_choice_compute_params xag_ps;
    xag_ps.ratio = ratio;

    begin = clock();

    if ( is_set( "aig" ) && !store<aig_network>().empty() )
    {
      aig_network aig = store<aig_network>().current();
      const aig_network awc = run_aig_choice_compute( aig, aig_ps );

      if ( is_set( "Standard" ) )
      {
        if ( store<std::vector<mockturtle::gate>>().empty() )
        {
          std::cerr << "Error: Standard cell mapping requires a gate library.\n";
          return;
        }
        const auto gates = store<std::vector<mockturtle::gate>>().current();
        tech_library_params lib_params;
        tech_library<6> lib( gates, lib_params );

        const auto mapped = also::choice_map( awc, lib, tech_ps, &tech_st );

        if ( is_set( "output" ) )
          write_verilog_with_binding( mapped, filename );

        std::cout << fmt::format( "Tech mapped AIG into #gates = {} area = {:.2f} delay = {:.2f}\n",
                                  mapped.num_gates(), tech_st.area, tech_st.delay );
        // store<aig_network>().extend();
        store<aig_network>().current() = cleanup_dangling( awc );
      }
      else if ( is_set( "LUT" ) )
      {
        mapping_view<aig_network, true> mapped{ awc };
        also::choice_lut_map_inplace<decltype( mapped ), true>( mapped, lut_ps );

        klut_network klut = *choice_to_luts<klut_network>( mapped );
        store<klut_network>().extend();
        store<klut_network>().current() = cleanup_dangling( klut );
        mockturtle::depth_view depth_klut{ klut };
        std::cout << fmt::format( "LUT mapped AIG into #gates = {} level = {}\n",
                                  klut.num_gates(), depth_klut.depth() );
        // store<aig_network>().extend();
        store<aig_network>().current() = cleanup_dangling( awc );
      }

      else
      {
        std::cerr << "Please specify either --Standard or --LUT for mapping.\n";
      }
    }
    else if ( is_set( "xmg" ) && !store<xmg_network>().empty() )
    {
      xmg_network xmg = store<xmg_network>().current();
      const xmg_network xwc = run_xmg_choice_compute( xmg, xmg_ps );

      if ( is_set( "Standard" ) )
      {
        if ( store<std::vector<mockturtle::gate>>().empty() )
        {
          std::cerr << "Error: Standard cell mapping requires a gate library.\n";
          return;
        }
        const auto gates = store<std::vector<mockturtle::gate>>().current();
        tech_library_params lib_params;
        tech_library<6> lib( gates, lib_params );

        const auto mapped = also::choice_map( xwc, lib, tech_ps, &tech_st );

        if ( is_set( "output" ) )
          write_verilog_with_binding( mapped, filename );

        std::cout << fmt::format( "Tech mapped XMG into #gates = {} area = {:.2f} delay = {:.2f}\n",
                                  mapped.num_gates(), tech_st.area, tech_st.delay );
        // store<xmg_network>().extend();
        store<xmg_network>().current() = cleanup_dangling( xwc );
      }
      else if ( is_set( "LUT" ) )
      {
        mapping_view<xmg_network, true> mapped{ xwc };
        also::choice_lut_map_inplace<decltype( mapped ), true>( mapped, lut_ps );

        klut_network klut = *choice_to_luts<klut_network>( mapped );
        store<klut_network>().extend();
        store<klut_network>().current() = cleanup_dangling( klut );
        mockturtle::depth_view depth_klut{ klut };
        std::cout << fmt::format( "LUT mapped XMG into #gates = {} level = {}\n",
                                  klut.num_gates(), depth_klut.depth() );
        // store<xmg_network>().extend();
        store<xmg_network>().current() = cleanup_dangling( xwc );
      }

      else
      {
        std::cerr << "Please specify either --Standard or --LUT for mapping.\n";
      }
    }
    else if ( is_set( "mig" ) && !store<mig_network>().empty() )
    {
      mig_network mig = store<mig_network>().current();
      const mig_network mwc = run_mig_choice_compute( mig, mig_ps );

      if ( is_set( "Standard" ) )
      {
        if ( store<std::vector<mockturtle::gate>>().empty() )
        {
          std::cerr << "Error: Standard cell mapping requires a gate library.\n";
          return;
        }
        const auto gates = store<std::vector<mockturtle::gate>>().current();
        tech_library_params lib_params;
        tech_library<6> lib( gates, lib_params );

        const auto mapped = also::choice_map( mwc, lib, tech_ps, &tech_st );

        if ( is_set( "output" ) )
          write_verilog_with_binding( mapped, filename );

        std::cout << fmt::format( "Tech mapped MIG into #gates = {} area = {:.2f} delay = {:.2f}\n",
                                  mapped.num_gates(), tech_st.area, tech_st.delay );
        // store<xmg_network>().extend();
        store<mig_network>().current() = cleanup_dangling( mwc );
      }
      else if ( is_set( "LUT" ) )
      {
        mapping_view<mig_network, true> mapped{ mwc };
        also::choice_lut_map_inplace<decltype( mapped ), true>( mapped, lut_ps );

        klut_network klut = *choice_to_luts<klut_network>( mapped );
        store<klut_network>().extend();
        store<klut_network>().current() = cleanup_dangling( klut );
        mockturtle::depth_view depth_klut{ klut };
        std::cout << fmt::format( "LUT mapped MIG into #gates = {} level = {}\n",
                                  klut.num_gates(), depth_klut.depth() );
        store<mig_network>().current() = cleanup_dangling( mwc );
      }
      else
      {
        std::cerr << "Please specify either --Standard or --LUT for mapping.\n";
      }
    }
    else if ( is_set( "xag" ) && !store<xag_network>().empty() )
    {
      xag_network xag = store<xag_network>().current();
      const xag_network gwc = run_xag_choice_compute( xag, xag_ps );

      if ( is_set( "Standard" ) )
      {
        if ( store<std::vector<mockturtle::gate>>().empty() )
        {
          std::cerr << "Error: Standard cell mapping requires a gate library.\n";
          return;
        }
        const auto gates = store<std::vector<mockturtle::gate>>().current();
        tech_library_params lib_params;
        tech_library<6> lib( gates, lib_params );

        const auto mapped = also::choice_map( gwc, lib, tech_ps, &tech_st );

        if ( is_set( "output" ) )
          write_verilog_with_binding( mapped, filename );

        std::cout << fmt::format( "Tech mapped XAG into #gates = {} area = {:.2f} delay = {:.2f}\n",
                                  mapped.num_gates(), tech_st.area, tech_st.delay );
        store<xag_network>().current() = cleanup_dangling( gwc );
      }
      else if ( is_set( "LUT" ) )
      {
        mapping_view<xag_network, true> mapped{ gwc };
        also::choice_lut_map_inplace<decltype( mapped ), true>( mapped, lut_ps );

        klut_network klut = *choice_to_luts<klut_network>( mapped );
        store<klut_network>().extend();
        store<klut_network>().current() = cleanup_dangling( klut );
        mockturtle::depth_view depth_klut{ klut };
        std::cout << fmt::format( "LUT mapped XAG into #gates = {} level = {}\n",
                                  klut.num_gates(), depth_klut.depth() );
        store<xag_network>().current() = cleanup_dangling( gwc );
      }
      else
      {
        std::cerr << "Please specify either --Standard or --LUT for mapping.\n";
      }
    }
    else if ( is_set( "MCH" ) )
    {
      switch ( mch_mode )
      {
      case 0u:
      {
        aig_network aig = store<aig_network>().current();
        store<xmg_network>().extend();
        store<xmg_network>().current() = also::xmg_from_aig( aig );
        xmg_network xmg = store<xmg_network>().current();
        const xmg_network xwc = run_xmg_choice_compute( xmg, xmg_ps );

        if ( is_set( "Standard" ) )
        {
          if ( store<std::vector<mockturtle::gate>>().empty() )
          {
            std::cerr << "Error: Standard cell mapping requires a gate library.\n";
            return;
          }
          const auto gates = store<std::vector<mockturtle::gate>>().current();
          tech_library_params lib_params;
          tech_library<6> lib( gates, lib_params );

          const auto mapped = also::choice_map( xwc, lib, tech_ps, &tech_st );

          if ( is_set( "output" ) )
            write_verilog_with_binding( mapped, filename );

          std::cout << fmt::format( "Tech mapped XMG into #gates = {} area = {:.2f} delay = {:.2f}\n",
                                    mapped.num_gates(), tech_st.area, tech_st.delay );
          // store<xmg_network>().extend();
          store<xmg_network>().current() = cleanup_dangling( xwc );
        }
        else if ( is_set( "LUT" ) )
        {
          mapping_view<xmg_network, true> mapped{ xwc };
          also::choice_lut_map_inplace<decltype( mapped ), true>( mapped, lut_ps );

          klut_network klut = *choice_to_luts<klut_network>( mapped );
          store<klut_network>().extend();
          store<klut_network>().current() = cleanup_dangling( klut );
          mockturtle::depth_view depth_klut{ klut };
          std::cout << fmt::format( "LUT mapped XMG into #gates = {} level = {}\n",
                                    klut.num_gates(), depth_klut.depth() );
          // store<xmg_network>().extend();
          store<xmg_network>().current() = cleanup_dangling( xwc );
        }

        else
        {
          std::cerr << "Please specify either --Standard or --LUT for mapping.\n";
        }
        break;
      }
      case 1u:
      {
        aig_network aig = store<aig_network>().current();
        store<xag_network>().extend();
        store<xag_network>().current() = also::xag_from_aig( aig );
        xag_network xag = store<xag_network>().current();
        const xag_network gwc = run_xag_choice_compute( xag, xag_ps );

        if ( is_set( "Standard" ) )
        {
          if ( store<std::vector<mockturtle::gate>>().empty() )
          {
            std::cerr << "Error: Standard cell mapping requires a gate library.\n";
            return;
          }
          const auto gates = store<std::vector<mockturtle::gate>>().current();
          tech_library_params lib_params;
          tech_library<6> lib( gates, lib_params );

          const auto mapped = also::choice_map( gwc, lib, tech_ps, &tech_st );

          if ( is_set( "output" ) )
            write_verilog_with_binding( mapped, filename );

          std::cout << fmt::format( "Tech mapped XAG into #gates = {} area = {:.2f} delay = {:.2f}\n",
                                    mapped.num_gates(), tech_st.area, tech_st.delay );
          store<xag_network>().current() = cleanup_dangling( gwc );
        }
        else if ( is_set( "LUT" ) )
        {
          mapping_view<xag_network, true> mapped{ gwc };
          also::choice_lut_map_inplace<decltype( mapped ), true>( mapped, lut_ps );

          klut_network klut = *choice_to_luts<klut_network>( mapped );
          store<klut_network>().extend();
          store<klut_network>().current() = cleanup_dangling( klut );
          mockturtle::depth_view depth_klut{ klut };
          std::cout << fmt::format( "LUT mapped XAG into #gates = {} level = {}\n",
                                    klut.num_gates(), depth_klut.depth() );
          store<xag_network>().current() = cleanup_dangling( gwc );
        }
        else
        {
          std::cerr << "Please specify either --Standard or --LUT for mapping.\n";
        }
        break;
      }
      case 2u:
      {
        aig_network aig = store<aig_network>().current();
        store<mig_network>().extend();
        store<mig_network>().current() = also::mig_from_aig( aig );
        mig_network mig = store<mig_network>().current();
        const mig_network mwc = run_mig_choice_compute( mig, mig_ps );

        if ( is_set( "Standard" ) )
        {
          if ( store<std::vector<mockturtle::gate>>().empty() )
          {
            std::cerr << "Error: Standard cell mapping requires a gate library.\n";
            return;
          }
          const auto gates = store<std::vector<mockturtle::gate>>().current();
          tech_library_params lib_params;
          tech_library<6> lib( gates, lib_params );

          const auto mapped = also::choice_map( mwc, lib, tech_ps, &tech_st );

          if ( is_set( "output" ) )
            write_verilog_with_binding( mapped, filename );

          std::cout << fmt::format( "Tech mapped MIG into #gates = {} area = {:.2f} delay = {:.2f}\n",
                                    mapped.num_gates(), tech_st.area, tech_st.delay );
          // store<xmg_network>().extend();
          store<mig_network>().current() = cleanup_dangling( mwc );
        }
        else if ( is_set( "LUT" ) )
        {
          mapping_view<mig_network, true> mapped{ mwc };
          also::choice_lut_map_inplace<decltype( mapped ), true>( mapped, lut_ps );

          klut_network klut = *choice_to_luts<klut_network>( mapped );
          store<klut_network>().extend();
          store<klut_network>().current() = cleanup_dangling( klut );
          mockturtle::depth_view depth_klut{ klut };
          std::cout << fmt::format( "LUT mapped MIG into #gates = {} level = {}\n",
                                    klut.num_gates(), depth_klut.depth() );
          store<mig_network>().current() = cleanup_dangling( mwc );
        }
        else
        {
          std::cerr << "Please specify either --Standard or --LUT for mapping.\n";
        }
        break;
      }
      case 3u:
      {
        mig_network mig = store<mig_network>().current();
        store<xmg_network>().extend();
        store<xmg_network>().current() = also::xmg_from_mig( mig );
        xmg_network xmg = store<xmg_network>().current();
        const xmg_network xwc = run_xmg_choice_compute( xmg, xmg_ps );

        if ( is_set( "Standard" ) )
        {
          if ( store<std::vector<mockturtle::gate>>().empty() )
          {
            std::cerr << "Error: Standard cell mapping requires a gate library.\n";
            return;
          }
          const auto gates = store<std::vector<mockturtle::gate>>().current();
          tech_library_params lib_params;
          tech_library<6> lib( gates, lib_params );

          const auto mapped = also::choice_map( xwc, lib, tech_ps, &tech_st );

          if ( is_set( "output" ) )
            write_verilog_with_binding( mapped, filename );

          std::cout << fmt::format( "Tech mapped XMG into #gates = {} area = {:.2f} delay = {:.2f}\n",
                                    mapped.num_gates(), tech_st.area, tech_st.delay );
          // store<xmg_network>().extend();
          store<xmg_network>().current() = cleanup_dangling( xwc );
        }
        else if ( is_set( "LUT" ) )
        {
          mapping_view<xmg_network, true> mapped{ xwc };
          also::choice_lut_map_inplace<decltype( mapped ), true>( mapped, lut_ps );

          klut_network klut = *choice_to_luts<klut_network>( mapped );
          store<klut_network>().extend();
          store<klut_network>().current() = cleanup_dangling( klut );
          mockturtle::depth_view depth_klut{ klut };
          std::cout << fmt::format( "LUT mapped XMG into #gates = {} level = {}\n",
                                    klut.num_gates(), depth_klut.depth() );
          // store<xmg_network>().extend();
          store<xmg_network>().current() = cleanup_dangling( xwc );
        }

        else
        {
          std::cerr << "Please specify either --Standard or --LUT for mapping.\n";
        }
        break;
      }
      case 4u:
      {
        std::cout << "prepare for other types later" << std::endl;
        break;
      }
      }
    }
    else
    {
      std::cerr << "Unsupported network type or no network stored.\n";
    }

    end = clock();
    totalTime = (double)( end - begin ) / CLOCKS_PER_SEC;

    cout.setf( ios::fixed );
    cout << "[CPU time]   " << setprecision( 3 ) << totalTime << " s" << endl;
  }

private:
  aig_network run_aig_choice_compute( const aig_network& aig, const aig_choice_compute_params& params ) const
  {
    choice_view<aig_network> choice_aig( aig );
    xag_npn_resynthesis<aig_network> npn_resyn;
    sop_factoring<aig_network> sop_resyn;
    dsd_resynthesis<aig_network, decltype( sop_resyn )> dsd_resyn( sop_resyn );
    exact_library<aig_network> exact_lib( npn_resyn );
    shannon_resynthesis<aig_network> shannon_resyn;

    return aig_choice_compute( choice_aig, npn_resyn, dsd_resyn, exact_lib,
                               sop_resyn, dsd_resyn, params );
  }

  xmg_network run_xmg_choice_compute( const xmg_network& xmg, const xmg_choice_compute_params& params ) const
  {
    choice_view<xmg_network> choice_xmg( xmg );
    xmg_npn_resynthesis xmg_npn_resyn;
    xmg3_npn_resynthesis<xmg_network> xmg3_npn_resyn;
    sop_factoring<xmg_network> sop_resyn;
    dsd_resynthesis<xmg_network, decltype( sop_resyn )> dsd_resyn( sop_resyn );
    exact_library<xmg_network> exact_lib( xmg_npn_resyn );
    shannon_resynthesis<xmg_network> shannon_resyn;

    return xmg_choice_compute( choice_xmg, xmg_npn_resyn, xmg3_npn_resyn, exact_lib,
                               sop_resyn, dsd_resyn, params );
  }

  mig_network run_mig_choice_compute( const mig_network& mig, const mig_choice_compute_params& params ) const
  {
    choice_view<mig_network> choice_mig( mig );
    mig_npn_resynthesis mig_npn_resyn;
    sop_factoring<mig_network> sop_resyn;
    dsd_resynthesis<mig_network, decltype( sop_resyn )> dsd_resyn( sop_resyn );
    exact_library<mig_network> exact_lib( mig_npn_resyn );
    shannon_resynthesis<mig_network> shannon_resyn;

    return mig_choice_compute( choice_mig, mig_npn_resyn, exact_lib,
                               sop_resyn, dsd_resyn, params );
  }

  xag_network run_xag_choice_compute( const xag_network& xag, const xag_choice_compute_params& params ) const
  {
    choice_view<xag_network> choice_xag( xag );
    xag_npn_resynthesis<xag_network> npn_resyn;
    sop_factoring<xag_network> sop_resyn;
    dsd_resynthesis<xag_network, decltype( sop_resyn )> dsd_resyn( sop_resyn );
    exact_library<xag_network> exact_lib( npn_resyn );
    shannon_resynthesis<xag_network> shannon_resyn;

    return xag_choice_compute( choice_xag, npn_resyn, dsd_resyn, exact_lib,
                               sop_resyn, dsd_resyn, params );
  }
};

ALICE_ADD_COMMAND( mch, "Mapping" )
} // namespace alice
#endif
