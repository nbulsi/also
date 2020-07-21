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
        add_option( "--num_functions, -n", num_functions, "set the number of functions to be synthesized, default = 1, works for default synthesize mode" );
      }

      rules validity_rules() const
      {
        return { has_store_element<optimum_network>( env ) };
      }

      private:
      int num_steps = 1;
      float error_rate = 0.1;
      int num_functions = 1;

      std::string print_expr( const also::mig3& mig3, const int& step_idx )
      {
        std::stringstream ss;
        std::vector<char> inputs;

        for( auto i = 0; i < 3; i++ )
        {
          if( mig3.steps[step_idx][i] == 0 )
          {
            inputs.push_back( '0' );
          }
          else
          {
            inputs.push_back( char( 'a' + mig3.steps[step_idx][i] - 1 ) );
          }
        }

        switch( mig3.operators[ step_idx ] )
        {
          default:
            assert( false && "illegal operator id" );
            break;

          case 0:
            ss << "<" << inputs[0] << inputs[1] << inputs[2] << "> "; 
            break;
          
          case 1:
            ss << "<!" << inputs[0] << inputs[1] << inputs[2] << "> "; 
            break;

          case 2:
            ss << "<" << inputs[0] << "!" << inputs[1] << inputs[2] << "> "; 
            break;

          case 3:
            ss << "<" << inputs[0] << inputs[1] << "!" << inputs[2] << "> "; 
            break;
        }

        return ss.str();
      }

      std::string print_all_expr( const spec& spec, also::mig3& mig3 )
      {
        std::stringstream ss;

        char pol = spec.out_inv ? '!' : ' ';

        std::cout << "[i] " << spec.nr_steps << " steps are required " << std::endl;
        for(auto i = 0; i < spec.nr_steps; i++ )
        {
          if(  i == spec.nr_steps - 1 ) 
          {
            ss << pol;
            ss << char( i + spec.nr_in + 'a' ) << "=" << print_expr( mig3, i ); 
          }
          else
          {
            ss << char( i + spec.nr_in + 'a' ) << "=" << print_expr( mig3, i ); 
          }
        }

        std::cout << "[expressions] " << ss.str() << std::endl;

        std::cout << "[i] There are " << spec.get_nr_out() << " functions be synthesized\n\n"; 

        for( int i = 0; i < spec.get_nr_out(); i++ )
        {
          std::cout << "[i] Function id " << i << " is " << kitty::to_binary( spec[i] ) << std::endl;
          auto outvar = mig3.get_output(i) >> 1;
          std::cout << "[i] PO " << i << " is " << char( outvar - 1 + 'a' ); 
          if( mig3.get_output(i) & 1 )
          {
            std::cout << " and inverted\n";
          }
          else
          {
            std::cout << std::endl;
          }
          
          std::cout << std::endl;
        }

        return ss.str();
      }

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
        assert( store<optimum_network>().size() >= num_functions );

        spec spec;
        also::mig3 mig3;

        //spec.verbosity = 3;
        spec.add_alonce_clauses    = true;
        spec.add_colex_clauses     = true;
        spec.add_lex_func_clauses  = true;
        spec.add_symvar_clauses    = true;

        spec.initial_steps = num_steps;

        for( int i = 0 ; i < num_functions; i++ )
        {
          auto& opt = store<optimum_network>()[i];
          auto copy = opt.function;
          if( copy.num_vars()  < 3 )
          {
            spec[i] = kitty::extend_to( copy, 3 );
          }
          else
          {
            spec[i] = copy;
          }
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
                  enumerate_m3ig( spec[0] ); //works for only one function
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
                }
              } );
        }
        
        std::cout << fmt::format( "[time]: {:5.2f} seconds\n", to_seconds( time ) );
      }

  };

  ALICE_ADD_COMMAND( exact_m3ig, "Exact synthesis" )
}

#endif
