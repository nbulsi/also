#ifndef EXACT_RM3_HPP
#define EXACT_RM3_HPP

#include <alice/alice.hpp>
#include <mockturtle/mockturtle.hpp>
#include <vector>

#include "../core/exact_rm.hpp"
#include "../core/exact_rm3ig_encoder.hpp"
#include "../networks/rm3/rm3_compl_rw.hpp"
#include "../store.hpp"
using namespace percy;
namespace alice
{

    class exact_rm3_command : public command
    {
    public:
        explicit exact_rm3_command(const environment::ptr &env) : command(env, "using exact synthesis to find optimal rm3ig")
        {
            add_flag("--verbose, -v", "print the information");
            add_flag( "--enumerate, -e",         "enumerate all the solutions" );
            add_flag("--aig_exact, -a", "from exact AIG synthesis");
            add_flag( "--rm3_exact, -r", "from exact RM3 synthesis" );
            add_flag( "--best_rm3ig, -b", "from exact RM3 synthesis" );
            add_option( "--init_steps, -s", num_steps, "reset the initial steps for synthesize" );
            add_option( "--num_functions, -n", num_functions, "set the number of functions to be synthesized, default = 1, works for default synthesize mode" );
        }
        rules validity_rules() const
        {
            return {has_store_element<optimum_network>(env)};
        }

        void enumerate_rm3ig( const kitty::dynamic_truth_table& tt )
        {
          bsat_wrapper solver;
          spec spec;
          also::rm3ig rm3ig;
          rm3_network rm3;

          auto copy = tt;
          if( copy.num_vars()  < 3 )
          {
            spec[0] = kitty::extend_to( copy, 3 );
          }
          else
          {
            spec[0] = tt;
          }

          also::rm_three_encoder encoder( solver );

          int nr_solutions = 0;

          while ( also::next_solution( spec, rm3ig, solver, encoder ) == success )
          {
            print_all_expr( spec, rm3ig );
            nr_solutions++;
            rm3 = rm3ig_to_rm3ig_network( spec, rm3ig );
            int x;
            x = mockturtle::num_inverters( rm3 );
            std::cout << "反相器个数为" << x << std::endl;
          }

          std::cout << "There are " << nr_solutions << " solutions found." << std::endl;

        }

      private:
        int num_steps = 1;
        int num_functions = 1;

      protected:
        void execute()
        {
            bool verb = false;
            auto store_size = store<optimum_network>().size();
            assert( store_size >= num_functions );

            spec spec;
            also::rm3ig rm3ig;
            rm3_network rm3;

            if ( !is_set( "num_functions" ) )
            {
              num_functions = 1;
            }

            if (is_set("verbose"))
            {
                verb = true;
            }

            spec.verbosity = 0;
            // spec.add_alonce_clauses = true;
            // spec.add_colex_clauses = true;
            // spec.add_symvar_clauses = true;

            spec.initial_steps = num_steps;

            for ( int i = 0; i < num_functions; i++ )
            {
                auto& opt = store<optimum_network>()[store_size - i - 1];
                auto copy = opt.function;
                if ( copy.num_vars() < 3 )
                {
                  spec[i] = kitty::extend_to( copy, 3 );
                }
                else
                {
                  spec[i] = copy;
                }
            }

            stopwatch<>::duration time{ 0 };

            if ( is_set( "rm3_exact" ) )
            {
              bsat_wrapper solver;
              also::rm_three_encoder encoder( solver );
              call_with_stopwatch( time, [&]()
                                   {
                if ( also::rm_three_synthesize( spec, rm3ig, solver, encoder ) == success )
                {
                  print_all_expr( spec, rm3ig );
                  rm3 = rm3ig_to_rm3ig_network( spec, rm3ig );
                  int x;
                  x = mockturtle::num_inverters( rm3 );
                  std::cout << "反相器个数为" << x << std::endl;

                  store<rm3_network>().extend();
                  store<rm3_network>().current() = rm3;
                } } );
            }

            auto& opt = store<optimum_network>().current();

            if(is_set("aig_exact"))
            {
                int min_gates = 0;
                call_with_stopwatch( time, [&]()
                                     { 
                  also::rm3_from_aig(opt.function, verb, min_gates);
                } );
            }

            if(is_set("best_rm3ig"))
            {
                int min_gates = 0;
                call_with_stopwatch( time, [&]()
                                     { 
                  rm3 = also::best_rm3ig(opt.function, verb, min_gates);
                  // also::print_stats( rm3 );
                  store<rm3_network>().extend();
                  store<rm3_network>().current() = rm3; 
                } );
            }

            else if( is_set( "enumerate" ) )
            {
              bsat_wrapper solver;
              also::rm_three_encoder encoder( solver );
              call_with_stopwatch( time, [&]()
                  {
                      enumerate_rm3ig( spec[0] ); //works for only one function
                  });
            }

            // else
            // {
            //   call_with_stopwatch( time, [&]()
            //                        {
            //     auto rm3 = also::nbu_rm3_encoder_test( opt.function);
            //     also::print_stats( rm3 );
            //     store<rm3_network>().extend(); 
            //     store<rm3_network>().current() = rm3; } );
            // }
            std::cout << fmt::format("[time]: {:5.2f} seconds\n", to_seconds(time));
        }
    };

    ALICE_ADD_COMMAND(exact_rm3, "Exact synthesis")
}

#endif

