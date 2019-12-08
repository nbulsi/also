/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file img_encoder.hpp
 *
 * @brief enonde SAT formulation to construct a implication graph
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#pragma once

#include <vector>
#include <kitty/kitty.hpp>
#include <percy/percy.hpp>
#include "misc.hpp"

using namespace percy;

namespace also
{
  /******************************************************************************
   * class select generate selection variables                                  *
   ******************************************************************************/
  class select_imp
  {
    private:
      int nr_steps;
      int nr_in;

    public:
      select_imp( int nr_steps, int nr_in )
        : nr_steps( nr_steps ), nr_in( nr_in )
      {
      }

      ~select_imp()
      {
      }

      //current step starts from 0, 1, ...
      int get_num_of_sel_vars_for_each_step( int current_step )
      {
        int count = 0;
        int total = nr_in + current_step;
        std::vector<unsigned> idx_array;

        //for total inputs, there are total - 1 combined with 0
        //such as s_0_10, s_0_20, ...
        count += total ;

        for( auto i = 1; i <= total; i++ )
        {
          idx_array.push_back( i );
        }

        //binom( total - 1, 2 ) * 2
        count += get_all_combination_index( idx_array, total, 2u ).size() * 2;

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

      /* map: 0,     0         _10 
       *      index, nr_steps, inputs ids
       **/
      std::map<int, std::vector<unsigned>> get_sel_var_map()
      {
        std::map<int, std::vector<unsigned>> map;
        std::vector<unsigned> idx_array;
        int count = 0;

        for( auto i = 0; i < nr_steps; i++ )
        {
          int total = nr_in + i;

          //for total inputs, there are total - 1 combined with 0
          //such as s_0_10, s_0_20, ...
          for( auto k = 1; k <= total; k++ )
          {
             std::vector<unsigned> c;
             c.push_back( i );
             c.push_back( k );
             c.push_back( 0 );

             map.insert( std::pair<int, std::vector<unsigned>>( count++, c ) );
          }

          idx_array.clear();
          idx_array.resize( total );
          std::iota( idx_array.begin(), idx_array.end(), 1 );

          //no const & 'a' const 
          for( const auto c : get_all_combination_index( idx_array, idx_array.size(), 2u ) )
          {
            auto tmp = c;
            tmp.insert( tmp.begin(), i );
            map.insert( std::pair<int, std::vector<unsigned>>( count++, tmp ) );

            std::vector<unsigned> reverse;
            reverse.push_back( i );
            reverse.push_back( c[1] );
            reverse.push_back( c[0] );
            map.insert( std::pair<int, std::vector<unsigned>>( count++, reverse ) );
          }
        }

        return map;
      }

  };

  /* public function */
  std::map<int, std::vector<unsigned>> comput_select_imp_vars_map( int nr_steps, int nr_in ) 
  { 
    select_imp s( nr_steps, nr_in );
    return s.get_sel_var_map();
  }

  int comput_select_imp_vars_for_each_step( int nr_steps, int nr_in, int step_idx ) 
  {
    assert( step_idx >= 0 && step_idx < nr_steps );
    select_imp s( nr_steps, nr_in );
    return s.get_num_of_sel_vars_for_each_step( step_idx );
  }
  
  /******************************************************************************
   * img chain *
   ******************************************************************************/
  class img
  {
    private:
      int nr_in;
      std::vector<int> outputs;
      using step = std::array<int, 2>;

    public:
      std::vector<std::array<int, 2>> steps;

      img()
      {
        reset( 0, 0, 0 );
      }

      std::array<int, 2> get_step_inputs( int i )
      {
        std::array<int, 2> array;

        array[0] = steps[i][0];
        array[1] = steps[i][1];

        return array;
      }
      
      void reset(int _nr_in, int _nr_out, int _nr_steps)
      {
        assert(_nr_steps >= 0 && _nr_out >= 0);
        nr_in = _nr_in;
        steps.resize(_nr_steps);
        outputs.resize(_nr_out);
      }
      
      int get_nr_steps() const { return steps.size(); }
        
      kitty::dynamic_truth_table imply(  kitty::dynamic_truth_table a,
                                         kitty::dynamic_truth_table b ) const
      {
        return ~a | b;
      }
      
