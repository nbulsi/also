#pragma once

#include <percy/spec.hpp>
#include <array>

namespace also 
{
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
}
