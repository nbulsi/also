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

using namespace percy;
using namespace mockturtle;

namespace also
{
  enum input_type3
  {
    none_const,
    first_const
  };

  /******************************************************************************
   * class mig3 for chain manipulation                                          *
   ******************************************************************************/
    class mig3
    {
    private:
        int nr_in;
        std::vector<int> outputs;
        using step = std::array<int, 3>;

    public:
        std::vector<std::array<int, 3>> steps;
        std::vector<int> operators;

        mig3()
        {
            reset(0, 0, 0);
        }

        void reset(int _nr_in, int _nr_out, int _nr_steps)
        {
            assert(_nr_steps >= 0 && _nr_out >= 0);
            nr_in = _nr_in;
            steps.resize(_nr_steps);
            operators.resize(_nr_steps);
            outputs.resize(_nr_out);
        }

        int get_nr_steps() const { return steps.size(); }

        kitty::dynamic_truth_table maj3( kitty::dynamic_truth_table a,
                                         kitty::dynamic_truth_table b,
                                         kitty::dynamic_truth_table c ) const 
        {
          return kitty::ternary_majority( a, b, c );
        }

        std::vector<kitty::dynamic_truth_table> simulate() const
        {
            std::vector<kitty::dynamic_truth_table> fs(outputs.size());
            std::vector<kitty::dynamic_truth_table> tmps(steps.size());

            kitty::dynamic_truth_table tt_in1(nr_in);
            kitty::dynamic_truth_table tt_in2(nr_in);
            kitty::dynamic_truth_table tt_in3(nr_in);


            auto tt_step = kitty::create<kitty::dynamic_truth_table>(nr_in);
            auto tt_inute = kitty::create<kitty::dynamic_truth_table>(nr_in);


            for (auto i = 0u; i < steps.size(); i++) 
            {
                const auto& step = steps[i];

                if (step[0] <= nr_in) 
                {
                    create_nth_var(tt_in1, step[0] - 1);

                    if( step[0] == 0)
                    {
                      kitty::clear( tt_in1 );
                    }
                } 
                else 
                {
                    tt_in1 = tmps[step[0] - nr_in - 1];
                }

                if (step[1] <= nr_in) 
                {
                    create_nth_var(tt_in2, step[1] - 1);
                    
                    if( step[1] == 0)
                    {
                      kitty::clear( tt_in2 );
                    }
                } 
                else 
                {
                    tt_in2 = tmps[step[1] - nr_in - 1];
                }

                if (step[2] <= nr_in) 
                {
                    create_nth_var(tt_in3, step[2] - 1);
                } 
                else 
                {
                    tt_in3 = tmps[step[2] - nr_in - 1];
                }
                
                kitty::clear(tt_step);
                switch (operators[i]) 
                {
                case 0:
                    tt_step = maj3( tt_in1, tt_in2, tt_in3 );
                    break;
                case 1:
                    tt_step = maj3( ~tt_in1, tt_in2, tt_in3 );
                    break;
                case 2:
                    tt_step = maj3( tt_in1, ~tt_in2, tt_in3 );
                    break;
                case 3:
                    tt_step = maj3( tt_in1, tt_in2, ~tt_in3 );
                    break;
                default:
                    assert( false && "ops are not known" );
                    break;
                }
                
                tmps[i] = tt_step;

                for (auto h = 0u; h < outputs.size(); h++) 
                {
                  //fs[h] = tt_step;
                    const auto out = outputs[h];
                    const auto var = out >> 1;
                    const auto inv = out & 1;
                    if (var - nr_in - 1 == static_cast<int>(i)) 
                    {
                        fs[h] = inv ? ~tt_step : tt_step;
                    }
                }
            }

            return fs;
        }

        void set_step(
            int i,
            int fanin1,
            int fanin2,
            int fanin3,
            int op)
        {
            steps[i][0] = fanin1;
            steps[i][1] = fanin2;
            steps[i][2] = fanin3;
            operators[i] = op;
        }


        void set_output(int out_idx, int lit) 
        {
            outputs[out_idx] = lit;
        }

        bool satisfies_spec(const percy::spec& spec)
        {
            if (spec.nr_triv == spec.get_nr_out()) 
            {
                return true;
            }
            auto tts = simulate();

            auto nr_nontriv = 0;
            for (int i = 0; i < spec.nr_nontriv; i++) 
            {
                if ((spec.triv_flag >> i) & 1) 
                {
                    continue;
                }
                
                if( tts[nr_nontriv++] != spec[i] )
                {
                  assert(false);
                  return false;
                }
            }


            return true;
        }

