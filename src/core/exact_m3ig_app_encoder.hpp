/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019-2021 Ningbo University, Ningbo, China */

/**
 * @file exact_m3ig_app_encoder.hpp
 *
 * @brief Synthesis for approximate computing under the constraint of error distance (ED)
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef EXACT_M3IG_APP_ENCODER_HPP
#define EXACT_M3IG_APP_ENCODER_HPP

#include <cmath>
#include <mockturtle/mockturtle.hpp>
#include "m3ig_helper.hpp"
#include "misc.hpp"

using namespace percy;
using namespace mockturtle;

namespace also
{

    class mig_three_app_encoder
    {
        private:
            int nr_sel_vars;
            int nr_sim_vars;
            int nr_op_vars;
            int nr_out_vars;
            int nr_pi_sim_vars = 0;
            int nr_pi_out_vars = 0;

            int sel_offset;
            int sim_offset;
            int op_offset;
            int out_offset;
            int pi_sim_offset;
            int pi_out_offset;

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

            unsigned max_error_distance = 0;
            unsigned min_num_of_nodes   = 0;
            bool allow_projection = false;

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
            
            int get_pi_out_var( const spec& spec, int h, int i ) const
            {
                assert( h < spec.nr_nontriv );
                assert( i < spec.nr_in );

                return pi_out_offset + spec.nr_in * h + i;
            }
            
            int get_pi_sim_var( const spec& spec, int pi_idx, int t ) const
            {
                assert( t < spec.tt_size );
                assert( pi_idx < spec.nr_in );

                return pi_sim_offset + spec.tt_size * pi_idx + t;
            }

        public:
            mig_three_app_encoder( solver_wrapper& solver, const unsigned& dist, const unsigned& min, const bool& allow )
            {
                vLits = pabc::Vec_IntAlloc( 128 );
                max_error_distance = dist;
                min_num_of_nodes   = min;
                allow_projection = allow;
                this->solver = &solver;
            }

            ~mig_three_app_encoder()
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

                /* if allow projection */
                if( allow_projection )
                {
                  nr_pi_out_vars = spec.nr_nontriv * spec.nr_in;
                  pi_out_offset  = nr_sel_vars + nr_op_vars + nr_sim_vars + nr_out_vars;

                  nr_pi_sim_vars = spec.nr_in * spec.tt_size;
                  pi_sim_offset  = nr_sel_vars + nr_op_vars + nr_sim_vars + nr_out_vars + nr_pi_out_vars;
                  total_nr_vars  = nr_sel_vars + nr_op_vars + nr_sim_vars + nr_out_vars + nr_pi_out_vars + nr_pi_sim_vars;
                }

                if( spec.verbosity )
                {
                    printf( "Creating variables (mig)\n");
                    printf( "allow_projection = %d\n", allow_projection );
                    printf( "nr_steps    = %d\n", spec.nr_steps );
                    printf( "nr_in       = %d\n", spec.nr_in );
                    printf( "nr_sel_vars = %d\n", nr_sel_vars );
                    printf( "nr_op_vars  = %d\n", nr_op_vars );
                    printf( "nr_out_vars = %d\n", nr_out_vars );
                    printf( "nr_pi_out_vars = %d\n", nr_pi_out_vars );
                    printf( "nr_sim_vars = %d\n", nr_sim_vars );
                    printf( "tt_size     = %d\n", spec.tt_size );
                    printf( "creating %d total variables\n", total_nr_vars);
                }

                /* declare in the solver */
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

            void verify( const spec& spec )
            {
                std::vector<kitty::dynamic_truth_table> tts;

                for( auto h = 0; h < spec.nr_nontriv; h++ )
                {
                    kitty::dynamic_truth_table tmp( spec.nr_in );

                    for( auto i = 0; i < spec.nr_steps; i++ )
                    {
                        if( solver->var_value( get_out_var( spec, h, i ) ) )
                        {
                            for( auto j = 0; j < spec.tt_size; j++ )
                            {
                                if( solver->var_value( get_sim_var( spec, i , j ) ) )
                                {
                                    kitty::set_bit( tmp, j + 1 );
                                }
                            }
                            tts.push_back( tmp );

                            if( spec.verbosity )
                            {
                                std::cout << "[i] output val g_" << h << "_" << i << " is 1.\n";
                                std::cout << "[verify] Function " << h << " : \n";
                                std::cout << kitty::to_binary( tmp ) << std::endl;
                            }
                        }
                    }

                    if( allow_projection )
                    {
                      for( auto i = 0; i < spec.nr_in; i++ )
                      {
                        if( solver->var_value( get_pi_out_var( spec, h, i ) ) )
                        {
                          kitty::create_nth_var( tmp, i );
                          tts.push_back( tmp );

                          if( spec.verbosity )
                          {
                            std::cout << "[i] output val g_" << h << "_pi_" << i << " is 1.\n";
                            std::cout << "[verify] Function " << h << " : \n";
                            std::cout << kitty::to_binary( tmp ) << std::endl;
                          }
                        }
                      }
                    }
                }

                assert( tts.size() == spec.nr_nontriv );

                //compare to original tts
                /* get the exact value of tt bit index t */
                unsigned max_ed   = 0u;
                unsigned total_ed = 0u;

                for( auto t = 0; t < spec.tt_size; t++ )
                {
                    auto s = get_bin_str( spec, t );
                    auto exact_value = bin_str_to_int( s );

                    //current output value
                    std::string cs = "";
                    for( auto h = spec.nr_nontriv - 1; h >= 0; h-- )
                    {
                        auto bit = kitty::get_bit( tts[h], t + 1 );
                        cs += std::to_string( bit );
                    }
                    auto approximate_value = bin_str_to_int( cs );

                    auto dist = also_subtract_abs( exact_value, approximate_value );
                    if( spec.verbosity )
                    {
                        std::cout << "t : " << t << " exact: " << exact_value << " appro: " << approximate_value << " dist: " << dist << std::endl;
                    }
                    assert( dist <= max_error_distance && "Verification failed" );
                    if( dist > max_error_distance )
                    {
                        std::cerr << "[ERROR]: dist larger than max_error_distance\n";
                    }

                    total_ed += dist;
                    if( dist > max_ed )
                    {
                        max_ed = dist;
                    }
                }

                float med = total_ed / ( float )( spec.tt_size + 1 );
                float nmed = med / ( float )( pow( 2, spec.nr_nontriv ) - 1 );
                std::cout << "[i] MAE: " << max_ed << " Total: " << total_ed << " MED: " <<  med
                          << " NMED: " << nmed << std::endl;
            }

            void show_verbose_result()
            {
                for( auto i = 0u; i < total_nr_vars; i++ )
                {
                    printf( "var %d : %d\n", i, solver->var_value( i ) );
                }
            }

            /* for multi-output function */
            bool multi_fix_output_sim_vars( const spec& spec, int h, int step_id, int t )
            {
                auto outbit = kitty::get_bit( spec[spec.synth_func( h )], t + 1 );

                if( ( spec.out_inv >> spec.synth_func( h ) ) & 1 )
                {
                    outbit = 1 - outbit;
                }

                pLits[0] = pabc::Abc_Var2Lit( get_out_var( spec, h, step_id ), 1 );
                pLits[1] = pabc::Abc_Var2Lit( get_sim_var( spec, step_id, t ), 1 - outbit );

                if( print_clause ) { print_sat_clause( solver, pLits, pLits + 2 ); }

                return solver->add_clause( pLits, pLits + 2 );
            }

            int also_get_bit( const spec& spec, int h, int t )
            {
                assert( h < spec.nr_nontriv );

                auto outbit = kitty::get_bit( spec[spec.synth_func( h )], t + 1 );

                if( ( spec.out_inv >> spec.synth_func( h ) ) & 1 )
                {
                    outbit = 1 - outbit;
                }

                return outbit;
            }

            std::string get_bin_str( const spec& spec, int t )
            {
                std::string s = "";
                for( auto h = spec.nr_nontriv - 1; h >= 0; h-- )
                {
                    auto bit = also_get_bit( spec, h, t );
                    s += std::to_string( bit );
                }
                return s;
            }

            int bin_str_to_int( const std::string& s )
            {
                return std::bitset<64>(s).to_ullong();
            }

            int also_subtract_abs( int a, int b )
            {
                return std::abs( a - b );
            }

            std::vector<std::string> get_all_tt_string( const unsigned& length )
            {
                std::vector<std::string> v;
                auto var = (unsigned)ceil( log2( length ) );
                kitty::dynamic_truth_table tt( var );
                unsigned num_loop( 0 );

                do
                {
                    v.push_back( kitty::to_binary( tt ) );
                    num_loop++;
                    kitty::next_inplace( tt );
                }while( !kitty::is_const0( tt ) && num_loop < pow( 2, length ) );

                return v;
            }

            std::vector<std::string> get_tt_string_exceed_error_distance( const std::vector<std::string>& all_str,
                                                                          const unsigned& exact_value,
                                                                          const unsigned& error_distance_constraint )
            {
                std::vector<std::string> v;
                for( const auto& tt_str : all_str )
                {
                    auto dist = also_subtract_abs( exact_value, bin_str_to_int( tt_str ) );
                    if( dist > error_distance_constraint )
                    {
                        v.push_back( tt_str );
                    }
                }
                return v;
            }

            std::vector<std::vector<unsigned>> get_all_output_combinations( const spec& spec )
            {
                std::vector<unsigned> v(spec.nr_steps*spec.nr_nontriv);
                for( auto n = 0; n < spec.nr_steps; n++ )
                {
                    /* we generate an int vector as 001122...
                     * such that we allow one node has multiple outputs
                     * */
                    for( auto m = 0; m < spec.nr_nontriv; m++ )
                    {
                        v.push_back( n );
                    }
                }

                return get_all_combination_index( v, v.size(), spec.nr_nontriv );
            }
            
            std::vector<std::vector<unsigned>> get_projection_all_output_combinations( const spec& spec )
            {
                std::vector<unsigned> v( ( spec.nr_steps + spec.nr_in ) * spec.nr_nontriv );
                for( auto n = 0; n < spec.nr_steps + spec.nr_in; n++ )
                {
                    /* we generate an int vector as 001122...
                     * such that we allow one node has multiple outputs
                     * */
                    for( auto m = 0; m < spec.nr_nontriv; m++ )
                    {
                        v.push_back( n );
                    }
                }

                return get_all_combination_index( v, v.size(), spec.nr_nontriv );
            }

            bool multi_appro_fix_output_sim_vars( const spec& spec, int t )
            {
                bool ret = true;

                /* get the exact value of tt bit index t */
                auto s = get_bin_str( spec, t );
                auto decimal = bin_str_to_int( s );

                if( spec.verbosity )
                {
                    std::cout << "[i] exact value on tt index " << t << " is " << decimal << std::endl;
                }

                auto all_str = get_all_tt_string( spec.nr_nontriv );
                auto strs = get_tt_string_exceed_error_distance( all_str, decimal, max_error_distance );

                /* if the possible output is not valid */
                if( strs.size() > 0u )
                {
                    auto cs = get_all_output_combinations( spec );

                    for( const auto& c : cs )
                    {
                        auto perm = get_all_permutation( c );

                        for( const auto& p : perm )
                        {
                            /* add clause to constraint the case */
                            int ctr = 0;

                            /* output variables */
                            for( int h = 0; h < spec.nr_nontriv; h++ )
                            {
                                pLits[ctr++] = pabc::Abc_Var2Lit( get_out_var( spec, h, p[h] ), 1 );
                            }

                            /* all invalid tt output */
                            int ctr_current = ctr;
                            for( const auto& tts : strs )
                            {
                                ctr = ctr_current;

                                if( spec.verbosity )
                                {
                                    for( int h = 0; h < spec.nr_nontriv; h++ )
                                    {
                                      std::cout << "!g_" << h  << "_" << p[h] + spec.nr_in + 1 << " + ";
                                    }
                                }

                                for( int h = 0; h < spec.nr_nontriv; h++ )
                                {
                                    auto polar = ( tts[tts.size() - h - 1] - '0' );
                                    pLits[ctr++] = pabc::Abc_Var2Lit( get_sim_var( spec, p[h], t ), polar );
                                    auto inv = polar ? "!" : "";

                                    if( spec.verbosity )
                                    {
                                        if( h == spec.nr_nontriv - 1 )
                                        {
                                            std::cout << inv << " t_" << p[h] + spec.nr_in + 1 << "_" << t + 1 << std::endl;
                                        }
                                        else
                                        {
                                            std::cout << inv << " t_" << p[h] + spec.nr_in + 1 << "_" << t + 1 << " + ";
                                        }
                                    }
                                }

                                if( spec.verbosity )
                                {
                                    std::cout << std::endl;
                                    std::cout << tts << " is an impossible tt combination\n";
                                }

                                ret &= solver->add_clause( pLits, pLits + ctr );
                            }
                        }
                    }
                }

                return ret;
            }

            bool fix_pi_sim_vars( const spec& spec, int pi_idx, int t )
            {
              assert( pi_idx < spec.nr_in );

              kitty::dynamic_truth_table pi( spec.nr_in );
              kitty::create_nth_var( pi, pi_idx );
              auto outbit = kitty::get_bit( pi, t + 1 );
              
              const auto sim_var = get_pi_sim_var( spec, pi_idx, t );
              pabc::lit sim_lit = pabc::Abc_Var2Lit( sim_var, 1 - outbit );
              
              return solver->add_clause( &sim_lit, &sim_lit + 1 );
            }
            
            bool multi_appro_projection_fix_output_sim_vars( const spec& spec, int t )
            {
              bool ret = true;
              
              /* get the exact value of tt bit index t */
              auto s = get_bin_str( spec, t );
              auto decimal = bin_str_to_int( s );

              if( spec.verbosity )
              {
                std::cout << "[i] exact value on tt index " << t << " is " << decimal << std::endl;
              }

              auto all_str = get_all_tt_string( spec.nr_nontriv );
              auto strs = get_tt_string_exceed_error_distance( all_str, decimal, max_error_distance );

              /* if the possible output is not valid */
              if( strs.size() > 0u )
              {
                auto cs = get_projection_all_output_combinations( spec );

                for( const auto& c : cs )
                {
                  auto perm = get_all_permutation( c );

                  for( const auto& p : perm )
                  {
                    /* add clause to constraint the case */
                    int ctr = 0;
                    
                    /* output variables */
                    for( int h = 0; h < spec.nr_nontriv; h++ )
                    {
                      if( p[h] >= spec.nr_in )
                      {
                        auto tmp = p[h] - spec.nr_in;
                        pLits[ctr++] = pabc::Abc_Var2Lit( get_out_var( spec, h, tmp ), 1 );
                      }
                      else
                      {
                        pLits[ctr++] = pabc::Abc_Var2Lit( get_pi_out_var( spec, h, p[h] ), 1 );
                      }
                    }

                    /* all invalid tt output */
                    int ctr_current = ctr;
                    for( const auto& tts : strs )
                    {
                      ctr = ctr_current;

                      if( spec.verbosity )
                      {
                        for( int h = 0; h < spec.nr_nontriv; h++ )
                        {
                          std::cout << "!g_" << h  << "_" << p[h] << " + ";
                        }
                      }

                      for( int h = 0; h < spec.nr_nontriv; h++ )
                      {
                        auto polar = ( tts[tts.size() - h - 1] - '0' );

                        if( p[h] >= spec.nr_in )
                        {
                          auto tmp = p[h] - spec.nr_in;
                          pLits[ctr++] = pabc::Abc_Var2Lit( get_sim_var( spec, tmp, t ),  polar );
                        }
                        else
                        {
                          pLits[ctr++] = pabc::Abc_Var2Lit( get_pi_sim_var( spec, p[h], t ),  polar );
                        }

                        auto inv = polar ? "!" : "";

                        if( spec.verbosity )
                        {
                          if( h == spec.nr_nontriv - 1 )
                          {
                            std::cout << inv << " t_" << p[h] << "_" << t + 1 << std::endl;
                          }
                          else
                          {
                            std::cout << inv << " t_" << p[h] << "_" << t + 1 << " + ";
                          }
                        }
                      }

                      if( spec.verbosity )
                      {
                        std::cout << std::endl;
                        std::cout << tts << " is an impossible tt combination\n";
                      }

                      ret &= solver->add_clause( pLits, pLits + ctr );
                    }
                  }
                }
              }

              return ret;
            }

            /*
             * for multi-output functions, create clauses:
             * (1) all outputs show have at least one output var to be
             * true,
             * g_0_3 + g_0_4
             * g_1_3 + g_1_4
             *
             * note one node can have multiple outputs, e.g., g_0_3 and g_1_3 both be true
             *
             * g_0_4 + g_1_4, at lease one output is the last step
             * function
             * */
            bool create_output_clauses( const spec& spec )
            {
                auto status = true;

                if( spec.nr_nontriv > 1 )
                {
                    // Every output points to an operand
                    for( int h = 0; h < spec.nr_nontriv; h++ )
                    {
                        for( int i = 0; i < spec.nr_steps; i++ )
                        {
                            pabc::Vec_IntSetEntry( vLits, i, pabc::Abc_Var2Lit( get_out_var( spec, h, i ), 0 ) );
                        }

                        if( allow_projection )
                        {
                          for( int i = 0; i < spec.nr_in; i++ )
                          {
                            pabc::Vec_IntSetEntry( vLits, i + spec.nr_steps, pabc::Abc_Var2Lit( get_pi_out_var( spec, h, i ), 0 ) );
                          }
                          
                          status &= solver->add_clause(
                              pabc::Vec_IntArray( vLits ),
                              pabc::Vec_IntArray( vLits ) + spec.nr_steps + spec.nr_in );
                        }
                        else
                        {
                          status &= solver->add_clause(
                              pabc::Vec_IntArray( vLits ),
                              pabc::Vec_IntArray( vLits ) + spec.nr_steps );
                        }

                        /* print clauses */
                        if( print_clause )
                        {
                            std::cout << "Add clause: ";
                            for( int i = 0; i < spec.nr_steps; i++ )
                            {
                                std::cout << " " << get_out_var( spec, h, i );
                            }
                            if( allow_projection )
                            {
                              for( int i = 0; i < spec.nr_in; i++ )
                              {
                                std::cout << " " << get_pi_out_var( spec, h, i );
                              }
                            }
                            std::cout << std::endl;
                        }
                    }

                    // Every output can have only one be true
                    // e.g., g_0_1, g_0_2 can have at most one be true
                    // !g_0_1 + !g_0_2
                    std::vector<int> all_out_vars;
                    for( int h = 0; h < spec.nr_nontriv; h++ )
                    {
                      all_out_vars.clear(); //for each nontriv function

                      for( int i = 0; i < spec.nr_steps; i++ )
                      {
                        all_out_vars.push_back( get_out_var( spec, h, i ) );
                      }

                      if( allow_projection )
                      {
                        for( int i = 0; i < spec.nr_in; i++ )
                        {
                          all_out_vars.push_back( get_pi_out_var( spec, h, i ) );
                        }
                      }

                      auto size = all_out_vars.size();
                      if( size > 1u )
                      {
                        for( int i = 0; i < size - 1; i++ )
                        {
                          for( int j = i + 1; j < size; j++ )
                          {
                            pLits[0] = pabc::Abc_Var2Lit( all_out_vars[i], 1 );
                            pLits[1] = pabc::Abc_Var2Lit( all_out_vars[j], 1 );
                            status &= solver->add_clause( pLits, pLits + 2 );
                          }
                        }
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

                if( ( spec.nr_steps < min_num_of_nodes && min_num_of_nodes != 0u )
                      || max_error_distance == 0u ) 
                {
                    for( int h = 0; h < spec.nr_nontriv; h++ )
                    {
                        for( int i = 0; i < spec.nr_steps; i++ )
                        {
                            ret &= multi_fix_output_sim_vars( spec, h, i, t );
                        }
                    }
                }
                else
                {
                    if( allow_projection )
                    {
                      ret &= multi_appro_projection_fix_output_sim_vars( spec, t );
                      
                      for( int i = 0; i < spec.nr_in; i++ )
                      {
                        ret &= fix_pi_sim_vars( spec, i, t );
                      }
                    }
                    else
                    {
                      ret &= multi_appro_fix_output_sim_vars( spec, t );
                    }
                }

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
                            pLits[ctr++] = pabc::Abc_Var2Lit( j, 1 );
                            break;
                        }
                    }

                    svar += num_svar_in_current_step;
                }

                assert( ctr == spec.nr_steps );

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

            bool is_dirty()
            {
                return dirty;
            }

            void set_dirty(bool _dirty)
            {
                dirty = _dirty;
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
                bool flag_po = false;
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

                    if( allow_projection )
                    {
                      flag_po = false;
                      for( int i = 0; i < spec.nr_in; i++ )
                      {
                        if( solver->var_value( get_pi_out_var( spec, h, i ) ) )
                        {
                          chain.set_output( h, ( i + 1 ) << 1 );
                          
                          if( spec.verbosity > 2 )
                          {
                            printf("[i] PO %d is PI %d\n", h, i + 1 );
                          }
                          nontriv_count++;
                          flag_po = true;
                          break;
                        }
                      }

                      if( flag_po ) continue;
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
    };

    /******************************************************************************
     * Public functions                                                           *
     ******************************************************************************/
    synth_result mig_three_app_synthesize( spec& spec, mig3& mig3, solver_wrapper& solver, mig_three_app_encoder& encoder )
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
                encoder.verify( spec );
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

    synth_result next_solution( spec& spec, mig3& mig3, solver_wrapper& solver, mig_three_app_encoder& encoder )
    {
        //spec.verbosity = 3;
       if (!encoder.is_dirty())
    {
            encoder.set_dirty(true);
            return mig_three_app_synthesize(spec, mig3, solver, encoder);
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
                encoder.extract_mig3( spec, mig3 );
                encoder.verify( spec );
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
