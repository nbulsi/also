#ifndef RM3_HELP_HPP
#define RM3_HELP_HPP

#include "../networks/rm3/RM3.hpp"
#include "misc.hpp"
#include <kitty/kitty.hpp>
#include <percy/percy.hpp>
#include <vector>

using namespace percy;

namespace also
{
  /******************************************************************************
   * rm3 chain *
   ******************************************************************************/
  class rm3
  {
  private:
      int nr_in;
      std::vector<int> outputs;
      using step = std::array<int, 3>;

  public:
      std::vector<std::array<int, 3>> steps;

      rm3()
      {
          reset(0, 0, 0);
      }

      std::array<int, 3> get_step_inputs(int i)
      {
          std::array<int, 3> array;

          array[0] = steps[i][0];
          array[1] = steps[i][1];
          array[2] = steps[i][2];

          return array;
      }

      bool is_output_inverted( int out_idx )
      {
          return outputs[out_idx] & 1;
      }

      void reset(int _nr_in, int _nr_out, int _nr_steps)
      {
          assert(_nr_steps >= 0 && _nr_out >= 0);
          nr_in = _nr_in;
          steps.resize(_nr_steps);
          outputs.resize(_nr_out);
      }

      int get_nr_steps() const { return steps.size(); }

      kitty::dynamic_truth_table RM3(kitty::dynamic_truth_table a,
                                     kitty::dynamic_truth_table b,
                                     kitty::dynamic_truth_table c) const
      {
          return (a & (~b)) | (a & c) | ((~b) & c);
          //return kitty::ternary_rm3(a, b, c);
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
              const auto &step = steps[i];

              if (step[0] <= nr_in)
              {
                  if (step[0] == 0)
                  {
                      create_nth_var(tt_in1, 0);
                      kitty::clear(tt_in1);
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
                  if (step[1] == 0)
                  {
                      create_nth_var(tt_in2, 0);
                      kitty::clear(tt_in2);
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
                  //create_nth_var(tt_in3, step[2] - 1);
                  if ( step[2] == 0 )
                  {
                      create_nth_var( tt_in3, 0 );
                      kitty::clear( tt_in3 );
                  }
                  else
                  {
                      create_nth_var( tt_in3, step[2] - 1 );
                  }
              }
              else
              {
                  tt_in3 = tmps[step[2] - nr_in - 1];
              }

              kitty::clear(tt_step);

              tt_step = RM3(tt_in1, tt_in2, tt_in3);

              tmps[i] = tt_step;

              for (auto h = 0u; h < outputs.size(); h++)
              {
                  fs[h] = tt_step;
                //   const auto out = outputs[h];
                //   const auto var = out >> 1;
                //   const auto inv = out & 1;
                //   if ( var - nr_in - 1 == static_cast<int>( i ) )
                //   {
                //       fs[h] = inv ? ~tt_step : tt_step;
                //   }
              }
          }

          return fs;
      }

      void set_step(int i, int fanin1, int fanin2, int fanin3)
      {
          steps[i][0] = fanin1;
          steps[i][1] = fanin2;
          steps[i][2] = fanin3;
      }

      void set_output(int out_idx, int lit)
      {
          outputs[out_idx] = lit;
      }

      int get_output(int out_idx)
      {
          return outputs[out_idx];
      }

      bool satisfies_spec(const percy::spec &spec)
      {
          if (spec.nr_triv == spec.get_nr_out())
          {
              return true;
          }
          auto tts = simulate();

          auto nr_nontriv = 0;
          for (int i = 0; i < spec.nr_nontriv; i++)
          {
              // 检查在 spec.triv_flag 变量中的第 i 位是否被设置为 1
            //   if ((spec.triv_flag >> i) & 1)
            //   {
            //       continue;
            //   }
              // 比较仿真结果和特定规范中的相应值是否相等
              if (tts[nr_nontriv++] != spec[i])
              {
                  assert(false);
                  return false;
              }
          }

          return true;
      }

      void to_expression(std::ostream &o, const int i)
      {
          if (i == 0)
          {
              o << "0";
          }
        //   if ( i == 1 )
        //   {
        //     o << "1";
        //   }
          if (i <= nr_in)
          {
              o << static_cast<char>('a' + i - 1 );
          }
          else
          {
              const auto &step = steps[i - nr_in - 1];
              o << "<";
              to_expression(o, step[0]);
              o << "!";
              to_expression(o, step[1]);
              to_expression(o, step[2]);
              o << ">";
          }
      }

      void to_expression(std::ostream &o)
      {
          //to_expression(o, nr_in + steps.size() + 1 );
          to_expression( o, nr_in + steps.size());
      }

      // print rm3 logic expressions for storage, ()
      // indicates the rm3 operation
      void rm3_to_expression(std::stringstream &ss, const int i)
      {
          if (i == 0)
          {
              ss << "0";
          }
        //   else if(i == 1)
        //   {
        //     ss << "1";
        //   }
          else if (i <= nr_in)
          {
              ss << static_cast<char>('a' + i - 1);
          }
          else
          {
              const auto &step = steps[i - nr_in - 1];
              ss << "<";
              rm3_to_expression(ss, step[0]);
              rm3_to_expression(ss, step[1]);
              rm3_to_expression(ss, step[2]);
              ss << ">";
          }
      }

      std::string rm3_to_expression()
      {
          std::stringstream ss;

          //rm3_to_expression(ss, nr_in + steps.size() + 1);
          rm3_to_expression( ss, nr_in + steps.size() );

          return ss.str();
      }  
  };

