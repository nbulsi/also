/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file exact_m5ig.hpp
 *
 * @brief exact synthesis using m5ig as the underlying data
 * structure
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef EXACT_M5IG_HPP
#define EXACT_M5IG_HPP

#include <alice/alice.hpp>
#include <mockturtle/mockturtle.hpp>

#include "../store.hpp"
#include "../core/exact_m5ig_encoder.hpp"
#include "../core/m5ig_helper.hpp"

namespace alice
{

  class exact_m5ig_command: public command
  {
    public:
      explicit exact_m5ig_command( const environment::ptr& env ) : command( env, "using exact synthesis to find optimal M5IGs" )
      {
        add_flag( "--verbose, -v",      "print the information" );
        add_flag( "--cegar, -c",        "CEGAR encoding" );
        add_flag( "--enumerate, -e",    "enumerate all the solutions" );
        add_flag( "--fence, -f",        "fence-based synthesize" );
        add_flag( "--parallel, -p",     "parallel fence-based synthesize" );
      }

      rules validity_rules() const
      {
        return { has_store_element<optimum_network>( env ) };
      }

    private:

      std::string print_expr( const also::mig5& mig5, const int& step_idx )
      {
        std::stringstream ss;
        std::vector<char> inputs;

        for( auto i = 0; i < 5; i++ )
        {
          if( mig5.steps[step_idx][i] == 0 )
          {
            inputs.push_back( '0' );
          }
          else
          {
            inputs.push_back( char( 'a' + mig5.steps[step_idx][i] - 1 ) );
          }
        }

        switch( mig5.operators[ step_idx ] )
        {
          default:
            break;

          case 0:
            ss << "<" << inputs[0] << inputs[1] << inputs[2] << inputs[3] << inputs[4] << "> "; 
            break;
          
          case 1:
            ss << "<!" << inputs[0] << inputs[1] << inputs[2] << inputs[3] << inputs[4] << "> "; 
            break;

          case 2:
            ss << "<" << inputs[0] << "!" << inputs[1] << inputs[2] << inputs[3] << inputs[4] << "> "; 
            break;

          case 3:
            ss << "<" << inputs[0] << inputs[1] << "!" << inputs[2] << inputs[3] << inputs[4] << "> "; 
            break;

          case 4:
            ss << "<" << inputs[0] << inputs[1] << inputs[2] << "!" << inputs[3] << inputs[4] << "> "; 
            break;

          case 5:
            ss << "<" << inputs[0] << inputs[1] << inputs[2] << inputs[3] << "!" << inputs[4] << "> "; 
            break;

          case 6:
            ss << "<!" << inputs[0] << "!" << inputs[1] << inputs[2] << inputs[3] << inputs[4] << "> "; 
            break;

          case 7:
            ss << "<!" << inputs[0] << inputs[1] << "!" << inputs[2] << inputs[3] << inputs[4] << "> "; 
            break;

          case 8:
            ss << "<!" << inputs[0] << inputs[1] << inputs[2] << "!" << inputs[3] << inputs[4] << "> "; 
            break;

          case 9:
            ss << "<!" << inputs[0] << inputs[1] << inputs[2] << inputs[3] << "!" << inputs[4] << "> "; 
            break;

          case 10:
            ss << "<" << inputs[0] << "!" << inputs[1] << "!" << inputs[2] << inputs[3] << inputs[4] << "> "; 
            break;

          case 11:
            ss << "<" << inputs[0] << "!" << inputs[1] << inputs[2] << "!" << inputs[3] << inputs[4] << "> "; 
            break;

          case 12:
            ss << "<" << inputs[0] << "!" << inputs[1] << inputs[2] << inputs[3] << "!" << inputs[4] << "> "; 
            break;

          case 13:
            ss << "<" << inputs[0] << inputs[1] << "!" << inputs[2] << "!" << inputs[3] << inputs[4] << "> "; 
            break;

          case 14:
            ss << "<" << inputs[0] << inputs[1] << "!" << inputs[2] << inputs[3] << "!" << inputs[4] << "> "; 
            break;

          case 15:
            ss << "<" << inputs[0] << inputs[1] << inputs[2] << "!" << inputs[3] << "!" << inputs[4] << "> "; 
            break;

        }

        return ss.str();
      }

