/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file rm3ig_helper.hpp
 *
 * @brief Helper for rm3ig chain extraction and rm3ig encoder
 * selection variables generation
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef RM3IG_HELPER_HPP
#define RM3IG_HELPER_HPP

#include "../networks/rm3/RM3.hpp"
#include "misc.hpp"

namespace also
{
enum type_input
{
  none_const1,
  first_const1,
  second_const1,
  third_const1
};

/******************************************************************************
 * class rmig3 for chain manipulation                                          *
 ******************************************************************************/
class rm3ig
{
private:
  int nr_in;
  std::vector<int> outputs;
  using step = std::array<int, 3>;

public:
  std::vector<std::array<int, 3>> steps;
  std::vector<int> operators;

  rm3ig()
  {
    reset( 0, 0, 0 );
  }

  std::array<int, 3> get_step_inputs( int i )
  {
    std::array<int, 3> array;

    array[0] = steps[i][0];
    array[1] = steps[i][1];
    array[2] = steps[i][2];

    return array;
  }

  int get_op( int i )
  {
    return operators[i];
  }

  bool is_output_inverted( int out_idx )
  {
    return outputs[out_idx] & 1;
  }

  void reset( int _nr_in, int _nr_out, int _nr_steps )
  {
    assert( _nr_steps >= 0 && _nr_out >= 0 );
    nr_in = _nr_in;
    steps.resize( _nr_steps );
    operators.resize( _nr_steps );
    outputs.resize( _nr_out );
  }

  int get_nr_steps() const { return steps.size(); }

  kitty::dynamic_truth_table RM3( kitty::dynamic_truth_table a,
                                  kitty::dynamic_truth_table b,
                                  kitty::dynamic_truth_table c ) const
  {
    return ( a & ( ~b ) ) | ( a & c ) | ( ( ~b ) & c );
    // return kitty::ternary_rm3(a, b, c);
  }

