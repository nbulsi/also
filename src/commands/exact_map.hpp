/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019-2021 Ningbo University, Ningbo, China */

/**
 * @file exact_map.hpp
 *
 * @brief Exact map used for logic networks transformation
 *
 * @author Zhufei Chu
 * @author Zhang Hu
 * @since  0.1
 */

#ifndef EXACT_MAP_HPP
#define EXACT_MAP_HPP

#include <mockturtle/algorithms/mapper.hpp>
#include <mockturtle/algorithms/node_resynthesis/mig_npn.hpp>
#include <mockturtle/algorithms/node_resynthesis/xag_npn.hpp>
#include <mockturtle/algorithms/node_resynthesis/xmg_npn.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/utils/tech_library.hpp>

using namespace mockturtle;
using namespace mockturtle::detail;

namespace alice
{

class exact_map_command : public command
{
public:
  explicit exact_map_command( const environment::ptr& env )
      : command( env, "Exact mapping for logic networks transformation" )
  {
    add_flag( "--logic_sharing, -l", "Enable logic sharing" );
    add_flag( "--aig, -a", "Use AIG as target logic network" );
    add_flag( "--xmg, -x", "Use XMG as target logic network" );
    add_flag( "--xag, -g", "Use XAG as target logic network" );
    add_flag( "--mig, -m", "Use MIG as target logic network" );
    add_flag( "--opt, -o", "Perform logic optimization" );
    add_flag( "--dc, -d", "Use don't cares" );
    add_flag( "--area, -A", "Optimize for area (skip delay round)" );
  }

protected:
  void execute() override
  {
    exact_library_params eps;

    if ( is_set( "xmg" ) )
    {
      if ( is_set( "opt" ) )
      {
        map_with_library<xmg_network, xmg_npn_resynthesis>( "xmg", eps );
      }
      else
      {
        map_from_aig<xmg_network, xmg_npn_resynthesis>( "xmg", eps );
      }
    }
    else if ( is_set( "aig" ) )
    {
      if ( is_set( "opt" ) )
      {
        map_with_library<aig_network, xag_npn_resynthesis<aig_network>>( "aig", eps );
      }
      else
      {
        map_from_xmg<aig_network, xag_npn_resynthesis<aig_network>>( "aig", eps );
      }
    }
    else if ( is_set( "xag" ) )
    {
      if ( is_set( "opt" ) )
      {
        map_with_library<xag_network, xag_npn_resynthesis<xag_network>>( "xag", eps );
      }
      else
      {
        map_from_aig<xag_network, xag_npn_resynthesis<xag_network>>( "xag", eps );
      }
    }
    else if ( is_set( "mig" ) )
    {
      if ( is_set( "opt" ) )
      {
        map_with_library<mig_network, mig_npn_resynthesis>( "mig", eps );
      }
      else
      {
        map_from_aig<mig_network, mig_npn_resynthesis>( "mig", eps );
      }
    }
  }

private:
  template<typename Ntk, typename Resyn>
  void map_with_library( const std::string& name, const exact_library_params& eps )
  {
    Ntk ntk = store<Ntk>().current();
    Resyn resyn;
    exact_library<Ntk> lib( resyn, eps );

    map_params ps;
    if ( is_set( "logic_sharing" ) )
      ps.enable_logic_sharing = true;
    if ( is_set( "dc" ) )
      ps.use_dont_cares = true;
    if ( is_set( "area" ) )
      ps.skip_delay_round = true;

    map_stats st;
    auto mapped = mockturtle::map( ntk, lib, ps, &st );
    store<Ntk>().extend();
    store<Ntk>().current() = cleanup_dangling( mapped );
  }

  template<typename Ntk, typename Resyn>
  void map_from_aig( const std::string& name, const exact_library_params& eps )
  {
    assert( store<aig_network>().size() > 0 );
    aig_network aig = store<aig_network>().current();
    Resyn resyn;
    exact_library<Ntk> lib( resyn, eps );

    map_params ps;
    if ( is_set( "logic_sharing" ) )
      ps.enable_logic_sharing = true;
    if ( is_set( "area" ) )
      ps.skip_delay_round = true;

    map_stats st;
    Ntk mapped = mockturtle::map( aig, lib, ps, &st );
    store<Ntk>().extend();
    store<Ntk>().current() = cleanup_dangling( mapped );
  }

  template<typename Ntk, typename Resyn>
  void map_from_xmg( const std::string& name, const exact_library_params& eps )
  {
    assert( store<xmg_network>().size() > 0 );
    xmg_network xmg = store<xmg_network>().current();
    Resyn resyn;
    exact_library<Ntk> lib( resyn, eps );

    map_params ps;
    if ( is_set( "logic_sharing" ) )
      ps.enable_logic_sharing = true;
    if ( is_set( "area" ) )
      ps.skip_delay_round = true;

    map_stats st;
    Ntk mapped = mockturtle::map( xmg, lib, ps, &st );
    store<Ntk>().extend();
    store<Ntk>().current() = cleanup_dangling( mapped );
  }
};

ALICE_ADD_COMMAND( exact_map, "Rewriting" )

} // namespace alice

#endif // EXACT_MAP_HPP
