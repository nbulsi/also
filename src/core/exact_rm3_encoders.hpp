#ifndef EXACT_RM3_ENCODERS_HPP
#define EXACT_RM3_ENCODERS_HPP

#include <kitty/kitty.hpp>
#include <mockturtle/mockturtle.hpp>
#include <percy/percy.hpp>
#include <vector>

#include "misc.hpp"
#include "rm3_help.hpp"

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
  int level_dist[32]; // How many steps are below a certain level
  int nr_levels;      // The number of levels in the Boolean fence
  int nr_sel_vars;
  int nr_sim_vars;
  int nr_out_vars;


  int total_nr_vars;

  int sel_offset;
  int sim_offset;
  int out_offset;

  bool dirty = false;
  bool print_clause = false;
  bool write_cnf_file = false;

  pabc::lit pLits[2048];
  solver_wrapper* solver;

  FILE* f = NULL;
  int num_clauses = 0;
  std::vector<std::vector<int>> clauses;
  pabc::Vec_Int_t* vLits; // dynamic vector of literals

  int rm3_input = 3;

  bool dev = false;

  std::map<int, std::vector<unsigned>> sel_map;
  int nr_in;
  int tt_size; // local variable if the inputs is less than 3

  static const int NR_SIM_TTS = 32;
  std::vector<kitty::dynamic_truth_table> sim_tts{ NR_SIM_TTS };

  int get_sim_var( const spec& spec, int step_idx, int t ) const
  {
    return sim_offset + spec.tt_size * step_idx + t;
  }

  int get_out_var( const spec& spec, int h, int i ) const
  {
    assert( h < spec.nr_nontriv );
    assert( i < spec.nr_steps );

    return out_offset + spec.nr_steps * h + i;
  }

  int get_sel_var(
      const spec& spec,
      const int i,
      const int j,
      const int k,
      const int l ) const
  {
    assert( i < spec.nr_steps );

    int offset = 0;
    for ( int ip = 0; ip < i; ip++ )
    {
      const auto n = spec.nr_in + ip;
      offset += ( n * ( n - 1 ) * ( n - 2 ) ) / 6;
    }

    int svar_ctr = 0;
    for ( int lp = 2; lp < spec.nr_in + i; lp++ )
    {
      for ( int kp = 1; kp < lp; kp++ )
      {
        for ( int jp = 0; jp < kp; jp++ )
        {
          if ( l == lp && k == kp && j == jp )
          {
            return sel_offset + offset + svar_ctr;
          }
          svar_ctr++;
        }
      }
    }

    assert( false && "sel var is not existed" );
    return -1;
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
    assert( sel_map.size() != 0 );
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
    sel_map = comput_select_rm3_vars_map( spec.nr_steps, spec.nr_in );
    nr_sel_vars = sel_map.size();

    /* number of truth table simulation variables */
    nr_sim_vars = spec.nr_steps * spec.tt_size;

    /* number of output selection variables */
    nr_out_vars = spec.nr_nontriv * spec.nr_steps;

    /* offsets, this is used to find varibles correspondence */
    sel_offset = 0;
    // op_offset = nr_sel_vars;
    sim_offset = nr_sel_vars;
    out_offset = nr_sel_vars + nr_sim_vars;

    /* total variables used in SAT formulation */
    total_nr_vars = nr_sel_vars + nr_sim_vars + nr_out_vars;

    if ( spec.verbosity > 1 )
    {
      printf( "Creating variables (rm3)\n" );
      printf( "nr steps    = %d\n", spec.nr_steps );
      printf( "nr_in       = %d\n", spec.nr_in );
      printf( "nr_sel_vars = %d\n", nr_sel_vars );
      printf( "nr_out_vars = %d\n", nr_out_vars );
      printf( "nr_sim_vars = %d\n", nr_sim_vars );
      printf( "tt_size     = %d\n", spec.tt_size );
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
    for ( auto l = first_step_on_level( level - 1 );
          l < first_step_on_level( level ); l++ )
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
      printf( "Creating fanin clauses (RM3)\n" );
      printf( "Nr. clauses = %d (PRE)\n", solver->nr_clauses() );
    }

    int svar = 0;
    for ( int i = 0; i < spec.nr_steps; i++ )
    {
      auto ctr = 0;

      auto num_svar_in_current_step = comput_select_rm3_vars_for_each_step( spec.nr_steps, spec.nr_in, i );

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
      for ( int t = 0; t < spec.tt_size; t++ )
      {
        printf( "tt_%d_%d is %d\n", i + spec.nr_in + 1, t + 1, get_sim_var( spec, i, t ) );
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
    auto outbit = kitty::get_bit( spec[spec.synth_func( 0 )], t + 1 );
    // auto outbit = kitty::get_bit( spec[spec.synth_func( 0 )], t  );

    if ( ( spec.out_inv >> spec.synth_func( 0 ) ) & 1 )
    {
      outbit = 1 - outbit;
    }

    const auto sim_var = get_sim_var( spec, ilast_step, t );
    pabc::lit sim_lit = pabc::Abc_Var2Lit( sim_var, 1 - outbit );

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
    auto outbit = kitty::get_bit( spec[spec.synth_func( h )], t + 1 );

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

  void print_solver_state( const spec& spec )
  {
    printf( "\n" );
    printf( "========================================"
            "========================================\n" );
    printf( "  SOLVER STATE\n\n" );

    printf( "  Nr. variables = %d\n", solver->nr_vars() );
    printf( "  Nr. clauses = %d\n\n", solver->nr_clauses() );

    for ( const auto e : sel_map )
    {
      auto svar_idx = e.first;
      auto step_idx = e.second[0];
      auto first_in = e.second[1];
      auto second_in = e.second[2];
      auto third_in = e.second[3];

      if ( solver->var_value( svar_idx ) )
      {
        printf( "x_%d has inputs: x_%d and x_%d x_%d\n", step_idx, first_in, second_in, third_in );
      }
    }

    printf( "========================================"
            "========================================\n" );
  }

  int rm_3( int a, int b, int c ) const
  {
    return ( a & ( ~b ) ) | ( a & c ) | ( ~b & c );
    //return ( a & b ) | ( a & c ) | ( b & c );
  }

  /*
   * for the select variable S_i_jkl
   * */
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
      if ( ( ( ( t + 1 ) & ( 1 << ( j - 1 ) ) ) ? 1 : 0 ) != a )
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
      if ( ( ( ( t + 1 ) & ( 1 << ( k - 1 ) ) ) ? 1 : 0 ) != (!b) )
      {
        return true;
      }
    }
    else
    {
      //std::cout << "kt" << get_sim_var( spec, k - spec.nr_in - 1, t ) << ( !b ) << std::endl;
      pLits[ctr++] = pabc::Abc_Var2Lit(
          get_sim_var( spec, k - spec.nr_in - 1, t ), ( !b ) );
    }

    if ( l <= spec.nr_in )
    {
      if ( ( ( ( t + 1 ) & ( 1 << ( l - 1 ) ) ) ? 1 : 0 ) != c )
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


  bool is_element_duplicate( const std::vector<unsigned>& array )
  {
    auto copy = array;
    copy.erase( copy.begin() ); // remove the first element that indicates step index
    auto last = std::unique( copy.begin(), copy.end() );

    return ( last == copy.end() ) ? false : true;
  }

  bool create_tt_clauses( const spec& spec, const int t )
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

      //printf( "s_%d_%d%d%d is %d\n", array[0] + spec.nr_in + 1, array[1], array[2], array[3], svar.first );

      //const auto sel_var = get_sel_var( spec, i, j, k, l );

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

    // for ( int i = 0; i < spec.nr_steps; i++ )
    // {
    //   for ( int l = 2; l < spec.nr_in + i; l++ )
    //   {
    //     for ( int k = 1; k < l; k++ )
    //     {
    //       for ( int j = 0; j < k; j++ )
    //       {
    //         // const auto sel_var = get_sel_var( i, j, k, l );
    //         const auto sel_var = get_sel_var( spec, i, j, k, l );
    //         ret &= add_simulation_clause( spec, t, i, j, k, l, 0, 0, 0, sel_var );
    //         ret &= add_simulation_clause( spec, t, i, j, k, l, 0, 0, 1, sel_var );
    //         ret &= add_simulation_clause( spec, t, i, j, k, l, 0, 1, 0, sel_var );
    //         ret &= add_simulation_clause( spec, t, i, j, k, l, 0, 1, 1, sel_var );
    //         ret &= add_simulation_clause( spec, t, i, j, k, l, 1, 0, 0, sel_var );
    //         ret &= add_simulation_clause( spec, t, i, j, k, l, 1, 0, 1, sel_var );
    //         ret &= add_simulation_clause( spec, t, i, j, k, l, 1, 1, 0, sel_var );
    //         ret &= add_simulation_clause( spec, t, i, j, k, l, 1, 1, 1, sel_var );
    //       }
    //     }
    //   }
    //   assert( ret );
    // }

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

  void create_main_clauses( const spec& spec )
  {
    for ( int t = 0; t < spec.tt_size; t++ )
    {
      (void)create_tt_clauses( spec, t );
    }
  }

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

  void extract_rm3( const spec& spec, rm3& chain )
  {
    int op_inputs[3] = { 0, 0, 0 };
    chain.reset( spec.nr_in, spec.get_nr_out(), spec.nr_steps );

    int svar = 0;
    for ( int i = 0; i < spec.nr_steps; i++ )
    {
      auto num_svar_in_current_step = comput_select_rm3_vars_for_each_step( spec.nr_steps, spec.nr_in, i );
      std::cout << "num_svar_in_current_step = " << num_svar_in_current_step << std::endl;
      for ( int j = svar; j < svar + num_svar_in_current_step; j++ )
      {
        if ( solver->var_value( j ) )
        {
          std::cout << "j = " << j << std::endl;
          int size1 = sel_map.size();
          std::cout << "sel_map.size = " << size1 << std::endl;

          auto array = sel_map[j];
          op_inputs[0] = array[1];
          op_inputs[1] = array[2];
          op_inputs[2] = array[3];
          break;
        }
      }

      svar += num_svar_in_current_step;

      chain.set_step( i, op_inputs[0], op_inputs[1], op_inputs[2] );

      if ( spec.verbosity > 2 )
      {
        // printf( "[i] Step %d performs op %d, inputs are:%d%d%d\n", i, op_inputs[0], op_inputs[1], op_inputs[2] );
        printf( "[i] Step %d, inputs are:%d%d%d\n", i, op_inputs[0], op_inputs[1], op_inputs[2] );
      }
    }

    const auto pol = 0;
    const auto tmp = ( ( spec.nr_steps + spec.nr_in ) << 1 ) + pol;
    chain.set_output( 0, tmp );

    // set outputs
    // auto triv_count = 0;
    // auto nontriv_count = 0;
    // for ( int h = 0; h < spec.get_nr_out(); h++ )
    // {
    //   if ( ( spec.triv_flag >> h ) & 1 )
    //   {
    //     chain.set_output( h, ( spec.triv_func( triv_count++ ) << 1 ) + ( ( spec.out_inv >> h ) & 1 ) );
    //     if ( spec.verbosity > 2 )
    //     {
    //       printf( "[i] PO %d is a trivial function.\n" );
    //     }
    //     continue;
    //   }

    //   for ( int i = 0; i < spec.nr_steps; i++ )
    //   {
    //     if ( solver->var_value( get_out_var( spec, nontriv_count, i ) ) )
    //     {
    //       chain.set_output( h, ( ( i + spec.get_nr_in() + 1 ) << 1 ) + ( ( spec.out_inv >> h ) & 1 ) );

    //       if ( spec.verbosity > 2 )
    //       {
    //         printf( "[i] PO %d is step %d\n", h, spec.nr_in + i + 1 );
    //       }
    //       nontriv_count++;
    //       break;
    //     }
    //   }
    // }
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

synth_result rm_three_synthesize( spec& spec, rm3& rm3g, solver_wrapper& solver, rm_three_encoder& encoder )
{
  spec.preprocess();

  // The special case when the Boolean chain to be synthesized
  // consists entirely of trivial functions.
  if ( spec.nr_triv == spec.get_nr_out() )
  {
    spec.nr_steps = 0;
    rm3g.reset( spec.get_nr_in(), spec.get_nr_out(), 0 );
    //rm3g.reset( spec.get_nr_in(), spec.get_nr_out(), spec.nr_steps );
    for ( int h = 0; h < spec.get_nr_out(); h++ )
    {
      rm3g.set_output( h, ( spec.triv_func( h ) << 1 ) +
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
      encoder.show_variable_correspondence( spec );
      encoder.extract_rm3( spec, rm3g );
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

synth_result rm3_syn_by_rm3_encoder( spec& spec, rm3& rm3g, solver_wrapper& solver, rm_three_encoder& encoder )
{
  spec.verbosity = 3;
  spec.preprocess();
  // spec.add_colex_clauses = true;
  // spec.add_symvar_clauses = true;
  // spec.add_alonce_clauses = true;

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
      encoder.show_variable_correspondence( spec );
      encoder.extract_rm3( spec, rm3g );
      std::cout << "[i] expression: ";
      rm3g.to_expression( std::cout );
      return success;
    }
    else if ( status == failure )
    {
      spec.nr_steps++;
    }
    else
    {
      std::cout << "[i] timeout" << std::endl;
      return timeout;
    }
  }

  return success;
}

rm3_network nbu_rm3_encoder_test( const kitty::dynamic_truth_table& tt )
{
  bsat_wrapper solver;
  spec spec;
  rm3 rm3g;
  // spec.add_colex_clauses = true;
  // spec.add_symvar_clauses = true;
  // spec.add_alonce_clauses = true;

  auto copy = tt;
  if ( copy.num_vars() < 3 )
  {
    spec[0] = kitty::extend_to( copy, 3 );
  }
  else
  {
    spec[0] = tt;
  }

  rm_three_encoder encoder( solver );

  if ( rm3_syn_by_rm3_encoder( spec, rm3g, solver, encoder ) == success )
  {
    std::cout << std::endl
              << "[i] Success. " << spec.nr_steps << " nodes are required." << std::endl;
    //encoder.show_variable_correspondence( spec );
    //encoder.print_solver_state( spec );
  }
  else
  {
    std::cout << "[i] Fail " << std::endl;
  }

  return rm3_from_expr( rm3g.rm3_to_expression(), spec[0].num_vars() );
}

} // namespace also
#endif