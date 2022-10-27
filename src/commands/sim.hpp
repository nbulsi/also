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
        add_flag( "-a, --aig_network", " simulate current aig network" );
        add_flag( "-x, --xmg_network", " simulate current xmg network" );
        add_flag( "-p, --partial_simulate", " simulate current logic network using partial simulator " );
      }
      
    protected:
      void execute()
      {
        if( is_set( "xmg_network" ) )
        {
          if( is_set( "partial_simulate" ) )
          {
            xmg_network xmg = store<xmg_network>().current();
            std::vector<kitty::partial_truth_table> pats( xmg.num_pis() );
            for( auto i = 0; i < xmg.num_pis(); i++ )
            {
              pats[i].add_bits( 0x12345678, 32 );
            }
            partial_simulator sim( pats );

            unordered_node_map<kitty::partial_truth_table, xmg_network> node_to_value( xmg );
            simulate_nodes( xmg, node_to_value, sim );
            
            xmg.foreach_po( [&]( auto const& f ) {
                std::cout << "tt: 0x" << ( xmg.is_complemented( f ) ? ~node_to_value[f] : node_to_value[f] )._bits[0] << std::endl; 
                } );
          }
          else
          {
            xmg_network xmg = store<xmg_network>().current();
            default_simulator<kitty::dynamic_truth_table> sim( xmg.num_pis() );
            const auto tt = simulate<kitty::dynamic_truth_table>( xmg, sim )[0];
            std::cout << "tt: 0x" << tt._bits[0] << std::endl;
          }  
        }
        else if( is_set( "aig_network" ) )
        {
          aig_network aig = store<aig_network>().current();
          default_simulator<kitty::dynamic_truth_table> sim( aig.num_pis() );
          const auto tt = simulate<kitty::dynamic_truth_table>( aig, sim )[0];
          std::cout << "tt: " << tt._bits[0] << std::endl;
        }
        else
        {
          std::cout << "At least one store should be specified, see 'sim -h' for help. \n";
        }
      }
  };


  ALICE_ADD_COMMAND( sim, "Various" )
}

#endif
