#pragma once

#include "mockturtle/mockturtle.hpp"

template<class TT>
bool maj_top_decomposition( const TT& tt, uint32_t var_index, TT* func_co0 = nullptr, TT* func_co1 = nullptr )
{
  static_assert( kitty::is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );
  const auto co0 = cofactor0( tt, var_index );
  const auto co1 = cofactor1( tt, var_index );
  std::vector<uint8_t> support_co0;
  std::vector<uint8_t> support_co1;
  for ( auto s_sd = 0u; s_sd < co0.num_vars(); ++s_sd )
  {
    if ( kitty::has_var( co0, s_sd ) )
    {
      support_co0.push_back( s_sd );
    }
  }
  for ( auto s_sd = 0u; s_sd < co1.num_vars(); ++s_sd )
  {
    if ( kitty::has_var( co1, s_sd ) )
    {
      support_co1.push_back( s_sd );
    }
  }

  for ( auto i = 0u; i < support_co0.size(); ++i )
  {
    for ( auto j = 0u; j < support_co0.size(); ++j )
    {
      if ( support_co0[i] == support_co1[j] )
      {
        return false;
      }
    }
  }

  if ( is_const0( binary_and( co0, ~co1 ) ) )
  {
    if ( func_co0 )
    {
      *func_co0 = co0;
    }
    if ( func_co1 )
    {
      *func_co1 = co1;
    }
    return true;
  }
  return false;
}