  std::string rm3_to_string(const spec &spec, const rm3 &rm3)
  {
      if (rm3.get_nr_steps() == 0)
      {
          return "";
      }

      assert( rm3.get_nr_steps() >= 1 );

      std::stringstream ss;

      for (auto i = 0; i < spec.nr_steps; i++)
      {
          ss << i + spec.nr_in + 1 << "-" << rm3.steps[i][0] 
             << rm3.steps[i][1] << rm3.steps[i][2] << " ";
      }

      return ss.str();
  }

  /* create rm3 from string */
  rm3_network rm3_from_expr(const std::string &expr, const unsigned &num_pis)
  {
      rm3_network rm3g;
      std::vector<rm3_network::signal> sig;
      std::stack<rm3_network::signal> inputs;
      sig.push_back(rm3g.get_constant(false));
      //sig.push_back( rm3.get_constant( true ) );

      for (auto i = 0u; i < num_pis; i++)
      {
          sig.push_back(rm3g.create_pi());
      }

      for (auto i = 0ul; i < expr.size(); i++)
      {
          if (expr[i] >= 'a')
          {
              inputs.push(sig[expr[i] - 'a' + 1]);
              //inputs.push( sig[expr[i] - 'a' + 2] );
          }

          if (expr[i] == '0')
          {
              inputs.push(sig[0]);
          }

        //   if ( expr[i] == '1' )
        //   {
        //       inputs.push( sig[1] );
        //   }

          if (expr[i] == '>')
          {
              assert(inputs.size() >= 3u);
              auto x1 = inputs.top();
              inputs.pop();
              auto x2 = inputs.top();
              inputs.pop();
              auto x3 = inputs.top();
              inputs.pop();

              //inputs.push(img.get_node(x1) == 0 ? img.create_not(x2) : img.create_imp(x2, x1));
              //inputs.push( rm3g.get_node( x3 ) == 0 && rm3g.get_node( x1 ) == 1 ? rm3g.create_not( x2 ) : rm3g.create_rm3( x3, x2, x1 ) );
              inputs.push( rm3g.create_rm3( x3, x2, x1 ) );
          }
      }

      rm3g.create_po(inputs.top());

      return rm3g;
  }

  /******************************************************************************
   * class select generate selection variables                                  *
   ******************************************************************************/
  class select_rm3
  {
  private:
      int nr_steps;
      int nr_in;

