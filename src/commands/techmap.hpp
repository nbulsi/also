/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019-2021 Ningbo University, Ningbo, China */

/**
 * @file techmap.hpp
 *
 * @brief Standard cell mapping
 *
 * @author Zhufei
 * @since  0.1
 */

#ifndef TECHMAP_HPP
#define TECHMAP_HPP

#include <mockturtle/algorithms/mapper.hpp>
#include <mockturtle/utils/tech_library.hpp>
#include <mockturtle/io/genlib_reader.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/xmg.hpp>

namespace alice
{

  class techmap_command : public command
  {
    public:
      explicit techmap_command( const environment::ptr& env ) : command( env, "Standard cell mapping : using AIG as default" )
      {
        add_flag( "--xmg, -x",  "LUT mapping for XMG" );
        add_flag( "--mig, -m",  "LUT mapping for MIG" );
        add_flag( "--verbose, -v", "print the information" );
      }
      
      rules validity_rules() const
      {
        return { has_store_element<std::vector<mockturtle::gate>>( env ) };
      }

    protected:
      void execute()
      {
        /* derive genlib */
        std::vector<mockturtle::gate> gates = store<std::vector<mockturtle::gate>>().current();

        mockturtle::tech_library<5> lib( gates );
        mockturtle::map_params ps;
        mockturtle::map_stats st;

        if( is_set( "xmg" ) )
        {
          if( store<xmg_network>().size() == 0u )
          {
            std::cerr << "[e] no XMG in the store\n";
          }
          else
          {
            auto xmg = store<xmg_network>().current();

            auto res = mockturtle::tech_map( xmg, lib, ps, &st );

            std::cout << fmt::format( "Mapped into {} gates, area = {}, delay = {}\n", res.num_gates(), st.area, st.delay );
          }
        }
        else if( is_set( "mig" ) )
        {
          if( store<mig_network>().size() == 0u )
          {
            std::cerr << "[e] no MIG in the store\n";
          }
          else
          {
            auto mig = store<mig_network>().current();

            auto res = mockturtle::tech_map( mig, lib, ps, &st );

            std::cout << fmt::format( "Mapped into {} gates, area = {}, delay = {}\n", res.num_gates(), st.area, st.delay );
          }
        }
        else
        {
          if( store<aig_network>().size() == 0u )
          {
            std::cerr << "[e] no AIG in the store\n";
          }
          else
          {
            auto aig = store<aig_network>().current();

            auto res = mockturtle::tech_map( aig, lib, ps, &st );

            std::cout << fmt::format( "Mapped into {} gates, area = {}, delay = {}\n", res.num_gates(), st.area, st.delay );
          }
        }
      }
  };

  ALICE_ADD_COMMAND( techmap, "Mapping" )

}

#endif
