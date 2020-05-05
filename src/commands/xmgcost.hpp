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

#ifndef XMGCOST_HPP
#define XMGCOST_HPP

#include <mockturtle/mockturtle.hpp>
#include <mockturtle/properties/migcost.hpp>

namespace alice
{

  class xmgcost_command : public command
  {
    public:
      explicit xmgcost_command( const environment::ptr& env ) : command( env, "XMG cost evaluation using nanotechnologies" )
      {
        add_flag( "--only_qca", "return the cost of QCA" );
      }

      rules validity_rules() const
      {
        return{ has_store_element<xmg_network>( env ) };
      }

      protected:
      void execute()
      {
        xmg_network xmg = store<xmg_network>().current();
        
        num_maj = 0;
        num_xor = 0;
        /* compute num_maj and num_xor */
        xmg.foreach_gate( [&]( auto n ) 
            {
              if( xmg.is_maj( n ) )
              {
                num_maj++;
              }
              else if( xmg.is_xor3( n ) )
              {
                num_xor++;
              }
              else
              {
                assert( false && "only support XMG now" );
              }
            }
            );

        num_gates = xmg.num_gates();
        num_inv = mockturtle::num_inverters( xmg );
        
        depth_view_params ps;
        ps.count_complements = false;
        mockturtle::depth_view depth_xmg{xmg, {}, ps};
        depth = depth_xmg.depth();

        ps.count_complements = true;
        mockturtle::depth_view depth_xmg2{xmg,{}, ps};
        depth_mixed = depth_xmg2.depth();
        std::tie( depth_maj, depth_inv, depth_xor ) = split_critical_path( depth_xmg2 );

        num_dangling = mockturtle::num_dangling_inputs( xmg );

        qca_area = num_maj * 0.0012 + num_inv * 0.004 + num_xor * 0.0027;
        qca_delay = depth_maj * 0.004 + depth_inv * 0.014 + depth_xor * 0.009;
        qca_energy = num_maj * 2.94 + num_inv * 9.8 + num_xor * 6.615;

        env->out() << fmt::format( "[i] Gates             = {}\n"
            "[i] Num MAJs          = {}\n"
            "[i] Num XORs          = {}\n"
            "[i] Inverters         = {}\n"
            "[i] Depth (def.)      = {}\n"
            "[i] Depth mixed       = {}\n"
            "[i] Depth mixed (MAJ) = {}\n"
            "[i] Depth mixed (INV) = {}\n"
            "[i] Depth mixed (XOR) = {}\n"
            "[i] Dangling inputs   = {}\n",
            num_gates, num_maj, num_xor, num_inv, depth, depth_mixed, depth_maj, depth_inv, depth_xor, num_dangling );

        env->out() << fmt::format( "[i] QCA (area)        = {:.2f} um^2\n"
            "[i] QCA (delay)       = {:.2f} ns\n"
            "[i] QCA (energy)      = {:.2f} E-21 J\n",
                                 qca_area, qca_delay, qca_energy );
      }

      private:
      template<class Ntk>
        std::tuple<unsigned, unsigned, unsigned> split_critical_path( Ntk const& ntk )
        {
          using namespace mockturtle;

          unsigned num_maj{0}, num_inv{0}, num_xor{0};

          node<Ntk> cp_node;
          ntk.foreach_po( [&]( auto const& f ) {
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

              return true;
              } );


          while ( !ntk.is_constant( cp_node ) && !ntk.is_pi( cp_node ) )
          {
            if( ntk.is_maj( cp_node ) )
            {
              num_maj++;
            }
            else if( ntk.is_xor3( cp_node ) )
            {
              num_xor++;
            }
            else
            {
              assert( false && "only support XMG now" );
            }

            ntk.foreach_fanin( cp_node, [&]( auto const& f ) {
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
                return true;
                } );
          }

          return {num_maj, num_inv, num_xor};
        }
      
      private:
      unsigned num_gates{0u}, num_inv{0u}, num_maj{0u}, num_xor{0u}, depth{0u}, depth_mixed{0u}, depth_maj{0u}, depth_inv{0u}, depth_xor{0u}, num_dangling{0u};
      double qca_area, qca_delay, qca_energy;
  };

  ALICE_ADD_COMMAND( xmgcost, "Various" )
}

#endif
