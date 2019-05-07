/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file m5ig_helper.hpp
 *
 * @brief Helper for m5ig chain extraction and m5ig encoder
 * selection variables generation
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef M5IG_HELPER_HPP
#define M5IG_HELPER_HPP

#include <mockturtle/mockturtle.hpp>
#include "misc.hpp"

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
   * class mig5 for chain manipulation                                          *
   ******************************************************************************/
    class mig5
    {
    private:
        int nr_in;
        std::vector<int> outputs;
        using step = std::array<int, 5>;

    public:
        std::vector<std::array<int, 5>> steps;
        std::vector<int> operators;

        mig5()
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

        kitty::dynamic_truth_table maj5( kitty::dynamic_truth_table a,
                                         kitty::dynamic_truth_table b,
                                         kitty::dynamic_truth_table c,
                                         kitty::dynamic_truth_table d,
                                         kitty::dynamic_truth_table e
                                         ) const 
        {
          auto m1 = kitty::ternary_majority( a, b, c );
          auto m2 = kitty::ternary_majority( a, b, d );
          auto m3 = kitty::ternary_majority( m2, c, d );
          
          return kitty::ternary_majority( m1, m3, e );
        }

        std::vector<kitty::dynamic_truth_table> simulate() const
        {
            std::vector<kitty::dynamic_truth_table> fs(outputs.size());
            std::vector<kitty::dynamic_truth_table> tmps(steps.size());

            kitty::dynamic_truth_table tt_in1(nr_in);
            kitty::dynamic_truth_table tt_in2(nr_in);
            kitty::dynamic_truth_table tt_in3(nr_in);
            kitty::dynamic_truth_table tt_in4(nr_in);
            kitty::dynamic_truth_table tt_in5(nr_in);


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
                
                if (step[3] <= nr_in) 
                {
                    create_nth_var(tt_in4, step[3] - 1);
                } 
                else 
                {
                    tt_in4 = tmps[step[3] - nr_in - 1];
                }
                
                if (step[4] <= nr_in) 
                {
                    create_nth_var(tt_in5, step[4] - 1);
                } 
                else 
                {
                    tt_in5 = tmps[step[4] - nr_in - 1];
                }

                kitty::clear(tt_step);
                switch (operators[i]) 
                {
                case 0:
                    tt_step = maj5( tt_in1, tt_in2, tt_in3, tt_in4, tt_in5 );
                    break;
                case 1:
                    tt_step = maj5( ~tt_in1, tt_in2, tt_in3, tt_in4, tt_in5 );
                    break;
                case 2:
                    tt_step = maj5( tt_in1, ~tt_in2, tt_in3, tt_in4, tt_in5 );
                    break;
                case 3:
                    tt_step = maj5( tt_in1, tt_in2, ~tt_in3, tt_in4, tt_in5 );
                    break;
                case 4:
                    tt_step = maj5( tt_in1, tt_in2, tt_in3, ~tt_in4, tt_in5 );
                    break;
                case 5:
                    tt_step = maj5( tt_in1, tt_in2, tt_in3, tt_in4, ~tt_in5 );
                    break;
                case 6:
                    tt_step = maj5( ~tt_in1, ~tt_in2, tt_in3, tt_in4, tt_in5 );
                    break;
                case 7:
                    tt_step = maj5( ~tt_in1, tt_in2, ~tt_in3, tt_in4, tt_in5 );
                    break;
                case 8:
                    tt_step = maj5( ~tt_in1, tt_in2, tt_in3, ~tt_in4, tt_in5 );
                    break;
                case 9:
                    tt_step = maj5( ~tt_in1, tt_in2, tt_in3, tt_in4, ~tt_in5 );
                    break;
                case 10:
                    tt_step = maj5( tt_in1, ~tt_in2, ~tt_in3, tt_in4, tt_in5 );
                    break;
                case 11:
                    tt_step = maj5( tt_in1, ~tt_in2, tt_in3, ~tt_in4, tt_in5 );
                    break;
                case 12:
                    tt_step = maj5( tt_in1, ~tt_in2, tt_in3, tt_in4, ~tt_in5 );
                    break;
                case 13:
                    tt_step = maj5( tt_in1, tt_in2, ~tt_in3, ~tt_in4, tt_in5 );
                    break;
                case 14:
                    tt_step = maj5( tt_in1, tt_in2, ~tt_in3, tt_in4, ~tt_in5 );
                    break;
                case 15:
                    tt_step = maj5( tt_in1, tt_in2, tt_in3, ~tt_in4, ~tt_in5 );
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
            int fanin4,
            int fanin5,
            int op)
        {
            steps[i][0] = fanin1;
            steps[i][1] = fanin2;
            steps[i][2] = fanin3;
            steps[i][3] = fanin4;
            steps[i][4] = fanin5;
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
                to_expression(o, step[3]);
                to_expression(o, step[4]);
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

}

#endif
