/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file write_dot.hpp
 *
 * @brief write dot files
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef WRITE_DOT_HPP
#define WRITE_DOT_HPP

#include <mockturtle/mockturtle.hpp>
#include <mockturtle/io/write_dot.hpp>

namespace alice
{

  class write_dot_command : public command
  {
    public:
      explicit write_dot_command( const environment::ptr& env ) : command( env, "write dot file" )
      {
        add_flag( "--xmg_network,-x", "write xmg_network into dot files" );
        add_flag( "--aig_network,-a", "write aig_network into dot files" );
        add_flag( "--mig_network,-m", "write mig_network into dot files" );
      }

      rules validity_rules() const
      {
        return { has_store_element<xmg_network>( env ) };
                 //has_store_element<aig_network>( env ) ||
                 //has_store_element<mig_network>( env ) };
      }
      
    protected:
      void execute()
      {
      }

      private:
  };

  ALICE_ADD_COMMAND( write_dot, "I/O" )

}

#endif
