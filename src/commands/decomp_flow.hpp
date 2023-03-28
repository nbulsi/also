#ifndef DECOMP_FLOW_HPP
#define DECOMP_FLOW_HPP

#include "../core/flow_detail.hpp"
#include <alice/alice.hpp>
#include <mockturtle/mockturtle.hpp>

namespace alice
{
class decomp_flow_command : public command
{
public:
  explicit decomp_flow_command( const environment::ptr& env ) : command( env, "perform decomposition flow" )
  {
    add_option( "--tt_from_hex, -a", tt_in, "truth table from hex" );
    add_option( "--tt_from_binary, -b", tt_in, "truth table from binary" );
    add_flag( "--noM3aj, -m", "don't ues M3aj,default = true" );
    add_flag( "--noXor, -x", "don't ues Xor,default = true" );
    add_flag( "--noMux, -u", "don't ues Mux,default = true" );
    add_flag( "--non-dsd, -n", "stop decomposition when func is non-dsd, default = false" );
    add_flag( "--shortsupport, -s", "The decomposition stops when the number of support <= 4, default = false " );
  }

protected:
  void execute()
  {
    mockturtle::klut_network ntk;
    std::vector<mockturtle::klut_network::signal> children;
    kitty::dynamic_truth_table remainder;
    mockturtle::decomposition_flow_params ps;

    if ( is_set( "--tt_from_hex" ) )
    {
      mockturtle::read_hax( tt_in, remainder, var_num, ntk, children );
    }
    else if ( is_set( "--tt_from_binary" ) )
    {
      mockturtle::read_binary( tt_in, remainder, var_num, ntk, children );
    }
    if ( is_set( "noM3aj" ) )
    {
      ps.allow_m3aj = false;
    }
    if ( is_set( "noXor" ) )
    {
      ps.allow_xor = false;
    }
    if ( is_set( "noMux" ) )
    {
      ps.allow_mux = false;
    }
    if ( is_set( "shortsupport" ) )
    {
      ps.allow_s = true;
    }
    if ( is_set( "non-dsd" ) )
    {
      ps.allow_n = true;
    }
    ntk.create_po( mockturtle::dsd_detail( ntk, remainder, children, ps ) );
    store<klut_network>().extend();
    store<klut_network>().current() = cleanup_dangling( ntk );
  }

private:
  std::string tt_in;
  int var_num;
};
ALICE_ADD_COMMAND( decomp_flow, "decomposition" );
} // namespace alice

#endif
