/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file lut_mapping.hpp
 *
 * @brief lut_mapping 
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef LUT_MAPPING_HPP
#define LUT_MAPPING_HPP

#include <mockturtle/mockturtle.hpp>
#include <mockturtle/algorithms/satlut_mapping.hpp>

#include "../core/cut_enumeration/img_cut.hpp"
#include "../core/cut_enumeration/xmg_cut.hpp"

#include "../core/xmg_lut_mapping.hpp"

namespace alice
{

  class lut_mapping_command : public command
  {
    public:
      explicit lut_mapping_command( const environment::ptr& env ) : command( env, "LUT mapping : using AIG as default" )
      {
        add_option( "cut_size, -k", cut_size, "set the cut size from 2 to 8, default = 4" );
        add_flag( "--verbose, -v", "print the information" );
        add_flag( "--satlut, -s",  "satlut mapping" );
        add_flag( "--xmg, -x",  "LUT mapping for XMG" );
        add_flag( "--xag, -g",  "LUT mapping for XAG" );
        add_flag( "--mig, -m",  "LUT mapping for MIG" );
        add_flag( "--opt_img, -o",  "Using optimal IMG size for 3-input function as the cost function" );
        add_flag( "--opt_xmg, -p",  "Using optimal XMG size/depth for 4-input function as the cost function" );
      }

    protected:
      void execute()
      {
        lut_mapping_params ps;
        
        if( is_set( "xmg" ) )
        {
          /* derive some XMG */
          assert( store<xmg_network>().size() > 0 );
          xmg_network xmg = store<xmg_network>().current();
        
          xmg_lut_mapping_params xmg_ps;

          mapping_view<xmg_network, true> mapped_xmg{xmg};
          xmg_ps.cut_enumeration_ps.cut_size = cut_size;
          xmg_lut_mapping<mapping_view<xmg_network, true>, true>( mapped_xmg, xmg_ps );
          
          /* collapse into k-LUT network */
          auto klut = *collapse_mapped_network<klut_network>( mapped_xmg );
          klut = cleanup_dangling( klut );
          store<klut_network>().extend(); 
          store<klut_network>().current() = klut;
        }
        else if( is_set( "mig" ) )
        {
          /* derive some MIG */
          assert( store<mig_network>().size() > 0 );
          mig_network mig = store<mig_network>().current();

          mapping_view<mig_network, true> mapped_mig{mig};
          ps.cut_enumeration_ps.cut_size = cut_size;
          lut_mapping<mapping_view<mig_network, true>, true>( mapped_mig, ps );
          
          /* collapse into k-LUT network */
          const auto klut = *collapse_mapped_network<klut_network>( mapped_mig );
          store<klut_network>().extend(); 
          store<klut_network>().current() = klut;
        }
        else if( is_set( "xag" ) )
        {
          /* derive some XAG */
          assert( store<xag_network>().size() > 0 );
          xag_network xag = store<xag_network>().current();

          mapping_view<xag_network, true> mapped_xag{xag};
          ps.cut_enumeration_ps.cut_size = cut_size;
          lut_mapping<mapping_view<xag_network, true>, true>( mapped_xag, ps );
          
          /* collapse into k-LUT network */
          const auto klut = *collapse_mapped_network<klut_network>( mapped_xag );
          store<klut_network>().extend(); 
          store<klut_network>().current() = klut;
        }
        else
        {
          if( store<aig_network>().size() == 0 )
          {
            assert( false && "no AIG in the store" );
          }
          /* derive some AIG */
          aig_network aig = store<aig_network>().current();

          mapping_view<aig_network, true> mapped_aig{aig};

          /* LUT mapping */
          if( is_set( "satlut" ) )
          {
            satlut_mapping_params ps;
            ps.cut_enumeration_ps.cut_size = cut_size;
            satlut_mapping<mapping_view<aig_network, true>, true>(mapped_aig, ps );
          }
          else
          {
            ps.cut_enumeration_ps.cut_size = cut_size;
            if( is_set( "opt_img" ) )
            {
              lut_mapping<mapping_view<aig_network, true>, true, cut_enumeration_img_cut>( mapped_aig, ps );
            }
            else if( is_set( "opt_xmg" ) )
            {
              lut_mapping<mapping_view<aig_network, true>, true, cut_enumeration_xmg_cut>( mapped_aig, ps );
            }
            else
            {
              lut_mapping<mapping_view<aig_network, true>, true, cut_enumeration_mf_cut>( mapped_aig, ps );
            }
          }

          /* collapse into k-LUT network */
          const auto klut = *collapse_mapped_network<klut_network>( mapped_aig );
          store<klut_network>().extend(); 
          store<klut_network>().current() = klut;
        }
        
      }
    private:
      unsigned cut_size = 4u;
  };

  ALICE_ADD_COMMAND( lut_mapping, "Mapping" )
}

#endif
