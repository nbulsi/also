/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file m3ig_helper.hpp
 *
 * @brief Helper for m3ig chain extraction and m3ig encoder
 * selection variables generation 
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef M3IG_HELPER_HPP
#define M3IG_HELPER_HPP

#include <mockturtle/mockturtle.hpp>
#include "misc.hpp"

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

        std::array<int,3> get_step_inputs( int i )
        {
          std::array<int, 3> array;

          array[0] = steps[i][0];
          array[1] = steps[i][1];
          array[2] = steps[i][2];

          return array;
        }

        int get_op( int i)
        {
          return operators[i];
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
  std::map<std::vector<int>, std::vector<int>> comput_input_and_set_map3( input_type3 type );
  std::map<int, std::vector<unsigned>> comput_select_vars_map3( int nr_steps, int nr_in );
  int comput_select_vars_for_each_step3( int nr_steps, int nr_in, int step_idx ); 

  /* mig3 to expressions */
  std::string mig3_to_string( const spec& spec, const mig3& mig3 );
  std::string print_expr( const mig3& mig3, const int& step_idx );
  std::string print_all_expr( const spec& spec, const mig3& mig3 );

}

#endif
