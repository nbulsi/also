

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
		  int nr=4;
		  std::cout<<"just test"<<std::endl;
		  system("./also -c \"stochastic -f vector.txt;\" > tempres.txt");
		  system("./abc -c \"read_pla temp.pla;print_kmap;write_pla -M nr test.pla;\" > tempres.txt");
      }
  };

  ALICE_ADD_COMMAND( test, "Mapping" )
}

#endif


