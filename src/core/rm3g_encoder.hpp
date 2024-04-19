#ifndef RM3G_ENCODER_HPP
#define RM3G_ENCODER_HPP

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
class rm3g_encoder
{
private:
  int level_dist[32]; // How many steps are below a certain level
  int nr_levels;      // The number of levels in the Boolean fence
  int nr_sel_vars;
  int nr_sim_vars;
  int nr_out_vars;
  int nr_ext_vars;

  int total_nr_vars;

  int sel_offset;
  int sim_offset;
  int out_offset;
  int ext_offset;

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
  int tt_size; // local variable if the inputs is less than 3

  std::map<int, std::vector<unsigned>> sel_map;
  int nr_in;

  static const int NR_SIM_TTS = 32;
  std::vector<kitty::dynamic_truth_table> sim_tts{ NR_SIM_TTS };

  /*
   * private functions
   * */
  int get_sel_var( int step_idx, int j, int k ) const
  {

    
  }

  int get_sim_var( const spec& spec, int step_idx, int t ) const
  {
    return sim_offset + spec.tt_size * step_idx + t;
  }

  int get_ext_var( const spec& spec, int step_idx, int sel_var, int t ) const
  {
    return ext_offset + spec.tt_size  * step_idx + sel_var * tt_size + t;
  }

public:
  rm3g_encoder( solver_wrapper& solver )
  {
    //vLits = pabc::Vec_IntAlloc( 128 );
    this->solver = &solver;
  }

  ~rm3g_encoder()
  {
    //pabc::Vec_IntFree( vLits );
  }