template<class TT>
int maj_bottom_decomposition( const TT& tt, uint32_t var_index1, uint32_t var_index2, uint32_t var_index3, TT* func = nullptr )
{
  static_assert( kitty::is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  const auto co0_var1 = cofactor0( tt, var_index1 );
  const auto co1_var1 = cofactor1( tt, var_index1 );
  const auto dif_var1 = kitty::binary_xor( co0_var1, co1_var1 );

  const auto co0_var2 = cofactor0( tt, var_index2 );
  const auto co1_var2 = cofactor1( tt, var_index2 );
  const auto dif_var2 = kitty::binary_xor( co0_var2, co1_var2 );

  const auto co0_var3 = cofactor0( tt, var_index3 );
  const auto co1_var3 = cofactor1( tt, var_index3 );
  const auto dif_var3 = kitty::binary_xor( co0_var3, co1_var3 );

  const auto dif1_co0 = cofactor0( cofactor0( dif_var1, var_index2 ), var_index3 );
  const auto dif2_co0 = cofactor0( cofactor0( dif_var2, var_index1 ), var_index3 );
  const auto dif3_co0 = cofactor0( cofactor0( dif_var3, var_index1 ), var_index2 );

  const auto isconst01 = is_const0( dif1_co0 );
  const auto isconst02 = is_const0( dif2_co0 );
  const auto isconst03 = is_const0( dif3_co0 );

  const auto num_pairs =
      static_cast<uint32_t>( isconst01 ) +
      static_cast<uint32_t>( isconst02 ) +
      static_cast<uint32_t>( isconst03 );

  const auto eq12 = equal( dif1_co0, dif2_co0 );
  const auto eq13 = equal( dif1_co0, dif3_co0 );
  const auto eq23 = equal( dif2_co0, dif3_co0 );

  const auto co0_var2_dif1 = cofactor0( dif_var1, var_index2 );
  const auto co1_var2_dif1 = cofactor1( dif_var1, var_index2 );
  const auto dif_var1_var2 = kitty::binary_xor( co0_var2_dif1, co1_var2_dif1 );

  const auto co0_var3_dif1 = cofactor0( dif_var1, var_index3 );
  const auto co1_var3_dif1 = cofactor1( dif_var1, var_index3 );
  const auto dif_var1_var3 = kitty::binary_xor( co0_var3_dif1, co1_var3_dif1 );

  const auto co0_var3_dif2 = cofactor0( dif_var2, var_index3 );
  const auto co1_var3_dif2 = cofactor1( dif_var2, var_index3 );
  const auto dif_var2_var3 = kitty::binary_xor( co0_var3_dif2, co1_var3_dif2 );

  // condition 1:
  if ( equal( dif_var1_var2, dif_var1_var3 ) && equal( dif_var1_var2, dif_var2_var3 ) && ( is_const0( ~dif_var1_var2 ) ) )
  {
    if ( is_const0( dif1_co0 ) && is_const0( dif2_co0 ) && is_const0( dif3_co0 ) )
    {
      auto co0_var1_co0_var2 = cofactor0( cofactor0( tt, var_index1 ), var_index2 );
      auto co0_var2_co1_var3_dif1 = cofactor1( cofactor0( dif_var1, var_index2 ), var_index3 );
      kitty::dynamic_truth_table tt_1( co0_var2_co1_var3_dif1.num_vars() );
      if ( func )
      {
        *func = kitty::binary_xor( mux_var( var_index1, co0_var2_co1_var3_dif1, tt_1 ), co0_var1_co0_var2 );
      }
      return 0;
    }
    if ( eq23 && isconst01 && ( is_const0( ~dif2_co0 ) ) )
    {
      auto co0_var1_co0_var2 = cofactor0( cofactor0( tt, var_index2 ), var_index3 );
      auto co0_var2_co1_var3_dif1 = cofactor0( cofactor1( dif_var1, var_index3 ), var_index2 );
      kitty::dynamic_truth_table tt_1( co0_var2_co1_var3_dif1.num_vars() );
      if ( func )
      {
        *func = kitty::binary_xor( mux_var( var_index1, co0_var2_co1_var3_dif1, tt_1 ), co0_var1_co0_var2 );
      }
      return 1;
    }
    if ( eq13 && isconst02 && ( is_const0( ~dif1_co0 ) ) )
    {
      auto co0_var1_co0_var2 = cofactor0( cofactor0( tt, var_index1 ), var_index3 );
      auto co0_var2_co1_var3_dif1 = cofactor0( cofactor1( dif_var2, var_index1 ), var_index3 );
      kitty::dynamic_truth_table tt_1( co0_var2_co1_var3_dif1.num_vars() );
      if ( func )
      {
        *func = kitty::binary_xor( mux_var( var_index1, co0_var2_co1_var3_dif1, tt_1 ), co0_var1_co0_var2 );
      }
      return 2;
    }
    if ( eq12 && isconst03 && ( is_const0( ~dif1_co0 ) ) )
    {
      auto co0_var1_co0_var2 = cofactor0( cofactor0( tt, var_index1 ), var_index2 );
      auto co0_var2_co1_var3_dif1 = cofactor0( cofactor1( dif_var3, var_index1 ), var_index2 );
      kitty::dynamic_truth_table tt_1( co0_var2_co1_var3_dif1.num_vars() );
      if ( func )
      {
        *func = kitty::binary_xor( mux_var( var_index1, co0_var2_co1_var3_dif1, tt_1 ), co0_var1_co0_var2 );
      }
      return 3;
    }
  }
  return -1;
}

template<class TT>
bool bottom_mux_decomposition( const TT& tt, uint32_t sel, uint32_t var_index1, uint32_t var_index2, TT* func = nullptr )
{
  static_assert( kitty::is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );
  const auto co0_var1 = cofactor0( tt, var_index1 );
  const auto co1_var1 = cofactor1( tt, var_index1 );
  const auto dif_var1 = kitty::binary_xor( co0_var1, co1_var1 );

  const auto co0_var2 = cofactor0( tt, var_index2 );
  const auto co1_var2 = cofactor1( tt, var_index2 );
  const auto dif_var2 = kitty::binary_xor( co0_var2, co1_var2 );

  const auto dif1_co0_var2 = cofactor0( dif_var1, var_index2 );
  const auto dif1_co1_var2 = cofactor1( dif_var1, var_index2 );
  const auto dif1_dif2 = kitty::binary_xor( dif1_co0_var2, dif1_co1_var2 );

  const auto co1_dif1_sel = cofactor1( dif_var1, sel );
  const auto co0_dif2_sel = cofactor0( dif_var2, sel );

  const auto co0_var1_var2 = cofactor0( cofactor0( tt, var_index1 ), var_index2 );
  const auto co1_var1_var2 = cofactor1( cofactor1( tt, var_index1 ), var_index2 );

  const auto dif0_sel = kitty::binary_xor( cofactor0( co0_var1_var2, sel ), cofactor1( co0_var1_var2, sel ) );
  const auto dif1_sel = kitty::binary_xor( cofactor0( co1_var1_var2, sel ), cofactor1( co1_var1_var2, sel ) );

  const auto co0_dif1_sel = cofactor0( dif_var1, sel );

  if ( equal( dif_var1, dif_var2 ) )
  {
    return false;
  }

  if ( is_const0( dif1_dif2 ) & is_const0( co1_dif1_sel ) & is_const0( co0_dif2_sel ) & is_const0( dif0_sel ) & is_const0( dif1_sel ) )
  {
    if ( func )
    {
      const auto co0_dif1_sel = cofactor0( dif_var1, sel );
      kitty::dynamic_truth_table tt_1( co0_dif1_sel.num_vars() );
      *func = kitty::binary_xor( mux_var( sel, co0_dif1_sel, tt_1 ), co0_var1_var2 );
    }
    return true;
  }
  return false;
}

template<class TT>
bool top_mux_decomposition( const TT& tt, uint32_t var_index1, TT* func_co0 = nullptr, TT* func_co1 = nullptr )
{
  static_assert( kitty::is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );
  bool has_decomp = true;
  const auto co0_var1 = cofactor0( tt, var_index1 );
  const auto co1_var1 = cofactor1( tt, var_index1 );
  std::vector<uint8_t> support_co0;
  std::vector<uint8_t> support_co1;
  for ( auto s_sd = 0u; s_sd < co0_var1.num_vars(); ++s_sd )
  {
    if ( kitty::has_var( co0_var1, s_sd ) )
    {
      support_co0.push_back( s_sd );
    }
  }
  for ( auto s_sd = 0u; s_sd < co1_var1.num_vars(); ++s_sd )
  {
    if ( kitty::has_var( co1_var1, s_sd ) )
    {
      support_co1.push_back( s_sd );
    }
  }

  for ( auto i = 0u; i < support_co0.size(); ++i )
  {
    if ( has_decomp == false )
    {
      break;
    }
    for ( auto j = 0u; j < support_co0.size(); ++j )
    {
      if ( support_co0[i] == support_co1[j] )
      {
        has_decomp = false;
        break;
      }
    }
  }
  if ( has_decomp )
  {
    if ( func_co0 )
    {
      *func_co0 = co0_var1;
    }
    if ( func_co1 )
    {
      *func_co1 = co1_var1;
    }
    return true;
  }
  return false;
}

template<class TT>
bool bottom_xor_decomposition( const TT& tt, uint32_t var_index1, uint32_t var_index2, TT* func = nullptr )
{
  static_assert( kitty::is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );
  const auto co0_var1 = cofactor0( tt, var_index1 );
  const auto co1_var1 = cofactor1( tt, var_index1 );
  const auto dif_var1 = kitty::binary_xor( co0_var1, co1_var1 );

  const auto co0_var2 = cofactor0( tt, var_index2 );
  const auto co1_var2 = cofactor1( tt, var_index2 );
  const auto dif_var2 = kitty::binary_xor( co0_var2, co1_var2 );

  const auto co11 = cofactor1( cofactor1( tt, var_index1 ), var_index2 );
  const auto co00 = cofactor0( cofactor0( tt, var_index1 ), var_index2 );
  const auto co01 = cofactor1( cofactor0( tt, var_index1 ), var_index2 );
  const auto co10 = cofactor0( cofactor1( tt, var_index1 ), var_index2 );

  if ( equal( dif_var1, dif_var2 ) )
  {

    if ( func )
    {
      kitty::dynamic_truth_table tt_1( dif_var1.num_vars() );
      *func = kitty::binary_xor( kitty::mux_var( var_index1, dif_var1, tt_1 ), co00 );
    }
    return true;
  }

  return false;
}