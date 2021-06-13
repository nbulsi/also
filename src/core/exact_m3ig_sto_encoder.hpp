/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file exact_m3ig_sto_encoder.hpp
 *
 * @brief SAT solver
 *
 * @author Zhufei
 * @since  0.1
 */

#ifndef EXACT_M3IG_STO_ENCODER_HPP
#define EXACT_M3IG_STO_ENCODER_HPP

#include <mockturtle/mockturtle.hpp>

#include "m3ig_helper.hpp"
#include "exact_sto_m3ig.hpp"

using namespace percy;
using namespace mockturtle;

namespace also
{
  template <typename TType>
    void print_vector(const std::vector<TType>& vec)
    {
      typename  std::vector<TType>::const_iterator it;
      std::cout << "(";
      for(it = vec.begin(); it != vec.end(); it++)
      {
        if(it!= vec.begin()) std::cout << ",";
        std::cout << (*it);
      }
      std::cout << ")\n";
    }

  class mig_three_sto_encoder
  {
    private:
      solver_wrapper* solver;
      Problem_Vector_t problem_vec;
      pabc::lit pLits[2048];

      bool is_unnormal = false;

      int nr_sel_vars;
      int nr_sim_vars;
      int nr_op_vars;
      int nr_res_vars = 0u;
      int total_nr_vars;

      int sel_offset;
      int sim_offset;
      int op_offset;
      int res_offset;

      int level_dist[32]; // How many steps are below a certain level
      int nr_levels; // The number of levels in the Boolean fence

      std::map<int, std::vector<unsigned>> sel_map;
      const int MIG_OP_VARS_PER_STEP = 4;

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

      /* idx in [0,n] */
      std::vector<int> get_res_var_set( int idx )
      {
        assert( idx >= 0 );
        assert( idx <= get_n_value() );

        std::vector<int> v;

        auto offset = 0;
        auto s = get_number_entries_sum_equal_C();

        for( auto i = 0u; i < idx; i++ )
        {
          if( i == 0 )
          {
            offset += ( problem_vec.v[i] + 2 ) * ( s[i] + 1 - 1 );
          }
          else
          {
            offset += ( problem_vec.v[i] + 2 ) * ( s[i] + 1 );
          }
        }

        int bound;
        if( idx == 0 )
        {
          bound = ( problem_vec.v[idx] + 2 ) * ( s[idx] + 1 - 1 );
        }
        else
        {
          bound = ( problem_vec.v[idx] + 2 ) * ( s[idx] + 1 );
        }

        for( auto i = 0u; i < bound; i++ )
        {
          v.push_back( res_offset + offset + i );
        }

        return v;
      }

    public:
      mig_three_sto_encoder( solver_wrapper& solver, Problem_Vector_t & problem_vec )
      {
        this->solver = &solver;
        this->problem_vec = problem_vec;
      }

      bool get_is_unnormal()
      {
          return is_unnormal;
      }

      void set_is_unnormal( bool b )
      {
            is_unnormal = b;
      }

      unsigned get_num_vars()
      {
        return problem_vec.num_vars;
      }

      unsigned get_n_value()
      {
        return problem_vec.n;
      }

      unsigned get_m_value()
      {
        return problem_vec.m;
      }

      std::vector<unsigned> get_problem_vec()
      {
        return problem_vec.v;
      }

      std::vector<unsigned> get_preoccupy_vec()
      {
          return problem_vec.pre_occupy_position_idxs;
      }

      void set_problem_vec( std::vector<unsigned> const& prob )
      {
        assert( problem_vec.v.size() == prob.size() );
        for( auto i = 0; i < get_n_value() + 1; i++ )
        {
          problem_vec.v[i] = prob[i];
        }
      }

      ~mig_three_sto_encoder()
      {
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

        /* number of result variables for cardinality constraints */
        auto v = get_number_entries_sum_equal_C();
        for( auto i = 0; i < get_n_value() + 1; i++ )
        {
          /* check the legality */
          if( problem_vec.v[ i ] > v[ i ] )
          {
            std::cout << "[e] There are " << v[ i ] << " entries, but the problem needs " << problem_vec.v[ i ] << " minterms \n";
            assert( false && "Illegal problem vector" );
          }

          /* compute the total result variables */
          if( i == 0) /* note that the first bit is 0 for normal function */
          {
            nr_res_vars += ( ( problem_vec.v[ i ] + 2 ) * ( v[i] - 1 + 1 ) );
          }
          else
          {
            nr_res_vars += ( ( problem_vec.v[ i ] + 2 ) * ( v[i] + 1 ) );
          }
        }

        /* offsets, this is used to find varibles correspondence */
        sel_offset = 0;
        op_offset  = nr_sel_vars;
        sim_offset = nr_sel_vars + nr_op_vars;
        res_offset = nr_sel_vars + nr_op_vars + nr_sim_vars;

        /* total variables used in SAT formulation */
        total_nr_vars = nr_op_vars + nr_sel_vars + nr_sim_vars + nr_res_vars;

        if( spec.verbosity > 1 )
        {
          printf( "Creating variables (mig)\n");
          printf( "nr_steps    = %d\n", spec.nr_steps );
          printf( "nr_in       = %d\n", spec.nr_in );
          printf( "nr_sel_vars = %d\n", nr_sel_vars );
          printf( "nr_op_vars  = %d\n", nr_op_vars );
          printf( "nr_sim_vars = %d\n", nr_sim_vars );
          printf( "nr_res_vars = %d\n", nr_res_vars );
          printf( "tt_size     = %d\n", spec.tt_size );
          printf( "creating %d total variables\n", total_nr_vars);
        }

        /* declare in the solver */
        solver->set_nr_vars(total_nr_vars);
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

      // Ensures that each gate has the proper number of fanins.
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
        }

