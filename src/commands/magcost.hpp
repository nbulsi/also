/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file magcost.hpp
 *
 * @brief The implemention cost of MAGs
 * @modified from https://github.com/msoeken/cirkit/blob/cirkit3/cli/algorithms/migcost.hpp
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef MAGCOST_HPP
#define MAGCOST_HPP

#include <mockturtle/mockturtle.hpp>
#include <mockturtle/properties/migcost.hpp>

namespace alice
{

  class magcost_command : public command
  {
    public:
      explicit magcost_command( const environment::ptr& env ) : command( env, "MAG cost evaluation " )
      {
      }

      rules validity_rules() const
      {
        return{ has_store_element<mag_network>( env ) };
      }

      protected:
      void execute()
      {
        mag_network mag = store<mag_network>().current();
        
        num_and = 0;
        num_mux = 0;

        /* compute num_and and num_mux */
        mag.foreach_gate( [&]( auto n ) 
            {
              if( mag.is_and( n ) )
              {
                num_and++;
              }
              else if( mag.is_ite( n ) )
              {
                num_mux++;
              }
              else
              {
                assert( false && "only support MAG now" );
              }
            }
            );

        num_gates = mag.num_gates();
        num_inv = mockturtle::num_inverters( mag );
        
        depth_view_params ps;
        ps.count_complements = false;
        mockturtle::depth_view depth_mag{mag, {}, ps};
        depth = depth_mag.depth();

        ps.count_complements = true;
        mockturtle::depth_view depth_mag2{mag,{}, ps};
        depth_mixed = depth_mag2.depth();
        std::tie( depth_and, depth_inv, depth_mux ) = split_critical_path( depth_mag2 );

        num_dangling = mockturtle::num_dangling_inputs( mag );

        //cost function
        area  = num_and * 1 + num_inv * 0.1 + num_mux * 3;
        delay = num_and * 1 + num_inv * 0.1 + num_mux * 2;

        env->out() << fmt::format( "[i] Gates             = {}\n"
            "[i] Num ANDs          = {}\n"
            "[i] Num MUXes          = {}\n"
            "[i] Inverters         = {}\n"
            "[i] Depth (def.)      = {}\n"
            "[i] Depth mixed       = {}\n"
            "[i] Depth mixed (AND) = {}\n"
            "[i] Depth mixed (INV) = {}\n"
            "[i] Depth mixed (MUX) = {}\n"
            "[i] Dangling inputs   = {}\n",
            num_gates, num_and, num_mux, num_inv, depth, depth_mixed, depth_and, depth_inv, depth_mux, num_dangling );

        env->out() << fmt::format( "[i] area        = {:.2f} \n"
            "[i] delay       = {:.2f} \n", area, delay );
      }

      private:
      template<class Ntk>
        std::tuple<unsigned, unsigned, unsigned> split_critical_path( Ntk const& ntk )
        {
          using namespace mockturtle;

          unsigned num_and{0}, num_inv{0}, num_mux{0};

          node<Ntk> cp_node;
          ntk.foreach_po( [&]( auto const& f ) {
              auto level = ntk.level( ntk.get_node( f ) );
              if ( ntk.is_complemented( f ) )
              {
                level++;
              }
                
              if ( level == ntk.depth() )
              {
                if ( ntk.is_complemented( f ) )
                {
                ++num_inv;
                }
                cp_node = ntk.get_node( f );
                return false;
              }

              return true;
              } );


          while ( !ntk.is_constant( cp_node ) && !ntk.is_pi( cp_node ) )
          {
            if( ntk.is_and( cp_node ) )
            {
              num_and++;
            }
            else if( ntk.is_ite( cp_node ) )
            {
              num_mux++;
            }
            else
            {
              assert( false && "only support MAG now" );
            }

            ntk.foreach_fanin( cp_node, [&]( auto const& f ) {
                auto level = ntk.level( ntk.get_node( f ) );
                if ( ntk.is_complemented( f ) )
                {
                  level++;
                }

                if ( level + 1 == ntk.level( cp_node ) )
                {
                  if ( ntk.is_complemented( f ) )
                  {
                  num_inv++;
                  }
                  cp_node = ntk.get_node( f );
                  return false;
                }
                return true;
                } );
          }

          return {num_and, num_inv, num_mux};
        }
      
      private:
      unsigned num_gates{0u}, num_inv{0u}, num_and{0u}, num_mux{0u}, depth{0u}, depth_mixed{0u}, depth_and{0u}, depth_inv{0u}, depth_mux{0u}, num_dangling{0u};
      double area, delay;
  };

  ALICE_ADD_COMMAND( magcost, "Various" )
}

#endif
