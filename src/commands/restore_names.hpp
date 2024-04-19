/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019-2021 Ningbo University, Ningbo, China */

/**
 * @file restore_names.hpp
 *
 * @brief restore network names after optimization (note that the optimized network
 * is XMG)
 *
 * @author Zhufei
 * @since  0.1
 */

#ifndef RESTORE_NAMES_HPP
#define RESTORE_NAMES_HPP

#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/views/names_view.hpp>
#include <mockturtle/utils/node_map.hpp>

namespace alice
{
  class restore_names_command : public command
  {
    public:
      explicit restore_names_command( const environment::ptr& env ) : command( env, "Restore netlist I/O names" )
      {
        add_flag( "-v,--verbose", "verbose information" );
        add_flag( "-a,--aiger", "aiger formar input file" );
        add_option( "infilename, -f", input_filename, "the original input file name" );
        add_option( "outfilename, -o", output_blif_filename, "the output XMG-based BLIF file name" );
      }

      rules validity_rules() const
      {
        return { has_store_element<xmg_network>( env ) };
      }

    protected:
      void execute()
      {
       /* store PI/PO names */
        std::vector<std::string> pi_names;
        std::vector<std::string> po_names;
        
        if( is_set( "aiger") )
        {
          aig_network aig;
          names_view<aig_network> named_aig{ aig };
          
          if ( lorina::read_aiger( input_filename, mockturtle::aiger_reader( named_aig ) ) != lorina::return_code::success )
          {
            std::cout << "[w] parse error for file" << input_filename << "\n";
          }
          
          named_aig.foreach_ci( [&]( auto const& node, auto index ){
              pi_names.push_back( named_aig.get_name( aig.make_signal( aig.pi_at( index ) ) ) );
              } );

          named_aig.foreach_co( [&]( auto const& node, auto index ){
              po_names.push_back( named_aig.get_output_name( index ) );
              } );
        }
        else //blif
        {
          klut_network klut;
          names_view<klut_network> named_klut{ klut, "netlist" };

          if ( lorina::read_blif( input_filename, mockturtle::blif_reader( named_klut ) ) != lorina::return_code::success )
          {
            std::cout << "[w] parse error for file" << input_filename << "\n";
          }

          named_klut.set_network_name( "netlist" );

          named_klut.foreach_ci( [&]( auto const& node, auto index ){
              pi_names.push_back( named_klut.get_name( node ) );
              } );

          named_klut.foreach_co( [&]( auto const& node, auto index ){
              po_names.push_back( named_klut.get_output_name( index ) );
              } );
        }

         /* set xmg_network names */
         xmg_network xmg = store<xmg_network>().current();
         names_view<xmg_network> named_xmg{ xmg };

         assert( named_xmg.num_pis() == pi_names.size() );
         assert( named_xmg.num_pos() == po_names.size() );

         named_xmg.foreach_ci( [&]( auto const& node, auto index ) {
                 named_xmg.set_name( 2*node, pi_names[index] );
                 } );

         named_xmg.foreach_co( [&]( auto const& node, auto index ){
                 named_xmg.set_output_name( index, po_names[index] );
                 } );

         write_blif( named_xmg, output_blif_filename );
      }

    private:
      bool verbose;
      std::string input_filename = "";
      std::string output_blif_filename = "xmg_out.blif";

    };

  ALICE_ADD_COMMAND( restore_names, "Various" )
}

#endif
