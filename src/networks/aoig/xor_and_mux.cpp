#include <catch.hpp>

#include <algorithm>
#include <vector>

#include <kitty/algorithm.hpp>
#include <kitty/bit_operations.hpp>
#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operations.hpp>
#include <kitty/operators.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/xor_and_mux.hpp>
#include <mockturtle/traits.hpp>

using namespace mockturtle;

TEST_CASE( "create and use constants in an xor_and_mux", "[xor_and_mux]" )
{
  xor_and_mux_network xor_and_mux;

  CHECK( xor_and_mux.size() == 1 );
  CHECK( has_get_constant_v<xor_and_mux_network> );
  CHECK( has_is_constant_v<xor_and_mux_network> );
  CHECK( has_get_node_v<xor_and_mux_network> );
  CHECK( has_is_complemented_v<xor_and_mux_network> );

  const auto c0 = xor_and_mux.get_constant( false );
  CHECK( xor_and_mux.is_constant( xor_and_mux.get_node( c0 ) ) );
  CHECK( !xor_and_mux.is_pi( xor_and_mux.get_node( c0 ) ) );

  CHECK( xor_and_mux.size() == 1 );
  CHECK( std::is_same_v<std::decay_t<decltype( c0 )>, xor_and_mux_network::signal> );
  CHECK( xor_and_mux.get_node( c0 ) == 0 );
  CHECK( !xor_and_mux.is_complemented( c0 ) );

  const auto c1 = xor_and_mux.get_constant( true );

  CHECK( xor_and_mux.get_node( c1 ) == 0 );
  CHECK( xor_and_mux.is_complemented( c1 ) );

  CHECK( c0 != c1 );
  CHECK( c0 == !c1 );
  CHECK( ( !c0 ) == c1 );
  CHECK( ( !c0 ) != !c1 );
  CHECK( -c0 == c1 );
  CHECK( -c1 == c1 );
  CHECK( c0 == +c1 );
  CHECK( c0 == +c0 );
}

