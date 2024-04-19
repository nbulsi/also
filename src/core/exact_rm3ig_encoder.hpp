/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file mig_three_encoder.hpp
 *
 * @brief enonde SAT formulation to construct a MIG
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef RM_THREE_ENCODER_HPP
#define RM_THREE_ENCODER_HPP

#include <mockturtle/mockturtle.hpp>
#include <vector>
// #include "../networks/rm3/RM3.hpp"
#include "misc.hpp"
#include "rm3ig_helper.hpp"

using namespace percy;
using namespace mockturtle;

namespace also
{

/******************************************************************************
 * The main encoder                                                           *
 ******************************************************************************/
class rm_three_encoder
{
private:
  int nr_sel_vars;
  int nr_sim_vars;
  int nr_op_vars;
  int nr_res_vars;
  int nr_out_vars;

  int sel_offset;
  int sim_offset;
  int op_offset;
  int res_offset;
  int out_offset;

  int total_nr_vars;

  bool dirty = false;
  bool print_clause = false;
  bool write_cnf_file = false;

  FILE* f = NULL;
  int num_clauses = 0;
  std::vector<std::vector<int>> clauses;
  pabc::Vec_Int_t* vLits; // dynamic vector of literals

  pabc::lit pLits[2048];
  solver_wrapper* solver;

  int rm3_input = 3;

  bool dev = false;

  std::map<int, std::vector<unsigned>> sel_map;

  int level_dist[32]; // How many steps are below a certain level
  int nr_levels;      // The number of levels in the Boolean fence

  // There are 4 possible operators for each rm3ig node:
  // <abc>        (0)
  // <!abc>       (1)
  // <a!bc>       (2)
  // <ab!c>       (3)
  // All other input patterns can be obained from these
  // by output inversion. Therefore we consider
  // them symmetries and do not encode them.
  const int RM3IG_OP_VARS_PER_STEP = 4;

  // const int NR_SIM_TTS = 32;
  std::vector<kitty::dynamic_truth_table> sim_tts{ 32 };

  /*
   * private functions
   * */
  int get_sim_var( const spec& spec, int step_idx, int t ) const
  {
    return sim_offset + ( spec.tt_size + 1 ) * step_idx + t;
  }

  int get_op_var( const spec& spec, int step_idx, int var_idx ) const
  {
    return op_offset + step_idx * RM3IG_OP_VARS_PER_STEP + var_idx;
  }

  int get_sel_var( const spec& spec, int step_idx, int var_idx ) const
  {
    assert( step_idx < spec.nr_steps );
    const auto nr_svars_for_idx = nr_svars_for_step( spec, step_idx );
    assert( var_idx < nr_svars_for_idx );
    auto offset = 0;
    for ( int i = 0; i < step_idx; i++ )
    {
      offset += nr_svars_for_step( spec, i );
    }
    return sel_offset + offset + var_idx;
  }

  int get_sel_var( const int i, const int j, const int k, const int l ) const
  {
    for ( const auto& e : sel_map )
    {
      auto sel_var = e.first;

      auto array = e.second;

      auto ip = array[0];
      auto jp = array[1];
      auto kp = array[2];
      auto lp = array[3];

      if ( i == ip && j == jp && k == kp && l == lp )
      {
        return sel_var;
      }
    }

    assert( false && "sel var is not existed" );
    return -1;
  }

  int get_out_var( const spec& spec, int h, int i ) const
  {
    assert( h < spec.nr_nontriv );
    assert( i < spec.nr_steps );

    return out_offset + spec.nr_steps * h + i;
  }

  int get_res_var( const spec& spec, int step_idx, int res_var_idx ) const
  {
    auto offset = 0;
    for ( int i = 0; i < step_idx; i++ )
    {
      offset += ( nr_svars_for_step( spec, i ) + 1 ) * ( 1 + 2 );
    }

    return res_offset + offset + res_var_idx;
  }

public:
  rm_three_encoder( solver_wrapper& solver )
  {
    vLits = pabc::Vec_IntAlloc( 128 );
    this->solver = &solver;
  }

  ~rm_three_encoder()
  {
    pabc::Vec_IntFree( vLits );
  }

