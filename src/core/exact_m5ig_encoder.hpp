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
#include <mockturtle/mockturtle.hpp>

#include "misc.hpp"
#include "extract_m5ig.hpp"
#include "../networks/m5ig.hpp"

using namespace percy;
using namespace mockturtle;

namespace also
{
  enum input_type
  {
    no_const,
    a_const,
    ab_const,
    ab_const_cd_equal,
    ab_equal,
    bc_equal,
    cd_equal,
    de_equal
  };

  /******************************************************************************
   * class comb generate various combinations                                   *
   ******************************************************************************/
  class comb
  {
    private:
      std::vector<std::vector<int>> polarity {  {0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0},  
        {0, 1, 0, 0, 0},  
        {0, 0, 1, 0, 0},  
        {0, 0, 0, 1, 0},  
        {0, 0, 0, 0, 1},  
        {1, 1, 0, 0, 0},  
        {1, 0, 1, 0, 0},  
        {1, 0, 0, 1, 0},  
        {1, 0, 0, 0, 1},  
        {0, 1, 1, 0, 0},  
        {0, 1, 0, 1, 0},  
        {0, 1, 0, 0, 1},  
        {0, 0, 1, 1, 0},  
        {0, 0, 1, 0, 1},  
        {0, 0, 0, 1, 1}  
      };

      kitty::static_truth_table<5> a, b, c, d, e;
      input_type type; 

      std::vector<kitty::static_truth_table<5>> all_tt; 

    public:
      comb( input_type type )
        : type( type ) 
      {
        kitty::create_nth_var( a, 0 );
        kitty::create_nth_var( b, 1 );
        kitty::create_nth_var( c, 2 );
        kitty::create_nth_var( d, 3 );
        kitty::create_nth_var( e, 4 );

        switch( type )
        {
          case no_const:
            {
            }
            break;

          case a_const:
            {
              kitty::create_from_hex_string( a, "00000000");
            }
            break;

          case ab_const:
            {
              kitty::create_from_hex_string( a, "00000000");
              kitty::create_from_hex_string( b, "00000000");
            }
            break;

          case ab_const_cd_equal:
            {
              kitty::create_from_hex_string( a, "00000000");
              kitty::create_from_hex_string( b, "00000000");
              kitty::create_nth_var( d, 2 );
            }
            break;

          case ab_equal:
            {
              kitty::create_nth_var( b, 0 );
            }
            break;

          case bc_equal:
            {
              kitty::create_nth_var( c, 1 );
            }
            break;

          case cd_equal:
            {
              kitty::create_nth_var( d, 2 );
            }
            break;

          case de_equal:
            {
              kitty::create_nth_var( e, 3 );
            }
            break;

          default:
            break;
        }

        all_tt = get_all_tt();
      }

      ~comb()
      {
      }

      kitty::static_truth_table<5> comput_maj5( int polar_idx )
      {
        kitty::static_truth_table<5> m1, m2, m3;

        int pa, pb, pc, pd, pe;

        pa = polarity[polar_idx][0];
        pb = polarity[polar_idx][1];
        pc = polarity[polar_idx][2];
        pd = polarity[polar_idx][3];
        pe = polarity[polar_idx][4];


        auto ia = pa ? ~a : a;
        auto ib = pb ? ~b : b;
        auto ic = pc ? ~c : c;
        auto id = pd ? ~d : d;
        auto ie = pe ? ~e : e;

        m1 = kitty::ternary_majority( ia, ib, ic );
        m2 = kitty::ternary_majority( ia, ib, id );
        m3 = kitty::ternary_majority( m2, ic, id );
        return kitty::ternary_majority( m1, m3, ie );
      }

      std::vector<kitty::static_truth_table<5>> get_all_tt()
      {
        std::vector<kitty::static_truth_table<5>> r;
        for( int i = 0; i < 16; i++ )
        {
          //std::cout << " i : " << i << " tt: " << kitty::to_binary( comput_maj5( i ) ) << std::endl;
          r.push_back( comput_maj5( i ) );
        }

        return r;
      }