  void create_variables( const spec& spec )
  {
    /* number of simulation variables, s_out_in1_in2_in3 */
    sel_map = comput_select_rm3_vars_map( spec.nr_steps, spec.nr_in );
    nr_sel_vars = sel_map.size();

    /* number of truth table simulation variables */
    nr_sim_vars = spec.nr_steps * spec.tt_size;
    nr_ext_vars = nr_sim_vars * nr_sel_vars;

    /* number of output selection variables */
    //nr_out_vars = spec.nr_nontriv * spec.nr_steps;

    /* offsets, this is used to find varibles correspondence */
    sel_offset = 0;
    sim_offset = nr_sel_vars;
    ext_offset = nr_sel_vars + nr_sim_vars;
    //out_offset = nr_sel_vars + nr_sim_vars;

    /* total variables used in SAT formulation */
    total_nr_vars = nr_sel_vars + nr_sim_vars + nr_ext_vars;

    if ( spec.verbosity > 1 )
    {
      printf( "Creating variables (rm3)\n" );
      printf( "nr steps    = %d\n", spec.nr_steps );
      printf( "nr_in       = %d\n", spec.nr_in );
      printf( "nr_sel_vars = %d\n", nr_sel_vars );
      //printf( "nr_out_vars = %d\n", nr_out_vars );
      printf( "nr_ext_vars = %d\n", nr_ext_vars );
      printf( "nr_sim_vars = %d\n", nr_sim_vars );
      printf( "tt_size     = %d\n", spec.tt_size );
      printf( "creating %d total variables\n", total_nr_vars );
    }

    /* declare in the solver */
    solver->set_nr_vars( total_nr_vars );
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
        std::cout << "pLits[" << ctr++ << "]的值是：" << pLits[ctr++] << std::endl;
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

  /* the function works for a single-output function */
  bool fix_output_sim_vars( const spec& spec, int t )
  {
    const auto ilast_step = spec.nr_steps - 1;
    auto outbit = kitty::get_bit( spec[spec.synth_func( 0 )], t + 1 );

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

  void show_variable_correspondence( const spec& spec )
  {
    printf( "**************************************\n" );
    printf( "selection variables \n" );
    
    for ( const auto e : sel_map )
    {
      auto array = e.second;
      // int size = array.size();
      // std::cout << "array.size = " << size << std::endl;
      // for ( const auto& element : array )
      //   std::cout << element << " ";
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

    // printf( "\noutput variables\n\n" );
    // for ( auto h = 0; h < spec.nr_nontriv; h++ )
    // {
    //   for ( int i = 0; i < spec.nr_steps; i++ )
    //   {
    //     printf( "g_%d_%d is %d\n", h + 1, i + spec.nr_in + 1, get_out_var( spec, h, i ) );
    //   }
    // }
    // printf( "**************************************\n" );
  }

  void show_verbose_result()
  {
    for ( auto i = 0u; i < total_nr_vars; i++ )
    {
      printf( "var %d : %d\n", i, solver->var_value( i ) );
    }
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
        printf( "x_%d has inputs: x_%d and x_%d x_%d\n", step_idx + nr_in + 1, first_in, second_in, third_in );
      }
    }

    printf( "========================================"
            "========================================\n" );
  }

  int rm_3( int a, int b, int c ) const
  {
    return ( a & ( ~b ) ) | ( a & c ) | ( ~b & c );
  }

  /*
   * for the select variable s_i_jk
   * */
  bool add_simulation_clause(
      const spec& spec,
      const int t,
      const int i,
      const int j,
      const int k,
      const int l,
      const int sel_var )
  {
    /****************************************************************
     * Initialization
     ***************************************************************/
    int ctr = 0;
    bool ret = true;
    bool flag_clause_true = false;

    pabc::lit ptmp[9];

    ptmp[0] = pabc::Abc_Var2Lit( sel_var, 1 ); // ~s_ijkl
    ptmp[1] = pabc::Abc_Var2Lit( get_sim_var( spec, i, t ), 0 ); //  x_it
    ptmp[2] = pabc::Abc_Var2Lit( get_sim_var( spec, i, t ), 1 ); // ~x_it
    ptmp[3] = pabc::Abc_Var2Lit( get_ext_var( spec, i, sel_var, t ), 0 ); //  c_jklt
    ptmp[4] = pabc::Abc_Var2Lit( get_ext_var( spec, i, sel_var, t ), 1 ); // ~c_jklt

    /****************************************************************
     * first clause
     * ~s_ijkl + ~x_it + c_jklt
     ***************************************************************/
    pLits[ctr++] = ptmp[0];
    pLits[ctr++] = ptmp[2];
    pLits[ctr++] = ptmp[3];

    ret &= solver->add_clause( pLits, pLits + ctr );
    if ( write_cnf_file )
    {
      add_print_clause( clauses, pLits, pLits + ctr );
    }
    if ( print_clause )
    {
      std::cout << "case 0 ";
      print_sat_clause( solver, pLits, pLits + ctr );
    }

    /****************************************************************
     * second clause
     * ~s_ijkl + x_it + ~c_jklt
     ***************************************************************/
    ctr = 0;
    pLits[ctr++] = ptmp[0];
    pLits[ctr++] = ptmp[1];
    pLits[ctr++] = ptmp[4];

    ret &= solver->add_clause( pLits, pLits + ctr );
    if ( write_cnf_file )
    {
      add_print_clause( clauses, pLits, pLits + ctr );
    }
    if ( print_clause )
    {
      std::cout << "case 1 ";
      print_sat_clause( solver, pLits, pLits + ctr );
    }
    /****************************************************************
     * third clause
     * x_jt + ~x_kt
     ***************************************************************/
    ctr = 0;
    flag_clause_true = false;

    if ( j <= spec.nr_in )
    {
      if ( ( ( ( t + 1 ) & ( 1 << ( j - 1 ) ) ) ? 1 : 0 ) == 1 )
      {
        flag_clause_true = true;
      }
    }
    else
    {
      pLits[ctr++] = pabc::Abc_Var2Lit( get_sim_var( spec, j - spec.nr_in - 1, t ), 0 ); // x_jt
    }

    if( k <= spec.nr_in )
    {
      if ( ( ( ( t + 1 ) & ( 1 << ( k - 1 ) ) ) ? 1 : 0 ) == 0 )
      {
        flag_clause_true = true;
      }
    }
    else
    {
      pLits[ctr++] = pabc::Abc_Var2Lit( get_sim_var( spec, k - spec.nr_in - 1, t ), 0 ); // x_kt
    }

    if(ctr!=0)
    {
      if ( !flag_clause_true )
      {

        ret &= solver->add_clause( pLits, pLits + ctr );

        if ( write_cnf_file )
        {
          add_print_clause( clauses, pLits, pLits + ctr );
        }
        if ( print_clause )
        {
          std::cout << "case 2 ";
          print_sat_clause( solver, pLits, pLits + ctr );
        }
      }
    }
    

    /****************************************************************
     * fouth clause
     * x_jt + x_lt
     ***************************************************************/
    ctr = 0;
    flag_clause_true = false;

    if ( j <= spec.nr_in )
    {
      if ( ( ( ( t + 1 ) & ( 1 << ( j - 1 ) ) ) ? 1 : 0 ) == 1 )
      {
        flag_clause_true = true;
      }
    }
    else
    {
      pLits[ctr++] = pabc::Abc_Var2Lit( get_sim_var( spec, j - spec.nr_in - 1, t ), 0 ); // x_jt
    }

    if ( l <= spec.nr_in )
    {
      if ( ( ( ( t + 1 ) & ( 1 << ( l - 1 ) ) ) ? 1 : 0 ) == 1 )
      {
        flag_clause_true = true;
      }
    }
    else
    {
      pLits[ctr++] = pabc::Abc_Var2Lit( get_sim_var( spec, l - spec.nr_in - 1, t ), 0 ); // x_kt
    }

    if ( ctr != 0 )
    {
      if ( !flag_clause_true )
      {

        ret &= solver->add_clause( pLits, pLits + ctr );

        if ( write_cnf_file )
        {
          add_print_clause( clauses, pLits, pLits + ctr );
        }
        if ( print_clause )
        {
          std::cout << "case 3 ";
          print_sat_clause( solver, pLits, pLits + ctr );
        }
      }
    }
    /****************************************************************
     * fifth clause
     * ~x_kt + x_lt
     ***************************************************************/
    ctr = 0;
    flag_clause_true = false;

    if ( k <= spec.nr_in )
    {
      if ( ( ( ( t + 1 ) & ( 1 << ( k - 1 ) ) ) ? 1 : 0 ) == 0 )
      {
        flag_clause_true = true;
      }
    }
    else
    {
      pLits[ctr++] = pabc::Abc_Var2Lit( get_sim_var( spec, k - spec.nr_in - 1, t ), 0 ); // x_jt
    }

    if ( l <= spec.nr_in )
    {
      if ( ( ( ( t + 1 ) & ( 1 << ( l - 1 ) ) ) ? 1 : 0 ) == 1 )
      {
        flag_clause_true = true;
      }
    }
    else
    {
      pLits[ctr++] = pabc::Abc_Var2Lit( get_sim_var( spec, l - spec.nr_in - 1, t ), 0 ); // x_kt
    }

    if ( ctr != 0 )
    {
      if ( !flag_clause_true )
      {

        ret &= solver->add_clause( pLits, pLits + ctr );

        if ( write_cnf_file )
        {
          add_print_clause( clauses, pLits, pLits + ctr );
        }
        if ( print_clause )
        {
          std::cout << "case 4 ";
          print_sat_clause( solver, pLits, pLits + ctr );
        }
      }
    }

    return ret;
  }

  bool create_tt_clauses( const spec& spec, const int t )
  {
    bool ret = true;

    for( const auto svar : sel_map )
    {
      auto s = svar.first;
      auto array = svar.second;

      auto i = array[0];
      auto j = array[1];
      auto k = array[2];
      auto l = array[3];

      ret &= add_simulation_clause( spec, t, i, j, k, l, s );

    }

    ret &= fix_output_sim_vars( spec, t );

    return ret;
  }

  void create_main_clauses( const spec& spec )
  {
    for ( int t = 0; t < spec.tt_size; t++ )
    {
      (void)create_tt_clauses( spec, t );
    }
  }

  bool encoder( const spec& spec )
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

    // if ( !create_output_clauses( spec ) )
    // {
    //   return false;
    // }

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

  void extract_rm3g(const spec& spec, rm3& chain)
  {
    int op_inputs[3] = { 0, 0, 0 };
    //chain.reset( spec.nr_in, 1, spec.nr_steps );
    chain.reset( spec.nr_in, spec.get_nr_out(), spec.nr_steps );

    int svar = 0;
    std::cout << "spec.nr_steps = " << spec.nr_steps << std::endl;

    for ( int i = 0; i < spec.nr_steps; i++ )
    {
      auto num_svar_in_current_step = comput_select_rm3_vars_for_each_step( spec.nr_steps, spec.nr_in, i );

      std::cout << "num_svar_in_current_step = " << num_svar_in_current_step << std::endl;
      // svar的初始值是0,会根据num_svar_in_current_step的值进行更新,svar += num_svar_in_current_step;
      for ( int j = svar; j < svar + num_svar_in_current_step; j++ )
      {
        if ( solver->var_value( j ) )
        {
          std::cout << "svar = " << svar << std::endl;
          std::cout << "j = " << j << std::endl;
          int size1 = sel_map.size();
          std::cout << "sel_map.size = " << size1 << std::endl;
         
          auto array = sel_map[j];
          
          int size = array.size();
          std::cout << "array.size = " << size << std::endl;
          for ( const auto& element : array )
            std::cout << element << " ";
          
          assert( !array.empty() );
          op_inputs[0] = array[1];
          op_inputs[1] = array[2];
          op_inputs[2] = array[3];
          break;
        }
        svar += num_svar_in_current_step;

        chain.set_step( i, op_inputs[0], op_inputs[1], op_inputs[2]);

        if ( spec.verbosity > 2 )
        {
          printf( "[i] Step %d, inputs are:%d%d%d\n", i,  op_inputs[0], op_inputs[1], op_inputs[2] );
        }
      }
    }

    const auto pol = 0;
    const auto tmp = ( ( spec.nr_steps + spec.nr_in ) << 1 ) + pol;
    chain.set_output( 0, tmp );
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
synth_result rm3g_synthesize( spec& spec, rm3& rm3g, solver_wrapper& solver, rm3g_encoder& encoder )
{
  spec.preprocess();

  // The special case when the Boolean chain to be synthesized
  // consists entirely of trivial functions.
  if ( spec.nr_triv == spec.get_nr_out() )
  {
    spec.nr_steps = 0;
    rm3g.reset( spec.get_nr_in(), spec.get_nr_out(), 0 );
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

    if ( !encoder.encoder( spec ) )
    {
      spec.nr_steps++;
      break;
    }

    const auto status = solver.solve( spec.conflict_limit );

    if ( status == success )
    {
      encoder.show_verbose_result();
      encoder.show_variable_correspondence( spec );
      encoder.extract_rm3g( spec, rm3g );
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

synth_result syn_by_rm3g_encoder( spec& spec, rm3& rm3g, solver_wrapper& solver, rm3g_encoder& encoder )
{
  spec.verbosity = 3;
  spec.preprocess();
  // spec.add_colex_clause = true;
  // spec.add_symvar_clause = true;

  spec.nr_steps = spec.initial_steps;

  while ( true )
  {
    solver.restart();

    if ( !encoder.encoder( spec ) )
    {
      spec.nr_steps++;
      break;
    }

    const auto status = solver.solve( spec.conflict_limit );

    if ( status == success )
    {
      encoder.show_variable_correspondence( spec );
      encoder.extract_rm3g( spec, rm3g );
      std::cout << "[i] expressiom: ";
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

  rm3_network rm3_encoder_test(const kitty::dynamic_truth_table& tt)
  {
    bsat_wrapper solver;
    spec spec;
    rm3 rm3g;
    //spec.add_colex_clauses = true;
    //spec.add_symvar_clauses = true;

    auto copy = tt;
    if(copy.num_vars() < 3)
    {
      spec[0] = kitty::extend_to( copy, 3 );
    }
    else
    {
      spec[0] = tt;
    }

    rm3g_encoder encoder( solver );

    if ( syn_by_rm3g_encoder (spec, rm3g, solver, encoder) == success)
    {
      std::cout << std::endl
                << "[i] Success. " << spec.nr_steps << " nodes are required. " << std::endl;
      encoder.show_variable_correspondence( spec );
      encoder.print_solver_state( spec );
    }
    else
    {
      std::cout << "[i] Fail " << std::endl;
    }

    return rm3_from_expr( rm3g.rm3_to_expression(), spec[0].num_vars() );
  }
}
#endif