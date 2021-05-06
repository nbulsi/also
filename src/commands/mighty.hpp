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
 * @file mighty.hpp
 *
 * @brief MIG algebraic depth rewriting
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef MIGHTY_HPP
#define MIGHTY_HPP

#include <mockturtle/networks/mig.hpp>
#include <mockturtle/views/depth_view.hpp>
#include <mockturtle/algorithms/mig_algebraic_rewriting.hpp>
#include <mockturtle/algorithms/mig_resub.hpp>
#include "../core/misc.hpp"

namespace alice
{
  
  class mighty_command : public command
  {
    public:
      explicit mighty_command( const environment::ptr& env ) : command( env, "Performs algebraic MIG rewriting" )
      {
        add_option( "strategy, -s", strategy, "dfs = 0, aggressive = 1, selective = 2" );
        add_flag( "--area_aware", "do not increase area" );
      }
      
      rules validity_rules() const
      {
        return {
          has_store_element<mig_network>( env ),
            {
              [this]() { return ( strategy <= 2 && strategy >= 0 ) ; },
              "strategy must in [0,2] "
            }
          };
      }

      void print_stats( const mig_network& mig )
      {
        depth_view depth_mig( mig );
        std::cout << fmt::format( "mig   i/o = {}/{}   gates = {}   level = {}\n", mig.num_pis(), mig.num_pos(), mig.num_gates(), depth_mig.depth() );
      }

      void execute()
      {
        mig_network mig = store<mig_network>().current();

        /* parameters */
        ps.allow_area_increase = !is_set( "area_aware" );

        if( strategy == 0u )
        {
          ps.strategy = mig_algebraic_depth_rewriting_params::dfs;
        }
        else if( strategy == 1u )
        {
          ps.strategy = mig_algebraic_depth_rewriting_params::aggressive;
        }
        else
        {
          ps.strategy = mig_algebraic_depth_rewriting_params::selective;
        }
        
        /* depth-aware rewriting */
        depth_view depth_mig( mig );
        mig_algebraic_depth_rewriting( depth_mig, ps );
        mig = cleanup_dangling( mig );
        
        /* resubstituion */
        using view_t = depth_view<fanout_view<mig_network>>;
        fanout_view<mig_network> fanout_view{mig};
        view_t resub_view{fanout_view};

        mig_resubstitution( resub_view );

        mig = cleanup_dangling( mig );
        
        std::cout << "[mighty] ";
        auto mig_copy = cleanup_dangling( mig );
        also::print_stats( mig_copy );

        store<mig_network>().extend();
        store<mig_network>().current() = mig;
      }
    
    private:
      mig_algebraic_depth_rewriting_params ps;
      int strategy = 0;
  };

  ALICE_ADD_COMMAND( mighty, "Rewriting" )

}

#endif
