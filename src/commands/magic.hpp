/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file magic.hpp
 *
 * @brief Mapping Boolean function to Magic NOR array
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef MAGIC_HPP
#define MAGIC_HPP

#include <mockturtle/mockturtle.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/print.hpp>

#include"../networks/nor.hpp"

namespace alice
{

  class magic_command : public command
  {
    public:
      explicit magic_command( const environment::ptr& env ) : command( env, "Performs MAGIC NOR mappings" )
      {
        add_flag( "-v,--verbose", "show statistics" );
      }
      
      rules validity_rules() const
      {
        return { has_store_element<aig_network>( env ) };
      }

    protected:
      void execute()
      {
        /* derive some aig */
         assert( store<aig_network>().size() > 0 );
         aig_network aig = store<aig_network>().current();
        
         std::cout << "TODO: Mapping Boolean functions to MAGIC NOR array." << std::endl;

           /* Show the level and I/O numbers of optimized AIG_network for the latter comparison with the NOR_network*/
			   depth_view aig_depth{ aig };
			   std::cout << "[I/O:" << aig.num_pis() << "/" << aig.num_pos() << "] AIG gates: "
				 << aig.num_gates() << " AIG depth: " << aig_depth.depth() << std::endl;

         /* Target NOR_network*/
			   nor_network nor;
          /* Ensure the topo logic order of aig_network */
			    topo_view topo{ aig };

          aig.foreach_gate([this](auto n)
			    {
			    	if (aig.fanout_size(n) == 0 || aig.value(n) == 0)
					    return;
            else if (aig.is_pi(n))
				    {
					    return nor.create_pi();
				    }

				    else if (aig.is_po())
				    {
					    return nor.create_po();
				    }
          });
      }
    
    private:
  };

  ALICE_ADD_COMMAND( magic, "Rewriting" )


}

#endif
