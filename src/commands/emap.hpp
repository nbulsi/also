#ifndef EMAP_HPP
#define EMAP_HPP

#include <fmt/format.h>

#include <lorina/aiger.hpp>
#include <lorina/genlib.hpp>
#include <mockturtle/algorithms/aig_balancing.hpp>
#include <mockturtle/algorithms/emap.hpp>
#include <mockturtle/io/aiger_reader.hpp>
#include <mockturtle/io/genlib_reader.hpp>
#include <mockturtle/io/write_verilog.hpp>
#include <mockturtle/mockturtle.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/block.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/utils/name_utils.hpp>
#include <mockturtle/utils/tech_library.hpp>
#include <mockturtle/views/cell_view.hpp>
#include <mockturtle/views/depth_view.hpp>
#include <mockturtle/views/names_view.hpp>

#include "../core/properties.hpp"

namespace alice
{

class emap_command : public command
{
public:
  explicit emap_command( const environment::ptr& env )
      : command( env, "Standard cell mapping " )
  {
    add_flag( "--aig, -a", "emap map for AIG" );
    add_flag( "--xag, -g", "emap map for XAG" );
    add_flag( "--mul", "Technology mapping using multioutput cells" );
    add_flag( "--area, -A", "toggles area-oriented mapping [default = false]" );
    add_option( "--output, -o", filename, "the verilog filename" );
    add_option( "--relax_required, -R", relax_required,
                "set the relax required times, default = 0" );
    add_option( "--match, -M", match,
                "set the matching mode\n\t0: boolean\n\t1: structural\n\t2: hybrid, "
                "default = 2" );
    add_option( "--required_time, -r", required_time,
                "set the required time, default = 0" );
    add_option( "--area_flow_rounds, -F", area_flow_rounds,
                "set the area flow optimization, default = 3" );
    add_option( "--ela_rounds, -e", ela_rounds,
                "set the exact area optimization, default = 2" );
    add_flag( "--verbose, -v", "print the information" );
  }

  rules validity_rules() const
  {
    return { has_store_element<std::vector<mockturtle::gate>>( env ) };
  }

private:
  std::string filename = "emap.v";
  uint32_t match{ 2u };
  double required_time{ 0.0f };
  double relax_required{ 0.0f };
  uint32_t area_flow_rounds{ 2u };
  uint32_t ela_rounds{ 2u };

protected:
  void execute()
  {
    /* derive genlib */
    std::vector<mockturtle::gate> gates =
        store<std::vector<mockturtle::gate>>().current();
    tech_library_params tps;
    // tps.ignore_symmetries = false; // set to true to drastically speed-up
    // mapping with minor delay increase
    tech_library<9> tech_lib( gates, tps );

    emap_params ps;
    emap_stats st;

    switch ( match )
    {
    case 0u:
      ps.matching_mode = emap_params::boolean;
      break;
    case 1u:
      ps.matching_mode = emap_params::structural;
      break;
    case 2u:
      ps.matching_mode = emap_params::hybrid;
      break;
    }
    if ( is_set( "area" ) )
    {
      ps.area_oriented_mapping = true;
    }
    if ( is_set( "mul" ) )
    {
      ps.map_multioutput = true;
    }
    if ( is_set( "relax_required" ) )
    {
      ps.relax_required = relax_required;
    }
    if ( is_set( "required_time" ) )
    {
      ps.required_time = required_time;
    }
    if ( is_set( "area_flow_rounds" ) )
    {
      ps.area_flow_rounds = area_flow_rounds;
    }
    if ( is_set( "ela_rounds" ) )
    {
      ps.ela_rounds = ela_rounds;
    }
    if ( is_set( "aig" ) )
    {
      if ( store<aig_network>().size() == 0u )
      {
        std::cerr << "[e] no AIG in the store\n";
      }
      else
      {
        auto aig = store<aig_network>().current();

        /* remove structural redundancies */
        aig_balancing_params bps;
        bps.minimize_levels = false;
        bps.fast_mode = false;
        aig_balance( aig, bps );

        cell_view<block_network> res = emap<9>( aig, tech_lib, ps, &st );

        if ( is_set( "output" ) )
        {
          write_verilog_with_cell( res, filename );
        }

        std::cout << fmt::format(
            "Mapped AIG into #gates = {}, area = {:.2f}, delay = {:.2f}, "
            "multioutputs = {},runtime = {}\n",
            res.num_gates(), res.compute_area(), res.compute_worst_delay(),
            st.multioutput_gates, to_seconds( st.time_total ) );
      }
    }
    else if ( is_set( "xag" ) )
    {
      if ( store<xag_network>().size() == 0u )
      {
        std::cerr << "[e] no XAG in the store\n";
      }
      else
      {
        auto xag = store<xag_network>().current();

        cell_view<block_network> res = emap<9>( xag, tech_lib, ps, &st );

        if ( is_set( "output" ) )
        {
          write_verilog_with_cell( res, filename );
        }

        std::cout << fmt::format(
            "Mapped XAG into #gates = {}, area = {:.2f}, delay = {:.2f}, "
            "multioutputs = {},runtime = {}\n",
            res.num_gates(), res.compute_area(), res.compute_worst_delay(),
            st.multioutput_gates, to_seconds( st.time_total ) );
      }
    }
    else
    {
      std::cerr << "[e] no network type selected\n";
    }
    if ( is_set( "verbose" ) )
    {
      tps.verbose = true;
    }
  }
};

ALICE_ADD_COMMAND( emap, "Mapping" )

} // namespace alice

#endif
