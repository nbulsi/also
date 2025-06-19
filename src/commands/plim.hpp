/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * @file plim.hpp
 *
 * @brief RRAM-based In-Memory Computing using RM3 Graphs
 *
 * @author YanMing
 * @since  0.1
 */

#ifndef PLIM_HPP
#define PLIM_HPP
#include "../core/plim_compiler.hpp"
#include <fstream>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/views/depth_view.hpp>
#include <mockturtle/views/fanout_view.hpp>
#include "../core/properties.hpp"

#include "../core/utils/range_utils.hpp"


namespace alice
{

class plim_command : public command
{
public:
  explicit plim_command( const environment::ptr& env ) : command( env, "PLiM compiler" )
  {
    add_flag( "-p, --print", "print the program" );
    add_flag( "--naive", "turn off all optimization" );
    add_option( "-s, --generator_strategy", generator_strategy, "memristor generator request strategy:\n0: LIFO\n1: FIFO" );
  }
  
  /* get settings with often-used pre-defined options */
  properties::ptr make_settings() const
  {
    auto settings = std::make_shared<properties>();
    return settings;
  }

protected:
  void execute()
  {
    mig_network mig = store<mig_network>().current();
    
    const auto settings = make_settings();
    settings->set( "enable_cost_function", !is_set( "naive" ) );
    settings->set( "generator_strategy", generator_strategy );
    settings->set( "progress", is_set( "progress" ) );
    const auto program = compile_for_plim( mig, settings, statistics );


    if ( is_set( "print" ) )
    {
      std::cout << program << std::endl;
    }

    std::cout << "[i] step count:   " << program.step_count() << std::endl
              << "[i] RRAM count:   " << program.rram_count() << std::endl
              << "[i] write counts: " << any_join( program.write_counts(), " " ) << std::endl;
  }

private:
  unsigned generator_strategy = 0u;
  properties::ptr statistics;
};

ALICE_ADD_COMMAND( plim, "Rewriting" )

}

#endif

