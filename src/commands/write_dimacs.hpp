/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019-2021 Ningbo University, Ningbo, China */

/**
 * @file write_dimacs.hpp
 *
 * @brief write logic network into CNF DIMACS format
 *
 * @author Zhufei
 * @since  0.1
 */

#ifndef WRITE_DIMACS_HPP
#define WRITE_DIMACS_HPP

#include <mockturtle/io/write_dimacs.hpp>

namespace alice
{

  class write_dimacs_command : public command
  {
    public:
      explicit write_dimacs_command( const environment::ptr& env ) : command( env, "write logic network into dimacs" )
      {
        add_flag( "--xag, -g",   "using xag as source logic network" );
        add_flag( "--xmg, -x",   "using xmg as source logic network" );
        add_flag( "--mig, -m",   "using mig as source logic network" );
        add_flag( "--klut, -l",  "using klut as source logic network" );
        add_option( "--filename, -f", filename, "specify the filename, default = /tmp/tmp.cnf" );
      }

    protected:
      void execute()
      {
        /* parameters */
        if( is_set( "xmg" ) )
        {
          xmg_network xmg = store<xmg_network>().current();
          write_dimacs( xmg, filename );
        }
        else if( is_set( "xag" ) )
        {
          xag_network xag = store<xag_network>().current();
          write_dimacs( xag, filename );
        }
        else if( is_set( "mig" ) )
        {
          mig_network mig = store<mig_network>().current();
          write_dimacs( mig, filename );
        }
        else if( is_set( "klut" ) )
        {
          klut_network klut = store<klut_network>().current();
          write_dimacs( klut, filename );
        }
        else
        {
          std::cout << "At least one store should be spcified, see write_dimacs -h for more info.\n"; 
        }
      }

    private:
      std::string filename = "/tmp/tmp.cnf";
  };


  ALICE_ADD_COMMAND( write_dimacs, "I/O" )
}

#endif