  void create_variables( const spec& spec )
  {
    /* number of simulation variables, s_out_in1_in2_in3 */
    sel_map = comput_select_vars_maps( spec.nr_steps, spec.nr_in );
    nr_sel_vars = sel_map.size();

    /* number of operators per step */
    nr_op_vars = spec.nr_steps * RM3IG_OP_VARS_PER_STEP;

    /* number of truth table simulation variables */
    nr_sim_vars = spec.nr_steps * ( spec.tt_size + 1 );

    /* number of output selection variables */
    nr_out_vars = spec.nr_nontriv * spec.nr_steps;

    /* offsets, this is used to find varibles correspondence */
    sel_offset = 0;
    op_offset = nr_sel_vars;
    sim_offset = nr_sel_vars + nr_op_vars;
    out_offset = nr_sel_vars + nr_op_vars + nr_sim_vars;

    /* total variables used in SAT formulation */
    total_nr_vars = nr_op_vars + nr_sel_vars + nr_sim_vars + nr_out_vars;

    if ( spec.verbosity > 1 )
    {
      printf( "Creating variables (rm3ig)\n" );
      printf( "nr steps    = %d\n", spec.nr_steps );
      printf( "nr_in       = %d\n", spec.nr_in );
      printf( "nr_sel_vars = %d\n", nr_sel_vars );
      printf( "nr_op_vars  = %d\n", nr_op_vars );
      printf( "nr_out_vars = %d\n", nr_out_vars );
      printf( "nr_sim_vars = %d\n", nr_sim_vars );
      printf( "tt_size     = %d\n", ( spec.tt_size + 1 ) );
      printf( "creating %d total variables\n", total_nr_vars );
    }

    /* declare in the solver */
    solver->set_nr_vars( total_nr_vars );
  }

  int first_step_on_level( int level ) const
  {
    if ( level == 0 )
    {
      return 0;
    }
    return level_dist[level - 1];
  }

  int nr_svars_for_step( const spec& spec, int i ) const
  {
    // Determine the level of this step.
    const auto level = get_level( spec, i + spec.nr_in + 1 );
    auto nr_svars_for_i = 0;
    assert( level > 0 );
    // 需要改
    for ( auto l = first_step_on_level( level - 1 ); l < first_step_on_level( level ); l++ )
    {
      // We select l as fanin 3, so have (l choose 2) options
      // (j,k in {0,...,(l-1)}) left for fanin 1 and 2.
      nr_svars_for_i += ( l * ( l - 1 ) ) / 2;
    }

    return nr_svars_for_i;
  }

  void update_level_map( const spec& spec, const fence& f )
  {
    nr_levels = f.nr_levels();
    level_dist[0] = spec.nr_in + 1;
    for ( int i = 1; i <= nr_levels; i++ )
    {
      level_dist[i] = level_dist[i - 1] + f.at( i - 1 );
    }
  }

  int get_level( const spec& spec, int step_idx ) const
  {
    // PIs are considered to be on level zero.
    if ( step_idx <= spec.nr_in )
    {
      return 0;
    }
    else if ( step_idx == spec.nr_in + 1 )
    {
      // First step is always on level one
      return 1;
    }
    for ( int i = 0; i <= nr_levels; i++ )
    {
      if ( level_dist[i] > step_idx )
      {
        return i;
      }
    }
    return -1;
  }

  /// Ensures that each gate has the proper number of fanins.
  bool create_fanin_clauses( const spec& spec )
  {
    auto status = true;

    if ( spec.verbosity > 2 )
    {
      printf( "Creating fanin clauses (rm3ig)\n" );
      printf( "Nr. clauses = %d (PRE)\n", solver->nr_clauses() );
    }

    int svar = 0;
    for ( int i = 0; i < spec.nr_steps; i++ )
    {
      auto ctr = 0;

      auto num_svar_in_current_step = comput_select_vars_for_each_steps( spec.nr_steps, spec.nr_in, i );
      // std::cout << "num_svar_in_current_step" << num_svar_in_current_step << std::endl;
      //  svar的初始值是0,会根据num_svar_in_current_step的值进行更新,svar += num_svar_in_current_step;
      for ( int j = svar; j < svar + num_svar_in_current_step; j++ )
      {
        pLits[ctr++] = pabc::Abc_Var2Lit( j, 0 );
      }

      svar += num_svar_in_current_step;

      status &= solver->add_clause( pLits, pLits + ctr );

      if ( print_clause )
      {
        print_sat_clause( solver, pLits, pLits + ctr );
      }
      if ( write_cnf_file )
      {
        add_print_clause( clauses, pLits, pLits + ctr );
      }
    }

    if ( spec.verbosity > 2 )
    {
      printf( "Nr. clauses = %d (POST)\n", solver->nr_clauses() );
    }

    return status;
  }

