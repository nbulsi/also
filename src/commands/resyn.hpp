/**
 * @file resyn.hpp
 *
 * @brief QBF-based resynthesis for Multiplicative complexity reduction
 *
 * @author Jun Zhu
 * @since  0.1
 */

#ifndef RESYN_HPP
#define RESYN_HPP

#include "../core/specification.hpp"
#include "../core/subcircuitSyn.hpp"
#include "../core/synthesisRun.hpp"
#include <../core/ps2_implemented.hpp>

#include <mockturtle/mockturtle.hpp>
#include "../core/misc.hpp"

namespace alice
{
class resyn_command : public command
{
public:
  explicit resyn_command( const environment::ptr& env ) : command( env, "QBF resynthesis." )
  {
    add_flag( "-v,--verbose", "example:resyn -g -l -t 60" );
    add_flag( "-a,--aig", "aig network" );
    add_flag( "-g,--xag", "xag network" );
    add_flag( "-l,--localCircuit", "useLocalCircuit for resynthesis");
    add_option( "-t,--use_time", use_time, "use_time for resynthesis");
    add_option( "-r,--root", root_mode, "how to get root,0 = random,1 = Reconvergent");
    add_option( "-s,--sub", sub_mode, "how to get subcircuit,0 = default,1 = BFS,2 = DFS");
  }

protected:
  void execute()
  {
    if ( is_set( "aig" ) )
    {
      aig_network aig = store<aig_network>().current();
      SynthesisManager<aig_network> r( aig, 1);
      std::pair<int, int> budget = { use_time, 0 }; // 运行时间/运行次数
      if ( is_set( "localCircuit" ) )
      {
        useLocalCircuit = true;
      }
      r.reduce( budget, useLocalCircuit );
      aig_network aig_new = r.getAig();
      store<aig_network>().extend();
      store<aig_network>().current() = aig_new;
      
      also::print_stats( aig_new );
    }
    else if (is_set( "xag" ) )
    {
      xag_network xag = store<xag_network>().current();
      SynthesisManager<xag_network> r( xag, 2);

      std::pair<int, int> budget = { use_time, 0 };
      r.set_mode_root( root_mode );
      r.set_mode_sub( sub_mode );
      if ( is_set( "localCircuit" ) )
      {
        useLocalCircuit = true;
      }
      r.reduce( budget, useLocalCircuit );
      xag_network xag_new = r.getXag();
      store<xag_network>().extend();
      store<xag_network>().current() = xag_new;

      /*function_stats*/
      also::function_counts counts1 = also::node_functions( xag_new );
       std::cout << fmt::format( "XAG   i/o = {}/{}   AND = {}   XOR = {}\n",
          xag_new.num_pis(), xag_new.num_pos(), std::to_string( counts1.and_num ),std::to_string( counts1.xor_num ) );
  
    }
  }

private:
  unsigned use_time = 1u;
  int root_mode = 0;
  int sub_mode = 0;
  bool useLocalCircuit = false;
};
 ALICE_ADD_COMMAND( resyn, "Exact synthesis" )
}
#endif