TEST_CASE( "special cases in xor_and_muxs", "[xor_and_mux]" )
{
  xor_and_mux_network xor_and_mux;
  auto x = xor_and_mux.create_pi();

  CHECK( xor_and_mux.create_xor3( xor_and_mux.get_constant( false ), xor_and_mux.get_constant( false ) , xor_and_mux.get_constant( false ) ) == xor_and_mux.get_constant( false ) );
  CHECK( xor_and_mux.create_xor3( xor_and_mux.get_constant( false ), xor_and_mux.get_constant( false ) , xor_and_mux.get_constant( true ) ) == xor_and_mux.get_constant( true ) );
  CHECK( xor_and_mux.create_xor3( xor_and_mux.get_constant( false ), xor_and_mux.get_constant( true ) , xor_and_mux.get_constant( false ) ) == xor_and_mux.get_constant( true ) );
  CHECK( xor_and_mux.create_xor3( xor_and_mux.get_constant( false ), xor_and_mux.get_constant( true ) , xor_and_mux.get_constant( true ) ) == xor_and_mux.get_constant( false ) );
  CHECK( xor_and_mux.create_xor3( xor_and_mux.get_constant( true ), xor_and_mux.get_constant( false ) , xor_and_mux.get_constant( false ) ) == xor_and_mux.get_constant( true ) );
  CHECK( xor_and_mux.create_xor3( xor_and_mux.get_constant( true ), xor_and_mux.get_constant( false ) , xor_and_mux.get_constant( true ) ) == xor_and_mux.get_constant(false ) );
  CHECK( xor_and_mux.create_xor3( xor_and_mux.get_constant( true ), xor_and_mux.get_constant( true ) , xor_and_mux.get_constant( false ) ) == xor_and_mux.get_constant( false ) );
  CHECK( xor_and_mux.create_xor3( xor_and_mux.get_constant( true ), xor_and_mux.get_constant( true ) , xor_and_mux.get_constant( true ) ) == xor_and_mux.get_constant( true ) );

  CHECK( xor_and_mux.create_mux( xor_and_mux.get_constant( false ), xor_and_mux.get_constant( false ), xor_and_mux.get_constant( false ) ) == xor_and_mux.get_constant( false ) );
  CHECK( xor_and_mux.create_mux( xor_and_mux.get_constant( false ), xor_and_mux.get_constant( false ), xor_and_mux.get_constant( true ) ) == xor_and_mux.get_constant( true ) );
  CHECK( xor_and_mux.create_mux( xor_and_mux.get_constant( false ), xor_and_mux.get_constant( true ), xor_and_mux.get_constant( false ) ) == xor_and_mux.get_constant( false ) );
  CHECK( xor_and_mux.create_mux( xor_and_mux.get_constant( false ), xor_and_mux.get_constant( true ), xor_and_mux.get_constant( true ) ) == xor_and_mux.get_constant( true ) );
  CHECK( xor_and_mux.create_mux( xor_and_mux.get_constant( true ), xor_and_mux.get_constant( false ), xor_and_mux.get_constant( false ) ) == xor_and_mux.get_constant( false ) );
  CHECK( xor_and_mux.create_mux( xor_and_mux.get_constant( true ), xor_and_mux.get_constant( false ), xor_and_mux.get_constant( true ) ) == xor_and_mux.get_constant( false ) );
  CHECK( xor_and_mux.create_mux( xor_and_mux.get_constant( true ), xor_and_mux.get_constant( true ), xor_and_mux.get_constant( false ) ) == xor_and_mux.get_constant( true ) );
  CHECK( xor_and_mux.create_mux( xor_and_mux.get_constant( true ), xor_and_mux.get_constant( true ), xor_and_mux.get_constant( true ) ) == xor_and_mux.get_constant( true ) );

  CHECK( xor_and_mux.create_and( xor_and_mux.get_constant( false ), xor_and_mux.get_constant( false ) ) == xor_and_mux.get_constant( false ) );
  CHECK( xor_and_mux.create_and( xor_and_mux.get_constant( false ), xor_and_mux.get_constant( true ) ) == xor_and_mux.get_constant( false ) );
  CHECK( xor_and_mux.create_and( xor_and_mux.get_constant( true ), xor_and_mux.get_constant( false ) ) == xor_and_mux.get_constant( false ) );
  CHECK( xor_and_mux.create_and( xor_and_mux.get_constant( true ), xor_and_mux.get_constant( true ) ) == xor_and_mux.get_constant( true ) );

  CHECK( xor_and_mux.create_and( !x, xor_and_mux.get_constant( false ) ) == xor_and_mux.get_constant( false ) );
  CHECK( xor_and_mux.create_and( !x, xor_and_mux.get_constant( true ) ) == !x );
  CHECK( xor_and_mux.create_and( x, xor_and_mux.get_constant( false ) ) == xor_and_mux.get_constant( false ) );
  CHECK( xor_and_mux.create_and( x, xor_and_mux.get_constant( true ) ) == x );

  CHECK( xor_and_mux.create_xor( x, x ) == xor_and_mux.get_constant( false ) );
  CHECK( xor_and_mux.create_xor( !x, x ) == xor_and_mux.get_constant( true ) );
  CHECK( xor_and_mux.create_xor( x, !x ) == xor_and_mux.get_constant( true ) );
  CHECK( xor_and_mux.create_xor( !x, !x ) == xor_and_mux.get_constant( false ) );

  CHECK( xor_and_mux.create_and( x, x ) == x );
  CHECK( xor_and_mux.create_and( !x, x ) == xor_and_mux.get_constant( false ) );
  CHECK( xor_and_mux.create_and( x, !x ) == xor_and_mux.get_constant( false ) );
  CHECK( xor_and_mux.create_and( !x, !x ) == !x );
}

TEST_CASE( "create and use primary inputs in an xor_and_mux", "[xor_and_mux]" )
{
  xor_and_mux_network xor_and_mux;

  CHECK( has_create_pi_v<xor_and_mux_network> );

  auto a = xor_and_mux.create_pi();

  CHECK( xor_and_mux.size() == 2 );
  CHECK( xor_and_mux.num_pis() == 1 );

  CHECK( std::is_same_v<std::decay_t<decltype( a )>, xor_and_mux_network::signal> );

  CHECK( a.index == 1 );
  CHECK( a.complement == 0 );

  a = !a;

  CHECK( a.index == 1 );
  CHECK( a.complement == 1 );

  a = +a;

  CHECK( a.index == 1 );
  CHECK( a.complement == 0 );

  a = +a;

  CHECK( a.index == 1 );
  CHECK( a.complement == 0 );

  a = -a;

  CHECK( a.index == 1 );
  CHECK( a.complement == 1 );

  a = -a;

  CHECK( a.index == 1 );
  CHECK( a.complement == 1 );

  a = a ^ true;

  CHECK( a.index == 1 );
  CHECK( a.complement == 0 );

  a = a ^ true;

  CHECK( a.index == 1 );
  CHECK( a.complement == 1 );
}

