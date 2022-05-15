#ifndef CREMITER_HPP
#define CREMITER_HPP

#include <mockturtle/algorithms/miter.hpp>
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
    add_option( "-i, --po_index", po_index, "specify the po index" );
  }

  /*rules validity_rules() const
      {
        return { has_store_element<aig_network>( env ) };
      }*/

protected:
  void execute()
  {
    timeval start, end; 
    gettimeofday( &start, NULL );

    /* derive some AIG and make a copy */
    mockturtle::xmg_network xmg = store<xmg_network>().current();
    mockturtle::aig_network aig = store<aig_network>().current();

    if ( is_set( "partial_miter" ) )
    {
      /* node resynthesis */
      const auto miter = *mockturtle::pmiter<xmg_network>( aig, xmg, 0 );
      store<xmg_network>().extend();
      store<xmg_network>().current() = miter;
    }
    else if ( is_set( "miter_for_xag" ) )
    {
      /* node resynthesis */
      const auto miter = *mockturtle::miter<xag_network>( aig, xmg );
      store<xag_network>().extend();
      store<xag_network>().current() = miter;
    }
    else if ( is_set( "miter_for_aig" ) )
    {
      /* node resynthesis */
      const auto miter = *mockturtle::miter<aig_network>( aig, xmg );
      store<aig_network>().extend();
      store<aig_network>().current() = miter;
    }
    else
    {
      const auto miter = *mockturtle::miter<xmg_network>( aig, xmg );
      store<xmg_network>().extend();
      store<xmg_network>().current() = miter;
    }

    gettimeofday( &end, NULL ); 
    double timeuse;
    timeuse = end.tv_sec - start.tv_sec + ( end.tv_usec - start.tv_usec ) / 1000000.0;
    std::cout << "The run time is: " << setiosflags( ios::fixed ) << setprecision( 4 ) << timeuse << " s" << endl;
  }

private:
  int po_index = 0;
};

ALICE_ADD_COMMAND( cremiter, "Optimization" )
} // namespace alice

#endif
