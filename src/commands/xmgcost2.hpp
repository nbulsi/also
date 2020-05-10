/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file xmgcost2.hpp
 *
 * @brief The nanotechnology implemention cost of XMGs (considing nni gates)
 * @modified from https://github.com/msoeken/cirkit/blob/cirkit3/cli/algorithms/migcost.hpp
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef XMGCOST2_HPP
#define XMGCOST2_HPP

#include <mockturtle/mockturtle.hpp>

namespace alice
{

  class xmgcost2_command : public command
  {
    public:
      explicit xmgcost2_command( const environment::ptr& env ) : command( env, "XMG cost evaluation using nanotechnologies" )
      {
        add_flag( "--only_qca", "return the cost of QCA" );
      }

      rules validity_rules() const
      {
        return{ has_store_element<klut_network>( env ) };
      }

      protected:
      void execute()
      {
        klut_network klut = store<klut_network>().current();
        
        /* compute num_maj and num_xor */
        klut.foreach_gate( [&]( auto n ) 
            {
              if( is_maj( klut, n ) )
              {
                num_maj++;
              }
              else if( is_xor3( klut, n ) )
              {
                num_xor++;
              }
              else if( is_nni( klut, n ) )
              {
                num_nni++;
              }
              else if( is_not( klut, n ) )
              {
                num_inv++;
              }
              else
              {
                assert( false && "unkonwn operator" );
              }
            }
            );

        num_gates = klut.num_gates();
        
        mockturtle::depth_view depth_klut{ klut };
        auto v = split_critical_path( depth_klut );

        depth_maj = v[0];
        depth_inv = v[1];
        depth_xor = v[2];
        depth_nni = v[3];

        num_dangling = mockturtle::num_dangling_inputs( klut );

        qca_area = num_maj * 0.0012 + num_inv * 0.004 + num_xor * 0.0027 + num_nni * 0.0015;
        qca_delay = depth_maj * 0.004 + depth_inv * 0.014 + depth_xor * 0.009 + depth_nni * 0.005;
        qca_energy = num_maj * 2.94 + num_inv * 9.8 + num_xor * 6.615 + num_nni * 3.68;

        env->out() << fmt::format( "[i] Gates             = {}\n"
            "[i] Num MAJs          = {}\n"
            "[i] Num XORs          = {}\n"
            "[i] Num NNIs          = {}\n"
            "[i] Num INVs          = {}\n"
            "[i] Depth mixed       = {}\n"
            "[i] Depth mixed (MAJ) = {}\n"
            "[i] Depth mixed (INV) = {}\n"
            "[i] Depth mixed (XOR) = {}\n"
            "[i] Depth mixed (NNI) = {}\n"
            "[i] Dangling inputs   = {}\n",
            num_gates, num_maj, num_xor, num_nni, num_inv, depth_klut.depth(), depth_maj, depth_inv, depth_xor, 
            depth_nni, num_dangling );

        env->out() << fmt::format( "[i] QCA (area)        = {:.2f} um^2\n"
            "[i] QCA (delay)       = {:.2f} ns\n"
            "[i] QCA (energy)      = {:.2f} E-21 J\n",
                                 qca_area, qca_delay, qca_energy );
      }

      private:
      std::string get_tt_str( klut_network const& klut, node<klut_network> const& n )
      {
        std::vector<kitty::dynamic_truth_table> xs;
        xs.emplace_back( 3u );
        xs.emplace_back( 3u );
        xs.emplace_back( 3u );
        kitty::create_nth_var( xs[0], 0 );
        kitty::create_nth_var( xs[1], 1 );
        kitty::create_nth_var( xs[2], 2 );

        const auto sim = klut.compute( n, xs.begin(), xs.end() );

        return kitty::to_hex( sim );
      }
      
      bool is_maj( klut_network const& klut, node<klut_network> const& n )
      {
        if( klut.fanin_size( n ) != 3u ) return false;
        return get_tt_str( klut, n ) == "e8";
      }
      
      bool is_xor3( klut_network const& klut, node<klut_network> const& n )
      {
        if( klut.fanin_size( n ) != 3u ) return false;
        return get_tt_str( klut, n ) == "96";
      }
      
      bool is_nni( klut_network const& klut, node<klut_network> const& n )
      {
        if( klut.fanin_size( n ) != 3u ) return false;
        return get_tt_str( klut, n ) == "71";
      }

      bool is_not( klut_network const& klut, node<klut_network> const& n )
      {
        if( klut.is_pi( n ) || klut.is_constant( n ) )
        {
          return false;
        }
        return klut.fanin_size( n ) == 1u;
      }

        std::vector<unsigned> split_critical_path( klut_network const& klut )
        {
          using namespace mockturtle;

          unsigned num_maj{0}, num_inv{0}, num_xor{0}, num_nni{0u};
          mockturtle::depth_view dklut{ klut };

          node<klut_network> cp_node;
          klut.foreach_po( [&]( auto const& f ) {
              auto level = dklut.level( klut.get_node( f ) );
                
              if ( level == dklut.depth() )
              {
                cp_node = klut.get_node( f );
                return false;
              }

              return true;
              } );


          while ( !klut.is_constant( cp_node ) && !klut.is_pi( cp_node ) )
          {
            if( is_maj( klut, cp_node ) )
            {
              num_maj++;
            }
            else if( is_xor3( klut, cp_node ) )
            {
              num_xor++;
            }
            else if( is_nni( klut, cp_node ) )
            {
              num_nni++;
            }
            else if( is_not( klut, cp_node ) )
            {
              num_inv++;
            }
            else
            {
              assert( false && "only support XMG now" );
            }

            klut.foreach_fanin( cp_node, [&]( auto const& f ) {
                auto level = dklut.level( klut.get_node( f ) );

                if ( level + 1 == dklut.level( cp_node ) )
                {
                  cp_node = klut.get_node( f );
                  return false;
                }
                return true;
                } );
          }

          std::vector<unsigned> v{ num_maj, num_inv, num_xor, num_nni };
          return v;
        }
      
      private:
      unsigned num_gates{0u}, num_inv{0u}, num_maj{0u}, num_xor{0u}, num_nni{0u}, depth{0u}, depth_mixed{0u}, depth_maj{0u}, depth_inv{0u}, depth_xor{0u}, depth_nni{0u}, num_dangling{0u};
      double qca_area, qca_delay, qca_energy;
  };

  ALICE_ADD_COMMAND( xmgcost2, "Various" )
}

#endif
