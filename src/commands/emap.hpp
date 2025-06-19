/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019-2021 Ningbo University, Ningbo, China */

/**
 * @file emap.hpp
 *
 * @brief Standard cell mapping
 *
 * @author Zhufei
 * @since  0.1
 */

#ifndef EMAP_HPP
#define EMAP_HPP

#include "../core/properties.hpp"
#include "/home/ym/mockturtle/include/mockturtle/algorithms/emap.hpp"
#include <mockturtle/algorithms/mapper.hpp>
#include <mockturtle/io/genlib_reader.hpp>
#include <mockturtle/io/write_verilog.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/properties/xmgcost.hpp>
#include <mockturtle/utils/tech_library.hpp>

namespace alice
{

class emap_command : public command
{
public:
  explicit emap_command( const environment::ptr& env ) : command( env, "Standard cell mapping : using AIG as default" )
  {
    //add_flag( "--xmg, -x", "Standard cell mapping for XMG" );
    //add_flag( "--mig, -m", "Standard cell mapping for MIG" );
    add_option( "--output, -o", filename, "the verilog filename" );
    add_flag( "--verbose, -v", "print the information" );
  }

  rules validity_rules() const
  {
    return { has_store_element<std::vector<mockturtle::gate>>( env ) };
  }

private:
  std::string filename = "techmap.v";

protected:
  void execute()
  {
    /* derive genlib */
    std::vector<mockturtle::gate> gates = store<std::vector<mockturtle::gate>>().current();

    /* read cell library in genlib format */
    std::vector<gate> gates;
    std::ifstream in( cell_libraries_path( library ) );

    mockturtle::map_params ps;
    mockturtle::map_stats st;
    mockturtle::tech_library<9> lib( gates, ps );

    if ( is_set( "aig" ) )
    {
        auto aig = store<aig_network>().current();

        /* perform technology mapping */
        emap_params ps;
        ps.area_oriented_mapping = true;
        ps.map_multioutput = true;
        cell_view<block_network> res = emap( aig, tech_lib, ps );

        if ( is_set( "output" ) )
        {
          write_verilog_with_binding( res, filename );
        }

        std::cout << fmt::format( "Mapped AIG into #gates = {} area = {:.2f} delay = {:.2f}\n", res.num_gates(), st.area, st.delay );
    }
  }
};

ALICE_ADD_COMMAND( emap, "Mapping" )

} // namespace alice

#endif
