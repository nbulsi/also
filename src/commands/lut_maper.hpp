#ifndef LUT_MAPER_HPP
#define LUT_MAPER_HPP
#include <iterator>
#include <kitty/print.hpp>
#include <mockturtle/mockturtle.hpp>

#include "../store.hpp"

using namespace std;
using namespace mockturtle;

namespace alice
{
class lut_mapper_command : public command
{
public:
  explicit lut_mapper_command( const environment::ptr& env )
      : command( env, "LUT Mapping from mockturtle" )
  {
    add_flag( "--aig, -a", "lut map for AIG" );
    add_flag( "--xag, -g", "lut map for XAG" );
    add_flag( "--xmg, -x", "lut map for XMG" );
    add_option( "cut_size, -k", cut_size,
                "set the cut size from 2 to 8, default = 6" );
    add_option( "cut_limit, -l", cut_limit,
                "set the lut limit from 2 to 49, default = 49" );
    add_flag( "--area, -A", "area mapping" );
    add_flag( "--verbose, -v", "print the information" );
  }

protected:
  void execute()
  {
    clock_t begin, end;
    double totalTime;
    begin = clock();
    mockturtle::lut_map_params ps;
    if ( is_set( "aig" ) )
    {
      if ( !store<aig_network>().empty() )
      {
        auto aig = store<aig_network>().current();
        if ( is_set( "area" ) )
        {
          ps.area_oriented_mapping = true;
        }
        mockturtle::mapping_view<mockturtle::aig_network, true> mapped{ aig };
        ps.cut_enumeration_ps.cut_size = cut_size;
        ps.cut_enumeration_ps.cut_limit = cut_limit;
        const auto klut =
            lut_map<mapping_view<aig_network, true>, true>( mapped, ps );
        // const auto klut = *collapse_mapped_network<klut_network>(mapped);
        store<klut_network>().extend();
        store<klut_network>().current() = klut;
      }
      else
      {
        std::cerr << "There is not an AIG network stored.\n";
      }
    }
    else if ( is_set( "xmg" ) )
    {
      if ( !store<xmg_network>().empty() )
      {
        auto xmg = store<xmg_network>().current();
        if ( is_set( "area" ) )
        {
          ps.area_oriented_mapping = true;
        }
        mockturtle::mapping_view<mockturtle::xmg_network, true> mapped{ xmg };
        ps.cut_enumeration_ps.cut_size = cut_size;
        ps.cut_enumeration_ps.cut_limit = cut_limit;
        const auto klut =
            lut_map<mapping_view<xmg_network, true>, true>( mapped, ps );
        // const auto klut = *collapse_mapped_network<klut_network>(mapped);
        store<klut_network>().extend();
        store<klut_network>().current() = klut;
      }
      else
      {
        std::cerr << "There is not an XMG network stored.\n";
      }
    }
    else if ( is_set( "xag" ) )
    {
      if ( !store<xag_network>().empty() )
      {
        auto xag = store<xag_network>().current();
        if ( is_set( "area" ) )
        {
          ps.area_oriented_mapping = true;
        }
        mockturtle::mapping_view<mockturtle::xag_network, true> mapped{ xag };
        ps.cut_enumeration_ps.cut_size = cut_size;
        ps.cut_enumeration_ps.cut_limit = cut_limit;
        const auto klut =
            lut_map<mapping_view<xag_network, true>, true>( mapped, ps );
        // const auto klut = *collapse_mapped_network<klut_network>(mapped);
        store<klut_network>().extend();
        store<klut_network>().current() = klut;
      }
      else
      {
        std::cerr << "There is not an XAG network stored.\n";
      }
    }
    end = clock();
    totalTime = (double)( end - begin ) / CLOCKS_PER_SEC;

    cout.setf( ios::fixed );
    cout << "[CPU time]   " << setprecision( 3 ) << totalTime << " s" << endl;
  }

private:
  int cut_size = 6;
  int cut_limit = 8;
};
ALICE_ADD_COMMAND( lut_mapper, "Mapping" )
} // namespace alice
#endif
