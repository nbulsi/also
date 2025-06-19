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

#ifndef MEMRISTOR_HPP
#define MEMRISTOR_HPP
#include "../core/memristor_costs.hpp"
#include "../networks/rm3/RM3.hpp"
#include <fstream>
#include <mockturtle/mockturtle.hpp>
//#include <mockturtle/views/depth_view.hpp>
using namespace mockturtle;
namespace alice
{
  class memristor_command : public command
  {
    public:
      explicit memristor_command( const environment::ptr& env ) : command( env, "memristor_costs" )
      {
        add_flag( "-c, --costs", "Show memristor costs" );
      }
    protected:
      void execute()
      {
        rm3_network rm3 = store<rm3_network>().current();
        if ( is_set( "costs" ) )
        {
          std::tie( memristors, operations ) = also::memristor_costs(rm3 );
          std::cout << "[i] #memristors: " << memristors << " #operations: " << operations << std::endl;
        }
        else
        {
          std::cout << "help";
        }
      }

    private:
      unsigned memristors;
      unsigned operations;
  };

  ALICE_ADD_COMMAND( memristor, "Rewriting" )

}

#endif
