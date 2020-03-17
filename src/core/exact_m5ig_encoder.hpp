/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file mig_five_encoder.hpp
 *
 * @brief enonde SAT formulation to construct a MIG
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef MIG_FIVE_ENCODER_HPP
#define MIG_FIVE_ENCODER_HPP

#include <vector>
#include <algorithm>
#include <mockturtle/mockturtle.hpp>

#include "misc.hpp"
#include "m5ig_helper.hpp"

using namespace percy;
using namespace mockturtle;

namespace also
{

  /******************************************************************************
   * The main encoder                                                           *
   ******************************************************************************/
  class mig_five_encoder
  {
    private:
      int nr_sel_vars;
      int nr_sim_vars;
      int nr_op_vars;
      int nr_res_vars;

      int sel_offset;
      int sim_offset;
      int op_offset;
      int res_offset;

      int total_nr_vars;

      bool dirty = false;
      bool print_clause = false;
      bool write_cnf_file = false;

      FILE* f = NULL;
      int num_clauses = 0;
      std::vector<std::vector<int>> clauses;
      
      pabc::lit pLits[65536];
      solver_wrapper* solver;


      int maj_input = 5;

      std::map<int, std::vector<unsigned>> sel_map;
      std::map<int, std::vector<unsigned>> fence_sel_map;
      
      // There are 16 possible operators for each MIG node:
      // <abcde>        (0)
      // <!abcde>       (1)
      // <a!bcde>       (2)
      // <ab!cde>       (3)
      // <abc!de>       (4)
      // <abcd!e>       (5)
      // <!a!bcde>      (6)
      // <!ab!cde>      (7)
      // <!abc!de>      (8)
      // <!abcd!e>      (9)
      // <a!b!cde>      (10)
      // <a!bc!de>      (11)
      // <a!bcd!e>      (12)
      // <ab!c!de>      (13)
      // <ab!cd!e>      (14)
      // <abc!d!e>      (15)
      // All other input patterns can be obained from these
      // by output inversion. Therefore we consider
      // them symmetries and do not encode them.
      const int MIG_OP_VARS_PER_STEP = 16;

      /* get all the ops that make ab complemented, such as <!abcde>, <a!bcde>.... */
      std::array<int, 8> ops_ab_compl{ 1, 2, 7, 8, 9, 10, 11, 12 };
      std::array<int, 8> ops_bc_compl{ 2, 3, 6, 7, 11, 12, 13, 14 };
      std::array<int, 8> ops_cd_compl{ 3, 4, 7, 8, 10, 11, 14, 15 };
      std::array<int, 8> ops_de_compl{ 4, 5, 8, 9, 11, 12, 13, 14 };

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
      
      int get_res_var(const spec& spec, const fence& f, int step_idx, int res_var_idx) const
      {
        auto offset = 0;
        for (int i = 0; i < step_idx; i++) 
        {
          auto nr_svars_for_i = fence_comput_select_vars_for_each_step( spec.nr_steps, spec.nr_in, f, i );
          offset += (nr_svars_for_i + 1) * (1 + 2);
        }

        return res_offset + offset + res_var_idx;
      }

      std::vector<int> get_all_svars_for_i( int step_idx )
      {
        assert( fence_sel_map.size() != 0 );

        std::vector<int> res;

        for( const auto& e : fence_sel_map )
        {
          auto svar = e.first;
          auto idx  = e.second[0];

          if( idx == step_idx )
          {
            res.push_back( svar );
          }

          if( idx > step_idx )
          {
            break;
          }
        }

        return res;
      }

      std::vector<std::pair<int, std::vector<unsigned>>> get_all_svars_map_for_i( 
                                                         const std::map<int, std::vector<unsigned>>& map,
                                                         int step_idx )
      {
        std::vector<std::pair<int, std::vector<unsigned>>> res;

        for( const auto& e : map )
        {
          if( e.second[0] == step_idx )
          {
            res.push_back( e );
          }

          if( e.second[0] > step_idx )
          {
            break;
          }
        }

        return res;
      }

      /*we assume the selection variable s_step_ghjkl */
      std::vector<int> get_all_svars_colex_less_than( const std::map<int, std::vector<unsigned>>& map,
                                                      const int& step_idx, const std::vector<unsigned>& base )
      {
        std::vector<int> res;

        const auto& smap = get_all_svars_map_for_i( map, step_idx ); 

        for( const auto& e : smap )
        {
          auto svar = e.first;

          auto lex_cmp = std::lexicographical_compare( e.second.begin(), e.second.end(),
                                                 base.begin(), base.end() );

          if( lex_cmp )
          {
            res.push_back( svar );
          }
        }

        return res;
      }

