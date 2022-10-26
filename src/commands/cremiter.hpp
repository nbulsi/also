#ifndef CREMITER_HPP
#define CREMITER_HPP

#include <mockturtle/algorithms/miter.hpp>
#include <mockturtle/algorithms/equivalence_checking.hpp>
#include <mockturtle/utils/stopwatch.hpp>
#include <sys/time.h>

namespace alice
{

class cremiter_command : public command
{
public:
  explicit cremiter_command( const environment::ptr& env ) : command( env, "Create a miter for two combinational circuits." )
  {
    add_flag( "-v,--verbose", "show statistics" );
    add_flag( "-p,--partial_miter", "create partial PO miter" );
    add_flag( "-g,--miter_for_xag", "create miter for aig/xag network" );
    add_flag( "-a,--miter_for_aig", "create miter for aig/aig network" );
    add_flag( "-x,--miter_for_xmg", "create miter for xmg/xmg network" );
    add_flag( "-e,--enable_ec", "enable equivalence checking" );
    add_flag( "-n,--new_entry", "save new miter network to the store" );
    add_option( "-l,--conflict_limit", conflict_limit, "conflict limit for SAT solver");
  }

protected:
  void execute()
  {
    /* derive some AIG and make a copy */
    mockturtle::xmg_network xmg = store<xmg_network>().current();
    mockturtle::aig_network aig = store<aig_network>().current();

    /* equivalence checking results */
    std::optional<bool> result;
    mockturtle::stopwatch<>::duration time{0};
    mockturtle::xmg_network xmg_ref;
    mockturtle::xmg_network xmg_impl;
    
    if( is_set( "miter_for_xmg" ) )
    {
      std::cout << "[warning] Most recent two XMGs will be derived\n";
      auto xmg_store_size = store<xmg_network>().size();
      assert( xmg_store_size >= 2u );
      xmg_impl = store<xmg_network>()[ xmg_store_size - 1 ];
      xmg_ref  = store<xmg_network>()[ xmg_store_size - 2 ];
    }

    if ( is_set( "partial_miter" ) )
    {
      unsigned success = 0u;
      unsigned fail = 0u;
      unsigned unknown = 0u;
      std::vector<unsigned> unknown_po_list;

      mockturtle::xmg_network xmg_miter;

      mockturtle::call_with_stopwatch( time, [&]() {
            
            if( is_set( "verbose" ) )
            {
              std::cout << "[i] Begin the equivalence checking process for " << xmg.num_pos() << " POs\n";
            }

            for( auto i = 0; i < xmg.num_pos(); i++ )
            {
                mockturtle::stopwatch<>::duration iteration_time{0};
                
                mockturtle::call_with_stopwatch( iteration_time, [&]() {
                if( is_set( "miter_for_xmg" ) )
                {
                  xmg_miter = *mockturtle::pmiter<xmg_network>( xmg_ref, xmg_impl, i );
                }
                else
                {
                  xmg_miter = *mockturtle::pmiter<xmg_network>( aig, xmg, i );
                }

                if( is_set( "verbose" ) )
                {
                  std::cout << fmt::format( "[i] {:6d} gates in miter for PO {:4d}, ", xmg_miter.num_gates(), i );
                }

                if( is_set( "enable_ec" ) ) 
                {
                  equivalence_checking_params ps;
                  equivalence_checking_stats st;
                  
                  ps.conflict_limit = conflict_limit;

                  result = mockturtle::equivalence_checking( xmg_miter, ps, &st );

                  if( result == std::nullopt )
                  {
                    unknown++;
                    unknown_po_list.push_back( i );
                    if( is_set( "verbose" ) ) 
                    {
                      std::cout << "UNKNOWN, ";
                    }
                  }
                  else
                  {
                    if( *result )
                    {
                      success++;
                      if( is_set( "verbose" ) ) 
                      {
                        std::cout << "EQU, "; 
                      }
                    }
                    else
                    {
                      fail++;
                      if( is_set( "verbose" ) ) 
                      {
                        std::cout << "NEQ, " << std::endl;
                      }
                      return;
                    }
                  }
                }
                } );
                
                if( is_set( "verbose" ) ) 
                {
                  std::cout << fmt::format( "iteration time: {:5.4f} seconds\n", mockturtle::to_seconds( iteration_time ) ); 
                }
            }
          } );
      
          std::cout << fmt::format( "[Equivalence Checking:] There are {} POs, {} success / {} unknown / {} failure.\n", xmg.num_pos(), success, unknown, fail );

          if( unknown_po_list.size() > 0u )
          {
            mockturtle::xmg_network new_miter;
            
            if( is_set( "miter_for_xmg" ) )
            {
              new_miter = *mockturtle::pmiter_w_po_list<xmg_network>( xmg_ref, xmg_impl, unknown_po_list );
            }
            else
            {
              new_miter = *mockturtle::pmiter_w_po_list<xmg_network>( aig, xmg, unknown_po_list );
            }
            
            store<xmg_network>().extend();
            store<xmg_network>().current() = new_miter;
          }
    }
    else if ( is_set( "miter_for_xag" ) )
    {
      /* node resynthesis */
      mockturtle::call_with_stopwatch( time, [&]() {
      const auto miter = *mockturtle::miter<xag_network>( aig, xmg );
      if( is_set( "enable_ec" ) ) {
      result = mockturtle::equivalence_checking( miter ); }
          
          if( is_set( "new_entry" ) )
          {
            store<xag_network>().extend();
            store<xag_network>().current() = miter;
          }
      } );
    }
    else if ( is_set( "miter_for_xmg" ) )
    {
      mockturtle::call_with_stopwatch( time, [&]() {

      const auto miter = *mockturtle::miter<xmg_network>( xmg_ref, xmg_impl );

      if( is_set( "enable_ec" ) ) {
      result = mockturtle::equivalence_checking( miter ); }
          
      if( is_set( "new_entry" ) )
      {
        store<xmg_network>().extend();
        store<xmg_network>().current() = miter;
      }
      } );
    }
    else if ( is_set( "miter_for_aig" ) )
    {
      /* node resynthesis */
      mockturtle::call_with_stopwatch( time, [&]() {
      const auto miter = *mockturtle::miter<aig_network>( aig, xmg );
      
      if( is_set( "enable_ec" ) ) {
      result = mockturtle::equivalence_checking( miter ); }
          
          if( is_set( "new_entry" ) )
          {
            store<aig_network>().extend();
            store<aig_network>().current() = miter;
          }
      } );
    }
    else
    {
      mockturtle::call_with_stopwatch( time, [&]() {
          const auto miter = *mockturtle::miter<xmg_network>( aig, xmg );
          
          if( is_set( "enable_ec" ) ) {
          result = mockturtle::equivalence_checking( miter ); }
          
          if( is_set( "new_entry" ) )
          {
            store<xmg_network>().extend();
            store<xmg_network>().current() = miter;
          }
      } );

    }
    
    std::cout << fmt::format( "[Total cremiter time]: {:5.4f} seconds\n", mockturtle::to_seconds( time ) );
  }

private:
  unsigned conflict_limit = 0u;
};

ALICE_ADD_COMMAND( cremiter, "Optimization" )
} // namespace alice

#endif
