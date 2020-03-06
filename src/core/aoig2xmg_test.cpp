#include <catch.hpp>
#include<set>
#include<mockturtle/networks/aoig.hpp>
#include<mockturtle/networks/xmg.hpp>
#include<mockturtle/algorithms/aoig2xmg.hpp>
using namespace mockturtle;
TEST_CASE( "aoig convert to xmg for xor function", "[aoig2xmg]" )
{
  aoig_network aoig;
  const auto x1 = aoig.create_pi();
  const auto x2 = aoig.create_pi();
  CHECK( aoig.size() == 4 );

  const auto f = aoig.create_xor( x1, x2 );
  aoig.create_po( f );

  std::set<node<aoig_network>> nodes;
  aoig.foreach_node( [&nodes]( auto node ) { nodes.insert( node ); } );
  CHECK( nodes.size() == 5 );

  xmg_network xmg;
  const auto x3 = xmg.create_pi();
  const auto x4 = xmg.create_pi();
  CHECK( xmg.size() == 3 );

  const auto f1 = xmg.create_xor( x3, x4 );
  xmg.create_po( f1 );
  CHECK( xmg.size() == 4 );

  CHECK( xmg_from_aoig( aoig ).size() == 4 );

  xmg_network xmg1;
  xmg1 = xmg_from_aoig( aoig );

  std::set<node<xmg_network>> nodes1;
  xmg1.foreach_node( [&nodes1]( auto node ) { nodes1.insert( node ); } );
  CHECK( nodes1.size() == 4);
}

TEST_CASE( "aoig convert to xmg for maj function", "[aoig2xmg]" )
{
  aoig_network aoig;
  const auto x1 = aoig.create_pi();
  const auto x2 = aoig.create_pi();
  const auto x3 = aoig.create_pi();
  CHECK( aoig.size() == 5 );

  const auto f = aoig.create_mux( x1, x2,x3 );
  aoig.create_po( f );
  CHECK( aoig.size() == 6 );

  xmg_network xmg;
  const auto x4 = xmg.create_pi();
  const auto x5 = xmg.create_pi();
  const auto x6 = xmg.create_pi();
  CHECK( xmg.size() == 4 );

  const auto f1 = xmg.create_ite( x4, x5,x6 );
  xmg.create_po( f1 );
  CHECK( xmg.size() == 7);

  xmg_network xmg1;
  xmg1 = xmg_from_aoig( aoig );
  CHECK( xmg1.size() == 7 );

}