TEST_CASE( "create and use primary outputs in an xor_and_mux", "[xor_and_mux]" )
{
  xor_and_mux_network xor_and_mux;

  CHECK( has_create_po_v<xor_and_mux_network> );

  const auto c0 = xor_and_mux.get_constant( false );
  const auto x1 = xor_and_mux.create_pi();

  CHECK( xor_and_mux.size() == 2 );
  CHECK( xor_and_mux.num_pis() == 1 );
  CHECK( xor_and_mux.num_pos() == 0 );

  xor_and_mux.create_po( c0 );
  xor_and_mux.create_po( x1 );
  xor_and_mux.create_po( !x1 );

  CHECK( xor_and_mux.size() == 2 );
  CHECK( xor_and_mux.num_pos() == 3 );

  xor_and_mux.foreach_po( [&]( auto s, auto i ) {
    switch ( i )
    {
    case 0:
      CHECK( s == c0 );
      break;
    case 1:
      CHECK( s == x1 );
      break;
    case 2:
      CHECK( s == !x1 );
      break;
    }
  } );
}

TEST_CASE( "create unary operations in an xor_and_mux", "[xor_and_mux]" )
{
  xor_and_mux_network xor_and_mux;

  CHECK( has_create_buf_v<xor_and_mux_network> );
  CHECK( has_create_not_v<xor_and_mux_network> );

  auto x1 = xor_and_mux.create_pi();

  CHECK( xor_and_mux.size() == 2 );

  auto f1 = xor_and_mux.create_buf( x1 );
  auto f2 = xor_and_mux.create_not( x1 );

  CHECK( xor_and_mux.size() == 2 );
  CHECK( f1 == x1 );
  CHECK( f2 == !x1 );
}

TEST_CASE( "create binary operations in an xor_and_mux", "[xor_and_mux]" )
{
  xor_and_mux_network xor_and_mux;

  CHECK( has_create_and_v<xor_and_mux_network> );
  CHECK( has_create_nand_v<xor_and_mux_network> );
  CHECK( has_create_or_v<xor_and_mux_network> );
  CHECK( has_create_nor_v<xor_and_mux_network> );
  CHECK( has_create_xor_v<xor_and_mux_network> );
  CHECK( has_create_xnor_v<xor_and_mux_network> );

  const auto x1 = xor_and_mux.create_pi();
  const auto x2 = xor_and_mux.create_pi();

  CHECK( xor_and_mux.size() == 3 );

  const auto f1 = xor_and_mux.create_and( x1, x2 );
  CHECK( xor_and_mux.size() == 4 );

  const auto f2 = xor_and_mux.create_nand( x1, x2 );
  CHECK( xor_and_mux.size() == 4 );
  CHECK( f1 == !f2 );

  const auto f3 = xor_and_mux.create_or( x1, x2 );
  CHECK( xor_and_mux.size() == 5 );

  const auto f4 = xor_and_mux.create_nor( x1, x2 );
  CHECK( xor_and_mux.size() == 5 );
  CHECK( f3 == !f4 );

  const auto f5 = xor_and_mux.create_xor( x1, x2 );
  CHECK( xor_and_mux.size() == 6 );

  const auto f6 = xor_and_mux.create_xnor( x1, x2 );
  CHECK( xor_and_mux.size() == 6 );
  CHECK( f5 == !f6 );
}

TEST_CASE( "hash nodes in xor_and_mux network", "[xor_and_mux]" )
{
  xor_and_mux_network xor_and_mux;

  auto a = xor_and_mux.create_pi();
  auto b = xor_and_mux.create_pi();

  auto f = xor_and_mux.create_and( a, b );
  auto g = xor_and_mux.create_and( a, b );

  CHECK( xor_and_mux.size() == 4u );
  CHECK( xor_and_mux.num_gates() == 1u );

  CHECK( xor_and_mux.get_node( f ) == xor_and_mux.get_node( g ) );
}

