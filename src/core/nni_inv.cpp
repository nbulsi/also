/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2020- Ningbo University, Ningbo, China */
// hmtian

#include <mockturtle/mockturtle.hpp>
#include <mockturtle/properties/migcost.hpp>
#include <mockturtle/utils/stopwatch.hpp>
#include <mockturtle/networks/mig.hpp>

using namespace mockturtle;

namespace also
{
  using ntk   = mig_network;
  using nd_t  = node<ntk>;

  class nniinv_manager
  {
    public:
    nniinv_manager( ntk mig );

    int num_invs_fanins( nd_t n );
    uint32_t mig_nni_inv_opt( ntk const& mig );
    int num_nni_incr( ntk const& mig );

    void mig_inv_optimization( mig_network& mig );
    void run();

    private:
    mig_network mig;
    unsigned num_inv_origin{0u}, num_inv_opt{0u}, num_nni_inc{0u};
  };

  nniinv_manager::nniinv_manager( ntk mig )
    : mig( mig )
  {
  }

/********************************************Functions used begin***************************************************************/
  /* Compute the total bumber of INV of all fanins of a node*/
  int nniinv_manager::num_invs_fanins( nd_t n ) 
  {
    int cost = 0;

    mig.foreach_fanin( n, [&]( auto s ) { if( mig.is_complemented( s ) ) { cost++; } } );
    return cost;
  }
/**************************************************************************************************************/
  /* Compute the inverters which can be optimized */
  uint32_t nniinv_manager::mig_nni_inv_opt( ntk const& mig ) 
  {
    int num1_inv = 0u;
    int num2_inv = 0u;
    int num3_inv = 0u;

//    auto tmp = num_invs_fanins( n );

    mig.foreach_gate( [&]( auto n ) 
        {
          auto tmp = num_invs_fanins( n );

          if( tmp == 1 )
          {
            num1_inv++;
          }
          else if( tmp == 2 )
          {
            num2_inv += 2;
          }
	  else if ( tmp == 3 )
          {
            num3_inv += 2;
          }
        }
        );

    int opt_inv = num1_inv + num2_inv + num3_inv;
    
    return opt_inv;
  }
/**************************************************************************************************************/
  /* The number of NNI gates increased to substitute MAJ, for eliminating INVs. */  
  int nniinv_manager::num_nni_incr( ntk const& mig ) 
  {
    int cost = 0;

    mig.foreach_gate( [&]( auto n ) 
        {
          auto tmp = num_invs_fanins( n );

          if( tmp >= 1 )
          {
            cost++;
          }
        }
        );
    
    return cost;
  }

/**************************************************************************************************************/ 
  void nniinv_manager::run()
  {
    num_inv_origin = num_inverters( mig );  
    num_inv_opt = mig_nni_inv_opt( mig );
    num_nni_inc = num_nni_incr( mig );

    std::cout << "[mig_nni_inv] " << " num_inv_origin: " << num_inv_origin << " num_inv_opt: " << num_inv_opt << std::endl;
    std::cout << "nni gate need to increase or maj reduced:"<< num_nni_inc <<std::endl; 
  }
/**************************************************************************************************************/   
  /* public function */
  void mig_inv_optimization( mig_network& mig )
  {
    nniinv_manager mgr( mig );
    mgr.run();
  }
/********************************************Functions used end*****************************************************************/
}
