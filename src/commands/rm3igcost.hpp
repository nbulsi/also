/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file xmgcost.hpp
 *
 * @brief The nanotechnology implemention cost of XMGs
 * @modified from https://github.com/msoeken/cirkit/blob/cirkit3/cli/algorithms/migcost.hpp
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef RM3IGCOST_HPP
#define RM3IGCOST_HPP

#include <mockturtle/mockturtle.hpp>
#include <mockturtle/properties/migcost.hpp>

namespace alice
{

class rm3igcost_command : public command
{
public:
  explicit rm3igcost_command( const environment::ptr& env ) : command( env, "rm3ig cost evaluation using rm3" )
  {
    add_flag( "--only_rm3", "return the cost of rm3" );
  }

  rules validity_rules() const
  {
    return { has_store_element<rm3_network>( env ) };
  }

protected:
  void execute()
  {
    rm3_network rm3ig = store<rm3_network>().current();

    num_rm3 = 0;

    /* compute num_rm3*/
    rm3ig.foreach_gate( [&]( auto n )
                      {
              if( rm3ig.is_rm3( n ) )
              {
                num_rm3++;
              }
              else
              {
                assert( false && "only support RM3IG now" );
              } } );

    num_gates = rm3ig.num_gates();
    num_inv = mockturtle::num_inverters( rm3ig );

    depth_view_params ps;
    ps.count_complements = false;
    mockturtle::depth_view depth_rm3ig{ rm3ig, {}, ps };
    depth = depth_rm3ig.depth();

    ps.count_complements = true;
    mockturtle::depth_view depth_rm3ig2{ rm3ig, {}, ps };
    depth_mixed = depth_rm3ig2.depth();
    std::tie( depth_rm3, depth_inv) = split_critical_path( depth_rm3ig2 );

    num_dangling = mockturtle::num_dangling_inputs( rm3ig );

    env->out() << fmt::format( "[i] Gates             = {}\n"
                               "[i] Num RM3s          = {}\n"
                               "[i] Inverters         = {}\n"
                               "[i] Depth (def.)      = {}\n"
                               "[i] Depth mixed       = {}\n"
                               "[i] Depth mixed (RM3) = {}\n"
                               "[i] Depth mixed (INV) = {}\n",
                               num_gates, num_rm3, num_inv, depth, depth_mixed, depth_rm3, depth_inv);

  }

private:
  template<class Ntk>
  std::tuple<unsigned, unsigned> split_critical_path( Ntk const& ntk )
  {
    using namespace mockturtle;

    unsigned num_rm3{ 0 }, num_inv{ 0 };

    node<Ntk> cp_node;
    ntk.foreach_po( [&]( auto const& f )
                    {
              auto level = ntk.level( ntk.get_node( f ) );
              if ( ntk.is_complemented( f ) )
              {
                level++;
              }
                
              if ( level == ntk.depth() )
              {
                if ( ntk.is_complemented( f ) )
                {
                ++num_inv;
                }
                cp_node = ntk.get_node( f );
                return false;
              }

              return true; } );

    while ( !ntk.is_constant( cp_node ) && !ntk.is_pi( cp_node ) )
    {
      if ( ntk.is_rm3( cp_node ) )
      {
        num_rm3++;
      }
      else
      {
        assert( false && "only support RM3IG now" );
      }

      ntk.foreach_fanin( cp_node, [&]( auto const& f )
                         {
                auto level = ntk.level( ntk.get_node( f ) );
                if ( ntk.is_complemented( f ) )
                {
                  level++;
                }

                if ( level + 1 == ntk.level( cp_node ) )
                {
                  if ( ntk.is_complemented( f ) )
                  {
                  num_inv++;
                  }
                  cp_node = ntk.get_node( f );
                  return false;
                }
                return true; } );
    }

    return { num_rm3, num_inv};
  }

private:
  unsigned num_gates{ 0u }, num_inv{ 0u }, num_rm3{ 0u }, depth{ 0u }, depth_mixed{ 0u }, depth_rm3{ 0u }, depth_inv{ 0u }, num_dangling{ 0u };
};

ALICE_ADD_COMMAND( rm3igcost, "Various" )
} // namespace alice

#endif
