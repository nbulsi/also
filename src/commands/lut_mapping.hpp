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

namespace alice
{

  class lut_mapping_command : public command
  {
    public:
      explicit lut_mapping_command( const environment::ptr& env ) : command( env, "LUT mapping" )
      {
        add_option( "cut_size, -k", cut_size, "set the cut size from 2 to 8, default = 4" );
        add_flag( "--verbose, -v", "print the information" );
      }

      rules validity_rules() const
      {
        return { has_store_element<aig_network>( env ) };
      }

    protected:
      void execute()
      {
        /* derive some AIG */
        aig_network aig = store<aig_network>().current();

        /* LUT mapping */
        mapping_view<aig_network, true> mapped_aig{aig};
        lut_mapping_params ps;
        ps.cut_enumeration_ps.cut_size = 4;
        lut_mapping<mapping_view<aig_network, true>, true>( mapped_aig, ps );

        /* collapse into k-LUT network */
        const auto klut = *collapse_mapped_network<klut_network>( mapped_aig );
        
        store<klut_network>().extend(); 
        store<klut_network>().current() = klut;
      }
    private:
      unsigned cut_size = 4u;
  };

  ALICE_ADD_COMMAND( lut_mapping, "Mapping commands" )
}

#endif
