#ifndef CREMITER_HPP
#define CREMITER_HPP

#include <mockturtle/mockturtle.hpp>
#include <sys/time.h>
namespace mockturtle
{

template<class NtkDest, class NtkSource1, class NtkSource2>
std::optional<NtkDest> allmiter( NtkSource1 const& ntk1, NtkSource2 const& ntk2 )
{
  static_assert( is_network_type_v<NtkSource1>, "NtkSource1 is not a network type" );
  static_assert( is_network_type_v<NtkSource2>, "NtkSource2 is not a network type" );
  static_assert( is_network_type_v<NtkDest>, "NtkDest is not a network type" );

  static_assert( has_num_pis_v<NtkSource1>, "NtkSource1 does not implement the num_pis method" );
  static_assert( has_num_pos_v<NtkSource1>, "NtkSource1 does not implement the num_pos method" );
  static_assert( has_num_pis_v<NtkSource2>, "NtkSource2 does not implement the num_pis method" );
  static_assert( has_num_pos_v<NtkSource2>, "NtkSource2 does not implement the num_pos method" );
  static_assert( has_create_pi_v<NtkDest>, "NtkDest does not implement the create_pi method" );
  static_assert( has_create_po_v<NtkDest>, "NtkDest does not implement the create_po method" );
  static_assert( has_create_xor_v<NtkDest>, "NtkDest does not implement the create_xor method" );
  static_assert( has_create_nary_or_v<NtkDest>, "NtkDest does not implement the create_nary_or method" );

  /* both networks must have same number of inputs and outputs */
  if ( ( ntk1.num_pis() != ntk2.num_pis() ) || ( ntk1.num_pos() != ntk2.num_pos() ) )
  {
    return std::nullopt;
  }

  /* create primary inputs */
  NtkDest dest;
  std::vector<signal<NtkDest>> pis;
  for ( auto i = 0u; i < ntk1.num_pis(); ++i )
  {
    pis.push_back( dest.create_pi() );
  }

  /* copy networks */
  const auto pos1 = cleanup_dangling( ntk1, dest, pis.begin(), pis.end() );
  const auto pos2 = cleanup_dangling( ntk2, dest, pis.begin(), pis.end() );

  /* create XOR of output pairs */
  std::vector<signal<NtkDest>> xor_outputs;
  std::transform( pos1.begin(), pos1.end(), pos2.begin(), std::back_inserter( xor_outputs ),
                  [&]( auto const& o1, auto const& o2 ) { return dest.create_xor( o1, o2 ); } );
                  
   for (int i = 0 ; i < xor_outputs.size(); i++)
   {
   dest.create_po( xor_outputs[i] );
   }
  
  return dest;
  
}

template<class NtkDest, class NtkSource1, class NtkSource2>
std::optional<NtkDest> pmiter( NtkSource1 const& ntk1, NtkSource2 const& ntk2 )
{
  static_assert( is_network_type_v<NtkSource1>, "NtkSource1 is not a network type" );
  static_assert( is_network_type_v<NtkSource2>, "NtkSource2 is not a network type" );
  static_assert( is_network_type_v<NtkDest>, "NtkDest is not a network type" );

  static_assert( has_num_pis_v<NtkSource1>, "NtkSource1 does not implement the num_pis method" );
  static_assert( has_num_pos_v<NtkSource1>, "NtkSource1 does not implement the num_pos method" );
  static_assert( has_num_pis_v<NtkSource2>, "NtkSource2 does not implement the num_pis method" );
  static_assert( has_num_pos_v<NtkSource2>, "NtkSource2 does not implement the num_pos method" );
  static_assert( has_create_pi_v<NtkDest>, "NtkDest does not implement the create_pi method" );
  static_assert( has_create_po_v<NtkDest>, "NtkDest does not implement the create_po method" );
  static_assert( has_create_xor_v<NtkDest>, "NtkDest does not implement the create_xor method" );
  static_assert( has_create_nary_or_v<NtkDest>, "NtkDest does not implement the create_nary_or method" );

  /* both networks must have same number of inputs and outputs */
  if ( ( ntk1.num_pis() != ntk2.num_pis() ) || ( ntk1.num_pos() != ntk2.num_pos() ) )
  {
    return std::nullopt;
  }

  /* create primary inputs */
  NtkDest dest;
  std::vector<signal<NtkDest>> pis;
  for ( auto i = 0u; i < ntk1.num_pis(); ++i )
  {
    pis.push_back( dest.create_pi() );
  }

  /* copy networks */
  const auto pos1 = cleanup_dangling( ntk1, dest, pis.begin(), pis.end() );
  const auto pos2 = cleanup_dangling( ntk2, dest, pis.begin(), pis.end() );

  /* create XOR of output pairs */
  std::vector<signal<NtkDest>> xor_outputs;
  std::transform( pos1.begin(), pos1.end(), pos2.begin(), std::back_inserter( xor_outputs ),
                  [&]( auto const& o1, auto const& o2 ) { return dest.create_xor( o1, o2 ); } );
  
  dest.create_po( xor_outputs[0] );
  dest = cleanup_dangling(dest);

  return dest;
}

} // namespace mockturtle

