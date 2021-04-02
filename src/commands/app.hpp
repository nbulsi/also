/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019-2021 Ningbo University, Ningbo, China */

/**
 * @file app.hpp
 *
 * @brief Synthesis for approximate computing
 *
 * @author Zhufei
 * @since  0.1
 */

#ifndef APP_HPP
#define APP_HPP

#include "../core/exact_app_m3ig.hpp"

namespace alice
{

    class app_command : public command
    {
        public:
            explicit app_command( const environment::ptr& env ) : command( env, "approximate circuit synthesis" )
            {
              add_flag( "--verbose, -v", "verbose output" );
              add_flag( "--enumerate, -e", "enumerate all the solutions" );
              add_option( "--num_functions, -n", num_functions, "set the number of functions to be synthesized, default = 1, works for default synthesize mode" );
              add_option( "--error_distance,-d", error_distance, "set the allowed maximum error distance" );
              add_option( "--min_num_nodes, -m", min_num_of_nodes, "set the allowed minimum number of network nodes" );
            }

            rules validity_rules() const
            {
                return { has_store_element<optimum_network>( env ) };
            }

        protected:
            void execute()
            {
                auto store_size = store<optimum_network>().size();
                assert( store_size >= num_functions );

                percy::spec spec;

                for( int i = 0 ; i < num_functions; i++ )
                {
                    auto& opt = store<optimum_network>()[store_size - i - 1];
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

                if( error_distance > pow( 2, num_functions ) - 1 )
                {
                  std::cout << "[warning] The error distance exceeds the maximum possible decimal value: " << pow( 2, num_functions ) - 1 << std::endl;
                }

                spec.verbosity = is_set( "verbose" ) ? 1 : 0;
                if( is_set( "enumerate" ) )
                {
                    also::enumerate_app_m3ig( spec, error_distance, min_num_of_nodes );
                }
                else
                {
                    auto mig = also::approximate_synthesis( spec, error_distance, min_num_of_nodes );

                    store<mig_network>().extend();
                    store<mig_network>().current() = mig;
                }
            }

        private:
            int num_functions = 1;
            unsigned error_distance = 0;
            unsigned min_num_of_nodes = 0;
    };

    ALICE_ADD_COMMAND( app, "Various" )
}

#endif