        if (spec.verbosity > 2)
        {
          printf("Nr. clauses = %d (POST)\n", solver->nr_clauses());
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
          printf( "s_%d_%d%d%dis %d\n", array[0], array[1], array[2], array[3], e.first );
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

        for( const auto offvar : offset )
        {
          pLits[ctr_idx_main + 2] = pabc::Abc_Var2Lit( offvar, 1 );
          ret &= solver->add_clause(pLits, pLits + ctr_idx_main + 3 );
        }

        /* output 0 */
        pLits[ctr_idx_main + 1] = pabc::Abc_Var2Lit(get_sim_var(spec, i, t), 0 );
        ctr = ctr_idx_main + 2;
        for( const auto offvar : offset )
        {
          pLits[ctr++] = pabc::Abc_Var2Lit( offvar, 0 );
        }
        ret = solver->add_clause(pLits, pLits + ctr);

        for( const auto onvar : onset )
        {
          pLits[ctr_idx_main + 2] = pabc::Abc_Var2Lit( onvar, 1 );
          ret &= solver->add_clause(pLits, pLits + ctr_idx_main + 3 );
        }

        assert(ret);

        return ret;
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

        return ret;
      }

      /*  Given a truth table, return a vector to indicate the total number of entries
       *  c \ a b
       *     0   0   1   1
       *     0   1   1   0
       *   +---+---+---+---+
       *  0 |   |   |  |   |
       *   +---+---+---+---+
       *  1 |   |  |   |   |
       *   +---+---+---+---+
       *   v[0] = 2, v[1] = 4, v[2] = 2
       * */
      std::vector<unsigned> get_number_entries_sum_equal_C()
      {
        auto m = get_m_value();
        auto n = get_n_value();

        std::vector<unsigned> v( n + 1 );

        unsigned total = 0u;
        auto var = (unsigned)ceil( log2( m + n ) );
        kitty::dynamic_truth_table tt( var );

        auto num_loop = 0u;
        do
        {
          auto sum = 0u;

          for( auto i = 0u; i < n; i++ )
          {
            sum += kitty::get_bit( tt, i + m );
          }

          v[sum]++;
          num_loop++;
          kitty::next_inplace( tt );
        }while( !kitty::is_const0( tt ) && num_loop < pow( 2, m + n ) );

        return v;
      }

      /*
       * enumerate all the truth table to record the entries idxs set
       * */
      std::vector<unsigned> get_all_entries_idxs_set( unsigned const& capacity )
      {
          auto m = get_m_value();
          auto n = get_n_value();

          std::vector<unsigned> res;

          /* create a truth table with (m + n) variables */
          auto var = (unsigned)ceil( log2( m + n ) );
          kitty::dynamic_truth_table tt( var );

          auto num_loop = 0u;
          do
          {
              auto sum = 0u;

              for( auto i = 0u; i < n; i++ )
              {
                  sum += kitty::get_bit( tt, i + m );
              }

              if( sum == capacity && num_loop != 0u )
              {
                res.push_back( num_loop - 1u );
              }

              num_loop++;
              kitty::next_inplace( tt );
          }while( !kitty::is_const0( tt ) && num_loop < pow( 2, m + n )  );

          return res;
      }

      void create_preoccupied_constraints( const spec& spec, const std::vector<unsigned>& sim_vars )
      {
        const auto ilast_step = spec.nr_steps - 1;

        for( const auto& idx : sim_vars )
        {
            int ctr = 0;
            auto svar = get_sim_var( spec, ilast_step, idx );
            pLits[ctr++] = pabc::Abc_Var2Lit( svar, get_is_unnormal() ? 1 : 0 );
            solver->add_clause( pLits, pLits + 1 );
        }
      }