  void show_variable_correspondence( const spec& spec )
  {
    printf( "**************************************\n" );
    printf( "selection variables \n" );
    for ( const auto e : sel_map )
    {
      auto array = e.second;
      printf( "s_%d_%d%d%d is %d\n", array[0] + spec.nr_in + 1, array[1], array[2], array[3], e.first );
    }

    printf( "\nsimulation variables\n\n" );
    for ( auto i = 0; i < spec.nr_steps; i++ )
    {
      for ( int t = 0; t < spec.tt_size + 1; t++ )
      {
        printf( "tt_%d_%d is %d\n", i + spec.nr_in + 1, t, get_sim_var( spec, i, t ) );
      }
    }

    printf( "\noperators variables\n\n" );
    for ( auto i = 0; i < spec.nr_steps; i++ )
    {
      for ( auto j = 0; j < RM3IG_OP_VARS_PER_STEP; j++ )
      {
        printf( "op_%d_%d is %d\n", i + spec.nr_in + 1, j, get_op_var( spec, i, j ) );
      }
    }

    printf( "\noutput variables\n\n" );
    for ( auto h = 0; h < spec.nr_nontriv; h++ )
    {
      for ( int i = 0; i < spec.nr_steps; i++ )
      {
        printf( "g_%d_%d is %d\n", h + 1, i + spec.nr_in + 1, get_out_var( spec, h, i ) );
      }
    }
    printf( "**************************************\n" );
  }

  void show_verbose_result()
  {
    for ( auto i = 0u; i < total_nr_vars; i++ )
    {
      printf( "var %d : %d\n", i, solver->var_value( i ) );
    }
  }

  /* the function works for a single-output function */
  bool fix_output_sim_vars( const spec& spec, int t )
  {
    const auto ilast_step = spec.nr_steps - 1;
    // std::cout << "spec.synth_func( 0 )" << spec.synth_func( 0 ) << std::endl;
    //  从逻辑函数中获取输出位 outbit 的值
    auto outbit = kitty::get_bit( spec[spec.synth_func( 0 )], t );
    // 检查输出是否需要取反，并根据需要修改 outbit 的值
    if ( ( spec.out_inv >> spec.synth_func( 0 ) ) & 1 )
    {
      outbit = 1 - outbit;
    }

    const auto sim_var = get_sim_var( spec, ilast_step, t );
    pabc::lit sim_lit = pabc::Abc_Var2Lit( sim_var, 1 - outbit ); //  _x_ilast_stept

    if ( print_clause )
    {
      print_sat_clause( solver, &sim_lit, &sim_lit + 1 );
    }
    if ( write_cnf_file )
    {
      add_print_clause( clauses, &sim_lit, &sim_lit + 1 );
    }
    return solver->add_clause( &sim_lit, &sim_lit + 1 );
  }

  /* for multi-output function */
  bool multi_fix_output_sim_vars( const spec& spec, int h, int step_id, int t )
  {
    // std::cout << "spec.synth_func( h )" << spec.synth_func( h ) << std::endl;
    auto outbit = kitty::get_bit( spec[spec.synth_func( h )], t );

    if ( ( spec.out_inv >> spec.synth_func( h ) ) & 1 )
    {
      outbit = 1 - outbit;
    }

    pLits[0] = pabc::Abc_Var2Lit( get_out_var( spec, h, step_id ), 1 );
    pLits[1] = pabc::Abc_Var2Lit( get_sim_var( spec, step_id, t ), 1 - outbit );

    if ( print_clause )
    {
      print_sat_clause( solver, pLits, pLits + 2 );
    }

    return solver->add_clause( pLits, pLits + 2 );
  }

  /*
   * for multi-output functions, create clauses:
   * (1) all outputs show have at least one output var to be
   * true,
   * g_0_3 + g_0_4
   * g_1_3 + g_1_4
   *
   * g_0_4 + g_1_4, at lease one output is the last step
   * function
   * */
  bool create_output_clauses( const spec& spec )
  {
    auto status = true;

    // std::cout << "spec.nr_nontriv" << spec.nr_nontriv << std::endl;
    if ( spec.nr_nontriv > 1 )
    {
      // Every output points to an operand
      for ( int h = 0; h < spec.nr_nontriv; h++ )
      {
        for ( int i = 0; i < spec.nr_steps; i++ )
        {
          pabc::Vec_IntSetEntry( vLits, i, pabc::Abc_Var2Lit( get_out_var( spec, h, i ), 0 ) );
        }

        status &= solver->add_clause(
            pabc::Vec_IntArray( vLits ),
            pabc::Vec_IntArray( vLits ) + spec.nr_steps );

        /* print clauses */
        if ( print_clause )
        {
          std::cout << "Add clause: ";
          for ( int i = 0; i < spec.nr_steps; i++ )
          {
            std::cout << " " << get_out_var( spec, h, i );
          }
          std::cout << std::endl;
        }
      }

      // Every output can have only one be true
      // e.g., g_0_1, g_0_2 can have at most one be true
      // !g_0_1 + !g_0_2
      for ( int h = 0; h < spec.nr_nontriv; h++ )
      {
        for ( int i = 0; i < spec.nr_steps - 1; i++ )
        {
          for ( int j = i + 1; j < spec.nr_steps; j++ )
          {
            pLits[0] = pabc::Abc_Var2Lit( get_out_var( spec, h, i ), 1 );
            pLits[1] = pabc::Abc_Var2Lit( get_out_var( spec, h, j ), 1 );
            status &= solver->add_clause( pLits, pLits + 2 );
          }
        }
      }
    }

    // At least one of the outputs has to refer to the final
    // operator
    const auto last_op = spec.nr_steps - 1;

    for ( int h = 0; h < spec.nr_nontriv; h++ )
    {
      pabc::Vec_IntSetEntry( vLits, h, pabc::Abc_Var2Lit( get_out_var( spec, h, last_op ), 0 ) );
    }

    status &= solver->add_clause(
        pabc::Vec_IntArray( vLits ),
        pabc::Vec_IntArray( vLits ) + spec.nr_nontriv );

    if ( print_clause )
    {
      std::cout << "Add clause: ";
      for ( int h = 0; h < spec.nr_nontriv; h++ )
      {
        std::cout << " " << get_out_var( spec, h, last_op );
      }
      std::cout << std::endl;
    }

    return status;
  }