  public:
      select_rm3(int nr_steps, int nr_in)
          : nr_steps(nr_steps), nr_in(nr_in)
      {
      }

      ~select_rm3()
      {
      }

      // current step starts from 0, 1, ...
      // 根据输入的当前步骤数 current_step 来计算每个步骤中的选择变量总数s
      int get_num_of_sel_vars_for_each_step(int current_step)
      {
          int count = 0;
          int total = nr_in + current_step + 1;
          //int total = nr_in + current_step + 2;  //算上常数0、1
          std::vector<unsigned> idx_array;

          // for total inputs, there are total - 1 combined with 0
          // such as s_0_010, s_0_020, ...
          count += total;

          for (auto i = 1; i < total; i++)
          {
              idx_array.push_back(i);
          }

          // binom( total - 1, 3 ) * 3
          count += get_all_combination_index(idx_array, total, 3u).size() * 3;
          //count += get_all_combination_index( idx_array, total, 3u ).size();

          return count;
      }
      // 计算所有步骤中选择变量的总数量
      int get_num_sels()
      {
          int count = 0;
          for (auto i = 0; i < nr_steps; i++)
          {
              count += get_num_of_sel_vars_for_each_step(i);
          }

          return count;
      }

      /* map: 0,     0         _012
       *      index, nr_steps, inputs ids
       **/
      // 映射将选择变量与它们的索引、步骤和输入ID关联起来
      std::map<int, std::vector<unsigned>> get_sel_var_map()
      {
          // int 是映射的键，代表选择变量的索引。
          // 而vector<unsigned> 是映射的值,值是一个无符号整数向量，包含步骤索引、输入标识和固定值
          std::map<int, std::vector<unsigned>> map;
          std::vector<unsigned> idx_array;
          int count = 0;
          // 基础选择变量
          for (auto i = 0; i < nr_steps; i++)
          {
              int total = nr_in + i + 1;
              //int total = nr_in + i + 2;

              // for total inputs, there are total - 1 combined with 0
              // such as s_0_010, s_0_020, ...
            //   for (auto k = 1; k < total; k++)
            //   {
            //       std::vector<unsigned> c;
            //       c.push_back(i);
            //       c.push_back( 0 );
            //       c.push_back(k);
            //       c.push_back(0);

            //       map.insert(std::pair<int, std::vector<unsigned>>(count++, c));
            //   }

              idx_array.clear();
              idx_array.resize(total);
              std::iota(idx_array.begin(), idx_array.end(), 0);

              // no const & 'a' const
              for (const auto c : get_all_combination_index(idx_array, idx_array.size(), 3u))
              {
                  auto tmp = c;
                  tmp.insert(tmp.begin(), i);
                  map.insert(std::pair<int, std::vector<unsigned>>(count++, tmp));

                  std::vector<unsigned> reverse1;
                  reverse1.push_back( i );
                  reverse1.push_back( c[0] );
                  reverse1.push_back( c[2] );
                  reverse1.push_back( c[1] );
                  map.insert( std::pair<int, std::vector<unsigned>>( count++, reverse1 ) );

                  std::vector<unsigned> reverse2;
                  reverse2.push_back( i );
                  reverse2.push_back( c[1] );
                  reverse2.push_back( c[0] );
                  reverse2.push_back( c[2] );
                  map.insert( std::pair<int, std::vector<unsigned>>( count++, reverse2 ) );

              }
          }

          return map;
      }
  };

  /* public function */
  std::map<int, std::vector<unsigned>> comput_select_rm3_vars_map(int nr_steps, int nr_in)
  {
      select_rm3 s(nr_steps, nr_in);
      return s.get_sel_var_map();
  }

  int comput_select_rm3_vars_for_each_step(int nr_steps, int nr_in, int step_idx)
  {
      assert(step_idx >= 0 && step_idx < nr_steps);
      select_rm3 s(nr_steps, nr_in);
      return s.get_num_of_sel_vars_for_each_step(step_idx);
  }
}

#endif