      std::vector<kitty::dynamic_truth_table> simulate() const
      {
        std::vector<kitty::dynamic_truth_table> fs(outputs.size());
        std::vector<kitty::dynamic_truth_table> tmps(steps.size());

        kitty::dynamic_truth_table tt_in1(nr_in);
        kitty::dynamic_truth_table tt_in2(nr_in);

        auto tt_step = kitty::create<kitty::dynamic_truth_table>(nr_in);
        auto tt_inute = kitty::create<kitty::dynamic_truth_table>(nr_in);


        for (auto i = 0u; i < steps.size(); i++) 
        {
          const auto& step = steps[i];

          if (step[0] <= nr_in) 
          {
            if( step[0] == 0)
            {
              create_nth_var( tt_in1, 0 );
              kitty::clear( tt_in1 );
            }
            else
            {
              create_nth_var(tt_in1, step[0] - 1);
            }
          } 
          else 
          {
            tt_in1 = tmps[step[0] - nr_in - 1];
          }

          if (step[1] <= nr_in) 
          {
            if( step[1] == 0)
            {
              create_nth_var( tt_in2, 0 );
              kitty::clear( tt_in2 );
            }
            else
            {
              create_nth_var(tt_in2, step[1] - 1);
            }
          } 
          else 
          {
            tt_in2 = tmps[step[1] - nr_in - 1];
          }

          kitty::clear(tt_step);
          
          tt_step = imply( tt_in1, tt_in2 );

          tmps[i] = tt_step;

          for (auto h = 0u; h < outputs.size(); h++) 
          {
            fs[h] = tt_step;
          }
        }

        return fs;
      }
        
      void set_step( int i, int fanin1, int fanin2 )
      {
        steps[i][0] = fanin1;
        steps[i][1] = fanin2;
      }
      
      void set_output(int out_idx, int lit) 
      {
        outputs[out_idx] = lit;
      }
      
      bool satisfies_spec(const percy::spec& spec)
      {
        auto tts = simulate();

        auto nr_nontriv = 0;

        //std::cout << "[i] simulated tt: " << kitty::to_hex( tts[0] ) << std::endl;

        for (int i = 0; i < spec.nr_nontriv; i++) 
        {
          if( tts[nr_nontriv++] != spec[i] )
          {
            assert(false);
            return false;
          }
        }

        return true;
      }
      
      void to_expression( std::ostream& o, const int i )
      {
        if( i == 0 )
        {
          o << "0";
        }
        else if (i <= nr_in) 
        {
          o << static_cast<char>('a' + i - 1);
        } 
        else 
        {
          const auto& step = steps[i - nr_in - 1];
          o << "(";
          to_expression(o, step[0]);
          o << "->";
          to_expression(o, step[1]);
          o << ")";
        }
      }

      void to_expression(std::ostream& o)
      {
        to_expression(o, nr_in + steps.size() );
      }

      //print implication logic expressions for storage, ()
      //indicates the implication operation
      void img_to_expression( std::stringstream& ss, const int i )
      {
        if( i == 0 )
        {
          ss << "0";
        }
        else if (i <= nr_in) 
        {
          ss << static_cast<char>('a' + i - 1);
        } 
        else 
        {
          const auto& step = steps[i - nr_in - 1];
          ss << "(";
          img_to_expression(ss, step[0]);
          img_to_expression(ss, step[1]);
          ss << ")";
        }
      }
      
      std::string img_to_expression()
      {
        std::stringstream ss;

        img_to_expression( ss, nr_in + steps.size() );

        return ss.str();
      }

  };
  
  std::string img_to_string( const spec& spec, const img& img )
  {
    if( img.get_nr_steps() == 0 )
    {
      return "";
    }

    std::stringstream ss;
    
    for(auto i = 0; i < spec.nr_steps; i++ )
    {
      ss << i + spec.nr_in + 1 << "-" << img.steps[i][0] << "-" 
                                      << img.steps[i][1] << " "; 
    }

    return ss.str();
  }

  /******************************************************************************
   * implication logic encoder                                                  *
   ******************************************************************************/
  class img_encoder
  {
    private:
      int nr_sel_vars;
      int nr_sim_vars;
      int nr_ext_vars;

      int sel_offset;
      int sim_offset;
      int ext_offset;

      int total_nr_vars;

      bool dirty = false;
      bool print_clause = false;
      bool write_cnf_file = false;

      int num_clauses = 0;
      std::vector<std::vector<int>> clauses;
      
      pabc::lit pLits[2048];
      solver_wrapper* solver;
      FILE* f = NULL;