  std::vector<int> idx_to_op_var( const spec& spec, const std::vector<int>& set, const int i )
  {
    std::vector<int> r;
    for ( const auto e : set )
    {
      r.push_back( get_op_var( spec, i, e ) );
    }
    return r;
  }

  // 获取onset的差集
  std::vector<int> get_set_diff( const std::vector<int>& onset )
  {
    std::vector<int> all;
    all.resize( 4 );
    std::iota( all.begin(), all.end(), 0 );

    if ( onset.size() == 0 )
    {
      return all;
    }

    std::vector<int> diff;
    std::set_difference( all.begin(), all.end(), onset.begin(), onset.end(),
                         std::inserter( diff, diff.begin() ) );

    return diff;
  }

  int rm_3( int a, int b, int c ) const
  {
    return ( a & ( ~b ) ) | ( a & c ) | ( ( ~b ) & c );
    // return ( a & b ) | ( a & c ) | ( b & c );
  }

  bool add_simulation_clause(
      const spec& spec,
      const int t,
      const int i,
      const int j,
      const int k,
      const int l,
      const int c,
      const int b,
      const int a,
      const int sel_var )
  {
    int ctr = 0;
    // assert(j > 0);
    // assert( entry.size() == 3 );

    if ( j <= spec.nr_in )
    {
      if ( ( ( ( t ) & ( 1 << ( j - 1 ) ) ) ? 1 : 0 ) != a )
      {
        return true;
      }
    }
    else
    {
      pLits[ctr++] = pabc::Abc_Var2Lit(
          get_sim_var( spec, j - spec.nr_in - 1, t ), a );
    }

    if ( k <= spec.nr_in )
    {
      if ( ( ( ( t ) & ( 1 << ( k - 1 ) ) ) ? 1 : 0 ) != b )
      {
        return true;
      }
    }
    else
    {
      // std::cout << "kt" << get_sim_var( spec, k - spec.nr_in - 1, t ) << ( !b ) << std::endl;
      pLits[ctr++] = pabc::Abc_Var2Lit(
          get_sim_var( spec, k - spec.nr_in - 1, t ), b );
    }

    if ( l <= spec.nr_in )
    {
      if ( ( ( ( t ) & ( 1 << ( l - 1 ) ) ) ? 1 : 0 ) != c )
      {
        return true;
      }
    }
    else
    {
      pLits[ctr++] = pabc::Abc_Var2Lit(
          get_sim_var( spec, l - spec.nr_in - 1, t ), c );
    }

    if ( sel_var != -1 )
    {
      pLits[ctr++] = pabc::Abc_Var2Lit( sel_var, 1 );
    }

    if ( rm_3( a, b, c ) )
    {
      pLits[ctr++] = pabc::Abc_Var2Lit( get_sim_var( spec, i, t ), 0 );
    }
    else
    {
      pLits[ctr++] = pabc::Abc_Var2Lit( get_sim_var( spec, i, t ), 1 );
    }

    const auto ret = solver->add_clause( pLits, pLits + ctr );
    assert( ret );
    return ret;
  }

