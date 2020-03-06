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
         //assert( store<aig_network>().size() > 0 );
         //aig_network aig = store<aig_network>().current();
        
         //std::cout << "TODO: Mapping Boolean functions to MAGIC NOR array." << std::endl;


         /* Target NOR_network*/
			   nor_network nor;

         auto a = nor.create_pi();
         auto b = nor.create_pi();

         auto c = nor.create_nor( a, b );

         auto f = nor.create_po( c );
           /* Show the level and I/O numbers of optimized AIG_network for the latter comparison with the NOR_network*/
			   depth_view nor_depth{ nor };
			   std::cout << "[I/O:" << nor.num_pis() << "/" << nor.num_pos() << "] nor gates: "
				 << nor.num_gates() << " nor depth: " << nor_depth.depth() << std::endl;
      }
    
    private:
  };

  ALICE_ADD_COMMAND( magic, "Rewriting" )


}

#endif
