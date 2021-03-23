/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file imgrw.hpp
 *
 * @brief implication logic network algebraic rewriting
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef IMGRW_COMMAND_HPP
#define IMGRW_COMMAND_HPP

#include "../core/img_rewriting.hpp"
#include "../core/misc.hpp"

using namespace also;

namespace alice
{

  class imgrw_command : public command
  {
    public:
      explicit imgrw_command( const environment::ptr& env ) : command( env, "Performs algebraic IMG rewriting" )
      {
        add_option( "strategy, -s", strategy, "dfs = 0, aggressive = 1, selective = 2" );
        add_flag( "--area_aware", "do not increase area" );
        add_flag( "--verbose", "verbose output" );
      }
      
      rules validity_rules() const
      {
        return {
          has_store_element<img_network>( env ),
            {
              [this]() { return ( strategy <= 2 && strategy >= 0 ) ; },
              "strategy must in [0,2] "
            }
          };
      }


      protected:
      void execute()
      {
        img_network img = store<img_network>().current();

        /* Examples: a -> ( a -> b ) = a -> b
        img_network img;

        const auto zero = img.get_constant( false );
        const auto a = img.create_pi();
        const auto b = img.create_pi();

        const auto f1 = img.create_imp( a, b );
        const auto f2 = img.create_imp( a, f1 );

        img.create_po( f2 );
        also::print_stats( img );*/

        /* parameters */
        ps.allow_area_increase = !is_set( "area_aware" );
        ps.verbose = is_set( "verbose" );

        if( strategy == 0 )
        {
          ps.strategy = img_depth_rewriting_params::dfs;
        }
        else if( strategy == 1 )
        {
          ps.strategy = img_depth_rewriting_params::aggressive;
        }
        else if( strategy == 2 )
        {
          ps.strategy = img_depth_rewriting_params::selective;
        }
        else
        {
          assert( false );
        }

        stopwatch<>::duration time{0};
        
        call_with_stopwatch( time, [&]() 
            {
              /* rewriting */
              depth_view depth_img{img};
              img_depth_rewriting( depth_img, ps );
              img = cleanup_dangling( img );
            } );

        /* print information */
        std::cout << "[imgrw] "; 
        auto img_copy = cleanup_dangling( img );
        also::print_stats( img_copy );
        
        store<img_network>().extend(); 
        store<img_network>().current() = img;
        
        std::cout << fmt::format( "[time]: {:5.2f} seconds\n", to_seconds( time ) );
      }
    
    private:
      img_depth_rewriting_params ps;
      int strategy = 0;
  };

  ALICE_ADD_COMMAND( imgrw, "Rewriting" )

}

#endif