      std::map<int, std::vector<unsigned>> sel_map;
      int nr_in;
      int tt_size; //local variable if the inputs is less than 3

      /*
       * private functions
       * */
      int get_sel_var( int step_idx, int j, int k ) const
      {
        assert( sel_map.size() != 0 );

        for( const auto& e : sel_map )
        {
          auto array = e.second;

          if( array[0] == step_idx )
          {
            if( array[1] == j && array[2] == k )
            {
              return e.first;
            }
          }
        }

        assert( false && "sel_var is out of reach" );
        return 0;
      }

      int get_sim_var( const spec& spec, int step_idx, int t ) const
      {
        return sim_offset + ( spec.tt_size + 1 ) * step_idx + t;
      }
      
      int get_ext_var( const spec& spec, int step_idx, int sel_var, int t ) const
      {
        return ext_offset + ( spec.tt_size + 1 ) * step_idx + sel_var * tt_size  + t;
      }
      
    public:
      img_encoder( solver_wrapper& solver )
      {
        this->solver = &solver;
      }

      ~img_encoder()
      {
      }
      
      bool is_dirty() 
      {
        return dirty;
      }

      void set_dirty(bool _dirty)
      {
        dirty = _dirty;
      }

      int imply(int a, int b ) const
      {
        int a_bar;
        if( a == 0 ) { a_bar = 1; }
        else if( a == 1 ) { a_bar = 0; }
        else{ assert( false ); }

        return a_bar | b; 
      }

