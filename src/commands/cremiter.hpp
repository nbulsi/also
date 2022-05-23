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
    add_flag( "-g,--miter_for_xag", "create miter for xag network" );
    add_flag( "-a,--miter_for_aig", "create miter for aig network" );
    add_flag( "-e,--enable_ec", "enable equivalence checking" );
    add_flag( "-n,--new_entry", "save new miter network to the store" );
  }

  rules validity_rules() const
  {
    return { has_store_element<aig_network>( env ) };
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

    if ( is_set( "partial_miter" ) )
    {
      mockturtle::call_with_stopwatch( time, [&]() {
            for( auto i = 0; i < xmg.num_pos(); i++ )
            {
                mockturtle::stopwatch<>::duration iteration_time{0};
                
                mockturtle::call_with_stopwatch( iteration_time, [&]() {
                const auto miter = *mockturtle::pmiter<xmg_network>( aig, xmg, i );
                if( is_set( "enable_ec" ) ) {
                result = mockturtle::equivalence_checking( miter ); }
                } );
                
                if( is_set( "verbose" ) ) {
                std::cout << fmt::format( "[Iteration time for {}]: {:5.4f} seconds\n", 
                    i, mockturtle::to_seconds( iteration_time ) ); }
            }
          } );
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
};

ALICE_ADD_COMMAND( cremiter, "Optimization" )
} // namespace alice

#endif