      std::string print_all_expr( const spec& spec, const also::mig5& mig5 )
      {
        std::stringstream ss;

        char pol = spec.out_inv ? '!' : ' ';

        std::cout << "[i] " << spec.nr_steps << " steps are required " << std::endl;
        for(auto i = 0; i < spec.nr_steps; i++ )
        {
          if(  i == spec.nr_steps - 1 ) 
          {
            ss << pol;
            ss << char( i + spec.nr_in + 'a' ) << "=" << print_expr( mig5, i ); 
          }
          else
          {
            ss << char( i + spec.nr_in + 'a' ) << "=" << print_expr( mig5, i ); 
          }
        }

        std::cout << "[expressions] " << ss.str() << std::endl;
        return ss.str();
      }

      std::string nbu_mig_five_encoder_test( const kitty::dynamic_truth_table& tt )
      {
        std::stringstream ss;
        bsat_wrapper solver;
        spec spec;
        also::mig5 mig5;

        spec.verbosity = 3;

        auto copy = tt;
        if( copy.num_vars()  < 5 )
        {
          spec[0] = kitty::extend_to( copy, 5 );
        }
        else
        {
          spec[0] = tt;
        }

        also::mig_five_encoder encoder( solver );

        if ( also::mig_five_synthesize( spec, mig5, solver, encoder ) == success )
        {
          return print_all_expr( spec, mig5 );
        }
        else
        {
          return " fail ";
        }
      }

      std::string nbu_mig_five_encoder_cegar_test( const kitty::dynamic_truth_table& tt )
      {
        std::stringstream ss;
        bsat_wrapper solver;
        spec spec;
        also::mig5 mig5;

        auto copy = tt;
        if( copy.num_vars()  < 5 )
        {
          spec[0] = kitty::extend_to( copy, 5 );
        }
        else
        {
          spec[0] = tt;
        }

        also::mig_five_encoder encoder( solver );

        if ( also::mig_five_cegar_synthesize( spec, mig5, solver, encoder ) == success )
        {
          return print_all_expr( spec, mig5 );
        }
        else
        {
          return " fail ";
        }
      }

      void enumerate_m5ig( const kitty::dynamic_truth_table& tt )
      {
        bsat_wrapper solver;
        spec spec;
        also::mig5 mig5;

        auto copy = tt;
        if( copy.num_vars()  < 5 )
        {
          spec[0] = kitty::extend_to( copy, 5 );
        }
        else
        {
          spec[0] = tt;
        }

        also::mig_five_encoder encoder( solver );
        
        int nr_solutions = 0;

        while( also::next_solution( spec, mig5, solver, encoder ) == success )
        {
          print_all_expr( spec, mig5 );
          nr_solutions++;
        }

        std::cout << "There are " << nr_solutions << " solutions found." << std::endl;

      }

    protected:
      void execute()
      {
        auto& opt = store<optimum_network>().current();
        
        spec spec;
        also::mig5 mig5;
        spec.verbosity = 3;

        auto copy = opt.function;
        if( copy.num_vars()  < 5 )
        {
          spec[0] = kitty::extend_to( copy, 5 );
        }
        else
        {
          spec[0] = copy;
        }

        stopwatch<>::duration time{0};
        if( is_set( "cegar" ) )
        {
          call_with_stopwatch( time, [&]() 
              { 
                nbu_mig_five_encoder_cegar_test( copy );
              } );
        }
        else if( is_set( "fence" ) )
        {
          bsat_wrapper solver;
          also::mig_five_encoder encoder( solver );
          
          call_with_stopwatch( time, [&]() 
              { 
                if ( also::mig_five_fence_synthesize( spec, mig5, solver, encoder ) == success )
                {
                  print_all_expr( spec, mig5 );
                }
              } );
        }
        else if( is_set( "enumerate" ) )
        {
          call_with_stopwatch( time, [&]() 
              { 
               enumerate_m5ig( copy ); 
              });
        }
        else if( is_set( "parallel" ) )
        {
          call_with_stopwatch( time, [&]() 
              { 
                if ( also::parallel_nocegar_mig_five_fence_synthesize( spec, mig5 ) == success )
                {
                  print_all_expr( spec, mig5 );
                }
              } );
        }
        else
        {
          call_with_stopwatch( time, [&]() 
              { 
               nbu_mig_five_encoder_test( copy );
              });
        }
        
        std::cout << fmt::format( "[time]: {:5.2f} seconds\n", to_seconds( time ) );
      }

  };

  ALICE_ADD_COMMAND( exact_m5ig, "Exact synthesis" )

}

#endif