namespace alice
{

  class cremiter_command : public command
  {
    public:
      explicit cremiter_command( const environment::ptr& env ) : command( env, "Create a miter, like all miter,partial miter!" )
      {
        add_flag( "-v,--verbose", "show statistics" );
        add_flag( "-l,--all_miter", "create all PO miter" );
        add_flag( "-p,--partial_miter", "create partial PO miter" );
        add_flag( "-g,--miter_for_xag", "create miter for xag network" );
        add_flag( "-a,--miter_for_aig", "create miter for aig network" );
      }

      /*rules validity_rules() const
      {
        return { has_store_element<aig_network>( env ) };
      }*/

    protected:
      void execute()
      {
  timeval start, end;                               //计时开始
  gettimeofday(&start, NULL); 
	/* derive some AIG and make a copy */
	mockturtle::xmg_network xmg = store<xmg_network>().current();
  mockturtle::aig_network aig = store<aig_network>().current();
  
   if( is_set( "all_miter" ) )
   {
   	/* node resynthesis */
	const auto miter  =*mockturtle::allmiter<xmg_network>( aig, xmg );
        store<xmg_network>().extend();
        store<xmg_network>().current() = miter;
        
   }
   
   else if( is_set( "partial_miter" ) )
   {
   	/* node resynthesis */
	const auto miter  =*mockturtle::pmiter<xmg_network>( aig, xmg );
        store<xmg_network>().extend();
        store<xmg_network>().current() = miter;
        
   }
   
   else if( is_set( "miter_for_xag" ) )
   {
   	/* node resynthesis */
	const auto miter  =*mockturtle::miter<xag_network>( aig, xmg );
        store<xag_network>().extend();
        store<xag_network>().current() = miter;
   }

      else if( is_set( "miter_for_aig" ) )
   {
   	/* node resynthesis */
	const auto miter  =*mockturtle::miter<aig_network>( aig, xmg );
        store<aig_network>().extend();
        store<aig_network>().current() = miter;
   }
   
   else
   { 
   const auto miter  =*mockturtle::miter<xmg_network>( aig, xmg );
        store<xmg_network>().extend();
        store<xmg_network>().current() = miter;
        
   }
        
  gettimeofday(&end, NULL); //计时结束
  double timeuse;
  timeuse = end.tv_sec - start.tv_sec + (end.tv_usec - start.tv_usec)/1000000.0;
  std::cout << "The run time is: "<< setiosflags(ios::fixed) << setprecision(4)<< timeuse<<" s"<< endl;
      }

    private:
  };

  ALICE_ADD_COMMAND( cremiter, "Optimization" )
}

#endif

