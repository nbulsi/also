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

#ifndef MIG_THREE_ENCODER_HPP
#define MIG_THREE_ENCODER_HPP

#include <vector>
#include <mockturtle/mockturtle.hpp>

#include "misc.hpp"
#include "m3ig_helper.hpp"

using namespace percy;
using namespace mockturtle;

namespace also
{

  /******************************************************************************
   * The main encoder                                                           *
   ******************************************************************************/
  class mig_three_encoder
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
      pabc::Vec_Int_t* vLits; //dynamic vector of literals

      pabc::lit pLits[2048];
      solver_wrapper* solver;

      int maj_input = 3;

      bool dev = false;

      std::map<int, std::vector<unsigned>> sel_map;

      int level_dist[32]; // How many steps are below a certain level
      int nr_levels; // The number of levels in the Boolean fence

      // There are 4 possible operators for each MIG node:
      // <abc>        (0)
      // <!abc>       (1)
      // <a!bc>       (2)
      // <ab!c>       (3)
      // All other input patterns can be obained from these
      // by output inversion. Therefore we consider
      // them symmetries and do not encode them.
      const int MIG_OP_VARS_PER_STEP = 4;

      //const int NR_SIM_TTS = 32;
      std::vector<kitty::dynamic_truth_table> sim_tts { 32 };

      /*
       * private functions
       * */
      int get_sim_var( const spec& spec, int step_idx, int t ) const
      {
          return sim_offset + spec.tt_size * step_idx + t;
      }

      int get_op_var( const spec& spec, int step_idx, int var_idx) const
      {
        return op_offset + step_idx * MIG_OP_VARS_PER_STEP + var_idx;
      }

      int get_sel_var(const spec& spec, int step_idx, int var_idx) const
      {
        assert(step_idx < spec.nr_steps);
        const auto nr_svars_for_idx = nr_svars_for_step(spec, step_idx);
        assert(var_idx < nr_svars_for_idx);
        auto offset = 0;
        for (int i = 0; i < step_idx; i++)
        {
          offset += nr_svars_for_step(spec, i);
        }
        return sel_offset + offset + var_idx;
      }