      void create_variables( const spec& spec )
      {
        /* init */
        nr_in = spec.nr_in;
        tt_size = ( 1 << nr_in ); //implication is not a normal function, should include all the tt size

        /* number of simulation variables, s_out_in1_in2*/
        sel_map = comput_select_imp_vars_map( spec.nr_steps, spec.nr_in );
        nr_sel_vars = sel_map.size();
        
        /* number of truth table simulation variables */
        nr_sim_vars = spec.nr_steps * tt_size;
        nr_ext_vars = nr_sim_vars * nr_sel_vars;
        
        /* offsets, this is used to find varibles correspondence */
        sel_offset  = 0;
        sim_offset  = nr_sel_vars;
        ext_offset  = nr_sel_vars + nr_sim_vars;

        /* total variables used in SAT formulation */
        total_nr_vars = nr_sel_vars + nr_sim_vars + nr_ext_vars;

        if( spec.verbosity )
        {
          printf( "Creating variables (MIG)\n");
          printf( "nr steps    = %d\n", spec.nr_steps );
          printf( "nr_in       = %d\n", nr_in );
          printf( "nr_sel_vars = %d\n", nr_sel_vars );
          printf( "nr_sim_vars = %d\n", nr_sim_vars );
          printf( "nr_ext_vars = %d\n", nr_ext_vars );
          printf( "tt_size     = %d\n", tt_size );
          printf( "creating %d total variables\n", total_nr_vars);
        }
        
        /* declare in the solver */
        solver->set_nr_vars(total_nr_vars);
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

        create_variables( spec );
        create_main_clauses( spec );

        if( !create_fanin_clauses( spec ))
        {
          return false;
        }

        if( spec.verbosity) 
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
      
      /*
       * cegar: counter-example guided abstract refine
       * do not create main clauses initially
       * */
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

        create_variables( spec );

        if( !create_fanin_clauses( spec ))
        {
          return false;
        }

        if( spec.verbosity) 
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

      void show_variable_correspondence( const spec& spec )
      {
        printf( "**************************************\n" );
        printf( "selection variables \n");

        for( auto e : sel_map )
        {
          auto array = e.second;
          printf( "s_%d_%d%d is %d\n", array[0], array[1], array[2], e.first );
        }
      }

      bool create_fanin_clauses( const spec& spec )
      {
        auto status = true;

        if (spec.verbosity > 2) 
        {
          printf("Creating fanin clauses (IMPLY)\n");
          printf("Nr. clauses = %d (PRE)\n", solver->nr_clauses());
        }

        int svar = 0;
        for (int i = 0; i < spec.nr_steps; i++) 
        {
          auto ctr = 0;

          auto num_svar_in_current_step = comput_select_imp_vars_for_each_step( spec.nr_steps, spec.nr_in, i ); 
          
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

      bool fix_output_sim_vars(const spec& spec, int t)
      {
        const auto ilast_step = spec.nr_steps - 1;
        auto outbit = kitty::get_bit( spec[spec.synth_func(0)], t );
        
        if ((spec.out_inv >> spec.synth_func(0)) & 1) 
        {
          //HERE we do not require a function is NORMAL
          //outbit = 1 - outbit; 
        }
        
        const auto sim_var = get_sim_var(spec, ilast_step, t);
        pabc::lit sim_lit = pabc::Abc_Var2Lit(sim_var, 1 - outbit);
        if( print_clause ) { print_sat_clause( solver, &sim_lit, &sim_lit + 1 ); }
        if( write_cnf_file ) { add_print_clause( clauses, &sim_lit, &sim_lit + 1); }
        return solver->add_clause(&sim_lit, &sim_lit + 1);
      }

      int getbit( int i, int t) //truth table of ith value on position t
      {
        assert( i <= nr_in );
        if( i == 0 ) return 0;
        kitty::dynamic_truth_table nth_tt( nr_in );
        kitty::create_nth_var( nth_tt, i - 1 );
        return kitty::get_bit( nth_tt, t );
        //return ( ( t + 1) & ( 1 << i ) ) ? 1 : 0;
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
                                  const int sel_var )
      {
        /****************************************************************
         * Initialization
         ***************************************************************/
        int ctr = 0;
        bool ret = true;
        bool flag_clause_true = false;

        pabc::lit ptmp[9];

        ptmp[0] = pabc::Abc_Var2Lit(sel_var, 1); // ~s_ijk
        ptmp[1] = pabc::Abc_Var2Lit(get_sim_var(spec, i, t), 0); //  x_it
        ptmp[2] = pabc::Abc_Var2Lit(get_sim_var(spec, i, t), 1); // ~x_it
        ptmp[3] = pabc::Abc_Var2Lit(get_ext_var(spec, i, sel_var, t), 0); //  c_jkt
        ptmp[4] = pabc::Abc_Var2Lit(get_ext_var(spec, i, sel_var, t), 1); // ~c_jkt
     

        /****************************************************************
         * first clause
         * ~s_ijk + ~x_it + ~x_jt + x_kt
         ***************************************************************/
        pLits[ctr++] = ptmp[0];
        pLits[ctr++] = ptmp[2];
        
        
        if( j <= spec.nr_in ) 
        {
          if( getbit( j, t ) == 0 )
          {
            flag_clause_true = true;
          }
        }
        else
        {
          pLits[ctr++] = pabc::Abc_Var2Lit(get_sim_var(spec, j - spec.nr_in - 1, t), 1); // ~x_jt
        }
        
        if( k <= spec.nr_in  ) 
        {
          if( getbit( k, t ) == 1 )
          {
            flag_clause_true = true;
          }
        }
        else
        {
          pLits[ctr++] = pabc::Abc_Var2Lit(get_sim_var(spec, k - spec.nr_in - 1, t), 0); // x_kt
        }

        if( !flag_clause_true )
        {
          ret &= solver->add_clause(pLits, pLits + ctr);
          if( write_cnf_file ) { add_print_clause( clauses, pLits, pLits + ctr); }
          if( print_clause ) { std::cout << "case 0 "; print_sat_clause( solver, pLits, pLits + ctr); }
        }

        /****************************************************************
         * second clause
         * ~s_ijk + x_it + c_jkt
         ***************************************************************/
        ctr = 0;
        pLits[ctr++] = ptmp[0];
        pLits[ctr++] = ptmp[1];
        pLits[ctr++] = ptmp[3];
        
        ret &= solver->add_clause(pLits, pLits + ctr);
        if( write_cnf_file ) { add_print_clause( clauses, pLits, pLits + ctr); }
        if( print_clause ) { std::cout << "case 1 "; print_sat_clause( solver, pLits, pLits + ctr); }

        /****************************************************************
         * third clause
         * ~x_jt + x_kt + c_jkt
         ***************************************************************/
        ctr = 0;
        flag_clause_true = false;

        if( j <= spec.nr_in ) 
        {
          if( getbit( j, t ) == 0 )
          {
            flag_clause_true = true;
          }
        }
        else
        {
          pLits[ctr++] = pabc::Abc_Var2Lit(get_sim_var(spec, j - spec.nr_in - 1, t), 1); // ~x_jt
        }
        
        if( k <= spec.nr_in  ) 
        {
          if( getbit( k, t ) == 1 )
          {
            flag_clause_true = true;
          }
        }
        else
        {
          pLits[ctr++] = pabc::Abc_Var2Lit(get_sim_var(spec, k - spec.nr_in - 1, t), 0); // x_kt
        }
        
        pLits[ctr++] = ptmp[3]; // c_it
        
        if( !flag_clause_true )
        {
          ret &= solver->add_clause(pLits, pLits + ctr);
          if( write_cnf_file ) { add_print_clause( clauses, pLits, pLits + ctr); }
          if( print_clause ) { std::cout << "case 2 "; print_sat_clause( solver, pLits, pLits + ctr); }
        }

        /****************************************************************
         * fouth clause
         * x_jt + ~c_jkt
         ***************************************************************/
        ctr = 0;
        flag_clause_true = false;

        if( j <= spec.nr_in ) 
        {
          if( getbit( j, t ) == 1 )
          {
            flag_clause_true = true;
          }
        }
        else
        {
          pLits[ctr++] = pabc::Abc_Var2Lit(get_sim_var(spec, j - spec.nr_in - 1, t), 0); // x_jt
        }
        
        pLits[ctr++] = ptmp[4]; // ~c_it
        if( !flag_clause_true )
        {
          ret &= solver->add_clause(pLits, pLits + ctr);
          if( write_cnf_file ) { add_print_clause( clauses, pLits, pLits + ctr); }
          if( print_clause ) { std::cout << "case 3 "; print_sat_clause( solver, pLits, pLits + ctr); }
        }

        /****************************************************************
         * fifth clause
         * ~x_kt + ~c_jkt
         ***************************************************************/
        ctr = 0;
        flag_clause_true = false;
        
        if( k <= spec.nr_in  ) 
        {
          if( getbit( k, t ) == 0 )
          {
            flag_clause_true = true;
          }
        }
        else
        {
          pLits[ctr++] = pabc::Abc_Var2Lit(get_sim_var(spec, k - spec.nr_in - 1, t), 1); // ~x_kt
        }
        
        pLits[ctr++] = ptmp[4]; // ~c_it
        
        if( !flag_clause_true )
        {
          ret &= solver->add_clause(pLits, pLits + ctr);
          if( write_cnf_file ) { add_print_clause( clauses, pLits, pLits + ctr); }
          if( print_clause ) { std::cout << "case 4 "; print_sat_clause( solver, pLits, pLits + ctr); }
        }
        
        return ret;
      }


      bool create_tt_clauses(const spec& spec, const int t)
      {
        bool ret = true;

        for( const auto svar : sel_map )
        {
          auto s = svar.first;
          auto array  = svar.second;

          auto i = array[0];
          auto j = array[1];
          auto k = array[2];

          ret &= add_simulation_clause( spec, t, i, j, k, s );
        }

        ret &= fix_output_sim_vars(spec, t);

        return ret;
      }
      
      bool create_symvar_clauses(const spec& spec)
      {
        for( const auto e: sel_map )
        {
          auto svar_idx  = e.first;
          auto step_idx  = e.second[0];
          auto first_in  = e.second[1];
          auto second_in = e.second[2];

          if( first_in < second_in && step_idx != 0 )
          {
            int ctr = 0;
            //std::cout << "svar: " << svar_idx << " i: " << step_idx << " j: " << first_in << " k: " << second_in << std::endl;
            pLits[ctr++] = pabc::Abc_Var2Lit( svar_idx, 1 );

            for( const auto ep : sel_map )
            {
              auto sp = ep.first;

              auto ap = ep.second;

              if( sp < svar_idx )
              {
                if( ap[0] < step_idx )
                {
                  if( ap[1] == second_in && ap[2] == first_in )
                  {
                    //std::cout << "symvar: " << sp << " ijk " << ap[0] << ap[1] << ap[2] << std::endl;
                    pLits[ctr++] = pabc::Abc_Var2Lit( sp, 1 );
                    solver->add_clause(pLits, pLits + ctr);
                    ctr--;
                  }
                }
              }
            }
          }
        }
        return true;
      }

      void create_main_clauses( const spec& spec )
      {
        for( int t = 0; t < spec.tt_size + 1; t++ )
        {
          (void) create_tt_clauses( spec, t );
        }

        create_symvar_clauses( spec );
      }

      void print_solver_state(const spec& spec)
      {
        printf("\n");
        printf("========================================"
            "========================================\n");
        printf("  SOLVER STATE\n\n");

        printf("  Nr. variables = %d\n", solver->nr_vars());
        printf("  Nr. clauses = %d\n\n", solver->nr_clauses());

        for( const auto e : sel_map )
        {
          auto svar_idx  = e.first;
          auto step_idx  = e.second[0];
          auto first_in  = e.second[1];
          auto second_in = e.second[2];

          if( solver->var_value( svar_idx ) )
          {
            printf( "x_%d has inputs: x_%d and x_%d\n", step_idx + nr_in + 1, first_in, second_in );
          }
        }

        printf("========================================"
            "========================================\n");
      }
      
      void show_verbose_result()
      {
        for( auto i = 0u; i < total_nr_vars; i++ )
        {
          printf( "var %d : %d\n", i, solver->var_value( i ) );
        }
      }
      
      void extract_img(const spec& spec, img& chain )
      {
        int op_inputs[2] = { 0, 0 };
        chain.reset( spec.nr_in, 1, spec.nr_steps );

        int svar = 0;
        for (int i = 0; i < spec.nr_steps; i++) 
        {
          auto num_svar_in_current_step = comput_select_imp_vars_for_each_step( spec.nr_steps, spec.nr_in, i ); 
          
          for( int j = svar; j < svar + num_svar_in_current_step; j++ )
          {
            if( solver->var_value( j ) )
            {
              auto array = sel_map[j];
              op_inputs[0] = array[1];
              op_inputs[1] = array[2];
              break;
            }
          }
          
          svar += num_svar_in_current_step;

          chain.set_step( i, op_inputs[0], op_inputs[1] );

          if( spec.verbosity > 2 )
          {
            printf("[i] Step %d performs op %d, inputs are:%d%d%d\n", i, op_inputs[0], op_inputs[1] );
          }

        }
        
        const auto pol = 0;
        const auto tmp = ( ( spec.nr_steps + spec.nr_in ) << 1 ) + pol;
        chain.set_output(0, tmp);

        //printf("[i] %d nodes are required\n", spec.nr_steps );

        //assert( chain.satisfies_spec( spec ) );
      }
      
      //block solution
      bool block_solution( const spec& spec )
      {
        int ctr  = 0;
        int svar = 0;
          
        for (int i = 0; i < spec.nr_steps; i++) 
        {
          auto num_svar_in_current_step = comput_select_imp_vars_for_each_step( spec.nr_steps, spec.nr_in, i );
          
          for( int j = svar; j < svar + num_svar_in_current_step; j++ )
          {
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
  };
  
  /******************************************************************************
   * Public functions                                                           *
   ******************************************************************************/
  synth_result implication_syn_by_img_encoder( spec& spec, img& img, solver_wrapper& solver, img_encoder& encoder )
  {
    spec.verbosity = 0;
    spec.preprocess();

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
        encoder.extract_img( spec, img );
        std::cout << "[i] expression: ";
        img.to_expression( std::cout );
        return success;

      }
      else if( status == failure )
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
  
  synth_result implication_syn_by_fixed_num_steps( spec& spec, img& img, solver_wrapper& solver, img_encoder& encoder, int& num_steps )
  {
    spec.verbosity = 0;
    spec.preprocess();

    spec.nr_steps = num_steps;
    solver.restart();

    if( !encoder.encode( spec ) )
    {
      return failure;
    }
    
    const auto status = solver.solve( spec.conflict_limit );

    return status;
  }

  synth_result img_cegar_synthesize( spec& spec, img& img, solver_wrapper& solver, img_encoder& encoder )
  {
    spec.verbosity = 0;
    spec.preprocess();
    
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
          encoder.extract_img( spec, img );
          auto sim_tt = img.simulate()[0];
          auto xot_tt = sim_tt ^ ( spec[0] );
          auto first_one = kitty::find_first_one_bit( xot_tt );
          if( first_one == -1 )
          {
            //std::cout << "[i] expression: ";
            //img.to_expression( std::cout );
            return success;
          }

          if( !encoder.create_tt_clauses( spec, first_one ) )
          {
            spec.nr_steps++;
            break;
          }
        }
        else if( status == failure )
        {
          spec.nr_steps++;
          if( spec.nr_steps == 13 )
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

  std::string nbu_img_aig_upper_bound_synthesize( const kitty::dynamic_truth_table& tt )
  {
    bsat_wrapper solver;
    spec spec;
    img img, best_img;
    spec.verbosity = 0;

    std::cout << "[i] Heuristic synthesize with upper bound of AIG." << std::endl;

    auto copy = tt;
    if( copy.num_vars()  < 2 )
    {
      spec[0] = kitty::extend_to( copy, 2 );
    }
    else
    {
      spec[0] = tt;
    }

    img_encoder encoder( solver );

    //get the upper bound by AIG synthesize
    int upper_bound;
    img_from_aig_syn( tt, false, upper_bound );

    if( upper_bound <= 1 )
    {
      std::cout << "Guranteed Optimum" << std::endl;
      return "Already optimum";
    }

    if( spec.verbosity )
    {
      std::cout << "UPPER BOUND: " << upper_bound << std::endl;
    }

    auto current_step = upper_bound - 1;

    while( true )
    {
      auto res = implication_syn_by_fixed_num_steps( spec, img, solver, encoder, current_step );
      
      if( spec.verbosity )
      {
        auto str = ( res == 0 ? "Success" : "Failure" );
        std::cout << "#Current_step: " << current_step << " Status: " << str << std::endl;
      }

      if( res == success && current_step >= 2 )
      {
        encoder.extract_img( spec, best_img);
        current_step -= 1;
      }
      else
      {
        std::cout << "No more improvement. The optimal number of nodes is " << current_step + 1 << std::endl;
        best_img.to_expression( std::cout );
        std::cout << std::endl;
        break;
      }
    }

    return img_to_string( spec, best_img );
  }
  
  void nbu_img_encoder_test( const kitty::dynamic_truth_table& tt )
  {
    bsat_wrapper solver;
    spec spec;
    img img;

    auto copy = tt;
    if( copy.num_vars()  < 2 )
    {
      spec[0] = kitty::extend_to( copy, 2 );
    }
    else
    {
      spec[0] = tt;
    }

    img_encoder encoder( solver );
    
    if ( implication_syn_by_img_encoder( spec, img, solver, encoder ) == success )
    {
      std::cout << std::endl << "[i] Success. " << spec.nr_steps << " nodes are required." << std::endl;
    }
    else
    {
      std::cout << "[i] Fail " << std::endl;
    }
  }
  
  void nbu_img_cegar_synthesize( const kitty::dynamic_truth_table& tt )
  {
    bsat_wrapper solver;
    spec spec;
    img img;

    auto copy = tt;
    if( copy.num_vars()  < 2 )
    {
      spec[0] = kitty::extend_to( copy, 2 );
    }
    else
    {
      spec[0] = tt;
    }

    img_encoder encoder( solver );

    if ( img_cegar_synthesize( spec, img, solver, encoder ) == success )
    {
      std::cout << std::endl << "[i] Success. " << spec.nr_steps << " nodes are required." << std::endl;
    }
    else
    {
      std::cout << "[i] Fail " << std::endl;
    }
  }
   
  synth_result next_solution( spec& spec, img& img, solver_wrapper& solver, img_encoder& encoder )
  {
    //spec.verbosity = 3;
    if (!encoder.is_dirty()) 
    {
      encoder.set_dirty(true);
      return implication_syn_by_img_encoder(spec, img, solver, encoder);
    }

    if (encoder.block_solution(spec)) 
    {
      const auto status = solver.solve(spec.conflict_limit);

      if (status == success) 
      {
        encoder.extract_img( spec, img );
        std::cout << std::endl;
        img.to_expression( std::cout );
        return success;
      } 
      else 
      {
        return status;
      }
    }

    return failure;
  }

  
  void enumerate_img( const kitty::dynamic_truth_table& tt )
  {
    bsat_wrapper solver;
    spec spec;
    img img;

    spec.verbosity = 1;

    auto copy = tt;
    if( copy.num_vars()  < 2 )
    {
      spec[0] = kitty::extend_to( copy, 3 );
    }
    else
    {
      spec[0] = tt;
    }

    img_encoder encoder( solver );

    int nr_solutions = 0;

    while( next_solution( spec, img, solver, encoder ) == success )
    {
      //img.to_expression( std::cout );
      encoder.print_solver_state( spec );
      nr_solutions++;
    }

    std::cout << "\nThere are " << nr_solutions << " solutions found." << std::endl;

  }

}
