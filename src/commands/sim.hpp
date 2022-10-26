/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019-2021 Ningbo University, Ningbo, China */

/**
 * @file sim.hpp
 *
 * @brief logic network simulation
 *
 * @author Zhufei
 * @since  0.1
 */

#ifndef SIM_HPP
#define SIM_HPP

#include <mockturtle/algorithms/simulation.hpp>

namespace alice
{

  class sim_command : public command
  {
    public:
      explicit sim_command( const environment::ptr& env ) : command( env, "logic network simulation" )
      {
      }
      
    protected:
      void execute()
      {
        xmg_network xmg = store<xmg_network>().current();
        default_simulator<kitty::dynamic_truth_table> sim( xmg.num_pis() );
        const auto tt = simulate<kitty::dynamic_truth_table>( xmg, sim )[0];
        std::cout << "tt: " << tt._bits[0] << std::endl;
      }
  };


  ALICE_ADD_COMMAND( sim, "Various" )
}

#endif
