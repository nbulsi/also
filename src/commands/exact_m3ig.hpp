/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file exact_m3ig.hpp
 *
 * @brief exact synthesis using MIG as the underlying data
 * structure
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef EXACT_M3IG_HPP
#define EXACT_M3IG_HPP

#include <alice/alice.hpp>
#include <mockturtle/mockturtle.hpp>

#include "../store.hpp"
#include "../core/exact_m3ig_encoder.hpp"
#include "../core/m3ig_helper.hpp"

namespace alice
{

  class exact_m3ig_command: public command
  {
    public:
      explicit exact_m3ig_command( const environment::ptr& env ) : 
                      command( env, "using exact synthesis to find optimal M3IGs" )
      {
        add_flag( "--verbose, -v",           "print the information" );
        add_flag( "--cegar, -c",             "cegar encoding" );
        add_flag( "--approximate, -x",       "approximate computing" );
        add_flag( "--enumerate, -e",         "enumerate all the solutions" );
        add_flag( "--fence, -f",             "fence-based synthesize" );
        add_flag( "--cegar_fence, -a",       "cegar fence-based synthesize" );
        add_flag( "--parallel, -p",          "parallel nocegar fence-based synthesize" );
        add_flag( "--para_cegar_fence, -b",  "parallel cegar fence-based synthesize" );
        add_option( "--init_steps, -s", num_steps, "reset the initial steps for synthesize" );
        add_option( "--error_rate, -r", error_rate, "set the error rate for approximate synthesis" );
      }

      rules validity_rules() const
      {
        return { has_store_element<optimum_network>( env ) };
      }

      private:
      int num_steps = 1;
      float error_rate = 0.1;

      void enumerate_m3ig( const kitty::dynamic_truth_table& tt )
      {
        bsat_wrapper solver;
        spec spec;
        also::mig3 mig3;

        auto copy = tt;
        if( copy.num_vars()  < 3 )
        {
          spec[0] = kitty::extend_to( copy, 3 );
        }
        else
        {
          spec[0] = tt;
        }

        also::mig_three_encoder encoder( solver );
        
        int nr_solutions = 0;

        while( also::next_solution( spec, mig3, solver, encoder ) == success )
        {
          print_all_expr( spec, mig3 );
          nr_solutions++;
        }

        std::cout << "There are " << nr_solutions << " solutions found." << std::endl;

      }

    protected:
      void execute()
      {
        auto& opt = store<optimum_network>().current();

        spec spec;
        also::mig3 mig3;
        mig_network mig;

        //spec.verbosity = 3;
        spec.add_alonce_clauses    = true;
        spec.add_colex_clauses     = true;
        spec.add_lex_func_clauses  = true;
        spec.add_symvar_clauses    = true;

        spec.initial_steps = num_steps;

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
        if( is_set( "cegar" ) )
        {
          bsat_wrapper solver;
          also::mig_three_encoder encoder( solver );
          call_with_stopwatch( time, [&]() 
              { 
                if ( also::mig_three_cegar_synthesize( spec, mig3, solver, encoder ) == success )
                {
                  print_all_expr( spec, mig3 );
                }
              } );
        }
        else if( is_set( "approximate" ) )
        {
          bsat_wrapper solver;
          also::mig_three_encoder encoder( solver );
          call_with_stopwatch( time, [&]() 
              { 
                if ( also::mig_three_cegar_approximate_synthesize( spec, mig3, solver, encoder, error_rate ) == success )
                {
                  print_all_expr( spec, mig3 );
                }
              } );
        }
        else if( is_set( "enumerate" ) )
        {
          bsat_wrapper solver;
          also::mig_three_encoder encoder( solver );
          call_with_stopwatch( time, [&]() 
              { 
                  enumerate_m3ig( copy );
              });
        }
        else if( is_set( "fence" ) )
        {
          bsat_wrapper solver;
          also::mig_three_encoder encoder( solver );
          call_with_stopwatch( time, [&]() 
              { 
                if ( also::mig_three_fence_synthesize( spec, mig3, solver, encoder ) == success )
                {
                  print_all_expr( spec, mig3 );
                }
              } );
        }
        else if( is_set( "parallel" ) )
        {
          bmcg_wrapper solver;
          also::mig_three_encoder encoder( solver );
          call_with_stopwatch( time, [&]() 
              { 
                if ( also::parallel_nocegar_mig_three_fence_synthesize( spec, mig3 ) == success )
                {
                  print_all_expr( spec, mig3 );
                }
              } );
        }
        else if( is_set( "para_cegar_fence" ) )
        {
          bmcg_wrapper solver;
          also::mig_three_encoder encoder( solver );
          call_with_stopwatch( time, [&]() 
              { 
                if ( also::parallel_mig_three_fence_synthesize( spec, mig3 ) == success )
                {
                  print_all_expr( spec, mig3 );
                }
              } );
        }
        else if( is_set( "cegar_fence" ) )
        {
          bsat_wrapper solver;
          also::mig_three_encoder encoder( solver );
          call_with_stopwatch( time, [&]() 
              { 
                if ( also::mig_three_cegar_fence_synthesize( spec, mig3, solver, encoder ) == success )
                {
                  print_all_expr( spec, mig3 );
                }
              } );
        }
        else
        {
          bsat_wrapper solver;
          also::mig_three_encoder encoder( solver );
          call_with_stopwatch( time, [&]() 
              { 
                if ( also::mig_three_synthesize( spec, mig3, solver, encoder ) == success )
                {
                  print_all_expr( spec, mig3 );
                  mig = mig3_to_mig_network( spec, mig3 );
                  store<mig_network>().extend(); 
                  store<mig_network>().current() = mig;
                }
              } );
        }
        
        std::cout << fmt::format( "[time]: {:5.2f} seconds\n", to_seconds( time ) );
      }

  };

  ALICE_ADD_COMMAND( exact_m3ig, "Exact synthesis" )
}

#endif