TEST_CASE( "clone a node in xor_and_mux network", "[xor_and_mux]" )
{
  xor_and_mux_network xor_and_mux1, xor_and_mux2;

  CHECK( has_clone_node_v<xor_and_mux_network> );

  auto a1 = xor_and_mux1.create_pi();
  auto b1 = xor_and_mux1.create_pi();
  auto f1 = xor_and_mux1.create_and(!a1, !b1 );
  CHECK( xor_and_mux1.size() == 4 );

  auto a2 = xor_and_mux2.create_pi();
  auto b2 = xor_and_mux2.create_pi();
  CHECK( xor_and_mux2.size() == 3 );

  auto f2 = xor_and_mux2.clone_node( xor_and_mux1, xor_and_mux1.get_node( f1 ), {!a2, !b2} );
  CHECK( xor_and_mux2.size() == 4 );

  xor_and_mux2.foreach_fanin( xor_and_mux2.get_node( f2 ), [&]( auto const& s ) {
    CHECK( xor_and_mux2.is_complemented( s )==true );
  } );
}

TEST_CASE( "structural properties of an xor_and_mux", "[xor_and_mux]" )
{
  xor_and_mux_network xor_and_mux;

  CHECK( has_size_v<xor_and_mux_network> );
  CHECK( has_num_pis_v<xor_and_mux_network> );
  CHECK( has_num_pos_v<xor_and_mux_network> );
  CHECK( has_num_gates_v<xor_and_mux_network> );
  CHECK( has_fanin_size_v<xor_and_mux_network> );
  CHECK( has_fanout_size_v<xor_and_mux_network> );

  const auto x1 = xor_and_mux.create_pi();
  const auto x2 = xor_and_mux.create_pi();

  const auto f1 = xor_and_mux.create_and( x1, x2 );
  const auto f2 = xor_and_mux.create_xor( x1, x2 );

  xor_and_mux.create_po( f1 );
  xor_and_mux.create_po( f2 );

  CHECK( xor_and_mux.size() == 5 );
  CHECK( xor_and_mux.is_and( xor_and_mux.get_node( f1 ) ) == true );
  CHECK( xor_and_mux.is_xor( xor_and_mux.get_node( f1 ) ) == false );
  CHECK( xor_and_mux.num_pis() == 2 );
  CHECK( xor_and_mux.num_pos() == 2 );
  CHECK( xor_and_mux.num_gates() == 2 );
  CHECK( xor_and_mux.fanin_size( xor_and_mux.get_node( x1 ) ) == 0 );
  CHECK( xor_and_mux.fanin_size( xor_and_mux.get_node( x2 ) ) == 0 );
  CHECK( xor_and_mux.fanin_size( xor_and_mux.get_node( f1 ) ) == 3 );
  CHECK( xor_and_mux.fanin_size( xor_and_mux.get_node( f2 ) ) == 3 );
  CHECK( xor_and_mux.fanout_size( xor_and_mux.get_node( x1 ) ) == 2 );
  CHECK( xor_and_mux.fanout_size( xor_and_mux.get_node( x2 ) ) == 2 );
  CHECK( xor_and_mux.fanout_size( xor_and_mux.get_node( f1 ) ) == 1 );
  CHECK( xor_and_mux.fanout_size( xor_and_mux.get_node( f2 ) ) == 1 );
}

