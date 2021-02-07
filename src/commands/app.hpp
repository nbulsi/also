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
            }

        protected:
            void execute()
            {
                percy::spec spec;
                auto mig = also::approximate_synthesis( spec );

                store<mig_network>().extend();
                store<mig_network>().current() = mig;
            }

        private:
    };

    ALICE_ADD_COMMAND( app, "Various" )
}

#endif
