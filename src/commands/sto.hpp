//copyright (C) 2019- Ningbo University, Ningbo, China */

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
#include<string>
#include "../core/exact_sto_m3ig.hpp"


namespace alice
{

     void execut(variableNumber, tm, tn, vector);
     void execut(variableNumber, tm, tn, vector)
      {
       

          stopwatch<>::duration time{0};
          mig_network mig;
          call_with_stopwatch( time, [&]() 
              {
              mig = stochastic_synthesis( variableNumber, tm, tn, vector );
              } );
          
          //store<mig_network>().extend(); 
          //store<mig_network>().current() = mig;
	
	  default_simulator<kitty::dynamic_truth_table> sim( m+n );
 	 const auto tt = simulate<kitty::dynamic_truth_table>( mig, sim )[0];
	// const auto tt = simulate<kitty::static_truth_table<3u>>( mig )[0];
	kitty::print_binary(tt, std::cout);
	//string str=tt;
	std::cout <<std::endl;
	std::cout <<"tt: 0x"<< kitty::to_hex(tt ) << std::endl; 
	//std::cout <<  kitty::to_hex(tt) << std::endl;			
	//std::cout<<store<mig_network>().current()<<std::endl;

          std::cout << fmt::format( "[time]: {:5.2f} seconds\n", to_seconds( time ) );
   
    
  };

}

#endif
