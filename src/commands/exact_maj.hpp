/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file exact_maj.hpp
 *
 * @brief exact synthsis using pure MAJ operators as underlying structure,
 * already implemented in PERCY, maintained by Winston Haaswijk,
 * EPFL
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef EXACT_MAJ_HPP
#define EXACT_MAJ_HPP

#include <alice/alice.hpp>
#include <mockturtle/mockturtle.hpp>
#include <percy/percy.hpp>

#include "../store.hpp"

namespace alice
{

  class exact_maj_command: public command
  {
    public:
      explicit exact_maj_command( const environment::ptr& env ) : command( env, "using exact synthesis to find optimal MAJs" )
      {
        add_flag( "--fence, -f",        "FENCE synthesize" );
      }
      
      rules validity_rules() const
      {
        return { has_store_element<optimum_network>( env ) };
      }


    protected:
      void execute()
      {
        auto& opt = store<optimum_network>().current();

        bsat_wrapper solver;
        spec spec;
        mig mig;
        maj_encoder encoder(solver);

        auto copy = opt.function;
        if( copy.num_vars()  < 3 )
        {
          spec[0] = kitty::extend_to( copy, 3 );
        }
        else
        {
          spec[0] = copy;
        }

        stopwatch<>::duration time{0};
        /* executions */
        if( is_set( "fence" ) )
        {
            call_with_stopwatch( time, [&]() 
                { 
                  if( maj_fence_synthesize( spec, mig, solver, encoder ) == success )
                  {
                    std::cout << "[e]: ";
                    mig.to_expression( std::cout );
                    std::cout << std::endl;
                  }
                } );

        }
        else
        {
         call_with_stopwatch( time, [&]() 
              { 
                if( maj_synthesize( spec, mig, solver, encoder ) == success )
                {
                  std::cout << "[e]: ";
                  mig.to_expression( std::cout );
                  std::cout << std::endl;
                }
              }
              );
        }
         
        std::cout << fmt::format( "[time]: {:5.2f} seconds\n", to_seconds( time ) );
      }

  };

  ALICE_ADD_COMMAND( exact_maj, "Exact synthesis" )

}

#endif
