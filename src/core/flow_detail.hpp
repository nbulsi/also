#ifndef FLOW_DETAIL_HPP
#define FLOW_DETAIL_HPP

#include "decomp_func.hpp"
#include <mockturtle/mockturtle.hpp>

namespace mockturtle
{
/* create truth table from hax  */
struct decomposition_flow_params
{
  bool allow_xor = true;
  bool allow_m3aj = true;
  bool allow_mux = true;
  bool allow_s = false;
  bool allow_n = false;
};

/* create truth table from hax  */
template<class Ntk>
void read_hax( std::string& hax_num, kitty::dynamic_truth_table& remainder, int& num, Ntk& ntk, std::vector<signal<Ntk>>& children )
{
  int var_num = 0;
  if ( hax_num.substr( 0, 2 ) == "0x" )
  {
    auto size = hax_num.substr( 2 ).size() << 2;
    while ( size >>= 1 )
    {
      ++var_num;
    }
    kitty::dynamic_truth_table tt( var_num );
    kitty::create_from_hex_string( tt, hax_num.substr( 2 ) );
    remainder = tt;
    num = var_num;
    for ( int j = 0; j < var_num; j++ )
    {
      children.push_back( ntk.create_pi() );
    }
  }
  else
  {
    auto size = hax_num.size() << 2;
    while ( size >>= 1 )
    {
      ++var_num;
    }
    kitty::dynamic_truth_table tt( var_num );
    kitty::create_from_hex_string( tt, hax_num );
    remainder = tt;
    num = var_num;
    for ( int j = 0; j < var_num; j++ )
    {
      children.push_back( ntk.create_pi() );
    }
  }
}

/* create truth table from binary  */
template<class Ntk>
void read_binary( std::string& binary, kitty::dynamic_truth_table& remainder, int& num, Ntk& ntk, std::vector<signal<Ntk>>& children )
{
  auto binary_num = binary.length();
  num = 0;
  while ( binary_num > 1 )
  {
    binary_num = binary_num >> 1;
    num++;
  }
  kitty::dynamic_truth_table tt( num );
  kitty::create_from_binary_string( tt, binary );
  remainder = tt;
  for ( int j = 0; j < num; j++ )
  {
    children.push_back( ntk.create_pi() );
  }
}

namespace detail
{
template<class Ntk>
class dsd_impl
{
public:
  dsd_impl( Ntk& ntk, kitty::dynamic_truth_table const& remainder, std::vector<signal<Ntk>> const& children, mockturtle::decomposition_flow_params& ps )
      : _ntk( ntk ),
        _remainder( remainder ),
        pis( children ),
        _ps( ps )
  {
    for ( auto i = 0u; i < _remainder.num_vars(); ++i )
    {
      if ( kitty::has_var( _remainder, i ) )
      {
        support.push_back( i );
      }
    }
  }

