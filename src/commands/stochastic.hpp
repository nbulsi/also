/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file stochastic.hpp
 *
 * @brief stochastic circuit synthesis
 *
 * @author Zhufei
 * @since  0.1
 */

#ifndef STOCHASTIC_HPP
#define STOCHASTIC_HPP

#include <fstream>
#include "../core/exact_sto_m3ig.hpp"

namespace alice
{

  class stochastic_command : public command
  {
    public:
      explicit stochastic_command( const environment::ptr& env ) : command( env, "stochastic circuit synthesis" )
      {
        add_option( "filename, -f", filename, "the input txt file name" );
        add_flag( "--verbose, -v", "verbose output" );
      }
    
    protected:
      void execute()
      {
        std::string line;
        std::ifstream myfile( filename );

        if( myfile.is_open() )
        {
          unsigned line_index = 0u;
          vector.clear();

          while( std::getline( myfile, line ) )
          {
            if( line_index == 0u ) { num_vars = std::stoi( line ); } 
            if( line_index == 1u ) { m = std::stoi( line ); } 
            if( line_index == 2u ) { n = std::stoi( line ); } 

            if( line_index == 3u ) 
            {
              std::stringstream ss( line );

              unsigned tmp;

              while( ss >> tmp )
              {
                vector.push_back( tmp );
              }
            }
            line_index++;
          }

          myfile.close();
          
            std::cout << " num_vars : " << num_vars << "\n" 
                      << " m        : " << m << "\n" 
                      << " n        : " << n << "\n";

            std::cout << " Problem vector: ";
            for( auto const& e : vector )
            {
              std::cout << e << " ";
            }
            std::cout << std::endl;
          

          if( is_set( "verbose" ) )
          {
            std::cout << "[i] num_vars : " << num_vars << "\n" 
                      << "[i] m        : " << m << "\n" 
                      << "[i] n        : " << n << "\n";

            std::cout << "[i] Problem vector: ";
            for( auto const& e : vector )
            {
              std::cout << e << " ";
            }
            std::cout << std::endl;
          }

          stopwatch<>::duration time{0};
          mig_network mig;
          call_with_stopwatch( time, [&]() 
              {
              mig = stochastic_synthesis( num_vars, m, n, vector );
              } );
          
          store<mig_network>().extend(); 
          store<mig_network>().current() = mig;
	
	  default_simulator<kitty::dynamic_truth_table> sim( m+n );
 	 const auto tt = simulate<kitty::dynamic_truth_table>( mig, sim )[0];
	// const auto tt = simulate<kitty::static_truth_table<3u>>( mig )[0];
	//kitty::print_binary(tt, std::cout);
	//std::cout <<std::endl;
	std::cout <<"tt: 0x"<< kitty::to_hex(tt ) << std::endl; 
	//std::cout <<  kitty::to_hex(tt) << std::endl;			
	//std::cout<<store<mig_network>().current()<<std::endl;

          std::cout << fmt::format( "[time]: {:5.2f} seconds\n", to_seconds( time ) );
        }
        else
        {
          std::cerr << "Cannot open input file \n"; 
        }
      }

    private:
      std::string filename = "vector.txt";

      unsigned num_vars; //the number of variables
      unsigned n; //the highest power
      unsigned m; //the number control the accuracy 
      std::vector<unsigned> vector;
  };

  ALICE_ADD_COMMAND( stochastic, "Various" )
}

#endif