TEST_CASE( "node and signal iteration in an xor_and_mux", "[xor_and_mux]" )
{
  xor_and_mux_network xor_and_mux;

  CHECK( has_foreach_node_v<xor_and_mux_network> );
  CHECK( has_foreach_pi_v<xor_and_mux_network> );
  CHECK( has_foreach_po_v<xor_and_mux_network> );
  CHECK( has_foreach_gate_v<xor_and_mux_network> );
  CHECK( has_foreach_fanin_v<xor_and_mux_network> );

  const auto x1 = xor_and_mux.create_pi();
  const auto x2 = xor_and_mux.create_pi();
  const auto f1 = xor_and_mux.create_and( x1, x2 );
  const auto f2 = xor_and_mux.create_or( x1, x2 );
  xor_and_mux.create_po( f1 );
  xor_and_mux.create_po( f2 );

  CHECK( xor_and_mux.size() == 5 );

  /* iterate over nodes */
  uint32_t mask{0}, counter{0};
  xor_and_mux.foreach_node( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; } );
  CHECK( mask == 31 );
  CHECK( counter == 10 );

  mask = 0;
  xor_and_mux.foreach_node( [&]( auto n ) { mask |= ( 1 << n ); } );
  CHECK( mask == 31 );

  mask = counter = 0;
  xor_and_mux.foreach_node( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; return false; } );
  CHECK( mask == 1 );
  CHECK( counter == 0 );

  mask = 0;
  xor_and_mux.foreach_node( [&]( auto n ) { mask |= ( 1 << n ); return false; } );
  CHECK( mask == 1 );

  /* iterate over PIs */
  mask = counter = 0;
  xor_and_mux.foreach_pi( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; } );
  CHECK( mask == 6 );
  CHECK( counter == 1 );

  mask = 0;
  xor_and_mux.foreach_pi( [&]( auto n ) { mask |= ( 1 << n ); } );
  CHECK( mask == 6 );

  mask = counter = 0;
  xor_and_mux.foreach_pi( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; return false; } );
  CHECK( mask == 2 );
  CHECK( counter == 0 );

  mask = 0;
  xor_and_mux.foreach_pi( [&]( auto n ) { mask |= ( 1 << n ); return false; } );
  CHECK( mask == 2 );

  /* iterate over POs */
  mask = counter = 0;
  xor_and_mux.foreach_po( [&]( auto s, auto i ) { mask |= ( 1 << xor_and_mux.get_node( s ) ); counter += i; } );
  CHECK( mask == 24 );
  CHECK( counter == 1 );

  mask = 0;
  xor_and_mux.foreach_po( [&]( auto s ) { mask |= ( 1 << xor_and_mux.get_node( s ) ); } );
  CHECK( mask == 24 );

  mask = counter = 0;
  xor_and_mux.foreach_po( [&]( auto s, auto i ) { mask |= ( 1 << xor_and_mux.get_node( s ) ); counter += i; return false; } );
  CHECK( mask == 8 );
  CHECK( counter == 0 );

  mask = 0;
  xor_and_mux.foreach_po( [&]( auto s ) { mask |= ( 1 << xor_and_mux.get_node( s ) ); return false; } );
  CHECK( mask == 8 );

  /* iterate over gates */
  mask = counter = 0;
  xor_and_mux.foreach_gate( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; } );
  CHECK( mask == 24 );
  CHECK( counter == 1 );

  mask = 0;
  xor_and_mux.foreach_gate( [&]( auto n ) { mask |= ( 1 << n ); } );
  CHECK( mask == 24 );

  mask = counter = 0;
  xor_and_mux.foreach_gate( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; return false; } );
  CHECK( mask == 8 );
  CHECK( counter == 0 );

  mask = 0;
  xor_and_mux.foreach_gate( [&]( auto n ) { mask |= ( 1 << n ); return false; } );
  CHECK( mask == 8 );

  /* iterate over fanins */
  mask = counter = 0;
  xor_and_mux.foreach_fanin( xor_and_mux.get_node( f1 ), [&]( auto s, auto i ) { mask |= ( 1 << xor_and_mux.get_node( s ) ); counter += i; } );
  CHECK( mask == 7 );
  CHECK( counter == 3 );

  mask = 0;
  xor_and_mux.foreach_fanin( xor_and_mux.get_node( f1 ), [&]( auto s ) { mask |= ( 1 << xor_and_mux.get_node( s ) ); } );
  CHECK( mask == 7 );

  mask = counter = 0;
  xor_and_mux.foreach_fanin( xor_and_mux.get_node( f1 ), [&]( auto s, auto i ) { mask |= ( 1 << xor_and_mux.get_node( s ) ); counter += i; return false; } );
  CHECK( mask == 4 );
  CHECK( counter == 0 );

  mask = 0;
  xor_and_mux.foreach_fanin( xor_and_mux.get_node( f1 ), [&]( auto s ) { mask |= ( 1 << xor_and_mux.get_node( s ) ); return false; } );
  CHECK( mask == 4 );
}

TEST_CASE( "compute values in xor_and_muxs", "[xor_and_mux]" )
{
  xor_and_mux_network xor_and_mux;

  CHECK( has_compute_v<xor_and_mux_network, bool> );
  CHECK( has_compute_v<xor_and_mux_network, kitty::dynamic_truth_table> );

  const auto x1 = xor_and_mux.create_pi();
  const auto x2 = xor_and_mux.create_pi();
  const auto f1 = xor_and_mux.create_and( !x1, x2 );
  const auto f2 = xor_and_mux.create_and( x1, !x2 );
  xor_and_mux.create_po( f1 );
  xor_and_mux.create_po( f2 );

  std::vector<bool> values{{true, false}};

  CHECK( xor_and_mux.compute( xor_and_mux.get_node( f1 ), values.begin(), values.end() ) == true );
  CHECK( xor_and_mux.compute( xor_and_mux.get_node( f2 ), values.begin(), values.end() ) == false );

  std::vector<kitty::dynamic_truth_table> xs{3, kitty::dynamic_truth_table( 2 )};
  kitty::create_nth_var( xs[0], 0 );
  kitty::create_nth_var( xs[1], 1 );
  kitty::create_nth_var( xs[2], 2 );


  CHECK( xor_and_mux.compute( xor_and_mux.get_node( f2 ), xs.begin(), xs.end() ) == ( ~xs[0] & xs[1] ) );
  CHECK( xor_and_mux.compute( xor_and_mux.get_node( f1 ), xs.begin(), xs.end() ) == ( xs[0] & ~xs[1] ) );
}