    public:
      mig_five_encoder( solver_wrapper& solver )
      {
        this->solver = &solver;
      }

      ~mig_five_encoder()
      {
      }

      void create_variables( const spec& spec )
      {
        /* number of simulation variables, s_out_in1_in2_in3_in4_in5 */
        sel_map = comput_select_vars_map( spec.nr_steps, spec.nr_in );
        nr_sel_vars = sel_map.size();
        
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
          printf( "Creating variables (MIG5)\n");
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
      
      void fence_create_variables( const spec& spec, const fence& f )
      {
        /* number of simulation variables, s_out_in1_in2_in3_in4_in5 */
        fence_sel_map = fence_comput_select_vars_map( spec.nr_steps, spec.nr_in, f );
        nr_sel_vars   = fence_sel_map.size();
        
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
      
      void cegar_fence_create_variables(const spec& spec, const fence& f)
      {
        nr_op_vars  = spec.nr_steps * MIG_OP_VARS_PER_STEP;
        nr_sim_vars = spec.nr_steps * spec.tt_size;
        
        /* number of simulation variables, s_out_in1_in2_in3_in4_in5 */
        fence_sel_map = fence_comput_select_vars_map( spec.nr_steps, spec.nr_in, f );
        nr_sel_vars   = fence_sel_map.size();

        nr_res_vars = 0;
        for (int i = 0; i < spec.nr_steps; i++) 
        {
          const auto nr_svars_for_i = fence_comput_select_vars_for_each_step( spec.nr_steps, spec.nr_in, f, i );

          nr_res_vars += (nr_svars_for_i + 1) * (1 + 2);
        }

        sel_offset = 0;
        res_offset = nr_sel_vars;
        op_offset = nr_sel_vars + nr_res_vars;
        sim_offset = nr_sel_vars + nr_res_vars + nr_op_vars;
        
        total_nr_vars = nr_sel_vars + nr_res_vars + nr_op_vars + nr_sim_vars;

        if (spec.verbosity) {
          printf("Creating variables (cegar fence)\n");
          printf("nr steps = %d\n", spec.nr_steps);
          printf("nr_sel_vars=%d\n", nr_sel_vars);
          printf("nr_res_vars=%d\n", nr_res_vars);
          printf("nr_op_vars = %d\n", nr_op_vars);
          printf("nr_sim_vars = %d\n", nr_sim_vars);
          printf("creating %d total variables\n", total_nr_vars);
        }

        solver->set_nr_vars(total_nr_vars);
      }
        
      /// Ensures that each gate has the proper number of fanins.
      bool create_fanin_clauses(const spec& spec)
      {
        auto status = true;

        if (spec.verbosity > 2) 
        {
          printf("Creating fanin clauses (MIG5)\n");
          printf("Nr. clauses = %d (PRE)\n", solver->nr_clauses());
        }

        int svar = 0;
        for (int i = 0; i < spec.nr_steps; i++) 
        {
          auto ctr = 0;

          auto num_svar_in_current_step = comput_select_vars_for_each_step( spec.nr_steps, spec.nr_in, i );  
          
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
      
      bool fence_create_fanin_clauses(const spec& spec, const fence& f)
      {
        auto status = true;
        int svar = 0;

        for(int i = 0; i < spec.nr_steps; i++ )
        {
          auto ctr = 0;

          auto num_svar_in_current_step = fence_comput_select_vars_for_each_step( spec.nr_steps, spec.nr_in, f, i ); 
          
          for( int j = svar; j < svar + num_svar_in_current_step; j++ )
          {
            pLits[ctr++] = pabc::Abc_Var2Lit(j, 0);
          }

          svar += num_svar_in_current_step;

          status &= solver->add_clause(pLits, pLits + ctr);
        }
        
        return status;
      }

      void show_variable_correspondence( const spec& spec )
      {
        printf( "**************************************\n" );
        printf( "selection variables \n");
        for( const auto e : sel_map )
        {
          auto array = e.second;
          printf( "s_%d_%d%d%d%d%d is %d\n", array[0], array[1], array[2], array[3], array[4], array[5], e.first );
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
        printf( "**************************************\n" );
      }

      void show_verbose_result()
      {
        for( auto i = 0u; i < total_nr_vars; i++ )
        {
          printf( "var %d : %d\n", i, solver->var_value( i ) );
        }
      }

        
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
        all.resize( 16 );
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
       * for the select variable S_i_ghjkl
       * */
      bool add_consistency_clause(
                                  const spec& spec,
                                  const int t,
                                  const int i,
                                  const int g, 
                                  const int h,
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

        assert( entry.size() == 5 );

        /* truth table computation main */
        if (g <= spec.nr_in) 
        {
          if ((((t + 1) & (1 << (g - 1))) ? 1 : 0) != entry[4] ) { return true;}
        } 
        else 
        {
          pLits[ctr++] = pabc::Abc_Var2Lit( get_sim_var(spec, g - spec.nr_in - 1, t), entry[4] );
        }

        if (h <= spec.nr_in) 
        {
          if ((((t + 1) & (1 << (h - 1))) ? 1 : 0) != entry[3] ) { return true; }
        } 
        else 
        {
          pLits[ctr++] = pabc::Abc_Var2Lit( get_sim_var(spec, h - spec.nr_in - 1, t), entry[3] );
        }

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
         * impossibility clauses, 00000 results all offset....
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
        auto last = std::unique( copy.begin(), copy.end() );

        return ( last == copy.end() ) ? false : true;
      }

      bool add_consistency_clause_init( const spec& spec, const int t, std::pair<int, std::vector<unsigned>> svar )
      {
        auto ret = true;
        /* for sel val S_i_ghjkl*/
        auto s      = svar.first;
        auto array  = svar.second;

        auto i = array[0];
        auto g = array[1];
        auto h = array[2];
        auto j = array[3];
        auto k = array[4];
        auto l = array[5];

        std::map<std::vector<int>, std::vector<int>> input_set_map;
        
        auto copy = array;
        copy.erase( copy.begin() ); //remove step_idx
        
        if( g != 0 ) /* no consts */
        {
          /* aabcd, abbcd, ... */
          if( is_element_duplicate( copy ) )
          {
            if( g == h )
            {
              input_set_map = comput_input_and_set_map( ab_equal );
            }
            else if( h == j )
            {
              input_set_map = comput_input_and_set_map( bc_equal );
            }
            else if( j == k )
            {
              input_set_map = comput_input_and_set_map( cd_equal );
            }
            else if( k == l )
            {
              input_set_map = comput_input_and_set_map( de_equal );
            }
            else
            {
              assert( false );
            }
          }
          else
          {
            input_set_map = comput_input_and_set_map( no_const );
          }
        }
        else if( g != h && g == 0 ) /* a_const */
        {
          if( is_element_duplicate( copy ) )
          {
            if( h == j )
            {
              input_set_map = comput_input_and_set_map( a_const_bc_equal );
            }
            else if( j == k )
            {
              input_set_map = comput_input_and_set_map( a_const_cd_equal );
            }
            else if( k == l )
            {
              input_set_map = comput_input_and_set_map( a_const_de_equal );
            }
            else
            {
              assert( false );
            }
          }
          else
          {
            input_set_map = comput_input_and_set_map( a_const );
          }
        }
        else if( g == h && g == 0) /* ab_const */
        {
          if( j == k ) /* ab_const_cd_equal */
          {
            input_set_map = comput_input_and_set_map( ab_const_cd_equal );
          }
          else
          {
            input_set_map = comput_input_and_set_map( ab_const );
          }
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

          ret &= add_consistency_clause( spec, t, i, g, h, j, k, l, s, entry, 
                                         idx_to_op_var( spec, onset, i), 
                                         idx_to_op_var( spec, offset, i) );
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
        
        ret &= fix_output_sim_vars(spec, t);

        return ret;
      }
      
      bool fence_create_tt_clauses(const spec& spec, const int t)
      {
        bool ret = true;
        
        for( const auto svar : fence_sel_map )
        {
          ret &= add_consistency_clause_init( spec, t, svar );
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
        
      //block the usage of some primary input
      bool create_block_input_clause( const int pi_id )
      {
        auto status = true;
        for( const auto e : sel_map )
        {
          auto sel_id = e.first;
          auto a = e.second;

          for( auto i = 1; i <= 5; i++ )
          {
            if( a[i] == pi_id )
            {
              auto ctr = 0;
              pLits[ctr++] = pabc::Abc_Var2Lit( sel_id, 1 );
          
              status &= solver->add_clause(pLits, pLits + ctr);
              if(print_clause) { print_sat_clause( solver, pLits, pLits + ctr ); }
              break;
            }
          }
        }

        return status;
      }

      //block solution
      bool block_solution(const spec& spec)
      {
        int ctr  = 0;
        int svar = 0;
          
        for (int i = 0; i < spec.nr_steps; i++) 
        {
          auto num_svar_in_current_step = comput_select_vars_for_each_step( spec.nr_steps, spec.nr_in, i ); 
          
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

        assert( spec.nr_in >= 5 );
        create_variables( spec );

        create_main_clauses( spec );
        
        if( !create_fanin_clauses( spec ) )
        {
          return false;
        }

        if( !add_svar_op_constraints( spec, sel_map ) )
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
        
        //block input 5 for 4-input function
        //if( !create_block_input_clause( 5 ) )
        //{
        //  return false;
       // }

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
          assert(spec.nr_in >= 5);
          assert(spec.nr_steps == f.nr_nodes());

          fence_create_variables(spec, f);
          
          if (!fence_create_main_clauses(spec)) 
          {
            return false;
          }
          
          if( !add_svar_op_constraints( spec, fence_sel_map ) )
          {
            return false;
          }
          
          if (spec.add_alonce_clauses) 
          {
            fence_create_alonce_clauses( spec, f );
          }
          
          if (spec.add_colex_clauses) 
          {
            fence_create_colex_clauses(spec, f );
          }
          
          fence_create_fanin_clauses(spec, f);

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

        assert( spec.nr_in >= 5 );
        create_variables( spec );
        
        if( !create_fanin_clauses( spec ) )
        {
          return false;
        }
        
        if( !add_svar_op_constraints( spec, sel_map ) )
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
        
        //block input 5 for 4-input function
        //if( !create_block_input_clause( 5 ) )
        //{
        //  return false;
       // }

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
      
      void create_cardinality_constraints(const spec& spec, const fence& f)
      {
        std::vector<int> svars;
        std::vector<int> rvars;

        fence_level flevel( f, spec.nr_in );

        for (int i = 0; i < spec.nr_steps; i++) 
        {
          svars.clear();
          rvars.clear();
          
          svars = get_all_svars_for_i( i );
          
          if( spec.verbosity > 2 )
          {
            const auto level      = flevel.get_level( i + spec.nr_in + 1 );
            const auto first_step = flevel.first_step_on_level( level ) - spec.nr_in - 1;

            auto nr_svars_for_i = fence_comput_select_vars_for_each_step( spec.nr_steps, spec.nr_in, f, first_step );

            assert(svars.size() == nr_svars_for_i );
            std::cout << " i : " << i
                      << " level: " << level
                      << " first step: " << first_step
                      << " nr_svars_for_i: " << nr_svars_for_i 
                      << " svar size: " << svars.size() << std::endl;
          }
          
          const auto nr_res_vars = (1 + 2) * (svars.size() + 1);
          
          for (int j = 0; j < nr_res_vars; j++) 
          {
            rvars.push_back( get_res_var( spec, f, i, j ) );
          }
          
          create_cardinality_circuit( solver, svars, rvars, 1 );

          // Ensure that the fanin cardinality for each step i 
          // is exactly FI.
          const auto fi_var = get_res_var( spec, f, i, svars.size() * (1 + 2) + 1 );
          auto fi_lit = pabc::Abc_Var2Lit( fi_var, 0 );
          (void)solver->add_clause( &fi_lit, &fi_lit + 1 );
        }
      }

      /*
       * add selection variables op constraints
       * For example, <00aab>, we do not allow the op like <00a!ab> appeared, in that case, the output is constant 0
       * also, for <0aabc>, ... <0cdee>, we do not allow the
       * duplicated inputs have the complemented polarities, which
       * make <0a!abc> reduced to <0bc>, since we already have
       * <00bbc> encodeded, this case is redundant
       *
       * At last, <aabcd>, the same principle, <a!abcd> reduced to
       * <bcd> that is equal to <01bcd>
       * */
      bool add_svar_op_constraints( const spec& spec, const std::map<int, std::vector<unsigned>>& map )
      {
        bool ret = true;
        for( const auto& e : map )
        {
          const auto& svar = e.first;
          const auto& step_idx = e.second[0];

          auto copy = e.second;
          copy.erase( copy.begin() ); // remove step_idx
          
          if( is_element_duplicate( copy ) )
          {
            auto g = copy[0];
            auto h = copy[1];
            auto j = copy[2];
            auto k = copy[3];
            auto l = copy[4];

            pLits[0] = pabc::Abc_Var2Lit( svar, 1 );

            if( g == h && g != 0)
            {
              for( const auto& op_idx : ops_ab_compl )
              {
                pLits[1] = pabc::Abc_Var2Lit( get_op_var( spec, step_idx, op_idx ), 1 );
                ret &= solver->add_clause(pLits, pLits + 2);
              }
            }
            else if( h == j )
            {
              for( const auto& op_idx : ops_bc_compl )
              {
                pLits[1] = pabc::Abc_Var2Lit( get_op_var( spec, step_idx, op_idx ), 1 );
                ret &= solver->add_clause(pLits, pLits + 2);
              }
            }
            else if( j == k )
            {
              for( const auto& op_idx : ops_cd_compl )
              {
                pLits[1] = pabc::Abc_Var2Lit( get_op_var( spec, step_idx, op_idx ), 1 );
                ret &= solver->add_clause(pLits, pLits + 2);
              }
            }
            else if( k == l )
            {
              for( const auto& op_idx : ops_de_compl )
              {
                pLits[1] = pabc::Abc_Var2Lit( get_op_var( spec, step_idx, op_idx ), 1 );
                ret &= solver->add_clause(pLits, pLits + 2);
              }
            }
            else
            {
              if( g == h && g == 0 )
              { 
                continue;
              }
              else
              {
                assert( false ); //cannot happen
              }
            }
          }
        }

        return ret;
      }

      bool cegar_encode(const spec& spec, const fence& f)
      {
          cegar_fence_create_variables(spec, f);

          fence_create_fanin_clauses(spec, f);

          create_cardinality_constraints(spec,f);
          
          if( !add_svar_op_constraints( spec, fence_sel_map ) )
          {
            return false;
          }
          
          if (spec.add_alonce_clauses) 
          {
            fence_create_alonce_clauses( spec, f );
          }
          
          if (spec.add_colex_clauses) 
          {
            fence_create_colex_clauses( spec, f );
          }
          
          return true;
      }

      void extract_mig5(const spec& spec, mig5& chain, bool is_cegar )
      {
        int op_inputs[5] = { 0, 0, 0, 0, 0 };
        chain.reset( spec.nr_in, 1, spec.nr_steps );

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

          auto num_svar_in_current_step = comput_select_vars_for_each_step( spec.nr_steps, spec.nr_in, i ); 
          
          for( int j = svar; j < svar + num_svar_in_current_step; j++ )
          {
            if( solver->var_value( j ) )
            {
              auto array = sel_map[j];
              op_inputs[0] = array[1];
              op_inputs[1] = array[2];
              op_inputs[2] = array[3];
              op_inputs[3] = array[4];
              op_inputs[4] = array[5];
              break;
            }
          }
          
          svar += num_svar_in_current_step;

          chain.set_step(i, op_inputs[0], op_inputs[1], op_inputs[2], op_inputs[3], op_inputs[4], op);

          if( spec.verbosity > 2 )
          {
            printf("[i] Step %d performs op %d, inputs are:%d%d%d%d%d\n", i, op, op_inputs[0], op_inputs[1], 
                                                                                              op_inputs[2],
                                                                                              op_inputs[3],
                                                                                              op_inputs[4] );
          }

        }
        
        const auto pol = spec.out_inv ? 1 : 0;
        const auto tmp = ( ( spec.nr_steps + spec.nr_in ) << 1 ) + pol;
        chain.set_output(0, tmp);

        //printf("[i] %d nodes are required\n", spec.nr_steps );

        if( spec.out_inv )
        {
         // printf( "[i] output is inverted\n" );
        }

        if( !is_cegar )
        {
          assert( chain.satisfies_spec( spec ) );
        }
      }
      
      void fence_extract_mig5(const spec& spec, mig5& chain, fence& f, bool is_cegar )
      {
        int op_inputs[5] = { 0, 0, 0, 0, 0 };
        chain.reset( spec.nr_in, 1, spec.nr_steps );

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

          auto num_svar_in_current_step = fence_comput_select_vars_for_each_step( spec.nr_steps, spec.nr_in, f, i ); 

          
          for( int j = svar; j < svar + num_svar_in_current_step; j++ )
          {
            if( solver->var_value( j ) )
            {
              auto array = fence_sel_map[j];
              op_inputs[0] = array[1];
              op_inputs[1] = array[2];
              op_inputs[2] = array[3];
              op_inputs[3] = array[4];
              op_inputs[4] = array[5];
              break;
            }
          }
          
          svar += num_svar_in_current_step;

          chain.set_step(i, op_inputs[0], op_inputs[1], op_inputs[2], op_inputs[3], op_inputs[4], op);

          if( spec.verbosity > 2 )
          {
            printf("[i] Step %d performs op %d, inputs are:%d%d%d%d%d\n", i, op, op_inputs[0], op_inputs[1], 
                                                                                              op_inputs[2],
                                                                                              op_inputs[3],
                                                                                              op_inputs[4] );
          }

        }
        
        const auto pol = spec.out_inv ? 1 : 0;
        const auto tmp = ( ( spec.nr_steps + spec.nr_in ) << 1 ) + pol;
        chain.set_output(0, tmp);

        //printf("[i] %d nodes are required\n", spec.nr_steps );

        if( spec.out_inv )
        {
         // printf( "[i] output is inverted\n" );
        }

        if( !is_cegar )
        {
          assert( chain.satisfies_spec( spec ) );
        }
      }

      /* 
       * additional constraints for symmetry breaking 
       * */
      void create_alonce_clauses(const spec& spec)
      {
        for ( int i = 0; i < spec.nr_steps - 1; i++ ) 
        {
          int ctr = 0;
          const auto idx = spec.nr_in + i + 1;
          
          for( const auto& e : sel_map )
          {
            auto sel_var = e.first;
            auto array   = e.second;

            auto ip = array[0];

            if( ip > i )
            {
              auto g = array[1];
              auto h = array[2];
              auto j = array[3];
              auto k = array[4];
              auto l = array[5];

              if( g == idx || h == idx || j == idx || k == idx || l == idx )
              {
                pLits[ctr++] = pabc::Abc_Var2Lit(sel_var, 0);
              }
            }

          }
          
          const auto res = solver->add_clause(pLits, pLits + ctr);
          assert(res);
        }
      }
      
      void fence_create_alonce_clauses(const spec& spec, const fence& f)
      {
        fence_level flevel( f, spec.nr_in );

        for ( int i = 0; i < spec.nr_steps - 1; i++ ) 
        {
          int ctr = 0;
          const auto idx = spec.nr_in + i + 1;

          auto level = flevel.get_level( idx );
          
          for( const auto& e : fence_sel_map )
          {
            auto sel_var = e.first;
            auto array   = e.second;

            auto ip = array[0];
            auto levelp = flevel.get_level( ip + spec.nr_in + 1 );
            
            if( ip > i && levelp >= level )
            {
              auto g = array[1];
              auto h = array[2];
              auto j = array[3];
              auto k = array[4];
              auto l = array[5];

              if( g == idx || h == idx || j == idx || k == idx || l == idx )
              {
                pLits[ctr++] = pabc::Abc_Var2Lit(sel_var, 0);
              }
            }

          }
          
          const auto res = solver->add_clause(pLits, pLits + ctr);
          assert(res);
        }
      }
      
      void create_colex_clauses(const spec& spec)
      {
        for ( int i = 0; i < spec.nr_steps - 1; i++ ) 
        {
          const auto& smap = get_all_svars_map_for_i( sel_map, i );
          
          for( const auto& e : smap )
          {
            auto sel_var = e.first;
            auto array   = e.second;
            
            pLits[0] = pabc::Abc_Var2Lit(sel_var, 1);

            const auto svars = get_all_svars_colex_less_than( sel_map, i + 1, e.second );
            for( const auto& s : svars )
            {
              pLits[1] = pabc::Abc_Var2Lit(s, 1);
              const auto res = solver->add_clause(pLits, pLits + 2);
              assert(res);
            }
          }
        }
      }
      
      void fence_create_colex_clauses(const spec& spec, const fence& f )
      {
        fence_level flevel( f, spec.nr_in );
      
        for ( int i = 0; i < spec.nr_steps - 1; i++ ) 
        {
          const auto& smap = get_all_svars_map_for_i( fence_sel_map, i );
          
          const auto level  = flevel.get_level( i + spec.nr_in + 1 );
          const auto levelp = flevel.get_level( i + 1 + spec.nr_in + 1 );
         
          if( level == levelp )
          {
            for( const auto& e : smap )
            {
              auto sel_var = e.first;
              auto array   = e.second;

              pLits[0] = pabc::Abc_Var2Lit(sel_var, 1);

              const auto svars = get_all_svars_colex_less_than( fence_sel_map, i + 1, e.second );
              for( const auto& s : svars )
              {
                pLits[1] = pabc::Abc_Var2Lit(s, 1);
                const auto res = solver->add_clause(pLits, pLits + 2);
                assert(res);
              }
            }
          }
        }
      }
      
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
   synth_result mig_five_synthesize( spec& spec, mig5& mig5, solver_wrapper& solver, mig_five_encoder& encoder )
   {
      spec.preprocess();
      
      // The special case when the Boolean chain to be synthesized
      // consists entirely of trivial functions.
      if (spec.nr_triv == spec.get_nr_out()) 
      {
        spec.nr_steps = 0;
        mig5.reset(spec.get_nr_in(), spec.get_nr_out(), 0);
        for (int h = 0; h < spec.get_nr_out(); h++) 
        {
          mig5.set_output(h, (spec.triv_func(h) << 1) +
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
          encoder.extract_mig5( spec, mig5, false );
          return success;
        }
        else if( status == failure )
        {
          spec.nr_steps++;
          if( spec.nr_steps == 10 )
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
   synth_result mig_five_cegar_synthesize( spec& spec, mig5& mig5, solver_wrapper& solver, mig_five_encoder& encoder )
   {
      spec.preprocess();
      
      // The special case when the Boolean chain to be synthesized
      // consists entirely of trivial functions.
      if (spec.nr_triv == spec.get_nr_out()) 
      {
        spec.nr_steps = 0;
        mig5.reset(spec.get_nr_in(), spec.get_nr_out(), 0);
        for (int h = 0; h < spec.get_nr_out(); h++) 
        {
          mig5.set_output(h, (spec.triv_func(h) << 1) +
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
            encoder.extract_mig5( spec, mig5, true );
            auto sim_tt = mig5.simulate()[0];
            auto xot_tt = sim_tt ^ ( spec[0] );
            auto first_one = kitty::find_first_one_bit( xot_tt );
            if( first_one == -1 )
            {
              encoder.extract_mig5( spec, mig5, false );
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
   synth_result mig_five_cegar_approximate_synthesize( spec& spec, mig5& mig5, solver_wrapper& solver, mig_five_encoder& encoder, float error_rate )
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
        mig5.reset(spec.get_nr_in(), spec.get_nr_out(), 0);
        for (int h = 0; h < spec.get_nr_out(); h++) 
        {
          mig5.set_output(h, (spec.triv_func(h) << 1) +
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
            encoder.extract_mig5( spec, mig5, true );
            auto sim_tt = mig5.simulate()[0];
            auto xor_tt = sim_tt ^ ( spec[0] );
            auto first_one = kitty::find_first_one_bit( xor_tt );
            
            auto num_diff = kitty::count_ones( xor_tt );

            std::cout << "[i] step: " << spec.nr_steps << " #errors: " << num_diff << std::endl;

            if( num_diff <= error_thres )
            {
              encoder.extract_mig5( spec, mig5, true );
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
   
   synth_result next_solution( spec& spec, mig5& mig5, solver_wrapper& solver, mig_five_encoder& encoder )
     {
       //spec.verbosity = 3;
       if (!encoder.is_dirty()) 
       {
            encoder.set_dirty(true);
            return mig_five_synthesize(spec, mig5, solver, encoder);
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
           encoder.extract_mig5(spec, mig5, false);
           //std::cout << "[i] simulation tt: " << kitty::to_hex( mig5.simulate()[0] ) << std::endl;
           return success;
         } 
         else 
         {
           return status;
         }
       }

       return failure;
    }
   
   synth_result mig_five_fence_synthesize(spec& spec, mig5& mig5, solver_wrapper& solver, mig_five_encoder& encoder)
    {
        spec.preprocess();

        // The special case when the Boolean chain to be synthesized
        // consists entirely of trivial functions.
        if (spec.nr_triv == spec.get_nr_out()) 
        {
            spec.nr_steps = 0;
            mig5.reset(spec.get_nr_in(), spec.get_nr_out(), 0);
            for (int h = 0; h < spec.get_nr_out(); h++) 
            {
                mig5.set_output(h, (spec.triv_func(h) << 1) +
                    ((spec.out_inv >> h) & 1));
            }

            return success;
        }

        std::cout << "begin to fence synthesize" << std::endl;
        // As the topological synthesizer decomposes the synthesis
        // problem, to fairly count the total number of conflicts we
        // should keep track of all conflicts in existence checks.
        fence f;
        po_filter<unbounded_generator> g( unbounded_generator(spec.initial_steps), spec.get_nr_out(), 5);
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
              encoder.fence_extract_mig5(spec, mig5, f, false);
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
   
   synth_result parallel_nocegar_mig_five_fence_synthesize( spec& spec, mig5& mig5, 
                                                              int num_threads = std::thread::hardware_concurrency() )
    {
        spec.preprocess();

        // The special case when the Boolean chain to be synthesized
        // consists entirely of trivial functions.
        if (spec.nr_triv == spec.get_nr_out()) {
          spec.nr_steps = 0;
            mig5.reset(spec.get_nr_in(), spec.get_nr_out(), 0);
            for (int h = 0; h < spec.get_nr_out(); h++) {
                mig5.set_output(h, (spec.triv_func(h) << 1) +
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

        spec.fanin = 5;
        spec.nr_steps = spec.initial_steps;
        while (true) 
        {
            for (int i = 0; i < num_threads; i++) 
            {
              //std::cout << "thread: " << i << std::endl;
                threads[i] = std::thread([&spec, pfinished, pfound, &found_mutex, &mig5, &q] 
                    {
                    bmcg_wrapper solver;
                    mig_five_encoder encoder(solver);
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
                                    encoder.fence_extract_mig5(spec, mig5, local_fence, false );
                                    //std::cout << "[i] simulation tt: " << kitty::to_hex( mig5.simulate()[0] ) << std::endl;
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
   
   synth_result parallel_mig_five_fence_synthesize( spec& spec, mig5& mig5,
                                                     int num_threads = std::thread::hardware_concurrency())
     { 
        spec.preprocess();

        // The special case when the Boolean chain to be synthesized
        // consists entirely of trivial functions.
        if (spec.nr_triv == spec.get_nr_out()) 
        {
          spec.nr_steps = 0;
            mig5.reset(spec.get_nr_in(), spec.get_nr_out(), 0);
            for (int h = 0; h < spec.get_nr_out(); h++) 
            {
                mig5.set_output(h, (spec.triv_func(h) << 1) +
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
        spec.fanin = 5;
        spec.nr_steps = spec.initial_steps;
        while (true) 
        {
            for (int i = 0; i < num_threads; i++) 
            {
                threads[i] = std::thread([&spec, pfinished, pfound, &found_mutex, &mig5, &q] {
                    also::mig5 local_mig;
                    bmcg_wrapper solver;
                    mig_five_encoder encoder(solver);
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
                            if (*pfound) 
                            {
                                break;
                            } 
                            else if (status == success) 
                            {
                                encoder.fence_extract_mig5(spec, local_mig, local_fence, true);
                                auto sim_tt = local_mig.simulate()[0];
                                //auto sim_tt = encoder.simulate(spec);
                                //if (spec.out_inv) {
                                //    sim_tt = ~sim_tt;
                                //}
                                auto xor_tt = sim_tt ^ (spec[0]);
                                auto first_one = kitty::find_first_one_bit(xor_tt);
                                if (first_one != -1) 
                                {
                                    if (!encoder.fence_create_tt_clauses(spec, first_one - 1)) 
                                    {
                                        break;
                                    }
                                    status = timeout;
                                    continue;
                                }
                                
                                std::lock_guard<std::mutex> vlock(found_mutex);
                                if (!(*pfound)) 
                                {
                                    encoder.fence_extract_mig5(spec, mig5, local_fence, false);
                                    *pfound = true;
                                    //std::cout << "simulation tt: " << kitty::to_hex( mig5.simulate()[0] ) << std::endl;
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
   
   synth_result mig_five_cegar_fence_synthesize( spec& spec, mig5& mig5, solver_wrapper& solver, mig_five_encoder& encoder)
   {
     assert(spec.get_nr_in() >= spec.fanin);

     spec.preprocess();

     // The special case when the Boolean chain to be synthesized
     // consists entirely of trivial functions.
     if (spec.nr_triv == spec.get_nr_out()) {
        spec.nr_steps = 0;
       mig5.reset(spec.get_nr_in(), spec.get_nr_out(), 0);
       for (int h = 0; h < spec.get_nr_out(); h++) {
         mig5.set_output(h, (spec.triv_func(h) << 1) +
             ((spec.out_inv >> h) & 1));
       }
       return success;
     }


     fence f;
     po_filter<unbounded_generator> g( unbounded_generator(spec.initial_steps), spec.get_nr_out(), 5);
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
           encoder.fence_extract_mig5(spec, mig5, f, true);
           auto sim_tt = mig5.simulate()[0];
           //auto sim_tt = encoder.simulate(spec);
           //if (spec.out_inv) {
           //    sim_tt = ~sim_tt;
           //}
           auto xor_tt = sim_tt ^ (spec[0]);
           auto first_one = kitty::find_first_one_bit(xor_tt);
           if (first_one == -1) 
           {
             encoder.fence_extract_mig5(spec, mig5, f, false);
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

}

#endif
