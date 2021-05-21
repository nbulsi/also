

#ifndef test_HPP
#define test_HPP

#include <mockturtle/mockturtle.hpp>
#include<stdlib.h>
#include <sstream>
//#include "../core/aig2xmg.hpp"

namespace alice
{
  class test_command : public command
  {
    public:
      explicit test_command( const environment::ptr& env ) : command( env, "" )
      {
      
      }
    
    protected:
      void execute()
      {
		  std::cout<<"just test"<<std::endl;
		  system("./also -c \"stochastic -f vector.txt;\" > tempres.txt");
      }
  };

  ALICE_ADD_COMMAND( test, "Mapping" )
}

#endif