      void create_problem_vector_constraints( const spec& spec )
      {
        std::vector<int> svars; //sum variables
        std::vector<int> rvars; //result variables

        const auto ilast_step = spec.nr_steps - 1;

        for( int i = 0u; i < get_n_value() + 1; i++ )
        {
          svars.clear();
          rvars.clear();

          auto sim_vars = get_all_entries_idxs_set( i );
          for( auto const& e : sim_vars )
          {
            svars.push_back( get_sim_var( spec, ilast_step, e ) );
          }

          rvars = get_res_var_set( i );

          if( spec.verbosity > 1 )
          {
            std::cout << " svar: " ; print_vector( svars );
            std::cout << " rvar: " ; print_vector( rvars );
          }

          assert( rvars.size() == ( problem_vec.v[i] + 2 ) * ( svars.size() + 1 ) );

          create_cardinality_circuit( solver, svars, rvars, problem_vec.v[i] );

          auto res_idx =  svars.size()  * ( problem_vec.v[i] + 2 ) + problem_vec.v[i];

          if( svars.size() == 1 && problem_vec.v[i] == 1 )
          {
            auto fi_lit = pabc::Abc_Var2Lit( svars[0], 0 );
            (void)solver->add_clause( &fi_lit, &fi_lit + 1 );
          }
          else
          {
            auto fi_lit = pabc::Abc_Var2Lit( rvars[res_idx], 0 );
            (void)solver->add_clause( &fi_lit, &fi_lit + 1 );
          }
        }
      }

      void create_main_clauses( const spec& spec )
      {
        for( int t = 0; t < spec.tt_size; t++ )
        {
          (void) create_tt_clauses( spec, t );
        }

        create_problem_vector_constraints( spec );

        if( get_preoccupy_vec().size() )
        {
            (void) create_preoccupied_constraints( spec, get_preoccupy_vec() );
        }
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
        assert( spec.nr_in >= 3 );
        create_variables( spec );

        if( spec.verbosity > 1 )
        {
          std::cout << " current problem vector: "; print_vector( problem_vec.v );
        }

        create_main_clauses( spec );

        if( !create_fanin_clauses( spec ) )
        {
          return false;
        }

        if( spec.verbosity > 2 )
        {
          show_variable_correspondence( spec );
        }

        return true;
      }

      void extract_mig3(const spec& spec, mig3& chain )
      {
        int op_inputs[3] = { 0, 0, 0 };
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

        const auto pol = spec.out_inv ? 1 : 0;
        const auto tmp = ( ( spec.nr_steps + spec.nr_in ) << 1 ) + pol;
        chain.set_output( 0, tmp );

        //printf("[i] %d nodes are required\n", spec.nr_steps );

        if( spec.out_inv )
        {
          printf( "[i] output is inverted\n" );
        }
          //assert( chain.satisfies_spec( spec ) );
      }

      std::vector<unsigned> get_inverted_problem_vec()
      {
        std::vector<unsigned> pinv;
        auto porigin = get_problem_vec();
        auto v = get_number_entries_sum_equal_C();

        for( auto i = 0; i < get_n_value() + 1; i++ )
        {
          pinv.push_back( v[i] - porigin[i] );
        }

        return pinv;
      }
  };


  /******************************************************************************
   * Public functions                                                           *
   ******************************************************************************/
   synth_result mig_three_sto_synthesize( spec& spec, mig3& mig3, solver_wrapper& solver, mig_three_sto_encoder& encoder )
   {
       //init the spec based on the problem vector
       spec.nr_in   = encoder.get_m_value() + encoder.get_n_value();
       spec.tt_size = ( 1 << spec.nr_in ) - 1;
       spec.nr_steps = spec.initial_steps;

       auto porigin = encoder.get_problem_vec();
       auto pinv = encoder.get_inverted_problem_vec();
       auto v = encoder.get_number_entries_sum_equal_C();

       //permanent unnormal function, where the first one must be 1
       if( porigin[0] == v[0] )
       {
        std::cout << "[i] PERMANENT UNNORMAL function." << std::endl;
        encoder.set_problem_vec( pinv );
        encoder.set_is_unnormal( true );
        spec.out_inv = true;

        while( true )
        {
            solver.restart();

            if( !encoder.encode( spec ) ){
                spec.nr_steps++;
                break;
            }

            const auto status = solver.solve( spec.conflict_limit );

            if( status == success ){
                encoder.extract_mig3( spec, mig3 );
                return success;
            }
            else if( status == failure ){
                spec.nr_steps++;

                if( spec.nr_steps == 20 )
                {
                    return failure;
                }
            }
            else
            {
                return timeout;
            }
        }
       }
       else
       {
           while( true )
           {
               solver.restart();

               if( !encoder.encode( spec ) ){
                   spec.nr_steps++;
                   break;
               }

               const auto status = solver.solve( spec.conflict_limit );

               if( status == success ){
                   spec.out_inv = encoder.get_is_unnormal() ? true : false;
                   encoder.extract_mig3( spec, mig3 );
                   return success;
               }
               else if( status == failure ){
                   //try unnormal function
                   //note that if pinv[0] = v[0], it will result in permanate unnormal function
                   if( !encoder.get_is_unnormal() && ( pinv[0] != v[0] ) )
                   {
                       std::cout << "Try unnormal function at step " << spec.nr_steps << std::endl;
                       encoder.set_problem_vec( pinv );
                       encoder.set_is_unnormal( true );
                   }
                   else
                   {
                       spec.nr_steps++;
                       encoder.set_problem_vec( porigin );
                       encoder.set_is_unnormal( false );
                   }

                   if( spec.nr_steps == 20 )
                   {
                       return failure;
                   }
               }
               else
               {
                   return timeout;
               }
           }
       }

       return success;
   }
}

#endif