        void to_expression(std::ostream& o, const int i)
        {
            if (i < nr_in) 
            {
                o << static_cast<char>('a' + i);
            } 
            else 
            {
                const auto& step = steps[i - nr_in];
                o << "<";
                to_expression(o, step[0]);
                to_expression(o, step[1]);
                to_expression(o, step[2]);
                o << ">";
            }
        }

        void to_expression(std::ostream& o)
        {
            to_expression(o, nr_in + steps.size() - 1);
        }
    };

  /******************************************************************************
   * class comb generate various combinations                                   *
   ******************************************************************************/
  class comb3
  {
    private:
      std::vector<std::vector<int>> polarity {  
        {0, 0, 0},
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1}
      };

      kitty::static_truth_table<3> a, b, c;
      input_type3 type; 

      std::vector<kitty::static_truth_table<3>> all_tt; 

    public:
      comb3( input_type3 type )
        : type( type ) 
      {
        kitty::create_nth_var( a, 0 );
        kitty::create_nth_var( b, 1 );
        kitty::create_nth_var( c, 2 );

        switch( type )
        {
          case none_const:
            {
            }
            break;

          case first_const:
            {
              kitty::create_from_hex_string( a, "00");
            }
            break;

          default:
            break;
        }

        all_tt = get_all_tt();
      }

      ~comb3()
      {
      }

      kitty::static_truth_table<3> comput_maj3( int polar_idx )
      {
        assert( polar_idx <= 3 );

        int pa, pb, pc;

        pa = polarity[polar_idx][0];
        pb = polarity[polar_idx][1];
        pc = polarity[polar_idx][2];


        auto ia = pa ? ~a : a;
        auto ib = pb ? ~b : b;
        auto ic = pc ? ~c : c;

        return kitty::ternary_majority( ia, ib, ic );
      }

      std::vector<kitty::static_truth_table<3>> get_all_tt()
      {
        std::vector<kitty::static_truth_table<3>> r;
        for( int i = 0; i < 4; i++ )
        {
          //std::cout << " i : " << i << " tt: " << kitty::to_binary( comput_maj3( i ) ) << std::endl;
          r.push_back( comput_maj3( i ) );
        }

        return r;
      }

      std::vector<std::vector<int>> get_on_set()
      {
        //return the onset index array for the number of idx input
        //we start idx 0 : 000, 7 : 111
        std::vector<std::vector<int>> r;
        std::vector<int> v;

        for( auto i = 0; i < 8; i++ )
        {
          v.clear();
          for( auto j = 0; j < 4; j++)
          {
            if( kitty::get_bit( all_tt[j], i ) )
            {
              v.push_back( j );
            }

          }

          r.push_back( v );
        }

        return r;
      }

      std::vector<std::vector<int>> get_all_inputs()
      {
        std::vector<std::vector<int>> combinations;
        std::vector<int> entry;

        for( auto i = 0; i < 8; i++ )
        {
          entry.clear();

          entry.push_back( kitty::get_bit( c, i ) );
          entry.push_back( kitty::get_bit( b, i ) );
          entry.push_back( kitty::get_bit( a, i ) );

          combinations.push_back( entry );
        }

        return combinations;
      }