      std::vector<std::vector<int>> get_on_set()
      {
        //return the onset index array for the number of idx input
        //we start idx 0 : 0000, 31 : 11111
        std::vector<std::vector<int>> r;
        std::vector<int> v;

        for( auto i = 0; i < 32; i++ )
        {
          v.clear();
          for( auto j = 0; j < 16; j++)
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

        for( auto i = 0; i < 32; i++ )
        {
          entry.clear();

          entry.push_back( kitty::get_bit( e, i ) );
          entry.push_back( kitty::get_bit( d, i ) );
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

        for( auto i = 0; i < 32; i++ )
        {
          map.insert( std::pair<std::vector<int>, std::vector<int>>( inputs[i], set[i] ) );
        }

        return map;
      }


  };

  /******************************************************************************
   * class select generate selection variables                                  *
   ******************************************************************************/
  class select
  {
    private:
      int nr_steps;
      int nr_in;
      bool allow_two_const;
      bool allow_two_equal;

    public:
      select( int nr_steps, int nr_in, bool allow_two_const, bool allow_two_equal )
        : nr_steps( nr_steps ), nr_in( nr_in ), allow_two_const( allow_two_const ), allow_two_equal( allow_two_equal )
      {
      }

      ~select()
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
        count += get_all_combination_index( idx_array, total, 5u ).size();

        //ab_const
        if( allow_two_const )
        {
          idx_array.erase( idx_array.begin() );
          count += get_all_combination_index( idx_array, idx_array.size(), 3u ).size();
        }

        if( allow_two_const && allow_two_equal )
        {
          //ab_const_cd_equal
          count += get_all_combination_index( idx_array, idx_array.size(), 2u ).size();
        }

        if( allow_two_equal )
        {
          //ab_equal
          count += get_all_combination_index( idx_array, idx_array.size(), 4u ).size() * 4;
        }
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
          for( const auto c : get_all_combination_index( idx_array, idx_array.size(), 5u ) )
          {
            auto tmp = c;
            tmp.insert( tmp.begin(), i );
            map.insert( std::pair<int, std::vector<unsigned>>( count++, tmp ) );
          }

          //erase 0
          idx_array.erase( idx_array.begin() );

          if( allow_two_const )
          {
            //ab_const
            for( const auto c : get_all_combination_index( idx_array, idx_array.size(), 3u ) )
            {
              std::vector<unsigned> tmp {0, 0};
              tmp.push_back( c[0] );
              tmp.push_back( c[1] );
              tmp.push_back( c[2] );
              tmp.insert( tmp.begin(), i );
              map.insert( std::pair<int, std::vector<unsigned>>( count++, tmp ) );
            }
          }

          if( allow_two_const && allow_two_equal )
          {
            //ab_const_cd_equal
            for( const auto c : get_all_combination_index( idx_array, idx_array.size(), 2u ) )
            {
              std::vector<unsigned> tmp {0, 0};
              assert( c.size() == 2 );

              tmp.push_back( c[0] );
              tmp.push_back( c[0] ); //cd equal
              tmp.push_back( c[1] );
              tmp.insert( tmp.begin(), i );
              map.insert( std::pair<int, std::vector<unsigned>>( count++, tmp ) );
            }
          }

          //ab_equal
          if( allow_two_equal )
          {
            for( const auto c : get_all_combination_index( idx_array, idx_array.size(), 4u ) )
            {
              for( auto k = 0; k < 4; k++ )
              {
                auto copy = c;
                auto val = copy[k];
                copy.insert( copy.begin() + k, val );
                copy.insert( copy.begin(), i );
                map.insert( std::pair<int, std::vector<unsigned>>( count++, copy ) );
              }
            }
          }

        }

        return map;
      }

  };


  /******************************************************************************
   * Public functions by class comb and select                                  *
   ******************************************************************************/
  std::map<std::vector<int>, std::vector<int>> comput_input_and_set_map( input_type type )
  {
    comb c( type );
    return c.get_on_set_map();
  }

  std::map<int, std::vector<unsigned>> comput_select_vars_map( int nr_steps, 
      int nr_in, 
      bool allow_two_const = false, 
      bool allow_two_equal = false )
  { 
    select s( nr_steps, nr_in, allow_two_const, allow_two_equal );
    return s.get_sel_var_map();
  }

  int comput_select_vars_for_each_step( int nr_steps, 
      int nr_in, 
      int step_idx, 
      bool allow_two_const = false, 
      bool allow_two_equal = false )
  {
    assert( step_idx >= 0 && step_idx < nr_steps );
    select s( nr_steps, nr_in, allow_two_const, allow_two_equal );
    return s.get_num_of_sel_vars_for_each_step( step_idx );
  }

  /******************************************************************************
   * The main encoder                                                           *
   ******************************************************************************/
  class mig_five_encoder
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
      bool allow_two_const = false;
      bool allow_two_equal = false;
      bool write_cnf_file = true;

      FILE* f = NULL;
      int num_clauses = 0;
      std::vector<std::vector<int>> clauses;
      
      pabc::lit pLits[2048];
      solver_wrapper* solver;


      int maj_input = 5;

      std::map<int, std::vector<unsigned>> sel_map;

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
      mig_five_encoder( solver_wrapper& solver )
      {
        this->solver = &solver;
      }

      ~mig_five_encoder()
      {
      }

      void create_variables( const spec& spec )
      {
        /* number of simulation variables, s_out_in1_in2_in3 */
        sel_map = comput_select_vars_map( spec.nr_steps, spec.nr_in, allow_two_const, allow_two_equal );
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

          auto num_svar_in_current_step = comput_select_vars_for_each_step( spec.nr_steps, spec.nr_in, i, 
                                                                            allow_two_const, 
                                                                            allow_two_equal );
          
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
        copy.erase( copy.begin() ); //remove the first element that indicates step index
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
        if( g != 0 ) /* no consts */
        {
          /* aabcd, abbcd, ... */
          if( is_element_duplicate( array ) )
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
          input_set_map = comput_input_and_set_map( a_const );
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

      void create_main_clauses( const spec& spec )
      {
        for( int t = 0; t < spec.tt_size; t++ )
        {
          (void) create_tt_clauses( spec, t );
        }
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

      void construct_m5ig( const spec& spec, m5ig_network& m5ig )
      {
        //to be implement
      }

      void extract_mig5(const spec& spec, mig5& chain )
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

          auto num_svar_in_current_step = comput_select_vars_for_each_step( spec.nr_steps, spec.nr_in, i, 
                                                                            allow_two_const, 
                                                                            allow_two_equal );
          
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

      void set_allow_two_const( bool _allow_two_const )
      {
        allow_two_const = _allow_two_const;
      }

      void set_allow_two_equal( bool _allow_two_equal )
      {
        allow_two_equal = _allow_two_equal;
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
      spec.verbosity = 1;
      encoder.set_dirty( true );
      encoder.set_print_clause( false );
      encoder.set_allow_two_const( true );
      encoder.set_allow_two_equal( true );

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
          encoder.extract_mig5( spec, mig5 );
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
      spec.verbosity = 1;
      encoder.set_dirty( true );
      encoder.set_print_clause( false );
      encoder.set_allow_two_const( true );
      encoder.set_allow_two_equal( true );

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
            encoder.extract_mig5( spec, mig5 );
            auto sim_tt = mig5.simulate()[0];
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

}

#endif