  bool create_tt_clauses1( const spec& spec, const int t )
  {
    bool ret = true;

    for ( const auto svar : sel_map )
    {
      auto sel_var = svar.first;
      auto array = svar.second;

      auto i = array[0];
      auto j = array[1];
      auto k = array[2];
      auto l = array[3];

      // printf( "s_%d_%d%d%d is %d\n", array[0] + spec.nr_in + 1, array[1], array[2], array[3], svar.first );

      // const auto sel_var = get_sel_var( spec, i, j, k, l );

      ret &= add_simulation_clause( spec, t, i, j, k, l, 0, 0, 0, sel_var );
      ret &= add_simulation_clause( spec, t, i, j, k, l, 0, 0, 1, sel_var );
      ret &= add_simulation_clause( spec, t, i, j, k, l, 0, 1, 0, sel_var );
      ret &= add_simulation_clause( spec, t, i, j, k, l, 0, 1, 1, sel_var );
      ret &= add_simulation_clause( spec, t, i, j, k, l, 1, 0, 0, sel_var );
      ret &= add_simulation_clause( spec, t, i, j, k, l, 1, 0, 1, sel_var );
      ret &= add_simulation_clause( spec, t, i, j, k, l, 1, 1, 0, sel_var );
      ret &= add_simulation_clause( spec, t, i, j, k, l, 1, 1, 1, sel_var );
      assert( ret );
    }

    ret &= fix_output_sim_vars( spec, t );
    // for ( int h = 0; h < spec.nr_nontriv; h++ )
    // {
    //   for ( int i = 0; i < spec.nr_steps; i++ )
    //   {
    //     ret &= multi_fix_output_sim_vars( spec, h, i, t );
    //   }
    // }

    return ret;
  };

  /*
   * for the select variable S_i_jkl
   * */
  bool add_consistency_clause(
      const spec& spec,
      const int t,
      const int i,
      const int j,
      const int k,
      const int l,
      const int s,                  // sel var
      const std::vector<int> entry, // truth table entry
      const std::vector<int> onset, // the entry to make which ops on
      const std::vector<int> offset // the entry to make which ops off
  )
  {
    int ctr = 0;

    assert( entry.size() == 3 );

    /* truth table computation main */
    // 检查布尔函数的某个特定输入变量在特定真值表t的值是否与给定的布尔函数的真值表中对应位置的值相匹配,如果匹配则继续执行，否则返回 true，表示约束条件不满足
    if ( j <= spec.nr_in )
    {
      if ( ( ( ( t ) & ( 1 << ( j - 1 ) ) ) ? 1 : 0 ) != entry[2] )
      {
        return true;
      }
    }
    else
    {
      pLits[ctr++] = pabc::Abc_Var2Lit( get_sim_var( spec, j - spec.nr_in - 1, t ), entry[2] );
    }

    if ( k <= spec.nr_in )
    {
      if ( ( ( ( t ) & ( 1 << ( k - 1 ) ) ) ? 1 : 0 ) != entry[1] )
      {
        return true;
      }
    }
    else
    {
      pLits[ctr++] = pabc::Abc_Var2Lit( get_sim_var( spec, k - spec.nr_in - 1, t ), entry[1] );
    }

    if ( l <= spec.nr_in )
    {
      if ( ( ( ( t ) & ( 1 << ( l - 1 ) ) ) ? 1 : 0 ) != entry[0] )
      {
        return true;
      }
    }
    else
    {
      pLits[ctr++] = pabc::Abc_Var2Lit( get_sim_var( spec, l - spec.nr_in - 1, t ), entry[0] );
    }

    /********************************************************************************************
     * impossibility clauses, 000 results all offset....
     * *****************************************************************************************/
    if ( onset.size() == 0 || offset.size() == 0 )
    {
      auto a = ( onset.size() == 0 ) ? 1 : 0;
      pLits[ctr++] = pabc::Abc_Var2Lit( s, 1 );                         // ~s_ijkl
      pLits[ctr++] = pabc::Abc_Var2Lit( get_sim_var( spec, i, t ), a ); // ~x_it
      auto ret = solver->add_clause( pLits, pLits + ctr );
      if ( print_clause )
      {
        print_sat_clause( solver, pLits, pLits + ctr );
      }
      if ( write_cnf_file )
      {
        add_print_clause( clauses, pLits, pLits + ctr );
      }

      return ret;
    }

    int ctr_idx_main = ctr;

    /* output 1 */
    pLits[ctr++] = pabc::Abc_Var2Lit( s, 1 );                         // ~s_ijkl
    pLits[ctr++] = pabc::Abc_Var2Lit( get_sim_var( spec, i, t ), 1 ); // ~x_it

    for ( const auto onvar : onset )
    {
      pLits[ctr++] = pabc::Abc_Var2Lit( onvar, 0 );
    }
    auto ret = solver->add_clause( pLits, pLits + ctr );
    if ( print_clause )
    {
      print_sat_clause( solver, pLits, pLits + ctr );
    }
    if ( write_cnf_file )
    {
      add_print_clause( clauses, pLits, pLits + ctr );
    }

    for ( const auto offvar : offset )
    {
      pLits[ctr_idx_main + 2] = pabc::Abc_Var2Lit( offvar, 1 );
      ret &= solver->add_clause( pLits, pLits + ctr_idx_main + 3 );

      if ( print_clause )
      {
        print_sat_clause( solver, pLits, pLits + ctr_idx_main + 3 );
      }
      if ( write_cnf_file )
      {
        add_print_clause( clauses, pLits, pLits + ctr_idx_main + 3 );
      }
    }

    /* output 0 */
    pLits[ctr_idx_main + 1] = pabc::Abc_Var2Lit( get_sim_var( spec, i, t ), 0 ); //  x_it
    ctr = ctr_idx_main + 2;
    for ( const auto offvar : offset )
    {
      pLits[ctr++] = pabc::Abc_Var2Lit( offvar, 0 );
    }
    ret = solver->add_clause( pLits, pLits + ctr );
    if ( print_clause )
    {
      print_sat_clause( solver, pLits, pLits + ctr );
    }
    if ( write_cnf_file )
    {
      add_print_clause( clauses, pLits, pLits + ctr );
    }

    for ( const auto onvar : onset )
    {
      pLits[ctr_idx_main + 2] = pabc::Abc_Var2Lit( onvar, 1 );
      ret &= solver->add_clause( pLits, pLits + ctr_idx_main + 3 );
      if ( print_clause )
      {
        print_sat_clause( solver, pLits, pLits + ctr_idx_main + 3 );
      }
      if ( write_cnf_file )
      {
        add_print_clause( clauses, pLits, pLits + ctr_idx_main + 3 );
      }
    }

    assert( ret );

    return ret;
  }