      int get_sel_var( const int i, const int j, const int k, const int l ) const
      {
        for( const auto& e : sel_map )
        {
          auto sel_var = e.first;

          auto array   = e.second;

          auto ip = array[0];
          auto jp = array[1];
          auto kp = array[2];
          auto lp = array[3];

          if( i == ip && j == jp && k == kp && l == lp )
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

      int get_res_var(const spec& spec, int step_idx, int res_var_idx) const
      {
        auto offset = 0;
        for (int i = 0; i < step_idx; i++) {
          offset += (nr_svars_for_step(spec, i) + 1) * (1 + 2);
        }

        return res_offset + offset + res_var_idx;
      }

    public:
      mig_three_encoder( solver_wrapper& solver )
      {
        vLits = pabc::Vec_IntAlloc( 128 );
        this->solver = &solver;
      }

      ~mig_three_encoder()
      {
        pabc::Vec_IntFree( vLits );
      }

      void create_variables( const spec& spec )
      {
        /* number of simulation variables, s_out_in1_in2_in3 */
        sel_map = comput_select_vars_map3( spec.nr_steps, spec.nr_in );
        nr_sel_vars = sel_map.size();

        /* number of operators per step */
        nr_op_vars = spec.nr_steps * MIG_OP_VARS_PER_STEP;

        /* number of truth table simulation variables */
        nr_sim_vars = spec.nr_steps * spec.tt_size;

        /* number of output selection variables */
        nr_out_vars = spec.nr_nontriv * spec.nr_steps;

        /* offsets, this is used to find varibles correspondence */
        sel_offset = 0;
        op_offset  = nr_sel_vars;
        sim_offset = nr_sel_vars + nr_op_vars;
        out_offset = nr_sel_vars + nr_op_vars + nr_sim_vars;

        /* total variables used in SAT formulation */
        total_nr_vars = nr_op_vars + nr_sel_vars + nr_sim_vars + nr_out_vars;

        if( spec.verbosity > 1 )
        {
          printf( "Creating variables (mig)\n");
          printf( "nr steps    = %d\n", spec.nr_steps );
          printf( "nr_in       = %d\n", spec.nr_in );
          printf( "nr_sel_vars = %d\n", nr_sel_vars );
          printf( "nr_op_vars  = %d\n", nr_op_vars );
          printf( "nr_out_vars = %d\n", nr_out_vars );
          printf( "nr_sim_vars = %d\n", nr_sim_vars );
          printf( "tt_size     = %d\n", spec.tt_size );
          printf( "creating %d total variables\n", total_nr_vars);
        }

        /* declare in the solver */
        solver->set_nr_vars(total_nr_vars);
      }

      void fence_create_variables( const spec& spec )
      {
        /* number of simulation variables, s_out_in1_in2_in3 */
        nr_sel_vars = 0;
        for (int i = 0; i < spec.nr_steps; i++)
        {
          nr_sel_vars += nr_svars_for_step(spec, i);
        }

        /* number of operators per step */
        nr_op_vars = spec.nr_steps * MIG_OP_VARS_PER_STEP;

        /* number of truth table simulation variables */
        nr_sim_vars = spec.nr_steps * spec.tt_size;

        /* offsets, this is used to find varibles correspondence */
        sel_offset = 0;
        op_offset  = nr_sel_vars;
        sim_offset = nr_sel_vars + nr_op_vars;

        /* total variables used in SAT formulation */
        total_nr_vars = nr_op_vars + nr_sel_vars + nr_sim_vars;

        if( spec.verbosity > 1 )
        {
          printf( "Creating variables (mig)\n");
          printf( "nr steps    = %d\n", spec.nr_steps );
          printf( "nr_in       = %d\n", spec.nr_in );
          printf( "nr_sel_vars = %d\n", nr_sel_vars );
          printf( "nr_op_vars  = %d\n", nr_op_vars );
          printf( "nr_sim_vars = %d\n", nr_sim_vars );
          printf( "tt_size     = %d\n", spec.tt_size );
          printf( "creating %d total variables\n", total_nr_vars);
        }

        /* declare in the solver */
        solver->set_nr_vars(total_nr_vars);
      }

      void cegar_fence_create_variables(const spec& spec)
      {
        nr_op_vars = spec.nr_steps * MIG_OP_VARS_PER_STEP;
        nr_sim_vars = spec.nr_steps * spec.tt_size;

        nr_sel_vars = 0;
        nr_res_vars = 0;
        for (int i = 0; i < spec.nr_steps; i++)
        {
          const auto nr_svars_for_i = nr_svars_for_step(spec, i);
          nr_sel_vars += nr_svars_for_i;
          nr_res_vars += (nr_svars_for_i + 1) * (1 + 2);
        }

        sel_offset = 0;
        res_offset = nr_sel_vars;
        op_offset  = nr_sel_vars + nr_res_vars;
        sim_offset = nr_sel_vars + nr_res_vars + nr_op_vars;
        total_nr_vars = nr_sel_vars + nr_res_vars + nr_op_vars + nr_sim_vars;

        if (spec.verbosity) {
          printf("Creating variables (MIG)\n");
          printf("nr steps = %d\n", spec.nr_steps);
          printf("nr_sel_vars=%d\n", nr_sel_vars);
          printf("nr_res_vars=%d\n", nr_res_vars);
          printf("nr_op_vars = %d\n", nr_op_vars);
          printf("nr_sim_vars = %d\n", nr_sim_vars);
          printf("creating %d total variables\n", total_nr_vars);
        }

        solver->set_nr_vars(total_nr_vars);
      }

      int first_step_on_level(int level) const
      {
        if (level == 0) { return 0; }
        return level_dist[level-1];
      }

      int nr_svars_for_step(const spec& spec, int i) const
      {
        // Determine the level of this step.
        const auto level = get_level(spec, i + spec.nr_in + 1);
        auto nr_svars_for_i = 0;
        assert(level > 0);
        for (auto l = first_step_on_level(level - 1); l < first_step_on_level(level); l++)
        {
          // We select l as fanin 3, so have (l choose 2) options
          // (j,k in {0,...,(l-1)}) left for fanin 1 and 2.
          nr_svars_for_i += (l * (l - 1)) / 2;
        }

        return nr_svars_for_i;
      }

      void update_level_map(const spec& spec, const fence& f)
      {
        nr_levels = f.nr_levels();
        level_dist[0] = spec.nr_in + 1;
        for (int i = 1; i <= nr_levels; i++) {
          level_dist[i] = level_dist[i-1] + f.at(i-1);
        }
      }

      int get_level(const spec& spec, int step_idx) const
      {
        // PIs are considered to be on level zero.
        if (step_idx <= spec.nr_in)
        {
          return 0;
        }
        else if (step_idx == spec.nr_in + 1)
        {
          // First step is always on level one
          return 1;
        }
        for (int i = 0; i <= nr_levels; i++)
        {
          if (level_dist[i] > step_idx)
          {
            return i;
          }
        }
        return -1;
      }

      /// Ensures that each gate has the proper number of fanins.
      bool create_fanin_clauses(const spec& spec)
      {
        auto status = true;

        if (spec.verbosity > 2)
        {
          printf("Creating fanin clauses (mig)\n");
          printf("Nr. clauses = %d (PRE)\n", solver->nr_clauses());
        }

        int svar = 0;
        for (int i = 0; i < spec.nr_steps; i++)
        {
          auto ctr = 0;

          auto num_svar_in_current_step = comput_select_vars_for_each_step3( spec.nr_steps, spec.nr_in, i );

          for( int j = svar; j < svar + num_svar_in_current_step; j++ )
          {
            pLits[ctr++] = pabc::Abc_Var2Lit(j, 0);
          }

          svar += num_svar_in_current_step;

          status &= solver->add_clause(pLits, pLits + ctr);

          if( print_clause ) { print_sat_clause( solver, pLits, pLits + ctr ); }
          if( write_cnf_file ) { add_print_clause( clauses, pLits, pLits + ctr ); }
        }

        if (spec.verbosity > 2)
        {
          printf("Nr. clauses = %d (POST)\n", solver->nr_clauses());
        }

        return status;
      }

      void fence_create_fanin_clauses(const spec& spec)
      {
        for (int i = 0; i < spec.nr_steps; i++)
        {
          const auto nr_svars_for_i = nr_svars_for_step(spec, i);

          for (int j = 0; j < nr_svars_for_i; j++)
          {
            const auto sel_var = get_sel_var(spec, i, j);
            pLits[j] = pabc::Abc_Var2Lit(sel_var, 0);
          }

          const auto res = solver->add_clause(pLits, pLits + nr_svars_for_i);
          //assert(res);
        }
      }

      void show_variable_correspondence( const spec& spec )
      {
        printf( "**************************************\n" );
        printf( "selection variables \n");
        for( const auto e : sel_map )
        {
          auto array = e.second;
          printf( "s_%d_%d%d%d is %d\n", array[0], array[1], array[2], array[3], e.first );
        }

        printf( "\noperators variables\n\n" );
        for( auto i = 0; i < spec.nr_steps; i++ )
        {
          for( auto j = 0; j < MIG_OP_VARS_PER_STEP; j++ )
          {
            printf( "op_%d_%d is %d\n", i + spec.nr_in, j, get_op_var( spec, i, j ) );
          }
        }

        printf( "\nsimulation variables\n\n" );
        for( auto i = 0; i < spec.nr_steps; i++ )
        {
          for( int t = 0; t < spec.tt_size; t++ )
          {
            printf( "tt_%d_%d is %d\n", i + spec.nr_in, t + 1, get_sim_var( spec, i, t ) );
          }
        }

        printf( "\noutput variables\n\n" );
        for( auto h = 0; h < spec.nr_nontriv; h++ )
        {
          for( int i = 0; i < spec.nr_steps; i++ )
          {
            printf( "g_%d_%d is %d\n", h, i + spec.nr_in, get_out_var( spec, h, i ) );
          }
        }
        printf( "**************************************\n" );
      }

      void show_verbose_result()
      {
        for( auto i = 0u; i < total_nr_vars; i++ )
        {
          printf( "var %d : %d\n", i, solver->var_value( i ) );
        }
      }

      /* the function works for a single-output function */
      bool fix_output_sim_vars(const spec& spec, int t)
      {
        const auto ilast_step = spec.nr_steps - 1;
        auto outbit = kitty::get_bit( spec[spec.synth_func(0)], t + 1);

        if ( (spec.out_inv >> spec.synth_func(0) ) & 1 )
        {
          outbit = 1 - outbit;
        }

        const auto sim_var = get_sim_var(spec, ilast_step, t);
        pabc::lit sim_lit = pabc::Abc_Var2Lit(sim_var, 1 - outbit);

        if( print_clause ) { print_sat_clause( solver, &sim_lit, &sim_lit + 1); }
        if( write_cnf_file ) { add_print_clause( clauses, &sim_lit, &sim_lit + 1); }
        return solver->add_clause(&sim_lit, &sim_lit + 1);
      }

      /* for multi-output function */
      bool multi_fix_output_sim_vars( const spec& spec, int h, int step_id, int t )
      {
        auto outbit = kitty::get_bit( spec[spec.synth_func( h )], t + 1 );

        if( ( spec.out_inv >> spec.synth_func( 0 ) ) & 1 )
        {
          outbit = 1 - outbit;
        }

        pLits[0] = pabc::Abc_Var2Lit( get_out_var( spec, h, step_id ), 1 );
        pLits[1] = pabc::Abc_Var2Lit( get_sim_var( spec, step_id, t ), 1 - outbit );

        if( print_clause ) { print_sat_clause( solver, pLits, pLits + 2 ); }

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

        // Every output points to an operand
        if( spec.nr_nontriv > 1 )
        {
          for( int h = 0; h < spec.nr_nontriv; h++ )
          {
            for( int i = 0; i < spec.nr_steps; i++ )
            {
              pabc::Vec_IntSetEntry( vLits, i, pabc::Abc_Var2Lit( get_out_var( spec, h, i ), 0 ) );
            }

            status &= solver->add_clause(
                pabc::Vec_IntArray( vLits ),
                pabc::Vec_IntArray( vLits ) + spec.nr_steps );

            /* print clauses */
            if( print_clause )
            {
              std::cout << "Add clause: ";
              for( int i = 0; i < spec.nr_steps; i++ )
              {
                std::cout << " " << get_out_var( spec, h, i );
              }
              std::cout << std::endl;
            }
          }
        }

        //At least one of the outputs has to refer to the final
        //operator
        const auto last_op = spec.nr_steps - 1;

        for( int h = 0; h < spec.nr_nontriv; h++ )
        {
          pabc::Vec_IntSetEntry( vLits, h, pabc::Abc_Var2Lit( get_out_var( spec, h, last_op ), 0 ) );
        }

        status &= solver->add_clause(
            pabc::Vec_IntArray( vLits ),
            pabc::Vec_IntArray( vLits ) + spec.nr_nontriv );

        if( print_clause )
        {
          std::cout << "Add clause: ";
          for( int h = 0; h < spec.nr_nontriv; h++ )
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
       for( const auto e : set )
       {
         r.push_back( get_op_var( spec, i, e ) );
       }
       return r;
     }

      std::vector<int> get_set_diff( const std::vector<int>& onset )
      {
        std::vector<int> all;
        all.resize( 4 );
        std::iota( all.begin(), all.end(), 0 );

        if( onset.size() == 0 )
        {
          return all;
        }

        std::vector<int> diff;
        std::set_difference(all.begin(), all.end(), onset.begin(), onset.end(),
                            std::inserter(diff, diff.begin()));

        return diff;
      }

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
                                  const int s, //sel var
                                  const std::vector<int> entry, //truth table entry
                                  const std::vector<int> onset, //the entry to make which ops on
                                  const std::vector<int> offset //the entry to make which ops off
                                  )
      {
        int ctr = 0;

        assert( entry.size() == 3 );

        /* truth table computation main */
        if (j <= spec.nr_in)
        {
          if ((((t + 1) & (1 << (j - 1))) ? 1 : 0) != entry[2]) { return true; }
        }
        else
        {
          pLits[ctr++] = pabc::Abc_Var2Lit( get_sim_var(spec, j - spec.nr_in - 1, t), entry[2] );
        }

        if (k <= spec.nr_in)
        {
          if ((((t + 1) & (1 << (k - 1))) ? 1 : 0) != entry[1] ) { return true; }
        }
        else
        {
          pLits[ctr++] = pabc::Abc_Var2Lit( get_sim_var(spec, k - spec.nr_in - 1, t), entry[1] );
        }

        if (l <= spec.nr_in)
        {
          if ((((t + 1) & (1 << (l - 1))) ? 1 : 0) != entry[0] ) { return true; }
        }
        else
        {
          pLits[ctr++] = pabc::Abc_Var2Lit( get_sim_var(spec, l - spec.nr_in - 1, t), entry[0] );
        }

        /********************************************************************************************
         * impossibility clauses, 000 results all offset....
         * *****************************************************************************************/
        if( onset.size() == 0 || offset.size() == 0)
        {
          auto a = ( onset.size() == 0 ) ? 1 : 0;
          pLits[ctr++] = pabc::Abc_Var2Lit(s, 1);
          pLits[ctr++] = pabc::Abc_Var2Lit(get_sim_var(spec, i, t), a);
          auto ret = solver->add_clause(pLits, pLits + ctr);
          if( print_clause ) { print_sat_clause( solver, pLits, pLits + ctr ); }
          if( write_cnf_file ) { add_print_clause( clauses, pLits, pLits + ctr ); }

          return ret;
        }

        int ctr_idx_main = ctr;

        /* output 1 */
        pLits[ctr++] = pabc::Abc_Var2Lit(s, 1 );
        pLits[ctr++] = pabc::Abc_Var2Lit(get_sim_var(spec, i, t), 1);

        for( const auto onvar : onset )
        {
          pLits[ctr++] = pabc::Abc_Var2Lit( onvar, 0 );
        }
        auto ret = solver->add_clause(pLits, pLits + ctr);
        if( print_clause ) { print_sat_clause( solver, pLits, pLits + ctr ); }
        if( write_cnf_file ) { add_print_clause( clauses, pLits, pLits + ctr ); }

        for( const auto offvar : offset )
        {
          pLits[ctr_idx_main + 2] = pabc::Abc_Var2Lit( offvar, 1 );
          ret &= solver->add_clause(pLits, pLits + ctr_idx_main + 3 );

          if( print_clause ) { print_sat_clause( solver, pLits, pLits + ctr_idx_main + 3 ); }
          if( write_cnf_file ) { add_print_clause( clauses, pLits, pLits + ctr_idx_main + 3 ); }
        }

        /* output 0 */
        pLits[ctr_idx_main + 1] = pabc::Abc_Var2Lit(get_sim_var(spec, i, t), 0 );
        ctr = ctr_idx_main + 2;
        for( const auto offvar : offset )
        {
          pLits[ctr++] = pabc::Abc_Var2Lit( offvar, 0 );
        }
        ret = solver->add_clause(pLits, pLits + ctr);
        if( print_clause ) { print_sat_clause( solver, pLits, pLits + ctr ); }
        if( write_cnf_file ) { add_print_clause( clauses, pLits, pLits + ctr ); }

        for( const auto onvar : onset )
        {
          pLits[ctr_idx_main + 2] = pabc::Abc_Var2Lit( onvar, 1 );
          ret &= solver->add_clause(pLits, pLits + ctr_idx_main + 3 );
          if( print_clause ) { print_sat_clause( solver, pLits, pLits + ctr_idx_main + 3 ); }
          if( write_cnf_file ) { add_print_clause( clauses, pLits, pLits + ctr_idx_main + 3 ); }
        }

        assert(ret);

        return ret;
      }

      bool is_element_duplicate( const std::vector<unsigned>& array )
      {
        auto copy = array;
        copy.erase( copy.begin() ); //remove the first element that indicates step index
        auto last = std::unique( copy.begin(), copy.end() );

        return ( last == copy.end() ) ? false : true;
      }

      bool add_consistency_clause_init( const spec& spec, const int t, std::pair<int, std::vector<unsigned>> svar )
      {
        auto ret = true;
        /* for sel val S_i_jkl*/
        auto s      = svar.first;
        auto array  = svar.second;

        auto i = array[0];
        auto j = array[1];
        auto k = array[2];
        auto l = array[3];

        std::map<std::vector<int>, std::vector<int>> input_set_map;
        if( j != 0 ) /* no consts */
        {
          input_set_map = comput_input_and_set_map3( none_const );
        }
        else if( j == 0)
        {
          input_set_map = comput_input_and_set_map3( first_const );
        }
        else
        {
          assert( false && "the selection variable is not supported." );
        }

        /* entrys, onset, offset */
        for( const auto e : input_set_map )
        {
          auto entry  = e.first;
          auto onset  = e.second;
          auto offset = get_set_diff( onset );

          ret &= add_consistency_clause( spec, t, i, j, k, l, s, entry,
                                         idx_to_op_var( spec, onset,  i ),
                                         idx_to_op_var( spec, offset, i ) );
        }

        return ret;
      }

      bool create_tt_clauses(const spec& spec, const int t)
      {
        bool ret = true;

        for( const auto svar : sel_map )
        {
          ret &= add_consistency_clause_init( spec, t, svar );
        }

        //ret &= fix_output_sim_vars(spec, t);
        for( int h = 0; h < spec.nr_nontriv; h++ )
        {
          for( int i = 0; i < spec.nr_steps; i++ )
          {
            ret &= multi_fix_output_sim_vars( spec, h, i, t );
          }
        }

        return ret;
      }

      bool fence_create_tt_clauses(const spec& spec, const int t)
      {
        bool ret = true;

        for (int i = 0; i < spec.nr_steps; i++)
        {
          const auto level = get_level(spec, i + spec.nr_in + 1);
          int ctr = 0;

          for (int l = first_step_on_level(level - 1); l < first_step_on_level(level); l++)
          {
            for (int k = 1; k < l; k++)
            {
              for (int j = 0; j < k; j++)
              {
                const auto sel_var = get_sel_var(spec, i, ctr++);
                std::pair<int, std::vector<unsigned>> v;

                v.first = sel_var;

                v.second.push_back(i);
                v.second.push_back(j);
                v.second.push_back(k);
                v.second.push_back(l);

                ret &= add_consistency_clause_init( spec, t, v );
              }
            }
          }

          assert(ret);
        }

        ret &= fix_output_sim_vars(spec, t);

        return ret;
      }

      void create_main_clauses( const spec& spec )
      {
        for( int t = 0; t < spec.tt_size; t++ )
        {
          (void) create_tt_clauses( spec, t );
        }
      }

      bool fence_create_main_clauses(const spec& spec)
      {
        bool ret = true;
        for (int t = 0; t < spec.tt_size; t++)
        {
          ret &= fence_create_tt_clauses(spec, t);
        }
        return ret;
      }

      //block solution
      bool block_solution(const spec& spec)
      {
        int ctr  = 0;
        int svar = 0;

        for (int i = 0; i < spec.nr_steps; i++)
        {
          auto num_svar_in_current_step = comput_select_vars_for_each_step3( spec.nr_steps, spec.nr_in, i );

          for( int j = svar; j < svar + num_svar_in_current_step; j++ )
          {
            //std::cout << "var: " << j << std::endl;
            if( solver->var_value( j ) )
            {
              pLits[ctr++] = pabc::Abc_Var2Lit(j, 1);
              break;
            }
          }

          svar += num_svar_in_current_step;
        }

        assert(ctr == spec.nr_steps);

        return solver->add_clause(pLits, pLits + ctr);
      }

      bool encode( const spec& spec )
      {
        if( write_cnf_file )
        {
          f = fopen( "out.cnf", "w" );
          if( f == NULL )
          {
            printf( "Cannot open output cnf file\n" );
            assert( false );
          }
          clauses.clear();
        }

        assert( spec.nr_in >= 3 );
        create_variables( spec );

        create_main_clauses( spec );

        if( !create_output_clauses( spec ) )
        {
          return false;
        }

        if( !create_fanin_clauses( spec ) )
        {
          return false;
        }

        if (spec.add_alonce_clauses)
        {
          create_alonce_clauses(spec);
        }

        if (spec.add_colex_clauses)
        {
          create_colex_clauses(spec);
        }

        if (spec.add_lex_func_clauses)
        {
          create_lex_func_clauses(spec);
        }

        if (spec.add_symvar_clauses && !create_symvar_clauses(spec))
        {
          return false;
        }

        if( print_clause )
        {
          show_variable_correspondence( spec );
        }

        if( write_cnf_file )
        {
          to_dimacs( f, solver, clauses );
          fclose( f );
        }

        return true;
      }

      bool encode(const spec& spec, const fence& f)
      {
          assert(spec.nr_in >= 3);
          assert(spec.nr_steps == f.nr_nodes());

          update_level_map(spec, f);
          fence_create_variables(spec);

          if (!fence_create_main_clauses(spec))
          {
            return false;
          }

          if (spec.add_alonce_clauses)
          {
            fence_create_alonce_clauses(spec);
          }

          if (spec.add_colex_clauses)
          {
            fence_create_colex_clauses(spec);
          }

          if (spec.add_lex_func_clauses)
          {
            fence_create_lex_func_clauses(spec);
          }

          if (spec.add_symvar_clauses)
          {
            fence_create_symvar_clauses(spec);
          }

          fence_create_fanin_clauses(spec);

        return true;
      }

      bool cegar_encode( const spec& spec )
      {
        if( write_cnf_file )
        {
          f = fopen( "out.cnf", "w" );
          if( f == NULL )
          {
            printf( "Cannot open output cnf file\n" );
            assert( false );
          }
          clauses.clear();
        }

        assert( spec.nr_in >= 3 );
        create_variables( spec );

        if( !create_fanin_clauses( spec ) )
        {
          return false;
        }

        if( !create_output_clauses( spec ) )
        {
          return false;
        }

        if (spec.add_alonce_clauses)
        {
          create_alonce_clauses(spec);
        }

        if (spec.add_colex_clauses)
        {
          create_colex_clauses(spec);
        }

        if (spec.add_lex_func_clauses)
        {
          create_lex_func_clauses(spec);
        }

        if (spec.add_symvar_clauses && !create_symvar_clauses(spec))
        {
          return false;
        }

        if( print_clause )
        {
          show_variable_correspondence( spec );
        }

        if( write_cnf_file )
        {
          to_dimacs( f, solver, clauses );
          fclose( f );
        }

        return true;
      }

      void create_cardinality_constraints(const spec& spec)
      {
        std::vector<int> svars;
        std::vector<int> rvars;

        for (int i = 0; i < spec.nr_steps; i++)
        {
          svars.clear();
          rvars.clear();
          const auto level = get_level(spec, spec.nr_in + i + 1);
          auto svar_ctr = 0;
          for (int l = first_step_on_level(level - 1); l < first_step_on_level(level); l++)
          {
            for (int k = 1; k < l; k++)
            {
              for (int j = 0; j < k; j++)
              {
                const auto sel_var = get_sel_var(spec, i, svar_ctr++);
                svars.push_back(sel_var);
              }
            }
          }
          assert(svars.size() == nr_svars_for_step(spec, i));
          const auto nr_res_vars = (1 + 2) * (svars.size() + 1);

          for (int j = 0; j < nr_res_vars; j++)
          {
            rvars.push_back(get_res_var(spec, i, j));
          }
          create_cardinality_circuit(solver, svars, rvars, 1);

          // Ensure that the fanin cardinality for each step i
          // is exactly FI.
          const auto fi_var =
            get_res_var(spec, i, svars.size() * (1 + 2) + 1);
          auto fi_lit = pabc::Abc_Var2Lit(fi_var, 0);
          (void)solver->add_clause(&fi_lit, &fi_lit + 1);
        }
      }

      bool cegar_encode(const spec& spec, const fence& f)
      {
          update_level_map(spec, f);
          cegar_fence_create_variables(spec);

          fence_create_fanin_clauses(spec);
          create_cardinality_constraints(spec);

          if (spec.add_alonce_clauses)
          {
            fence_create_alonce_clauses(spec);
          }

          if (spec.add_colex_clauses)
          {
            fence_create_colex_clauses(spec);
          }

          if (spec.add_lex_func_clauses)
          {
            fence_create_lex_func_clauses(spec);
          }

          if (spec.add_symvar_clauses)
          {
            fence_create_symvar_clauses(spec);
          }

          return true;
      }

      void construct_mig( const spec& spec, mig_network& mig )
      {
        //to be implement
      }

      void extract_mig3(const spec& spec, mig3& chain )
      {
        int op_inputs[3] = { 0, 0, 0 };
        chain.reset( spec.nr_in, spec.get_nr_out(), spec.nr_steps );

        int svar = 0;
        for (int i = 0; i < spec.nr_steps; i++)
        {
          int op = 0;
          for (int j = 0; j < MIG_OP_VARS_PER_STEP; j++)
          {
            if ( solver->var_value( get_op_var( spec, i, j ) ) )
            {
              op = j;
              break;
            }
          }

          auto num_svar_in_current_step = comput_select_vars_for_each_step3( spec.nr_steps, spec.nr_in, i );

          for( int j = svar; j < svar + num_svar_in_current_step; j++ )
          {
            if( solver->var_value( j ) )
            {
              auto array = sel_map[j];
              op_inputs[0] = array[1];
              op_inputs[1] = array[2];
              op_inputs[2] = array[3];
              break;
            }
          }

          svar += num_svar_in_current_step;

          chain.set_step(i, op_inputs[0], op_inputs[1], op_inputs[2], op);

          if( spec.verbosity > 2 )
          {
            printf("[i] Step %d performs op %d, inputs are:%d%d%d\n", i, op, op_inputs[0], op_inputs[1], op_inputs[2] );
          }

        }

        //set outputs
        auto triv_count = 0;
        auto nontriv_count = 0;
        for( int h = 0; h < spec.get_nr_out(); h++ )
        {
          if( ( spec.triv_flag >> h ) & 1 )
          {
            chain.set_output( h, ( spec.triv_func( triv_count++ ) << 1 ) + ( ( spec.out_inv >> h ) & 1 ) );
            if( spec.verbosity > 2 )
            {
              printf( "[i] PO %d is a trivial function.\n" );
            }
            continue;
          }

          for( int i = 0; i < spec.nr_steps; i++ )
          {
            if( solver->var_value( get_out_var( spec, nontriv_count, i ) ) )
            {
              chain.set_output( h, (( i + spec.get_nr_in() + 1 ) << 1 ) + (( spec.out_inv >> h ) & 1 ) );

              if( spec.verbosity > 2 )
              {
                printf("[i] PO %d is step %d\n", h, spec.nr_in + i + 1 );
              }
              nontriv_count++;
              break;
            }
          }
        }
      }

      void fence_extract_mig3(const spec& spec, mig3& chain)
      {
        int op_inputs[3] = { 0, 0, 0 };

        chain.reset(spec.nr_in, 1, spec.nr_steps);

        for (int i = 0; i < spec.nr_steps; i++)
        {
          int op = 0;
          for (int j = 0; j < MIG_OP_VARS_PER_STEP; j++)
          {
            if (solver->var_value(get_op_var(spec, i, j)))
            {
              op = j;
              break;
            }
          }

          int ctr = 0;
          const auto level = get_level(spec, spec.nr_in + i + 1);
          for (int l = first_step_on_level(level - 1); l < first_step_on_level(level); l++)
          {
            for (int k = 1; k < l; k++)
            {
              for (int j = 0; j < k; j++)
              {
                const auto sel_var = get_sel_var(spec, i, ctr++);
                if (solver->var_value(sel_var))
                {
                  op_inputs[0] = j;
                  op_inputs[1] = k;
                  op_inputs[2] = l;
                  break;
                }
              }
            }
          }
          chain.set_step(i, op_inputs[0], op_inputs[1], op_inputs[2], op);
        }

        chain.set_output(0,
            ((spec.nr_steps + spec.nr_in) << 1) +
            ((spec.out_inv) & 1));
      }

      /*
       * additional constraints for symmetry breaking
       * */
      void create_alonce_clauses(const spec& spec)
      {
        for (int i = 0; i < spec.nr_steps - 1; i++)
        {
          int ctr = 0;
          const auto idx = spec.nr_in + i + 1;

          for (int ip = i + 1; ip < spec.nr_steps; ip++)
          {
            for (int l = spec.nr_in + i; l <= spec.nr_in + ip; l++)
            {
              for (int k = 1; k < l; k++)
              {
                for (int j = 0; j < k; j++)
                {
                  if (j == idx || k == idx || l == idx)
                  {
                    const auto sel_var = get_sel_var( ip, j, k, l);
                    pLits[ctr++] = pabc::Abc_Var2Lit(sel_var, 0);
                  }
                }
              }
            }
          }
          const auto res = solver->add_clause(pLits, pLits + ctr);
          assert(res);
        }
      }

      void fence_create_alonce_clauses(const spec& spec)
      {
        for (int i = 0; i < spec.nr_steps - 1; i++) {
          auto ctr = 0;
          const auto idx = spec.nr_in + i + 1;
          const auto level = get_level(spec, idx);
          for (int ip = i + 1; ip < spec.nr_steps; ip++) {
            auto levelp = get_level(spec, ip + spec.nr_in + 1);
            assert(levelp >= level);
            if (levelp == level) {
              continue;
            }
            auto svctr = 0;
            for (int l = first_step_on_level(levelp - 1);
                l < first_step_on_level(levelp); l++) {
              for (int k = 1; k < l; k++) {
                for (int j = 0; j < k; j++) {
                  if (j == idx || k == idx || l == idx) {
                    const auto sel_var = get_sel_var(spec, ip, svctr);
                    pLits[ctr++] = pabc::Abc_Var2Lit(sel_var, 0);
                  }
                  svctr++;
                }
              }
            }
            assert(svctr == nr_svars_for_step(spec, ip));
          }
          solver->add_clause(pLits, pLits + ctr);
        }
      }

      void create_colex_clauses(const spec& spec)
      {
        for (int i = 0; i < spec.nr_steps - 1; i++)
        {
          for (int l = 2; l <= spec.nr_in + i; l++)
          {
            for (int k = 1; k < l; k++)
            {
              for (int j = 0; j < k; j++)
              {
                pLits[0] = pabc::Abc_Var2Lit(get_sel_var( i, j, k, l), 1);

                // Cannot have lp < l
                for (int lp = 2; lp < l; lp++)
                {
                  for (int kp = 1; kp < lp; kp++)
                  {
                    for (int jp = 0; jp < kp; jp++)
                    {
                      pLits[1] = pabc::Abc_Var2Lit(get_sel_var( i + 1, jp, kp, lp), 1);
                      const auto res = solver->add_clause(pLits, pLits + 2);
                      assert(res);
                    }
                  }
                }

                // May have lp == l and kp > k
                for (int kp = 1; kp < k; kp++)
                {
                  for (int jp = 0; jp < kp; jp++)
                  {
                    pLits[1] = pabc::Abc_Var2Lit(get_sel_var( i + 1, jp, kp, l), 1);
                    const auto res = solver->add_clause(pLits, pLits + 2);
                    assert(res);
                  }
                }
                // OR lp == l and kp == k
                for (int jp = 0; jp < j; jp++)
                {
                  pLits[1] = pabc::Abc_Var2Lit(get_sel_var( i + 1, jp, k, l), 1);
                  const auto res = solver->add_clause(pLits, pLits + 2);
                  assert(res);
                }
              }
            }
          }
        }
      }

      void fence_create_colex_clauses(const spec& spec)
      {
        for (int i = 0; i < spec.nr_steps - 1; i++)
        {
          const auto level = get_level(spec, i + spec.nr_in + 1);
          const auto levelp = get_level(spec, i + 1 + spec.nr_in + 1);
          int svar_ctr = 0;
          for (int l = first_step_on_level(level-1);
              l < first_step_on_level(level); l++)
          {
            for (int k = 1; k < l; k++)
            {
              for (int j = 0; j < k; j++)
              {
                if (l < 3)
                {
                  svar_ctr++;
                  continue;
                }
                const auto sel_var = get_sel_var(spec, i, svar_ctr);
                pLits[0] = pabc::Abc_Var2Lit(sel_var, 1);
                int svar_ctrp = 0;
                for (int lp = first_step_on_level(levelp - 1);
                    lp < first_step_on_level(levelp); lp++)
                {
                  for (int kp = 1; kp < lp; kp++)
                  {
                    for (int jp = 0; jp < kp; jp++)
                    {
                      if ((lp == l && kp == k && jp < j) || (lp == l && kp < k) || (lp < l))
                      {
                        const auto sel_varp = get_sel_var(spec, i + 1, svar_ctrp);
                        pLits[1] = pabc::Abc_Var2Lit(sel_varp, 1);
                        (void)solver->add_clause(pLits, pLits + 2);
                      }
                      svar_ctrp++;
                    }
                  }
                }
                svar_ctr++;
              }
            }
          }
        }
      }

      void create_lex_func_clauses(const spec& spec)
      {
        for (int i = 0; i < spec.nr_steps - 1; i++)
        {
          for (int l = 2; l <= spec.nr_in + i; l++)
          {
            for (int k = 1; k < l; k++)
            {
              for (int j = 0; j < k; j++)
              {
                pLits[0] = pabc::Abc_Var2Lit(get_sel_var(i, j, k, l), 1);
                pLits[1] = pabc::Abc_Var2Lit(get_sel_var(i + 1, j, k, l), 1);
                pLits[2] = pabc::Abc_Var2Lit(get_op_var(spec, i, 3), 1);
                pLits[3] = pabc::Abc_Var2Lit(get_op_var(spec, i + 1, 3), 1);
                auto status = solver->add_clause(pLits, pLits + 4);
                assert(status);
                pLits[2] = pabc::Abc_Var2Lit(get_op_var(spec, i, 2), 1);
                pLits[3] = pabc::Abc_Var2Lit(get_op_var(spec, i + 1, 0), 0);
                pLits[4] = pabc::Abc_Var2Lit(get_op_var(spec, i + 1, 1), 0);
                status = solver->add_clause(pLits, pLits + 5);
                assert(status);
                pLits[2] = pabc::Abc_Var2Lit(get_op_var(spec, i, 1), 1);
                pLits[3] = pabc::Abc_Var2Lit(get_op_var(spec, i + 1, 0), 0);
                status = solver->add_clause(pLits, pLits + 4);
                assert(status);
                pLits[2] = pabc::Abc_Var2Lit(get_op_var(spec, i, 0), 1);
                status = solver->add_clause(pLits, pLits + 3);
                assert(status);
              }
            }
          }
        }
      }

      void fence_create_lex_func_clauses(const spec& spec)
      {
        for (int i = 0; i < spec.nr_steps - 1; i++)
        {
          const auto level = get_level(spec, spec.nr_in + i + 1);
          const auto levelp = get_level(spec, spec.nr_in + i + 2);
          int svar_ctr = 0;
          for (int l = first_step_on_level(level - 1);
              l < first_step_on_level(level); l++)
          {
            for (int k = 1; k < l; k++)
            {
              for (int j = 0; j < k; j++)
              {
                const auto sel_var = get_sel_var(spec, i, svar_ctr++);
                pLits[0] = pabc::Abc_Var2Lit(sel_var, 1);

                int svar_ctrp = 0;
                for (int lp = first_step_on_level(levelp - 1);
                    lp < first_step_on_level(levelp); lp++)
                {
                  for (int kp = 1; kp < lp; kp++)
                  {
                    for (int jp = 0; jp < kp; jp++)
                    {
                      const auto sel_varp = get_sel_var(spec, i + 1, svar_ctrp++);
                      if (j != jp || k != kp || l != lp)
                      {
                        continue;
                      }
                      pLits[1] = pabc::Abc_Var2Lit(sel_varp, 1);
                      pLits[2] = pabc::Abc_Var2Lit(get_op_var(spec, i, 3), 1);
                      pLits[3] = pabc::Abc_Var2Lit(get_op_var(spec, i + 1, 3), 1);
                      auto status = solver->add_clause(pLits, pLits + 4);
                      assert(status);
                      pLits[2] = pabc::Abc_Var2Lit(get_op_var(spec, i, 2), 1);
                      pLits[3] = pabc::Abc_Var2Lit(get_op_var(spec, i + 1, 0), 0);
                      pLits[4] = pabc::Abc_Var2Lit(get_op_var(spec, i + 1, 1), 0);
                      status = solver->add_clause(pLits, pLits + 5);
                      assert(status);
                      pLits[2] = pabc::Abc_Var2Lit(get_op_var(spec, i, 1), 1);
                      pLits[3] = pabc::Abc_Var2Lit(get_op_var(spec, i + 1, 0), 0);
                      status = solver->add_clause(pLits, pLits + 4);
                      assert(status);
                      pLits[2] = pabc::Abc_Var2Lit(get_op_var(spec, i, 0), 1);
                      status = solver->add_clause(pLits, pLits + 3);
                      assert(status);
                    }
                  }
                }
              }
            }
          }
        }
      }

      bool create_symvar_clauses(const spec& spec)
      {
        for (int q = 2; q <= spec.nr_in; q++)
        {
          for (int p = 1; p < q; p++)
          {
            auto symm = true;
            for (int i = 0; i < spec.nr_nontriv; i++)
            {
              auto f = spec[spec.synth_func(i)];
              if (!(swap(f, p - 1, q - 1) == f))
              {
                symm = false;
                break;
              }
            }
            if (!symm)
            {
              continue;
            }

            for (int i = 1; i < spec.nr_steps; i++)
            {
              for (int l = 2; l <= spec.nr_in + i; l++)
              {
                for (int k = 1; k < l; k++)
                {
                  for (int j = 0; j < k; j++)
                  {
                    if (!(j == q || k == q || l == q) || (j == p || k == p))
                    {
                      continue;
                    }
                    pLits[0] = pabc::Abc_Var2Lit(get_sel_var(i, j, k, l), 1);
                    auto ctr = 1;
                    for (int ip = 0; ip < i; ip++)
                    {
                      for (int lp = 2; lp <= spec.nr_in + ip; lp++)
                      {
                        for (int kp = 1; kp < lp; kp++)
                        {
                          for (int jp = 0; jp < kp; jp++)
                          {
                            if (jp == p || kp == p || lp == p)
                            {
                              pLits[ctr++] = pabc::Abc_Var2Lit(get_sel_var(ip, jp, kp, lp), 0);
                            }
                          }
                        }
                      }
                    }
                    if (!solver->add_clause(pLits, pLits + ctr)) {
                      return false;
                    }
                  }
                }
              }
            }
          }
        }
        return true;
      }

      void fence_create_symvar_clauses(const spec& spec)
      {
        for (int q = 2; q <= spec.nr_in; q++) {
          for (int p = 1; p < q; p++) {
            auto symm = true;
            for (int i = 0; i < spec.nr_nontriv; i++) {
              auto& f = spec[spec.synth_func(i)];
              if (!(swap(f, p - 1, q - 1) == f)) {
                symm = false;
                break;
              }
            }
            if (!symm) {
              continue;
            }
            for (int i = 1; i < spec.nr_steps; i++) {
              const auto level = get_level(spec, i + spec.nr_in + 1);
              int svar_ctr = 0;
              for (int l = first_step_on_level(level - 1);
                  l < first_step_on_level(level); l++) {
                for (int k = 1; k < l; k++) {
                  for (int j = 0; j < k; j++) {
                    if (!(j == q || k == q || l == q) || (j == p || k == p)) {
                      svar_ctr++;
                      continue;
                    }
                    const auto sel_var = get_sel_var(spec, i, svar_ctr);
                    pLits[0] = pabc::Abc_Var2Lit(sel_var, 1);
                    auto ctr = 1;
                    for (int ip = 0; ip < i; ip++) {
                      const auto levelp = get_level(spec, spec.nr_in + ip + 1);
                      auto svar_ctrp = 0;
                      for (int lp = first_step_on_level(levelp - 1);
                          lp < first_step_on_level(levelp); lp++) {
                        for (int kp = 1; kp < lp; kp++) {
                          for (int jp = 0; jp < kp; jp++) {
                            if (jp == p || kp == p || lp == p) {
                              const auto sel_varp = get_sel_var(spec, ip, svar_ctrp);
                              pLits[ctr++] = pabc::Abc_Var2Lit(sel_varp, 0);
                            }
                            svar_ctrp++;
                          }
                        }
                      }
                    }
                    (void)solver->add_clause(pLits, pLits + ctr);
                    svar_ctr++;
                  }
                }
              }
            }
          }
        }
      }
      /* end of symmetry breaking clauses */

      bool is_dirty()
      {
          return dirty;
      }

      void set_dirty(bool _dirty)
      {
          dirty = _dirty;
      }

      void set_print_clause(bool _print_clause)
      {
          print_clause = _print_clause;
      }

      int get_maj_input()
      {
        return maj_input;
      }

      void set_maj_input( int _maj_input )
      {
        maj_input = _maj_input;
      }

  };

/******************************************************************************
 * Public functions                                                           *
 ******************************************************************************/
   synth_result mig_three_synthesize( spec& spec, mig3& mig3, solver_wrapper& solver, mig_three_encoder& encoder )
   {
      spec.preprocess();

      // The special case when the Boolean chain to be synthesized
      // consists entirely of trivial functions.
      if (spec.nr_triv == spec.get_nr_out())
      {
        spec.nr_steps = 0;
        mig3.reset(spec.get_nr_in(), spec.get_nr_out(), 0);
        for (int h = 0; h < spec.get_nr_out(); h++)
        {
          mig3.set_output(h, (spec.triv_func(h) << 1) +
              ((spec.out_inv >> h) & 1));
        }
        return success;
      }

      spec.nr_steps = spec.initial_steps;

      while( true )
      {
        solver.restart();

        if( !encoder.encode( spec ) )
        {
          spec.nr_steps++;
          break;
        }

        const auto status = solver.solve( spec.conflict_limit );

        if( status == success )
        {
          //encoder.show_verbose_result();
          encoder.extract_mig3( spec, mig3 );
          return success;
        }
        else if( status == failure )
        {
          spec.nr_steps++;
          if( spec.nr_steps == 20 )
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


   /* cegar synthesis */
   synth_result mig_three_cegar_synthesize( spec& spec, mig3& mig3, solver_wrapper& solver, mig_three_encoder& encoder )
   {
      spec.preprocess();

      // The special case when the Boolean chain to be synthesized
      // consists entirely of trivial functions.
      if (spec.nr_triv == spec.get_nr_out())
      {
        spec.nr_steps = 0;
        mig3.reset(spec.get_nr_in(), spec.get_nr_out(), 0);
        for (int h = 0; h < spec.get_nr_out(); h++)
        {
          mig3.set_output(h, (spec.triv_func(h) << 1) +
              ((spec.out_inv >> h) & 1));
        }
        return success;
      }

      spec.nr_steps = spec.initial_steps;

      while( true )
      {
        solver.restart();

        if( !encoder.cegar_encode( spec ) )
        {
          spec.nr_steps++;
          continue;
        }

        while( true )
        {
          const auto status = solver.solve( spec.conflict_limit );

          if( status == success )
          {
            encoder.extract_mig3( spec, mig3 );
            auto sim_tt = mig3.simulate()[0];
            auto xot_tt = sim_tt ^ ( spec[0] );
            auto first_one = kitty::find_first_one_bit( xot_tt );
            if( first_one == -1 )
            {
              return success;
            }

            if( !encoder.create_tt_clauses( spec, first_one - 1 ) )
            {
              spec.nr_steps++;
              break;
            }
          }
          else if( status == failure )
          {
            spec.nr_steps++;
            if( spec.nr_steps == 10 )
            {
              break;
            }
            break;
          }
          else
          {
            return timeout;
          }
        }

      }

      return success;
   }

   /* cegar synthesis for approximate computing
    * Given a n-bit truth table, and the allowed error rate, say
    * 10%, then the synthesized tt is acceptable for n * (1 - 10%)
    * * n correct outputs.
    * */
   synth_result mig_three_cegar_approximate_synthesize( spec& spec, mig3& mig3, solver_wrapper& solver, mig_three_encoder& encoder, float error_rate )
   {
      spec.preprocess();

      int error_thres = ( spec.tt_size + 1 ) * error_rate;
      std::cout << " tt_size: " << spec.tt_size + 1
                << " error_thres: " << error_thres << std::endl;

      // The special case when the Boolean chain to be synthesized
      // consists entirely of trivial functions.
      if (spec.nr_triv == spec.get_nr_out())
      {
        spec.nr_steps = 0;
        mig3.reset(spec.get_nr_in(), spec.get_nr_out(), 0);
        for (int h = 0; h < spec.get_nr_out(); h++)
        {
          mig3.set_output(h, (spec.triv_func(h) << 1) +
              ((spec.out_inv >> h) & 1));
        }
        return success;
      }

      spec.nr_steps = spec.initial_steps;

      while( true )
      {
        solver.restart();

        if( !encoder.cegar_encode( spec ) )
        {
          spec.nr_steps++;
          continue;
        }

        while( true )
        {
          const auto status = solver.solve( spec.conflict_limit );

          if( status == success )
          {
            encoder.extract_mig3( spec, mig3 );
            auto sim_tt = mig3.simulate()[0];
            auto xor_tt = sim_tt ^ ( spec[0] );
            auto first_one = kitty::find_first_one_bit( xor_tt );

            auto num_diff = kitty::count_ones( xor_tt );

            std::cout << "[i] step: " << spec.nr_steps << " #errors: " << num_diff << std::endl;

            if( num_diff <= error_thres )
            {
              return success;
            }

            if( !encoder.create_tt_clauses( spec, first_one - 1 ) )
            {
              spec.nr_steps++;
              break;
            }
          }
          else if( status == failure )
          {
            spec.nr_steps++;
            if( spec.nr_steps == 10 )
            {
              break;
            }
            break;
          }
          else
          {
            return timeout;
          }
        }

        std::cout << std::endl;

      }

      return success;
   }

   synth_result next_solution( spec& spec, mig3& mig3, solver_wrapper& solver, mig_three_encoder& encoder )
     {
       //spec.verbosity = 3;
       if (!encoder.is_dirty())
       {
            encoder.set_dirty(true);
            return mig_three_synthesize(spec, mig3, solver, encoder);
        }

       // The special case when the Boolean chain to be synthesized
       // consists entirely of trivial functions.
       // In this case, only one solution exists.
       if (spec.nr_triv == spec.get_nr_out()) {
         return failure;
       }

       if (encoder.block_solution(spec))
       {
         const auto status = solver.solve(spec.conflict_limit);

         if (status == success)
         {
           encoder.extract_mig3(spec, mig3);
           return success;
         }
         else
         {
           return status;
         }
       }

       return failure;
    }

   synth_result mig_three_fence_synthesize(spec& spec, mig3& mig3, solver_wrapper& solver, mig_three_encoder& encoder)
    {
        spec.preprocess();

        // The special case when the Boolean chain to be synthesized
        // consists entirely of trivial functions.
        if (spec.nr_triv == spec.get_nr_out())
        {
        spec.nr_steps = 0;
            mig3.reset(spec.get_nr_in(), spec.get_nr_out(), 0);
            for (int h = 0; h < spec.get_nr_out(); h++)
            {
                mig3.set_output(h, (spec.triv_func(h) << 1) +
                    ((spec.out_inv >> h) & 1));
            }
            return success;
        }

        // As the topological synthesizer decomposes the synthesis
        // problem, to fairly count the total number of conflicts we
        // should keep track of all conflicts in existence checks.
        fence f;
        po_filter<unbounded_generator> g( unbounded_generator(spec.initial_steps), spec.get_nr_out(), 3);
        auto fence_ctr = 0;
        while (true)
        {
            ++fence_ctr;
            g.next_fence(f);
            spec.nr_steps = f.nr_nodes();
            solver.restart();
            if (!encoder.encode(spec, f))
            {
                continue;
            }

            if (spec.verbosity)
            {
                printf("next fence (%d):\n", fence_ctr);
                print_fence(f);
                printf("\n");
                printf("nr_nodes=%d, nr_levels=%d\n", f.nr_nodes(),
                    f.nr_levels());
                for (int i = 0; i < f.nr_levels(); i++) {
                    printf("f[%d] = %d\n", i, f[i]);
                }
            }
            auto status = solver.solve(spec.conflict_limit);
            if (status == success)
            {
              std::cout << " success " << std::endl;
              encoder.fence_extract_mig3(spec, mig3);
              //encoder.show_variable_correspondence( spec );
              //encoder.show_verbose_result();
                return success;
            }
            else if (status == failure)
            {
                continue;
            }
            else
            {
                return timeout;
            }
        }
    }

   synth_result mig_three_cegar_fence_synthesize( spec& spec, mig3& mig3, solver_wrapper& solver, mig_three_encoder& encoder)
   {
     assert(spec.get_nr_in() >= spec.fanin);

     spec.preprocess();

     // The special case when the Boolean chain to be synthesized
     // consists entirely of trivial functions.
     if (spec.nr_triv == spec.get_nr_out()) {
        spec.nr_steps = 0;
       mig3.reset(spec.get_nr_in(), spec.get_nr_out(), 0);
       for (int h = 0; h < spec.get_nr_out(); h++) {
         mig3.set_output(h, (spec.triv_func(h) << 1) +
             ((spec.out_inv >> h) & 1));
       }
       return success;
     }


     fence f;
     po_filter<unbounded_generator> g( unbounded_generator(spec.initial_steps), spec.get_nr_out(), 3);
     int fence_ctr = 0;
     while (true)
     {
       ++fence_ctr;
       g.next_fence(f);
       spec.nr_steps = f.nr_nodes();

       if (spec.verbosity)
       {
         printf("  next fence (%d):\n", fence_ctr);
         print_fence(f);
         printf("\n");
         printf("nr_nodes=%d, nr_levels=%d\n", f.nr_nodes(),
             f.nr_levels());
         for (int i = 0; i < f.nr_levels(); i++) {
           printf("f[%d] = %d\n", i, f[i]);
         }
       }

       solver.restart();
       if (!encoder.cegar_encode(spec, f))
       {
         continue;
       }
       while (true)
       {
         auto status = solver.solve(spec.conflict_limit);
         if (status == success)
         {
           encoder.fence_extract_mig3(spec, mig3);
           auto sim_tt = mig3.simulate()[0];
           //auto sim_tt = encoder.simulate(spec);
           //if (spec.out_inv) {
           //    sim_tt = ~sim_tt;
           //}
           auto xor_tt = sim_tt ^ (spec[0]);
           auto first_one = kitty::find_first_one_bit(xor_tt);
           if (first_one == -1)
           {
             return success;
           }

           if (!encoder.fence_create_tt_clauses(spec, first_one - 1))
           {
             break;
           }
         }
         else if (status == failure)
         {
           break;
         }
         else
         {
           return timeout;
         }
       }
     }
    }

   /* parallel fence-based synthesis */
   /// Performs fence-based parallel synthesis.
   /// One thread generates fences and places them on a concurrent
   /// queue. The remaining threads dequeue fences and try to
   /// synthesize chains with them.
   synth_result parallel_nocegar_mig_three_fence_synthesize( spec& spec, mig3& mig3,
                                                              int num_threads = std::thread::hardware_concurrency() )
    {
        spec.preprocess();

        // The special case when the Boolean chain to be synthesized
        // consists entirely of trivial functions.
        if (spec.nr_triv == spec.get_nr_out()) {
        spec.nr_steps = 0;
            mig3.reset(spec.get_nr_in(), spec.get_nr_out(), 0);
            for (int h = 0; h < spec.get_nr_out(); h++) {
                mig3.set_output(h, (spec.triv_func(h) << 1) +
                    ((spec.out_inv >> h) & 1));
            }
            return success;
        }

        std::vector<std::thread> threads(num_threads);
        moodycamel::ConcurrentQueue<fence> q(num_threads * 3);

        bool finished_generating = false;
        bool* pfinished = &finished_generating;
        bool found = false;
        bool* pfound = &found;
        std::mutex found_mutex;

        spec.fanin = 3;
        spec.nr_steps = spec.initial_steps;
        while (true)
        {
            for (int i = 0; i < num_threads; i++)
            {
              //std::cout << "thread: " << i << std::endl;
                threads[i] = std::thread([&spec, pfinished, pfound, &found_mutex, &mig3, &q]
                    {
                    bmcg_wrapper solver;
                    mig_three_encoder encoder(solver);
                    fence local_fence;

                    while (!(*pfound))
                    {
                        if (!q.try_dequeue(local_fence))
                        {
                            if (*pfinished)
                            {
                                std::this_thread::yield();
                                if (!q.try_dequeue(local_fence))
                                {
                                    break;
                                }
                            }
                            else
                            {
                                std::this_thread::yield();
                                continue;
                            }
                        }

                        if (spec.verbosity)
                        {
                            std::lock_guard<std::mutex> vlock(found_mutex);
                            printf("  next fence:\n");
                            print_fence(local_fence);
                            printf("\n");
                            printf("nr_nodes=%d, nr_levels=%d\n",
                                local_fence.nr_nodes(),
                                local_fence.nr_levels());
                        }

                        synth_result status;
                        solver.restart();
                        if (!encoder.encode(spec, local_fence))
                        {
                            continue;
                        }
                        do
                        {
                            status = solver.solve(10);
                            if (*pfound)
                            {
                                break;
                            }
                            else if (status == success)
                            {
                                std::lock_guard<std::mutex> vlock(found_mutex);
                                if (!(*pfound))
                                {
                                    encoder.fence_extract_mig3(spec, mig3);
                                    *pfound = true;
                                }
                            }
                        } while (status == timeout);
                    }
                });
            }

            generate_fences(spec, q);
            finished_generating = true;

            for (auto& thread : threads)
            {
                thread.join();
            }
            if (found)
            {
                break;
            }
            finished_generating = false;
            spec.nr_steps++;
        }

        return success;
    }

   synth_result parallel_mig_three_fence_synthesize( spec& spec, mig3& mig3,
                                                     int num_threads = std::thread::hardware_concurrency())
     {
        spec.preprocess();

        // The special case when the Boolean chain to be synthesized
        // consists entirely of trivial functions.
        if (spec.nr_triv == spec.get_nr_out())
        {
          spec.nr_steps = 0;
            mig3.reset(spec.get_nr_in(), spec.get_nr_out(), 0);
            for (int h = 0; h < spec.get_nr_out(); h++)
            {
                mig3.set_output(h, (spec.triv_func(h) << 1) +
                    ((spec.out_inv >> h) & 1));
            }
            return success;
        }

        std::vector<std::thread> threads(num_threads);
        moodycamel::ConcurrentQueue<fence> q(num_threads * 3);

        bool finished_generating = false;
        bool* pfinished = &finished_generating;
        bool found = false;
        bool* pfound = &found;
        std::mutex found_mutex;

        spec.nr_rand_tt_assigns = 0;// 2 * spec.get_nr_in();
        spec.fanin = 3;
        spec.nr_steps = spec.initial_steps;
        while (true) {
            for (int i = 0; i < num_threads; i++)
            {
                threads[i] = std::thread([&spec, pfinished, pfound, &found_mutex, &mig3, &q] {
                    also::mig3 local_mig;
                    bmcg_wrapper solver;
                    mig_three_encoder encoder(solver);
                    fence local_fence;

                    while (!(*pfound)) {
                        if (!q.try_dequeue(local_fence)) {
                            if (*pfinished) {
                                std::this_thread::yield();
                                if (!q.try_dequeue(local_fence)) {
                                    break;
                                }
                            } else {
                                std::this_thread::yield();
                                continue;
                            }
                        }

                        if (spec.verbosity)
                        {
                            std::lock_guard<std::mutex> vlock(found_mutex);
                            printf("  next fence:\n");
                            print_fence(local_fence);
                            printf("\n");
                            printf("nr_nodes=%d, nr_levels=%d\n",
                                local_fence.nr_nodes(),
                                local_fence.nr_levels());
                        }

                        synth_result status;
                        solver.restart();
                        if (!encoder.cegar_encode(spec, local_fence)) {
                            continue;
                        }
                        do {
                            status = solver.solve(10);
                            if (*pfound) {
                                break;
                            } else if (status == success) {
                                encoder.fence_extract_mig3(spec, local_mig);
                                auto sim_tt = local_mig.simulate()[0];
                                //auto sim_tt = encoder.simulate(spec);
                                //if (spec.out_inv) {
                                //    sim_tt = ~sim_tt;
                                //}
                                auto xor_tt = sim_tt ^ (spec[0]);
                                auto first_one = kitty::find_first_one_bit(xor_tt);
                                if (first_one != -1) {
                                    if (!encoder.fence_create_tt_clauses(spec, first_one - 1)) {
                                        break;
                                    }
                                    status = timeout;
                                    continue;
                                }
                                std::lock_guard<std::mutex> vlock(found_mutex);
                                if (!(*pfound)) {
                                    encoder.fence_extract_mig3(spec, mig3);
                                    *pfound = true;
                                }
                            }
                        } while (status == timeout);
                    }
                });
            }
            generate_fences(spec, q);
            finished_generating = true;

            for (auto& thread : threads) {
                thread.join();
            }
            if (found) {
                break;
            }
            finished_generating = false;
            spec.nr_steps++;
        }

        return success;
    }

}

#endif
