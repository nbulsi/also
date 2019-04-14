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
        /* derive some XMG */
         aig_network aig = store<aig_network>().current();
        
         std::cout << "TODO: Mapping Boolean functions to MAGIC NOR array." << std::endl;
      }
    
    private:
  };

  ALICE_ADD_COMMAND( magic, "Rewriting" )


}

#endif