  bool is_element_duplicate( const std::vector<unsigned>& array )
  {
    auto copy = array;
    copy.erase( copy.begin() ); // remove the first element that indicates step index
    auto last = std::unique( copy.begin(), copy.end() );

    return ( last == copy.end() ) ? false : true;
  }

  bool add_consistency_clause_init( const spec& spec, const int t, std::pair<int, std::vector<unsigned>> svar )
  {
    auto ret = true;
    /* for sel val S_i_jkl*/
    auto s = svar.first;
    auto array = svar.second;

    auto i = array[0];
    auto j = array[1];
    auto k = array[2];
    auto l = array[3];
    // std::cout << "j" << j << "k" << k << "l" << l << std::endl;
    //  这里自动定义了输入的排序，j的索引最小
    std::map<std::vector<int>, std::vector<int>> input_set_map;
    if ( j != 0 && k != 0 ) /* no consts */
    {
      input_set_map = comput_input_and_set_maps( none_const1 );
    }
    else if ( j == 0 )
    {
      input_set_map = comput_input_and_set_maps( first_const1 );
    }
    else if ( k == 0 )
    {
      input_set_map = comput_input_and_set_maps( second_const1 );
    }
    // else if ( l == 0 )
    // {
    //   input_set_map = comput_input_and_set_maps( third_const1 );
    // }
    else
    {
      assert( false && "the selection variable is not supported." );
    }

    /* entrys, onset, offset */
    for ( const auto e : input_set_map )
    {
      auto entry = e.first;
      // int size1 = entry.size();
      // std::cout << "entry = " << size1 << std::endl;
      // for ( const auto& element : entry )
      //   std::cout << "entry = " << element << std::endl;

      auto onset = e.second;
      // int size2 = onset.size();
      // std::cout << "onset = " << size2 << std::endl;
      // for ( const auto& element : onset )
      //   std::cout << element << " " << std::endl;

      auto offset = get_set_diff( onset );

      // auto onset1 = idx_to_op_var( spec, onset, i );
      // std::cout << "onset1 = " << onset1.size() << std::endl;
      //  for ( const auto& element : onset1 )
      //    std::cout << element << " " << std::endl;

      // auto offset1 = idx_to_op_var( spec, offset, i );
      // std::cout << "offset1 = " << offset1.size() << std::endl;
      //  for ( const auto& element : offset1 )
      //    std::cout << element << " " << std::endl;

      ret &= add_consistency_clause( spec, t, i, j, k, l, s, entry,
                                     idx_to_op_var( spec, onset, i ),
                                     idx_to_op_var( spec, offset, i ) );
    }

    return ret;
  }

  bool create_tt_clauses( const spec& spec, const int t )
  {
    bool ret = true;

    for ( const auto svar : sel_map )
    {
      ret &= add_consistency_clause_init( spec, t, svar );
    }

    // ret &= fix_output_sim_vars(spec, t);
    for ( int h = 0; h < spec.nr_nontriv; h++ )
    {
      for ( int i = 0; i < spec.nr_steps; i++ )
      {
        ret &= multi_fix_output_sim_vars( spec, h, i, t );
      }
    }

    return ret;
  }

  void create_main_clauses( const spec& spec )
  {
    for ( int t = 0; t < spec.tt_size + 1; t++ )
    {
      (void)create_tt_clauses( spec, t );
    }
  }

