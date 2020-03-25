/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file imgfc.hpp
 *
 * @brief construct a fanout-free implication logic network
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef IMGFC_HPP
#define IMGFC_HPP

#include "../core/misc.hpp"
#include "../core/img_fc.hpp"

namespace alice
{

  class imgfc_command : public command
  {
    public:
      explicit imgfc_command( const environment::ptr& env ) : command( env, "construct fanout-free IMG" )
      {
        add_flag( "--verbose", "verbose output" );
      }
      
      rules validity_rules() const
      {
        return { has_store_element<img_network>( env ) };         
      }


      protected:
      void execute()
      {
        img_network img = store<img_network>().current();

        /*
         * detect fanout-issue img node pairs
         * */
        
        stopwatch<>::duration time{0};
        
        call_with_stopwatch( time, [&]() 
            {
              depth_view depth_img( img );
              cut_ps.cut_size = 3u;
              also::img_fc_rewriting( depth_img, ps );
              img = cleanup_dangling( img );
            } );

        /* print information */
        std::cout << "[imgfc] "; 
        also::print_stats( img );
        store<img_network>().extend(); 
        store<img_network>().current() = img;
        
        std::cout << fmt::format( "[time]: {:5.2f} seconds\n", to_seconds( time ) );
      }
    
    private:
      img_fc_rewriting_params ps;
      cut_enumeration_params  cut_ps;
  };

  ALICE_ADD_COMMAND( imgfc, "Rewriting" )


}

#endif
