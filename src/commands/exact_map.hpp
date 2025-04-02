/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019-2021 Ningbo University, Ningbo, China */

/**
 * @file exact_map.hpp
 *
 * @brief Exact map used for logic networks transformation
 *
 * @author Zhufei
 * @since  0.1
 */

#ifndef EXACT_MAP_HPP
#define EXACT_MAP_HPP

#include <mockturtle/algorithms/mapper.hpp>
#include <mockturtle/algorithms/node_resynthesis/xmg_npn.hpp>
#include <mockturtle/algorithms/node_resynthesis/mig_npn.hpp>
#include <mockturtle/algorithms/node_resynthesis/xag_npn.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/utils/tech_library.hpp>

using namespace mockturtle;

namespace alice
{
  class exact_map_command : public command
  {
    public:
      explicit exact_map_command( const environment::ptr& env ) : command( env, "exact map for logic networks transformation" )
      {
        add_flag( "--logic_sharing, -l",  "Enable logic sharing" );
      }

    protected:
      void execute()
      {
          assert( store<aig_network>().size() > 0 );
          aig_network aig = store<aig_network>().current();
          exact_library_params eps;
          xmg_npn_resynthesis resyn;
          exact_library<xmg_network> lib( resyn, eps );
          map_params ps;
          if( is_set( "logic_sharing" ) )
          {
              ps.enable_logic_sharing = true;
          }
          map_stats st;

          auto xmg = mockturtle::map( aig, lib, ps, &st );
          also::print_stats( xmg );

          store<xmg_network>().extend();
          store<xmg_network>().current() = xmg;
      }

    private:

  };

  ALICE_ADD_COMMAND( exact_map, "Rewriting" )
}
#endif