  /// Assumes that a solution has been found by the current encoding.
  /// Blocks the current solution such that the solver is forced to
  /// find different ones (if they exist).
  bool block_solution( const spec& spec )
  {
    int ctr = 0;
    int svar = 0;

    for ( int i = 0; i < spec.nr_steps; i++ )
    {
      auto num_svar_in_current_step = comput_select_vars_for_each_steps( spec.nr_steps, spec.nr_in, i );

      for ( int j = svar; j < svar + num_svar_in_current_step; j++ )
      {
        // std::cout << "var: " << j << std::endl;
        if ( solver->var_value( j ) )
        {
          pLits[ctr++] = pabc::Abc_Var2Lit( j, 1 );
          break;
        }
      }

      svar += num_svar_in_current_step;
    }

    assert( ctr == spec.nr_steps );

    return solver->add_clause( pLits, pLits + ctr );
  }

  /// Encodes specifciation for use in standard synthesis flow.
  bool encode( const spec& spec )
  {
    if ( write_cnf_file )
    {
      f = fopen( "out.cnf", "w" );
      if ( f == NULL )
      {
        printf( "Cannot open output cnf file\n" );
        assert( false );
      }
      clauses.clear();
    }

    assert( spec.nr_in >= 3 );
    create_variables( spec );

    create_main_clauses( spec );

    if ( !create_output_clauses( spec ) )
    {
      return false;
    }

    if ( !create_fanin_clauses( spec ) )
    {
      return false;
    }

    // if ( spec.add_alonce_clauses )
    // {
    //   create_alonce_clauses( spec );
    // }

    // if ( spec.add_colex_clauses )
    // {
    //   create_colex_clauses( spec );
    // }

    // if ( spec.add_lex_func_clauses )
    // {
    //   create_lex_func_clauses( spec );
    // }

    // if ( spec.add_symvar_clauses && !create_symvar_clauses( spec ) )
    // {
    //   return false;
    // }

    if ( print_clause )
    {
      show_variable_correspondence( spec );
    }

    if ( write_cnf_file )
    {
      to_dimacs( f, solver, clauses );
      fclose( f );
    }

    return true;
  }

  void construct_rm3ig( const spec& spec, rm3_network& rm3 )
  {
    // to be implement
  }

  /// Extracts chain from encoded CNF solution.
  void extract_rm3ig( const spec& spec, rm3ig& chain )
  {
    int op_inputs[3] = { 0, 0, 0 };
    chain.reset( spec.nr_in, spec.get_nr_out(), spec.nr_steps );

    int svar = 0;
    for ( int i = 0; i < spec.nr_steps; i++ )
    {
      int op = 0;
      for ( int j = 0; j < RM3IG_OP_VARS_PER_STEP; j++ )
      {
        if ( solver->var_value( get_op_var( spec, i, j ) ) )
        {
          op = j;
          break;
        }
      }

      auto num_svar_in_current_step = comput_select_vars_for_each_steps( spec.nr_steps, spec.nr_in, i );
      // std::cout << "num_svar_in_current_step = " << num_svar_in_current_step << std::endl;
      for ( int j = svar; j < svar + num_svar_in_current_step; j++ )
      {
        if ( solver->var_value( j ) )
        {
          // std::cout << "j = " << j << std::endl;
          // int size1 = sel_map.size();
          // std::cout << "sel_map.size = " << size1 << std::endl;
          auto array = sel_map[j];
          op_inputs[0] = array[1];
          op_inputs[1] = array[2];
          op_inputs[2] = array[3];
          break;
        }
      }

      svar += num_svar_in_current_step;

      chain.set_step( i, op_inputs[0], op_inputs[1], op_inputs[2], op );

      if ( spec.verbosity > 2 )
      {
        printf( "[i] Step %d performs op %d, inputs are:%d%d%d\n", i, op, op_inputs[0], op_inputs[1], op_inputs[2] );
      }
    }

    // set outputs
    auto triv_count = 0;
    auto nontriv_count = 0;
    for ( int h = 0; h < spec.get_nr_out(); h++ )
    {
      if ( ( spec.triv_flag >> h ) & 1 )
      {
        chain.set_output( h, ( spec.triv_func( triv_count++ ) << 1 ) + ( ( spec.out_inv >> h ) & 1 ) );
        if ( spec.verbosity > 2 )
        {
          printf( "[i] PO %d is a trivial function.\n" );
        }
        continue;
      }

      for ( int i = 0; i < spec.nr_steps; i++ )
      {
        if ( solver->var_value( get_out_var( spec, nontriv_count, i ) ) )
        {
          chain.set_output( h, ( ( i + spec.get_nr_in() + 1 ) << 1 ) + ( ( spec.out_inv >> h ) & 1 ) );

          if ( spec.verbosity > 2 )
          {
            printf( "[i] PO %d is step %d\n", h, spec.nr_in + i + 1 );
          }
          nontriv_count++;
          break;
        }
      }
    }
  }

