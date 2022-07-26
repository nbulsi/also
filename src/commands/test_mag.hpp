/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file test_mag.hpp
 *
 * @brief test mux-and graph 
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef TEST_MAG_HPP
#define TEST_MAG_HPP

#include <mockturtle/mockturtle.hpp>
#include "../networks/mag/mag.hpp"

namespace alice
{

  class test_mag_command: public command
  {
      public:
      explicit test_mag_command( const environment::ptr& env ) : command( env, " test mux-and graph" )
      {
      }

      private:
      void create_mag()
      {
        mag_network mag;
        auto a = mag.create_pi();
        auto b = mag.create_pi();
        auto c = mag.create_pi();

        auto n1 = mag.create_ite( a, c, b );
        auto n2 = mag.create_and( a, b );
        auto n3 = mag.create_and( c, n2 );
        auto n4 = mag.create_and( !n1, n3 );
        
        mag.create_po( n3 );
        mag.create_po( n4 );

        depth_view_params ps;
        ps.count_complements = true;
        mockturtle::depth_view depth_mag{ mag, {}, ps };

        auto depth = depth_mag.depth();

        std::cout << " PI: " << mag.num_pis() 
                  << " PO: " << mag.num_pos() 
                  << " size: " << mag.size() 
                  << " #gates: " << mag.num_gates() 
                  << " #invs: "  << mockturtle::num_inverters( mag ) 
                  << " depth: " << depth << std::endl;

        /* check fanin/fanout size */
        assert( mag.fanin_size( mag.get_node( a ) ) == 0 );
        assert( mag.fanin_size( mag.get_node( b ) ) == 0 );
        assert( mag.fanin_size( mag.get_node( c ) ) == 0 );
        assert( mag.fanout_size( mag.get_node( a ) ) == 2 );
        assert( mag.fanout_size( mag.get_node( b ) ) == 2 );
        assert( mag.fanout_size( mag.get_node( c ) ) == 2 );
      }

      protected:
      void execute()
      {
        create_mag();
      }
    
      private:
  };

  ALICE_ADD_COMMAND( test_mag, "Various" )
}

#endif