TEST_CASE( "custom node values in xor_and_muxs", "[xor_and_mux]" )
{
  xor_and_mux_network xor_and_mux;

  CHECK( has_clear_values_v<xor_and_mux_network> );
  CHECK( has_value_v<xor_and_mux_network> );
  CHECK( has_set_value_v<xor_and_mux_network> );
  CHECK( has_incr_value_v<xor_and_mux_network> );
  CHECK( has_decr_value_v<xor_and_mux_network> );

  const auto x1 = xor_and_mux.create_pi();
  const auto x2 = xor_and_mux.create_pi();
  const auto f1 = xor_and_mux.create_and( x1, x2 );
  const auto f2 = xor_and_mux.create_or( x1, x2 );
  xor_and_mux.create_po( f1 );
  xor_and_mux.create_po( f2 );

  CHECK( xor_and_mux.size() == 5 );

  xor_and_mux.clear_values();
  xor_and_mux.foreach_node( [&]( auto n ) {
    CHECK( xor_and_mux.value( n ) == 0 );
    xor_and_mux.set_value( n, n );
    CHECK( xor_and_mux.value( n ) == n );
    CHECK( xor_and_mux.incr_value( n ) == n );
    CHECK( xor_and_mux.value( n ) == n + 1 );
    CHECK( xor_and_mux.decr_value( n ) == n );
    CHECK( xor_and_mux.value( n ) == n );
  } );
  xor_and_mux.clear_values();
  xor_and_mux.foreach_node( [&]( auto n ) {
    CHECK( xor_and_mux.value( n ) == 0 );
  } );
}

TEST_CASE( "visited values in xor_and_muxs", "[xor_and_mux]" )
{
  xor_and_mux_network xor_and_mux;

  CHECK( has_clear_visited_v<xor_and_mux_network> );
  CHECK( has_visited_v<xor_and_mux_network> );
  CHECK( has_set_visited_v<xor_and_mux_network> );

  const auto x1 = xor_and_mux.create_pi();
  const auto x2 = xor_and_mux.create_pi();
  const auto f1 = xor_and_mux.create_and( x1, x2 );
  const auto f2 = xor_and_mux.create_or( x1, x2 );
  xor_and_mux.create_po( f1 );
  xor_and_mux.create_po( f2 );

  CHECK( xor_and_mux.size() == 5 );

  xor_and_mux.clear_visited();
  xor_and_mux.foreach_node( [&]( auto n ) {
    CHECK( xor_and_mux.visited( n ) == 0 );
    xor_and_mux.set_visited( n, static_cast<uint32_t>( n ) );
    CHECK( xor_and_mux.visited( n ) == static_cast<uint32_t>( n ) );
  } );
  xor_and_mux.clear_visited();
  xor_and_mux.foreach_node( [&]( auto n ) {
    CHECK( xor_and_mux.visited( n ) == 0 );
  } );
}

TEST_CASE( "simulate some special functions in xor_and_muxs", "[xor_and_mux]" )
{
  xor_and_mux_network xor_and_mux;
  const auto x1 = xor_and_mux.create_pi();
  const auto x2 = xor_and_mux.create_pi();
  const auto x3 = xor_and_mux.create_pi();

  const auto f1 = xor_and_mux.create_mux( x1, x2, x3 );
  const auto f2 = xor_and_mux.create_xor3( x1, x2, x3 );

  xor_and_mux.create_po( f1 );
  xor_and_mux.create_po( f2 );

  CHECK( xor_and_mux.num_gates() == 2u );

  auto result = simulate<kitty::dynamic_truth_table>( xor_and_mux, default_simulator<kitty::dynamic_truth_table>( 3 ) );

  CHECK( result[0]._bits[0] == 0xd8u );
  CHECK( result[1]._bits[0] == 0x96u );
}



