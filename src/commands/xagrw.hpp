/* Brief xag_rewriting optimization
 * author: hmtian
 * 2020.4.23
 */



#ifndef XAGRW_HPP
#define XAGRW_HPP

#include <mockturtle/mockturtle.hpp>
#include<mockturtle/networks/xag.hpp>
#include"../core/misc.hpp"
#include"../core/xag_rewriting.hpp"
#include <mockturtle/algorithms/xag_optimization.hpp>
#include <mockturtle/properties/migcost.hpp>
#include <mockturtle/algorithms/equivalence_checking.hpp>
//#include <kitty/print.hpp>

namespace alice
{

  class xagrw_command : public command
  {
    public:
      explicit xagrw_command( const environment::ptr& env ) : command( env, "Performs XAG rewriting" )
      {
        add_option( "strategy, -s", strategy, "aggressive = 0, selective = 1, dfs = 2" );
        add_flag( "--cec, -c,", "apply equivalence checking in rewriting");
        add_flag( "-v,--verbose", "show statistics" );
      }
      
      rules validity_rules() const
      {
        return {
          has_store_element<xag_network>( env ),
            {
              [this]() { return ( strategy <= 2 && strategy >= 0 ) ; },
              "strategy must in [0,2] "
            }
          };
      }

    protected:
      void execute()
      {
        xag_network xag = store<xag_network>().current();
        unsigned num_inv_ori = num_inverters( xag );

        if( strategy == 0 )
        {
          ps_xag.strategy = xag_depth_rewriting_params::aggressive;
        }
        else if( strategy == 1 )
        {
          ps_xag.strategy = xag_depth_rewriting_params::selective;
        }
        else if( strategy == 2 )
        {
          ps_xag.strategy = xag_depth_rewriting_params::dfs;
        }
        else
        {
          assert( false );
        }

        xag_network xag1, xag2;
        xag1 = xag;

        depth_view depth_xag1( xag );
        xag_depth_rewriting( depth_xag1, ps_xag );
        xag = cleanup_dangling( xag );

        xag2 = xag;

        if(is_set( "cec" ))
        {
          /* equivalence checking */
          const auto miter_xag = *miter<xag_network>( xag1, xag2 ); 
          equivalence_checking_stats eq_st;
          const auto result = equivalence_checking( miter_xag, {}, &eq_st );
          assert( *result );
        }

        unsigned num_inv_opt = num_inverters( xag );

        std::cout << "[xagrw] "; 
        also::print_stats( xag );
        //std::cout << "num_inv_ori:" << num_inv_ori <<std::endl;
        //std::cout << "num_inv_opt:" << num_inv_opt << std::endl;

        store<xag_network>().extend(); 
        store<xag_network>().current() = xag;	
      }

    private:
      xag_depth_rewriting_params ps_xag;
      int strategy = 0;
  };

  ALICE_ADD_COMMAND( xagrw, "Rewriting" )
}

#endif