  bool is_dirty()
  {
    return dirty;
  }

  void set_dirty( bool _dirty )
  {
    dirty = _dirty;
  }

  void set_print_clause( bool _print_clause )
  {
    print_clause = _print_clause;
  }

  int get_rm3_input()
  {
    return rm3_input;
  }

  void set_rm3_input( int _rm3_input )
  {
    rm3_input = _rm3_input;
  }
};

/******************************************************************************
 * Public functions                                                           *
 ******************************************************************************/
synth_result rm_three_synthesize( spec& spec, rm3ig& rm3ig, solver_wrapper& solver, rm_three_encoder& encoder )
{
  spec.preprocess();

  // The special case when the Boolean chain to be synthesized
  // consists entirely of trivial functions.
  if ( spec.nr_triv == spec.get_nr_out() )
  {
    spec.nr_steps = 0;
    rm3ig.reset( spec.get_nr_in(), spec.get_nr_out(), 0 );
    for ( int h = 0; h < spec.get_nr_out(); h++ )
    {
      rm3ig.set_output( h, ( spec.triv_func( h ) << 1 ) +
                               ( ( spec.out_inv >> h ) & 1 ) );
    }
    return success;
  }

  spec.nr_steps = spec.initial_steps;

  while ( true )
  {
    solver.restart();

    if ( !encoder.encode( spec ) )
    {
      spec.nr_steps++;
      break;
    }

    const auto status = solver.solve( spec.conflict_limit );

    if ( status == success )
    {
      // encoder.show_verbose_result();
      //encoder.show_variable_correspondence( spec );
      encoder.extract_rm3ig( spec, rm3ig );
      return success;
    }
    else if ( status == failure )
    {
      spec.nr_steps++;
      if ( spec.nr_steps == 20 )
      {
        break;
      }
    }
    else
    {
      return timeout;
    }
  }

  return success;
}

synth_result next_solution( spec& spec, rm3ig& rm3ig, solver_wrapper& solver, rm_three_encoder& encoder )
{
  // spec.verbosity = 3;
  if ( !encoder.is_dirty() )
  {
    encoder.set_dirty( true );
    return rm_three_synthesize( spec, rm3ig, solver, encoder );
  }

  // The special case when the Boolean chain to be synthesized
  // consists entirely of trivial functions.
  // In this case, only one solution exists.
  if ( spec.nr_triv == spec.get_nr_out() )
  {
    return failure;
  }

  if ( encoder.block_solution( spec ) )
  {
    const auto status = solver.solve( spec.conflict_limit );

    if ( status == success )
    {
      encoder.extract_rm3ig( spec, rm3ig );
      return success;
    }
    else
    {
      return status;
    }
  }

  return failure;
}

rm3_network best_rm3ig( const kitty::dynamic_truth_table& tt, const bool& verbose, int& min_gates )
{
  bsat_wrapper solver;
  spec spec;
  also::rm3ig rm3ig;

  auto copy = tt;
  if ( copy.num_vars() < 3 )
  {
    spec[0] = kitty::extend_to( copy, 3 );
  }
  else
  {
    spec[0] = tt;
  }

  also::rm_three_encoder encoder( solver );

  int nr_solutions = 0;

  int min_num_gates = INT_MAX;
  int current_num_gates = 0;
  rm3_network rm3;
  rm3_network best_rm3ig;

  while ( also::next_solution( spec, rm3ig, solver, encoder ) == success )
  {
    // print_all_expr( spec, rm3ig );
    nr_solutions++;
    rm3 = rm3ig_to_rm3ig_network( spec, rm3ig );
    current_num_gates = rm3.num_gates() + mockturtle::num_inverters( rm3 );

    int x;
    x = mockturtle::num_inverters( rm3 );
    std::cout << "反相器个数为" << x << std::endl;

    if ( current_num_gates < min_num_gates )
    {
      min_num_gates = current_num_gates;
      best_rm3ig = rm3;
    }

    if ( verbose )
    {
      printf( "NEW RM3 size: %d\n", current_num_gates );
      also::rm3_to_expression( std::cout, rm3 );
    }
  }

  if ( nr_solutions != 0 )
  {
    printf( "[i]: found rm3_network with minimum number of gates: %d\n", min_num_gates );
    printf( "[i]: expression: " );
    also::rm3_to_expression( std::cout, best_rm3ig );
  }

  min_gates = min_num_gates;

  std::cout << "There are " << nr_solutions << " solutions found." << std::endl;

  return best_rm3ig;
}

} // namespace also

#endif