  std::vector<kitty::dynamic_truth_table> simulate() const
  {
    std::vector<kitty::dynamic_truth_table> fs( outputs.size() );
    std::vector<kitty::dynamic_truth_table> tmps( steps.size() );

    kitty::dynamic_truth_table tt_in1( nr_in );
    kitty::dynamic_truth_table tt_in2( nr_in );
    kitty::dynamic_truth_table tt_in3( nr_in );

    auto tt_step = kitty::create<kitty::dynamic_truth_table>( nr_in );
    auto tt_inute = kitty::create<kitty::dynamic_truth_table>( nr_in );

    for ( auto i = 0u; i < steps.size(); i++ )
    {
      const auto& step = steps[i];

      if ( step[0] <= nr_in )
      {
        if ( step[0] == 0 )
        {
          create_nth_var( tt_in1, 0 );
          kitty::clear( tt_in1 );
        }
        else
        {
          create_nth_var( tt_in1, step[0] - 1 );
        }
      }
      else
      {
        tt_in1 = tmps[step[0] - nr_in - 1];
      }

      if ( step[1] <= nr_in )
      {
        if ( step[1] == 0 )
        {
          create_nth_var( tt_in2, 0 );
          kitty::clear( tt_in2 );
        }
        else
        {
          create_nth_var( tt_in2, step[1] - 1 );
        }
      }
      else
      {
        tt_in2 = tmps[step[1] - nr_in - 1];
      }

      if ( step[2] <= nr_in )
      {
        // create_nth_var(tt_in3, step[2] - 1);
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

      kitty::clear( tt_step );
      switch ( operators[i] )
      {
      case 0:
        tt_step = RM3( tt_in1, tt_in2, tt_in3 );
        break;
      case 1:
        tt_step = RM3( ~tt_in1, tt_in2, tt_in3 );
        break;
      case 2:
        tt_step = RM3( tt_in1, ~tt_in2, tt_in3 );
        break;
      case 3:
        tt_step = RM3( tt_in1, tt_in2, ~tt_in3 );
        break;
      default:
        assert( false && "ops are not known" );
        break;
      }

      tmps[i] = tt_step;

      for ( auto h = 0u; h < outputs.size(); h++ )
      {
        // fs[h] = tt_step;
        const auto out = outputs[h];
        const auto var = out >> 1;
        const auto inv = out & 1;
        if ( var - nr_in - 1 == static_cast<int>( i ) )
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
      int op )
  {
    steps[i][0] = fanin1;
    steps[i][1] = fanin2;
    steps[i][2] = fanin3;
    operators[i] = op;
  }

  void set_output( int out_idx, int lit )
  {
    outputs[out_idx] = lit;
  }

  int get_output( int out_idx )
  {
    return outputs[out_idx];
  }

  bool satisfies_spec( const percy::spec& spec )
  {
    if ( spec.nr_triv == spec.get_nr_out() )
    {
      return true;
    }
    auto tts = simulate();

    auto nr_nontriv = 0;
    for ( int i = 0; i < spec.nr_nontriv; i++ )
    {
      if ( ( spec.triv_flag >> i ) & 1 )
      {
        continue;
      }

      if ( tts[nr_nontriv++] != spec[i] )
      {
        assert( false );
        return false;
      }
    }

    return true;
  }

  void to_expression( std::ostream& o, const int i )
  {
    if ( i < nr_in )
    {
      o << static_cast<char>( 'a' + i );
    }
    else
    {
      const auto& step = steps[i - nr_in];
      o << "<";
      to_expression( o, step[0] );
      o << "~";
      to_expression( o, step[1] );
      to_expression( o, step[2] );
      o << ">";
    }
  }

  void to_expression( std::ostream& o )
  {
    to_expression( o, nr_in + steps.size() - 1 );
  }
};

/******************************************************************************
 * class comb generate various combinations                                   *
 ******************************************************************************/
class comb4
{
private:
  // 定义一个表格，表示变量的极性
  std::vector<std::vector<int>> polarity{
      { 0, 0, 0 },
      { 1, 0, 0 },
      { 0, 1, 0 },
      { 0, 0, 1 } };

  kitty::static_truth_table<3> a, b, c;
  type_input type;

  // 存储所有可能的真值表
  std::vector<kitty::static_truth_table<3>> all_tt;

public:
  comb4( type_input type )
      : type( type )
  {
    kitty::create_nth_var( a, 0 );
    kitty::create_nth_var( b, 1 );
    kitty::create_nth_var( c, 2 );

    switch ( type )
    {
    case none_const1:
    {
    }
    break;

    case first_const1:
    {
      // 将第一个变量 a 的真值表初始化为 "00"
      kitty::create_from_hex_string( a, "00" );
    }
    break;

    case second_const1:
    {
      kitty::create_from_hex_string( b, "00" );
    }
    break;

    case third_const1:
    {
      kitty::create_from_hex_string( c, "00" );
    }
    break;

    default:
      break;
    }

    all_tt = get_all_tt();
  }

  ~comb4()
  {
  }

  kitty::static_truth_table<3> comput_rm3( int polar_idx )
  {
    assert( polar_idx <= 3 );

    int pa, pb, pc;
    
    // 从预定义的表中获取当前极性下的输入变量值
    pa = polarity[polar_idx][0];
    pb = polarity[polar_idx][1];
    pc = polarity[polar_idx][2];
    
    // 根据输入变量的极性，创建相应的变量
    auto ia = pa ? ~a : a;
    auto ib = pb ? b : ~b;
    auto ic = pc ? ~c : c;

    return kitty::ternary_majority( ia, ib, ic );
    //return kitty::ternary_rm3( ia, ib, ic );
    //return ( ia & ( ~ib ) ) | ( ia & ic ) | ( ( ~ib ) & ic );
  }

  std::vector<kitty::static_truth_table<3>> get_all_tt()
  {
    // 创建一个空向量来存储真值表
    std::vector<kitty::static_truth_table<3>> r;
    //int j = 0;
    for ( int i = 0; i < 4; i++ )
    {
      //j++;
      //std::cout << "j" << j << std::endl;
      //std::cout << " i : " << i << " tt: " << kitty::to_binary( comput_rm3( i ) ) << std::endl;
      // 使用当前值 i 调用 comput_rm3 函数
      // 并将得到的真值表添加到向量中
      r.push_back( comput_rm3( i ) );
    }
    // 返回包含所有真值表的向量
    return r;
  }

  std::vector<std::vector<int>> get_on_set()
  {
    //onset 是指在布尔函数的真值表中，输出结果为1的那些输入组合
    // return the onset index array for the number of idx input
    // we start idx 0 : 000, 7 : 111
    std::vector<std::vector<int>> r;    // 用于存储结果的向量
    std::vector<int> v;                 // 用于暂时存储每个输入组合的索引

    for ( auto i = 0; i < 8; i++ )
    {
      v.clear();
      for ( auto j = 0; j < 4; j++ )
      {
        // 如果真值表中的第 j 个函数的第 i 位是1
        if ( kitty::get_bit( all_tt[j], i ) )
        {
          v.push_back( j );
        }
      }
      // 将当前输入组合的索引数组添加到结果向量中
      r.push_back( v );
    }
    // 返回包含所有输入组合的 onset 索引数组的向量
    return r;
  }

  std::vector<std::vector<int>> get_all_inputs()
  {
    std::vector<std::vector<int>> combinations;
    std::vector<int> entry;

    for ( auto i = 0; i < 8; i++ )
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

    const auto inputs = get_all_inputs();

    const auto set = get_on_set();

    for ( auto i = 0; i < 8; i++ )
    {
      map.insert( std::pair<std::vector<int>, std::vector<int>>( inputs[i], set[i] ) );
    }

    return map;
  }
};

/******************************************************************************
 * class select generate selection variables                                  *
 ******************************************************************************/
class select4
{
private:
  int nr_steps;
  int nr_in;

public:
  select4( int nr_steps, int nr_in )
      : nr_steps( nr_steps ), nr_in( nr_in )
  {
  }

  ~select4()
  {
  }

  // current step starts from 0, 1, ...
  // 根据输入的当前步骤数 current_step 来计算每个步骤中的选择变量总数s
  int get_num_of_sel_vars_for_each_step( int current_step )
  {
    int count = 0;
    int total = nr_in + 1 + current_step;
    std::vector<unsigned> idx_array;

    //count += total;

    for ( auto i = 0; i < total; i++ )
    {
      idx_array.push_back( i );
    }

    // no const & ('a'|'b') const
    count += get_all_combination_index( idx_array, total, 3u ).size() * 3;

    return count;
  }

  int get_num_sels()
  {
    int count = 0;
    for ( auto i = 0; i < nr_steps; i++ )
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

    for ( auto i = 0; i < nr_steps; i++ )
    {
      int total = nr_in + 1 + i;

      idx_array.clear();
      idx_array.resize( total );
      std::iota( idx_array.begin(), idx_array.end(), 0 );

      // no const & ('a' | 'b') const
      for ( const auto c : get_all_combination_index( idx_array, idx_array.size(), 3u ) )
      {
        auto tmp = c;
        tmp.insert( tmp.begin(), i );
        map.insert( std::pair<int, std::vector<unsigned>>( count++, tmp ) );

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

/******************************************************************************
 * Public functions by class comb and select                                  *
 ******************************************************************************/
std::map<std::vector<int>, std::vector<int>> comput_input_and_set_maps( type_input type );
std::map<int, std::vector<unsigned>> comput_select_vars_maps( int nr_steps, int nr_in );
int comput_select_vars_for_each_steps( int nr_steps, int nr_in, int step_idx );

/* rm3ig to expressions */
std::string rm3ig_to_string( const spec& spec, const rm3ig& rm3ig );
std::string print_expr( const rm3ig& rm3ig, const int& step_idx );
std::string print_all_expr( const spec& spec, rm3ig& rm3ig );

rm3_network rm3ig_to_rm3ig_network( const spec& spec, rm3ig& rm3ig );

} // namespace also

#endif