      std::map<std::vector<int>, std::vector<int>> get_on_set_map()
      {
        std::map<std::vector<int>, std::vector<int>> map;

        const auto inputs   = get_all_inputs();

        const auto set = get_on_set();

        for( auto i = 0; i < 8; i++ )
        {
          map.insert( std::pair<std::vector<int>, std::vector<int>>( inputs[i], set[i] ) );
        }

        return map;
      }


  };

  /******************************************************************************
   * class select generate selection variables                                  *
   ******************************************************************************/
  class select3
  {
    private:
      int nr_steps;
      int nr_in;

    public:
      select3( int nr_steps, int nr_in )
        : nr_steps( nr_steps ), nr_in( nr_in )
      {
      }

      ~select3()
      {
      }

      //current step starts from 0, 1, ...
      int get_num_of_sel_vars_for_each_step( int current_step )
      {
        int count = 0;
        int total = nr_in + 1 + current_step;
        std::vector<unsigned> idx_array;

        for( auto i = 0; i < total; i++ )
        {
          idx_array.push_back( i );
        }

        //no const & 'a' const 
        count += get_all_combination_index( idx_array, total, 3u ).size();

        return count;
      }

      int get_num_sels()
      {
        int count = 0;
        for( auto i = 0; i < nr_steps; i++ )
        {
          count += get_num_of_sel_vars_for_each_step( i );
        }

        return count;
      }

      /* map: 0,     0         _01234 
       *      index, nr_steps, inputs ids
       **/
      std::map<int, std::vector<unsigned>> get_sel_var_map()
      {
        std::map<int, std::vector<unsigned>> map;
        std::vector<unsigned> idx_array;
        int count = 0;

        for( auto i = 0; i < nr_steps; i++ )
        {
          int total = nr_in + 1 + i;

          idx_array.clear();
          idx_array.resize( total );
          std::iota( idx_array.begin(), idx_array.end(), 0 );

          //no const & 'a' const 
          for( const auto c : get_all_combination_index( idx_array, idx_array.size(), 3u ) )
          {
            auto tmp = c;
            tmp.insert( tmp.begin(), i );
            map.insert( std::pair<int, std::vector<unsigned>>( count++, tmp ) );
          }
        }

        return map;
      }

  };


  /******************************************************************************
   * Public functions by class comb and select                                  *
   ******************************************************************************/
  std::map<std::vector<int>, std::vector<int>> comput_input_and_set_map3( input_type3 type )
  {
    comb3 c( type );
    return c.get_on_set_map();
  }

  std::map<int, std::vector<unsigned>> comput_select_vars_map3( int nr_steps, int nr_in )
  { 
    select3 s( nr_steps, nr_in );
    return s.get_sel_var_map();
  }

  int comput_select_vars_for_each_step3( int nr_steps, int nr_in, int step_idx ) 
  {
    assert( step_idx >= 0 && step_idx < nr_steps );
    select3 s( nr_steps, nr_in );
    return s.get_num_of_sel_vars_for_each_step( step_idx );
  }
  
  /******************************************************************************
   * The main encoder                                                           *
   ******************************************************************************/
  class mig_three_encoder
  {
    private:
      int nr_sel_vars;
      int nr_sim_vars;
      int nr_op_vars;

      int sel_offset;
      int sim_offset;
      int op_offset;

      int total_nr_vars;

      bool dirty = false;
      bool print_clause = false;
      bool write_cnf_file = false;

      FILE* f = NULL;
      int num_clauses = 0;
      std::vector<std::vector<int>> clauses;
      
      pabc::lit pLits[2048];
      solver_wrapper* solver;


      int maj_input = 3;

      std::map<int, std::vector<unsigned>> sel_map;

      // There are 4 possible operators for each MIG node:
      // <abc>        (0)
      // <!abc>       (1)
      // <a!bc>       (2)
      // <ab!c>       (3)
      // All other input patterns can be obained from these
      // by output inversion. Therefore we consider
      // them symmetries and do not encode them.
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

    public:
      mig_three_encoder( solver_wrapper& solver )
      {
        this->solver = &solver;
      }

      ~mig_three_encoder()
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
        
        if( !create_fanin_clauses( spec ) )
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

      void construct_mig( const spec& spec, mig_network& mig )
      {
        //to be implement
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
        chain.set_output(0, tmp);

        //printf("[i] %d nodes are required\n", spec.nr_steps );

        if( spec.out_inv )
        {
         // printf( "[i] output is inverted\n" );
        }

        //assert( chain.satisfies_spec( spec ) );
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
   synth_result mig_three_synthesize( spec& spec, mig3& mig3, solver_wrapper& solver, mig_three_encoder& encoder )
   {
      spec.preprocess();
      
      // The special case when the Boolean chain to be synthesized
      // consists entirely of trivial functions.
      if (spec.nr_triv == spec.get_nr_out()) 
      {
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
   synth_result mig_three_cegar_synthesize( spec& spec, mig3& mig3, solver_wrapper& solver, mig_three_encoder& encoder )
   {
      spec.preprocess();
      
      // The special case when the Boolean chain to be synthesized
      // consists entirely of trivial functions.
      if (spec.nr_triv == spec.get_nr_out()) 
      {
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

}

#endif
