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

#include "../core/direct_mapping.hpp"
#include "../core/misc.hpp"
#include "../networks/aoig/build_xag_db.hpp"
#include "../networks/aoig/xag_lut_dec.hpp"
#include "../networks/aoig/xag_lut_npn.hpp"
#include "../networks/img/img.hpp"
#include "../networks/img/img_npn.hpp"
#include "../networks/rm3/RM3.hpp"
#include "../networks/rm3/m3ig_npn.hpp"
#include "../networks/rm3/rm3ig_npn.hpp"
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

namespace alice
{
class exact_map_command : public command
{
public:
  explicit exact_map_command( const environment::ptr& env ) : command( env, "exact map for logic networks transformation" )
  {
    add_flag( "--logic_sharing, -l", "Enable logic sharing" );
    add_flag( "--aig", "aig by exact synthsis" );
    add_flag( "--aig_to_xmg, -a", "aig convert to xmg by exact synthsis" );
    add_flag( "--xmg_to_aig, -x", "xmg convert to aig by exact synthsis" );
    add_flag( "--aig_to_xag", "aig convert to xag by exact synthsis" );
    add_flag( "--aig_to_mig, -m", "aig convert to mig by exact synthsis" );
    add_flag( "--aig_to_rm3, -r", "aig convert to rm3 by exact synthsis" );
    add_flag( "--rm3_to_aig", "rm3 convert to aig by exact synthsis" );
    add_flag( "--mig_to_rm3, -y", "mig convert to rm3 by exact synthsis" );
    add_flag( "--rm3_to_rm3", "rm3 by exact synthsis" );
    add_flag( "--direct_rm3, -d", "mig convert to rm3 by direct map" );
  }

protected:
  void execute()
  {
    clock_t begin, end;
    double totalTime = 0.0;
    map_params ps;
    if ( is_set( "aig" ) )
    {
      assert( store<aig_network>().size() > 0 );
      aig_network aig1 = store<aig_network>().current();
      
      xag_npn_resynthesis<aig_network> resyn;
      exact_library<aig_network, xag_npn_resynthesis<aig_network>> lib( resyn );

      map_stats st;
      ps.skip_delay_round = true;
      //ps.use_dont_cares = true;

      aig_network aig = mockturtle::map( aig1, lib, ps, &st );
      also::print_stats( aig );

      store<aig_network>().extend();
      store<aig_network>().current() = aig;
    }
    else if ( is_set( "aig_to_xmg" ) )
    {
      assert( store<aig_network>().size() > 0 );
      aig_network aig = store<aig_network>().current();
      aig = cleanup_dangling( aig );
      also::print_stats( aig );
      xmg_npn_resynthesis resyn;
      exact_library<xmg_network, xmg_npn_resynthesis> lib( resyn );

      map_stats st;

      auto xmg = mockturtle::map( aig, lib, ps, &st );
      also::print_stats( xmg );

      store<xmg_network>().extend();
      store<xmg_network>().current() = xmg;
    }
    else if ( is_set( "logic_sharing" ) )
    {
      ps.enable_logic_sharing = true;
    }

    else if ( is_set( "xmg_to_aig" ) )
    {
      assert( store<xmg_network>().size() > 0 );
      // aig_network aig;
      xmg_network xmg = store<xmg_network>().current();
      xag_npn_resynthesis<aig_network> resyn;
      exact_library<aig_network, xag_npn_resynthesis<aig_network>> lib( resyn );
      map_stats st;

      aig_network aig = mockturtle::map( xmg, lib, ps, &st );
      also::print_stats( aig );

      store<aig_network>().extend();
      store<aig_network>().current() = aig;
    }

    else if ( is_set( "aig_to_xag" ) )
    {
      assert( store<aig_network>().size() > 0 );
      aig_network aig = store<aig_network>().current();
      xag_npn_lut_resynthesis resyn;
      exact_library<xag_network, xag_npn_lut_resynthesis> lib( resyn );

      map_stats st;

      auto xag = mockturtle::map( aig, lib, ps, &st );
      also::print_stats( xag );

      store<xag_network>().extend();
      store<xag_network>().current() = xag;
    }
    else if ( is_set( "aig_to_mig" ) )
    {
      assert( store<aig_network>().size() > 0 );
      aig_network aig = store<aig_network>().current();
      m3ig_npn_resynthesis resyn;
      exact_library<mig_network, m3ig_npn_resynthesis> lib( resyn );

      map_stats st;

      auto mig = mockturtle::map( aig, lib, ps, &st );
      also::print_stats( mig );

      store<mig_network>().extend();
      store<mig_network>().current() = mig;
    }
    else if ( is_set( "aig_to_rm3" ) )
    {
      //begin = clock();
      ps.enable_logic_sharing = true;
      assert( store<aig_network>().size() > 0 );
      aig_network aig = store<aig_network>().current();

      stopwatch<>::duration time{ 0 };
      call_with_stopwatch( time, [&]()
                           { rm3ig_npn_resynthesis resyn;
      exact_library<rm3_network, rm3ig_npn_resynthesis> lib( resyn );
      map_stats st;
      auto rm3 = mockturtle::map( aig, lib, ps, &st );
      also::print_stats( rm3 );
      int x;
      x = mockturtle::num_inverters( rm3 );
      std::cout << "反相器个数为" << x << std::endl;
      store<rm3_network>().extend();
      store<rm3_network>().current() = rm3; } );

      // rm3ig_npn_resynthesis resyn;
      // exact_library<rm3_network, rm3ig_npn_resynthesis> lib( resyn );

      // map_stats st;

      // auto rm3 = mockturtle::map( aig, lib, ps, &st );
      //also::print_stats( rm3 );
      
      //end = clock();
      //totalTime = (double)( end - begin ) / CLOCKS_PER_SEC;

      // int x;
      // x = mockturtle::num_inverters( rm3 );
      // std::cout << "反相器个数为" << x << std::endl;

      //store<rm3_network>().extend();
      //store<rm3_network>().current() = rm3;
      std::cout << fmt::format( "[time]: {:5.2f} seconds\n", to_seconds( time ) );
    }
    else if ( is_set( "rm3_to_rm3" ) )
    {
      //ps.enable_logic_sharing = true;
      //begin = clock();
      assert( store<rm3_network>().size() > 0 );
      rm3_network rm3ig = store<rm3_network>().current();

      stopwatch<>::duration time{ 0 };
      call_with_stopwatch( time, [&]()
                           { rm3ig_npn_resynthesis resyn;
      exact_library<rm3_network, rm3ig_npn_resynthesis> lib( resyn );
      map_stats st;
      auto rm3 = mockturtle::map( rm3ig, lib, ps, &st );
      also::print_stats( rm3 );
      int x;
      x = mockturtle::num_inverters( rm3 );
      std::cout << "反相器个数为" << x << std::endl;
      store<rm3_network>().extend();
      store<rm3_network>().current() = rm3; } );

      // rm3ig_npn_resynthesis resyn;
      // exact_library<rm3_network, rm3ig_npn_resynthesis> lib( resyn );

      // map_stats st;

      // auto rm3 = mockturtle::map( rm3ig, lib, ps, &st );
      //also::print_stats( rm3 );
      
      //end = clock();
      //totalTime = (double)( end - begin ) / CLOCKS_PER_SEC;

      // int x;
      // x = mockturtle::num_inverters( rm3 );
      // std::cout << "反相器个数为" << x << std::endl;

      //store<rm3_network>().extend();
      //store<rm3_network>().current() = rm3;
      std::cout << fmt::format( "[time]: {:5.2f} seconds\n", to_seconds( time ) );
    }
    // else if ( is_set( "img" ) )
    // {
    //   // ps.enable_logic_sharing = true;
    //   assert( store<img_network>().size() > 0 );
    //   img_network img = store<img_network>().current();
    //   const mockturtle::img_npn_resynthesis resyn;
    //   exact_library<mockturtle::img_network, mockturtle::img_npn_resynthesis> lib( resyn );

    //   map_stats st;

    //   auto imply = mockturtle::map( img, lib, ps, &st );
    //   also::print_stats( imply );

    //   store<img_network>().extend();
    //   store<img_network>().current() = imply;
    // }

    else if ( is_set( "rm3_to_aig" ) )
    {
      assert( store<rm3_network>().size() > 0 );
      // aig_network aig;
      rm3_network rm3ig = store<rm3_network>().current();
      xag_npn_resynthesis<aig_network> resyn;
      exact_library<aig_network, xag_npn_resynthesis<aig_network>> lib( resyn );
      map_stats st;

      aig_network aig = mockturtle::map( rm3ig, lib, ps, &st );
      also::print_stats( aig );

      store<aig_network>().extend();
      store<aig_network>().current() = aig;
    }

    else if ( is_set( "mig_to_rm3" ) )
    {
      assert( store<mig_network>().size() > 0 );
      // aig_network aig;
      mig_network mig = store<mig_network>().current();
      rm3ig_npn_resynthesis resyn;
      exact_library<rm3_network, rm3ig_npn_resynthesis> lib( resyn );
      map_stats st;

      rm3_network rm3 = mockturtle::map( mig, lib, ps, &st );
      also::print_stats( rm3 );

      int x;
      x = mockturtle::num_inverters( rm3 );
      std::cout << "反相器个数为" << x << std::endl;

      store<rm3_network>().extend();
      store<rm3_network>().current() = rm3;
    }
    else if ( is_set( "direct_rm3" ) )
    {
      rm3_network rm3;
      assert( store<mig_network>().size() > 0 );
      // aig_network aig;
      mig_network mig = store<mig_network>().current();
      rm3 = also::rm3_from_mig( mig );
      int x;
      x = mockturtle::num_inverters( rm3 );
      std::cout << "反相器个数为" << x << std::endl;

      also::print_stats( rm3 );

      store<rm3_network>().extend();
      store<rm3_network>().current() = rm3;
    }
  }

private:
};

ALICE_ADD_COMMAND( exact_map, "Mapping" )
} // namespace alice
#endif
