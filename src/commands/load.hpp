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
 * @file load.hpp
 *
 * @brief load a hexdecimal string represented truth table
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef LOAD_HPP
#define LOAD_HPP

#include "../store.hpp"

namespace alice
{

  void add_optimum_network_entry( command& cmd, kitty::dynamic_truth_table& function )
  {
    if ( cmd.env->variable( "npn" ) != "" )
    {
      function = std::get<0>( kitty::exact_npn_canonization( function ) );
    }

    optimum_network entry( function );

    if ( !entry.exists() )
    {
      cmd.store<optimum_network>().extend();
      cmd.store<optimum_network>().current() = entry;
    }
  }
  
  class load_command : public command
  {
    public:
      load_command( const environment::ptr& env ) : command( env, "Load new entry" )
    {
      add_option( "truth_table,--tt", truth_table, "truth table in hex format" );
      add_flag( "--binary,-b", "read truth table as binary string" );
    }

    protected:
      void execute() override
      {
        auto function = [this]() {
          if ( is_set( "binary" ) )
          {
            const unsigned num_vars = ::log( truth_table.size() ) / ::log( 2.0 );
            kitty::dynamic_truth_table function( num_vars );
            kitty::create_from_binary_string( function, truth_table );
            return function;
          }
          else
          {
            const unsigned num_vars = ::log( truth_table.size() * 4 ) / ::log( 2.0 );
            kitty::dynamic_truth_table function( num_vars );
            kitty::create_from_hex_string( function, truth_table );
            return function;
          }
        }();

        add_optimum_network_entry( *this, function );
      }

    private:
      std::string truth_table;
  };

  ALICE_ADD_COMMAND( load, "Loading" );

}

#endif
