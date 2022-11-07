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
#include <fstream>
#include <mockturtle/mockturtle.hpp>
#include <mockturtle/properties/migcost.hpp>

namespace alice
{

  class magcost_command : public command
  {
    public:
      explicit magcost_command( const environment::ptr& env ) : command( env, "MAG cost evaluation " )
      {
        add_option( "Library Name, -l", filename, "Specify a different library, default: area (Mux:2.5 And:1 Not:0.1) delay (Mux:2 And:1 Not:0.1)" );
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

        if ( is_set( "-l" ) )
           node_fn.parser_csv( filename );

        mockturtle::depth_view depth_mag2{ mag, node_fn, ps };
        
        depth_mixed = 1.0 * depth_mag2.depth() / node_fn.times;
        std::tie( depth_and, depth_inv, depth_mux ) = split_critical_path( depth_mag2 );

        num_dangling = mockturtle::num_dangling_inputs( mag );

        //cost function
        area = num_and * node_fn.and_area + num_inv * node_fn.not_area + num_mux * node_fn.mux_area;
        delay = depth_and * node_fn.and_delay + depth_inv * node_fn.not_delay + depth_mux * node_fn.mux_delay;

        env->out() << fmt::format( "[i] Gates             = {}\n"
            "[i] Num ANDs          = {}\n"
            "[i] Num MUXes         = {}\n"
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
         struct node_cost
      {
        node_cost() : mux_area( 2.5 ), and_area( 1 ), not_area( 0.1 ), mux_delay( 2 ), and_delay( 1 ), not_delay( 0.1 )
        {
          times = int( 1 / not_delay );
        }

        double operator()( Ntk const& ntk, node<Ntk> const& node ) const
        {
          unsigned cost = 0;
          if ( ntk.is_and( node ) )
          {
            cost += int( and_delay * times );
          }
          else if ( ntk.is_ite( node ) )
          {
            cost += int( mux_delay * times );
          }
          else
          { 
            assert( false && "only support MAG now" );
          }
         return cost;
        }

        void parser_csv( const string& csv )
      {
         ifstream is( csv );
         int row = 0;
         for ( string line; getline( is, line, '\n' ); )
        {
          row++;
          if ( row == 1 )
            continue;
          std::vector<string> temp( 3, "" );
          int j = 0;
          for ( unsigned int i = 0; i < line.size(); i++ )
          {
            if ( line[i] != ',' )
            temp[j] += line[i];
            else
            {
               j++;
            }
          }

        for ( unsigned int i = 1; i < temp.size(); i++ )
        {
          for ( unsigned int j = 0; j < temp[i].size(); )
          {
            if ( temp[i][j] == ' ' )
              temp[i].erase( temp[i].begin() + j );
            else
              j++;
          }
        }

        if ( row == 2 )
        {
          mux_area = std::stold( temp[1] );
          mux_delay = std::stold( temp[2] );
        }
        else if ( row == 3 )
        {
          and_area = std::stold( temp[1] );
          and_delay = std::stold( temp[2] );
        }
        else
        {
          not_area = std::stold( temp[1] );
          not_delay = std::stold( temp[2] );
        }
      }
      times = int( 1 / not_delay );
      std::cout<<mux_area<< " "<<mux_delay<<" "<<and_area<<" "<<and_delay<<" "<<not_area<<" "<<not_delay<<std::endl;
    }
  
    
        double mux_area;
        double and_area;
        double not_area;

        double mux_delay;
        double and_delay;
        double not_delay;
        int times;
  };

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
            int node_level = 0;
            if( ntk.is_and( cp_node ) )
            {
              num_and++;
              node_level = int(node_fn.and_delay * node_fn.times);;
            }
            else if( ntk.is_ite( cp_node ) )
            {
              num_mux++;
              node_level = int(node_fn.and_delay * node_fn.times);;
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

                if ( level + node_level == ntk.level( cp_node ) )
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
      unsigned num_gates{0u}, num_inv{0u}, num_and{0u}, num_mux{0u}, depth{0u}, depth_and{0u}, depth_inv{0u}, depth_mux{0u}, num_dangling{0u};
      double area, delay;
      double depth_mixed;
      std::string filename;
      node_cost<mag_network> node_fn;
  };

  ALICE_ADD_COMMAND( magcost, "Various" )
}

#endif