  signal<Ntk> run()
  {
    /* terminal cases */
    for ( auto var : support )
    {
      if ( kitty::has_var( _remainder, var ) )
      {
        continue;
      }
      else
      {
        support.erase( std::remove( support.begin(), support.end(), var ), support.end() );
      }
    }
    if ( kitty::is_const0( _remainder ) )
    {
      return _ntk.get_constant( false );
    }
    if ( kitty::is_const0( ~_remainder ) )
    {
      return _ntk.get_constant( true );
    }

    /* projection case */
    if ( support.size() < 1u )
    {
      std::cout << "var is 0" << std::endl;
      assert( false );
    }
    if ( support.size() == 1u )
    {
      auto var = _remainder.construct();
      kitty::create_nth_var( var, support.front() );
      if ( _remainder == var )
      {
        return pis[support.front()];
      }
      else
      {
        if ( _remainder != ~var )
        {
          fmt::print( "remainder = {}, vars = {}\n", kitty::to_binary( _remainder ), _remainder.num_vars() );
          assert( false );
        }
        assert( _remainder == ~var );
        return _ntk.create_not( pis[support.front()] );
      }
    }

    /* decomposition stops when the number of variables <= 4 */
    if ( _ps.allow_s )
    {
      if ( support.size() <= 4u )
      {
        std::vector<signal<Ntk>> new_pis_short;
        for ( auto var : support )
        {
          if ( kitty::has_var( _remainder, var ) )
          {
            new_pis_short.push_back( pis[var] );
          }
        }
        auto prime_large = _remainder;
        kitty::min_base_inplace( prime_large );
        auto prime = kitty::shrink_to( prime_large, static_cast<unsigned int>( new_pis_short.size() ) );
        return _ntk.create_node( new_pis_short, prime );
      }
    }

    /* try top decomposition */
    for ( auto var : support )
    {
      if ( auto res = kitty::is_top_decomposable( _remainder, var, &_remainder, _ps.allow_xor );
           res != kitty::top_decomposition::none )
      {
        /* remove var from support, pis do not change */
        support.erase( std::remove( support.begin(), support.end(), var ), support.end() );
        const auto right = run();
        switch ( res )
        {
        default:
          assert( false );
        case kitty::top_decomposition::and_:
          return _ntk.create_and( pis[var], right );
        case kitty::top_decomposition::or_:
          return _ntk.create_or( pis[var], right );
        case kitty::top_decomposition::lt_:
          return _ntk.create_lt( pis[var], right );
        case kitty::top_decomposition::le_:
          return _ntk.create_le( pis[var], right );
        case kitty::top_decomposition::xor_:
          return _ntk.create_xor( pis[var], right );
        }
      }
    }

    /* try bottom decomposition */
    for ( auto j = 1u; j < support.size(); ++j )
    {
      for ( auto i = 0u; i < j; ++i )
      {
        if ( auto res = kitty::is_bottom_decomposable( _remainder, support[i], support[j], &_remainder, false ); res != kitty::bottom_decomposition::none )
        {
          switch ( res )
          {
          default:
            assert( false );
          case kitty::bottom_decomposition::and_:
            pis[support[i]] = _ntk.create_and( pis[support[i]], pis[support[j]] );
            break;
          case kitty::bottom_decomposition::or_:
            pis[support[i]] = _ntk.create_or( pis[support[i]], pis[support[j]] );
            break;
          case kitty::bottom_decomposition::lt_:
            pis[support[i]] = _ntk.create_lt( pis[support[i]], pis[support[j]] );
            break;
          case kitty::bottom_decomposition::le_:
            pis[support[i]] = _ntk.create_le( pis[support[i]], pis[support[j]] );
            break;
          case kitty::bottom_decomposition::xor_:
            pis[support[i]] = _ntk.create_xor( pis[support[i]], pis[support[j]] );
            break;
          }
          support.erase( support.begin() + j );
          return run();
        }
      }
    }

    /* try bottom xor decomposition */
    for ( auto j = 1u; j < support.size(); ++j )
    {
      for ( auto i = 0u; i < j; ++i )
      {
        if ( bottom_xor_decomposition( _remainder, support[i], support[j], &_remainder ) )
        {
          pis[support[i]] = _ntk.create_xor( pis[support[i]], pis[support[j]] );
          support.erase( support.begin() + j );
          return run();
        }
      }
    }

    /* try bottom mux decomposition the method from V . Callegaro, F. S. Marranghello, M. G. Martins, R. P . Ribas and A. I. Reis, “Bottom-up disjoint-support decomposition based on cofactor and boolean difference analysis,” In ICCAD, pp. 680–687, 2015.*/
    if ( _ps.allow_mux )
    {
      for ( auto j = 0u; j < support.size(); ++j )
      {
        for ( auto i = 0u; i < support.size(); ++i )
        {
          for ( auto k = 0u; k < support.size(); ++k )
          {
            if ( ( j == i ) | ( j == k ) | ( i == k ) )
            {
              continue;
            }
            if ( bottom_mux_decomposition( _remainder, support[j], support[i], support[k], &_remainder ) )
            {
              pis[support[j]] = _ntk.create_ite( pis[support[j]], pis[support[k]], pis[support[i]] );
              if ( k > i )
              {
                support.erase( support.begin() + k );
                support.erase( support.begin() + i );
              }
              else
              {
                support.erase( support.begin() + i );
                support.erase( support.begin() + k );
              }
              return run();
            }
          }
        }
      }
    }

    /* try bottom maj decomposition */
    if ( _ps.allow_m3aj )
    {
      for ( auto j = 2u; j < support.size(); ++j )
      {
        for ( auto i = 1u; i < j; ++i )
        {
          for ( auto k = 0u; k < i; ++k )
          {
            auto stats = maj_bottom_decomposition( _remainder, support[k], support[i], support[j], &_remainder );
            if ( stats != -1 )
            {
              if ( stats == 0 )
              {
                pis[support[k]] = _ntk.create_maj( pis[support[k]], pis[support[i]], pis[support[j]] );
                support.erase( support.begin() + j );
                support.erase( support.begin() + i );
                return run();
              }
              else if ( stats == 1 )
              {
                pis[support[k]] = _ntk.create_maj( _ntk.create_not( pis[support[k]] ), pis[support[i]], pis[support[j]] );
                support.erase( support.begin() + j );
                support.erase( support.begin() + i );
                return run();
              }
              else if ( stats == 2 )
              {
                pis[support[k]] = _ntk.create_maj( pis[support[k]], _ntk.create_not( pis[support[i]] ), pis[support[j]] );
                support.erase( support.begin() + j );
                support.erase( support.begin() + i );
                return run();
              }
              else if ( stats == 3 )
              {
                pis[support[k]] = _ntk.create_maj( pis[support[k]], pis[support[i]], _ntk.create_not( pis[support[j]] ) );
                support.erase( support.begin() + j );
                support.erase( support.begin() + i );
                return run();
              }
            }
          }
        }
      }
    }

    /* can't dsd anymore*/

    if ( _ps.allow_n )
    {
      /* non-dsd part is not operated */
      std::vector<signal<Ntk>> new_pis;
      for ( auto var : support )
      {
        new_pis.push_back( pis[var] );
      }
      auto prime_large = _remainder;
      kitty::min_base_inplace( prime_large );
      auto prime = kitty::shrink_to( prime_large, static_cast<unsigned int>( support.size() ) );
      return _ntk.create_node( new_pis, prime );
    }
    else
    {
      /* find min support var */
      auto var_min = support[0u];
      for ( auto var : support )
      {
        const auto co00 = cofactor0( _remainder, var );
        const auto co11 = cofactor1( _remainder, var );
        s_co0 = 0;
        s_co1 = 0;
        for ( auto s_sd = 0u; s_sd < co00.num_vars(); ++s_sd )
        {
          if ( kitty::has_var( co00, s_sd ) )
          {
            s_co0 += 1;
          }
        }
        for ( auto s_sd = 0u; s_sd < co11.num_vars(); ++s_sd )
        {
          if ( kitty::has_var( co11, s_sd ) )
          {
            s_co1 += 1;
          }
        }
        int add = s_co0 + s_co1;
        if ( add < s_num )
        {
          s_num = add;
          var_min = var;
        }
      }

      /* try top mux decomposition */
      kitty::dynamic_truth_table func_mux;
      if ( _ps.allow_mux )
      {
        if ( top_mux_decomposition( _remainder, var_min, &_remainder, &func_mux ) )
        {
          support.erase( std::remove( support.begin(), support.end(), var_min ), support.end() );

          std::vector<uint8_t> support_cp_mux;
          std::vector<signal<Ntk>> pis_cp_mux;
          support_cp_mux.assign( support.begin(), support.end() );
          pis_cp_mux.assign( pis.begin(), pis.end() );

          const auto run_co0 = run();

          _remainder = func_mux;

          support.assign( support_cp_mux.begin(), support_cp_mux.end() );
          pis.assign( pis_cp_mux.begin(), pis_cp_mux.end() );

          const auto run_co1 = run();
          return _ntk.create_ite( pis[var_min], run_co1, run_co0 );
        }
      }

      /* try top maj decomposition the method from  Z. Chu, M. Soeken, Y . Xia, L. Wang and G. De Micheli, “Advanced functional decomposition using majority and its applications,” IEEE Transactions on Computer-Aided Design of Integrated Circuits and Systems, vol. 39, no. 8, pp. 1621–1634, 2019. */
      if ( _ps.allow_m3aj )
      {
        kitty::dynamic_truth_table func_maj;
        if ( maj_top_decomposition( _remainder, var_min, &_remainder, &func_maj ) )
        {
          support.erase( std::remove( support.begin(), support.end(), var_min ), support.end() );

          std::vector<uint8_t> support_cp_maj;
          std::vector<signal<Ntk>> pis_cp_maj;
          support_cp_maj.assign( support.begin(), support.end() );
          pis_cp_maj.assign( pis.begin(), pis.end() );

          const auto right = run();

          _remainder = func_maj;

          support.assign( support_cp_maj.begin(), support_cp_maj.end() );
          pis.assign( pis_cp_maj.begin(), pis_cp_maj.end() );

          const auto center = run();
          return _ntk.create_maj( pis[var_min], center, right );
        }
      }

      /* do shannon decomposition */
      const auto co0 = cofactor0( _remainder, var_min );
      const auto co1 = cofactor1( _remainder, var_min );
      support.erase( std::remove( support.begin(), support.end(), var_min ), support.end() );

      std::vector<uint8_t> support_cp;
      std::vector<signal<Ntk>> pis_cp;

      support_cp.assign( support.begin(), support.end() );
      pis_cp.assign( pis.begin(), pis.end() );

      _remainder = co0;
      const auto left = run();

      pis.assign( pis_cp.begin(), pis_cp.end() );
      support.assign( support_cp.begin(), support_cp.end() );

      _remainder = co1;
      const auto right = run();

      return _ntk.create_ite( pis[var_min], right, left );
    }
  }

private:
  int ps = 0;
  int stats;
  Ntk& _ntk;
  kitty::dynamic_truth_table _remainder;
  std::vector<uint8_t> support;
  std::vector<signal<Ntk>> pis;
  mockturtle::decomposition_flow_params const& _ps;
  int s_co0;
  int s_co1;
  int s_num = 50;
};

} // namespace detail

template<class Ntk>
signal<Ntk> dsd_detail( Ntk& ntk, kitty::dynamic_truth_table const& remainder, std::vector<signal<Ntk>> const& children, mockturtle::decomposition_flow_params& ps )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_create_not_v<Ntk>, "Ntk does not implement the create_not method" );
  static_assert( has_create_and_v<Ntk>, "Ntk does not implement the create_and method" );
  static_assert( has_create_or_v<Ntk>, "Ntk does not implement the create_or method" );
  static_assert( has_create_lt_v<Ntk>, "Ntk does not implement the create_lt method" );
  static_assert( has_create_le_v<Ntk>, "Ntk does not implement the create_le method" );
  static_assert( has_create_xor_v<Ntk>, "Ntk does not implement the create_xor method" );
  static_assert( has_create_maj_v<Ntk>, "Ntk does not implement the create_maj method" );
  static_assert( has_create_ite_v<Ntk>, "Ntk does not implement the create_ite method" );
  detail::dsd_impl<Ntk> impl( ntk, remainder, children, ps );
  return impl.run();
}
} // namespace mockturtle

#endif