#ifndef SYNTHESISRUN_HPP
#define SYNTHESISRUN_HPP

#include <cassert>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "specification.hpp"
#include "subcircuitSyn.hpp"

using namespace alice;
using namespace mockturtle;

std::string Dag_filename;

class Synthesiser
{

private:
  Specification specification;
  std::unordered_map<int, int> taboo_dict;
  int time_subcircuit_selection;
  bool use_taboo_list;
  SubcircuitSynthesiser subcircuitsyn;
  int total_available_time;
  int mode_root;
  int mode_sub;
  std::chrono::_V2::system_clock::time_point start;
  aig_network aig_new;
  xag_network xag_new;
  std::vector<int> topological_gate;
  std::mt19937 rng;

public:
  Synthesiser( Specification spec, int DAG_Flag ) : specification( spec ),
                                                    time_subcircuit_selection( 0 ),
                                                    use_taboo_list( true ),
                                                    subcircuitsyn( spec, DAG_Flag ),
                                                    total_available_time( 0 ),
                                                    mode_root( 0 ),
                                                    mode_sub( 0 ),
                                                    topological_gate( subcircuitsyn.specification.topological_order ),
                                                    rng( std::random_device{}() )
  {
  }

  Specification reduce( std::pair<int, int> budget, int subcircuit_size, int gate_size ,std::vector<int> mode_select, bool useLocalCircuit)
  {
    int available_time = budget.first;        // 运行时间
    int available_iterations = budget.second; // 运行次数
    total_available_time = available_time;
    start = std::chrono::high_resolution_clock::now();
    mode_root = mode_select[0];
    mode_sub = mode_select[1];
    traverseGates( available_iterations, subcircuit_size, gate_size, useLocalCircuit );

    return specification;
  }

  aig_network getAig()
  {
    aig_new = convert_blif2Aig( Dag_filename );
    std::remove( Dag_filename.c_str() );
    return aig_new;
  }

  xag_network getXag()
  { //"_temp.blif"
    xag_new = convert_blif2Xag( Dag_filename );
    std::remove(Dag_filename.c_str());
    return xag_new;
  }

  void traverseGates( int budget, int subcircuit_size, int nof_inputs, bool useLocalCircuit ) // 运行次数/子电路大小/节点输入个数
  {
    if ( specification.getNofGates() < nof_inputs )
    {
      return;
    }
    randomTraversal( budget, subcircuit_size, nof_inputs, useLocalCircuit );
  }
  // 记录时间
  bool checkTime()
  {
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = now - start;
    return elapsed.count() < total_available_time;
  }
  // 用来输出Vector
  void printVector( const std::vector<int>& vec )
  {
    for ( const auto& value : vec )
    {
      std::cout << value << " ";
    }
    std::cout << std::endl;
  }
  // 生成随机输出文件名
  std::string generateRandomString( size_t length )
  {
    const char charset[] =
        "0123456789"
        "abcdefghijklmnopqrstuvwxyz";
    const size_t max_index = sizeof( charset ) - 1;

    std::random_device rd;    // 获得随机数种子
    std::mt19937 gen( rd() ); // 使用 Mersenne Twister 引擎
    std::uniform_int_distribution<> dis( 0, max_index - 1 );

    std::ostringstream oss;
    for ( size_t i = 0; i < length; ++i )
    {
      oss << charset[dis( gen )];
    }
    return oss.str();
  }

  std::tuple<bool, std::tuple<std::vector<int>, std::unordered_map<int, int>, std::unordered_set<int>>, bool> replaceCircuit( std::vector<int>& to_replace, int nof_inputs, bool require_reduction, bool useLocalCircuit )
  {
    return subcircuitsyn.reduceSubcircuit( to_replace, nof_inputs, require_reduction, useLocalCircuit);
  }
  // 算法的主函数
  void randomTraversal( int budget, int subcircuit_size, int nof_inputs, bool useLocalCircuit )
  {
    bool check_budget = budget > 0;
    int counter = 0;
    int intermediate_counter = 0;
    bool require_reduction = false; // false:允许等面积替换
    int root_gate;
    // 调用函数并传入空集合
    std::unordered_set<int> forbidden;
    
    while ( true )
    {
      if ( check_budget && counter >= budget )
      {
        std::cout << "Available iterations used up. Nof considered subcircuits: " << counter << std::endl;
        return;
      }
      if ( !checkTime() )
      {
        subcircuitsyn.printStats();
        std::string random_string = generateRandomString( 5 ); // 生成5个字符长的随机字符串
        std::string filename = random_string + ".blif";
        Dag_filename = random_string + ".blif";
        writeSpecification( filename, subcircuitsyn.specification );
        std::cout << "Considered subcircuits: " << counter << std::endl;
        return;
      }

      auto start = std::chrono::high_resolution_clock::now();
      counter++;

      int subcircuit_search_counter = 0;
      static unsigned int flag = 0;
      std::vector<int> to_replace;
      // for ( const auto& pair : taboo_dict )
      // {
      //   forbidden.insert( pair.first ); 
      // }
      while ( true )
      {
        subcircuit_search_counter++;

        if ( mode_root == 0 )
        {
          root_gate = getRandomGate(); // 所有门得随机
        }
        else if ( mode_root == 1 )
        {
          root_gate = getReconvergentGate();  //优先考虑ReconvergentGate
        }
 
        if ( root_gate == -2 )
        {
          if ( !flag )
          {
            std::cout << "can't find Reconvergent PATH" << std::endl;
            flag++;
          }
          root_gate = getRandomGate(); // 所有门得随机
        }
        if ( root_gate == -1 )
        {
          std::cout << "Too many subcircuits of size 1 -- it is unlikely to reduce the circuit" << std::endl;
          return;
        }

        if ( mode_sub == 0 )
        {
          to_replace = getSubcircuitGates( root_gate, subcircuit_size ); // 需要被替代的子电路
        }
        else if ( mode_sub == 1 )
        {
          to_replace = getSubcircuitBFS( root_gate, subcircuit_size, forbidden );
        }
        else if ( mode_sub == 2 )
        {
          to_replace = getSubcircuitDFS( root_gate, subcircuit_size, forbidden );
        }

        //子电路太小不支持优化
        if ( to_replace.size() == 1 )
        {
          taboo_dict[root_gate] = counter;
        }
        else
        {
          break;
        }
      }

      auto end = std::chrono::high_resolution_clock::now();
      time_subcircuit_selection += std::chrono::duration_cast<std::chrono::milliseconds>( end - start ).count();
      auto [replaceable, subcir_data, timeout] = replaceCircuit( to_replace, nof_inputs, require_reduction, useLocalCircuit );

      if ( replaceable )
      { // std::vector<int>,std::unordered_map<int,int>,std::unordered_set<int>
        auto [gate_names, output_assoc, unused] = subcir_data;

        bool reduced = gate_names.size() < to_replace.size();

        if ( reduced )
        {
          for ( auto g : to_replace )
          {
            taboo_dict.erase( g );
          }
        }

        for ( auto g : to_replace )
        {
          taboo_dict.erase( g );
        }
        for ( auto g : unused )
        {
          taboo_dict.erase( g );
        }

        if ( specification.getNofGates() == 0 )
        {
          std::cout << "No Gates left.\n";
          return;
        }

        if ( use_taboo_list )
        {
          auto it = output_assoc.find( root_gate );
          if ( it == output_assoc.end() )
          {
            std::cerr << "Root gate not in output association. root: " << root_gate << "\n";
          }
          else
          {
            int root_representation = it->second;
            taboo_dict[root_representation] = counter;
          }
        }
      }
      if ( use_taboo_list )
      {
        taboo_dict[root_gate] = counter;
        auto it = taboo_dict.begin();
        while ( taboo_dict.size() > 0 && taboo_dict.size() >= (size_t)( 0.8 * specification.getNofGates() ) )
        {
          taboo_dict.erase( it++ );
          if ( taboo_dict.size() > 0 )
          {
            it = taboo_dict.begin();
          }
        }
      }
    }
  }

  int getRandomGate() // root门挑选
  {
    auto gates = subcircuitsyn.specification.getGateAliasesSet();
    std::vector<int> gate_var_list;
    std::set<int> taboo_keys;
    for ( const auto& item : taboo_dict )
    {
      taboo_keys.insert( item.first );
    }
    std::set_difference( gates.begin(), gates.end(), taboo_keys.begin(), taboo_keys.end(), std::back_inserter( gate_var_list ) );

    if ( gate_var_list.empty() )
    {
      return -1;
    }
    std::random_device rd;
    std::mt19937 gen( rd() );
    std::uniform_int_distribution<> dis( 0, gate_var_list.size() - 1 );
    int rv = dis( gen );
    return gate_var_list[rv];
  }

  int getTopoGate()
  {
    auto gates = subcircuitsyn.specification.topological_order;
    std::vector<int> gate_var_list;
    std::set<int> taboo_keys;
    for ( const auto& item : taboo_dict )
    {
      taboo_keys.insert( item.first );
    }
    std::set_difference( gates.begin(), gates.end(), taboo_keys.begin(), taboo_keys.end(), std::back_inserter( gate_var_list ) );
    if ( gate_var_list.empty() )
    {
      return -3;
    }
    int value = topological_gate.front();
    topological_gate.erase( topological_gate.begin() ); // 移除第一个元素
    topological_gate.push_back( value );

    return value;
  }

  int getReconvergentGate()
  {
    auto gates = subcircuitsyn.specification.getReconvergent();
    std::vector<int> gate_var_list;
    std::set<int> taboo_keys;
    for ( const auto& item : taboo_dict )
    {
      taboo_keys.insert( item.first );
    }
    std::set_difference( gates.begin(), gates.end(), taboo_keys.begin(), taboo_keys.end(), std::back_inserter( gate_var_list ) );

    if ( gate_var_list.empty() )
    {
      return -2;
    }
    std::random_device rd;
    std::mt19937 gen( rd() );
    std::uniform_int_distribution<> dis( 0, gate_var_list.size() - 1 );
    int rv = dis( gen );
    return gate_var_list[rv];
  }

  std::vector<int> getSubcircuitGates( int root_gate_var ) // 返回值是构建的子电路
  {
    int size = 6;
    std::unordered_set<int> selected_gates;
    std::unordered_set<int> potential_successors = { root_gate_var };
    std::unordered_set<int> current_output_set;
    auto Circuit_output = specification.getOutputs();
    auto Inputs = specification.getInputs();

    while ( !potential_successors.empty() && selected_gates.size() < size )
    {
      auto it = potential_successors.begin();
      int best_gate = *it;
      std::vector<int> outputs;
      for ( int x : specification.getGateOutputs( best_gate ) )
      {
        if ( selected_gates.find( x ) == selected_gates.end() ) // POs need to be considered
        {
          outputs.push_back( x );
        }
      }
      int best_nof_outputs = outputs.size();

      if ( std::find( Circuit_output.begin(), Circuit_output.end(), best_gate ) != Circuit_output.end() ) // 检查当前best_gate门是否是电路的输出之一
      {
        best_nof_outputs++;
      }
      int best_nof_inputs = 0;
      for ( int x : specification.getGateInputs( best_gate ) )
      {
        if ( selected_gates.find( x ) == selected_gates.end() )
        {
          best_nof_inputs++;
        }
      }
      int best_level = specification.getGateLevel( best_gate ); // 得到输入和层级

      for ( ++it; it != potential_successors.end(); ++it )
      {
        int gate = *it;
        std::vector<int> gate_outputs;
        for ( int x : specification.getGateOutputs( gate ) )
        {
          if ( selected_gates.find( x ) == selected_gates.end() )
          {
            gate_outputs.push_back( x );
          }
        }
        int nof_outputs = gate_outputs.size();
        int nof_inputs = 0;
        for ( int x : specification.getGateInputs( gate ) )
        {
          if ( selected_gates.find( x ) == selected_gates.end() )
          {
            nof_inputs++;
          }
        }
        int level = specification.getGateLevel( gate ); // 收集该节点信息

        if ( std::find( Circuit_output.begin(), Circuit_output.end(), gate ) != Circuit_output.end() )
        {
          nof_outputs++;
        }
        if ( current_output_set.find( gate ) != current_output_set.end() )
        {
          nof_outputs--;
        }
        if ( nof_outputs < best_nof_outputs ) // 以节点的输出优先
        {
          best_gate = gate;
          best_nof_inputs = nof_inputs;
          best_nof_outputs = nof_outputs;
          outputs = gate_outputs;
          best_level = level;
        }
        else if ( nof_outputs == best_nof_outputs ) // 输出数量相同，继续比较输入数量和级别以选择最佳门。
        {
          if ( nof_inputs < best_nof_inputs )
          {
            best_gate = gate;
            best_nof_inputs = nof_inputs;
            best_nof_outputs = nof_outputs;
            outputs = gate_outputs;
            best_level = level;
          }
          else if ( nof_inputs == best_nof_inputs && level < best_level ) // 层级对比
          {
            best_gate = gate;
            best_nof_inputs = nof_inputs;
            best_nof_outputs = nof_outputs;
            outputs = gate_outputs;
            best_level = level;
          }
        }
      }
      selected_gates.insert( best_gate );
      potential_successors.erase( best_gate );
      current_output_set.insert( outputs.begin(), outputs.end() );
      for ( int x : specification.getGateInputs( best_gate ) )
      {
        if ( std::find( Inputs.begin(), Inputs.end(), x ) != Inputs.end() && selected_gates.find( x ) == selected_gates.end() )
        {
          potential_successors.insert( x );
        }
      }
    }
    // The root gate shall be the first element of the list
    selected_gates.erase( root_gate_var );
    std::vector<int> result = { root_gate_var };
    result.insert( result.end(), selected_gates.begin(), selected_gates.end() );
    return result;
  }
  // 优先PO最小的方式
  std::vector<int> getSubcircuitGates( int root_gate_var, int size )
  {
    std::unordered_set<int> selected_gates;
    std::unordered_set<int> potential_successors = { root_gate_var };
    std::unordered_set<int> current_output_set;
    auto Circuit_output = subcircuitsyn.specification.getOutputs();
    auto Inputs = subcircuitsyn.specification.getInputs();

    while ( !potential_successors.empty() && selected_gates.size() < size )
    {
      auto it = potential_successors.begin();
      int best_gate = *it;
      // std::cout <<  best_gate << std::endl;
      std::vector<int> outputs;

      try
      {
        for ( int x : subcircuitsyn.specification.getGateOutputs( best_gate ) )
        {
          if ( selected_gates.find( x ) == selected_gates.end() )
          {
            outputs.push_back( x );
          }
        }
      }
      catch ( const std::out_of_range& e )
      {
        std::cerr << "Error: getGateOutputs out of range for best_gate: " << best_gate << std::endl;
        throw;
      }

      int best_nof_outputs = outputs.size();

      if ( std::find( Circuit_output.begin(), Circuit_output.end(), best_gate ) != Circuit_output.end() )
      {
        best_nof_outputs++;
      }

      int best_nof_inputs = 0;

      try
      {
        for ( int x : subcircuitsyn.specification.getGateInputs( best_gate ) )
        {
          if ( selected_gates.find( x ) == selected_gates.end() )
          {
            best_nof_inputs++;
          }
        }
      }
      catch ( const std::out_of_range& e )
      {
        std::cerr << "Error: getGateInputs out of range for best_gate: " << best_gate << std::endl;
        throw;
      }

      int best_level;
      try
      {
        best_level = subcircuitsyn.specification.getGateLevel( best_gate );
      }
      catch ( const std::out_of_range& e )
      {
        std::cerr << "Error: getGateLevel out of range for best_gate: " << best_gate << std::endl;
        throw;
      }

      for ( ++it; it != potential_successors.end(); ++it )
      {
        int gate = *it;
        std::vector<int> gate_outputs;

        try
        {
          for ( int x : subcircuitsyn.specification.getGateOutputs( gate ) )
          {
            if ( selected_gates.find( x ) == selected_gates.end() )
            {
              gate_outputs.push_back( x );
            }
          }
        }
        catch ( const std::out_of_range& e )
        {
          std::cerr << "Error: getGateOutputs out of range for gate: " << gate << std::endl;
          throw;
        }

        int nof_outputs = gate_outputs.size();
        int nof_inputs = 0;

        try
        {
          for ( int x : subcircuitsyn.specification.getGateInputs( gate ) )
          {
            if ( selected_gates.find( x ) == selected_gates.end() )
            {
              nof_inputs++;
            }
          }
        }
        catch ( const std::out_of_range& e )
        {
          std::cerr << "Error: getGateInputs out of range for gate: " << gate << std::endl;
          throw;
        }

        int level;
        try
        {
          level = subcircuitsyn.specification.getGateLevel( gate );
        }
        catch ( const std::out_of_range& e )
        {
          std::cerr << "Error: getGateLevel out of range for gate: " << gate << std::endl;
          throw;
        }

        if ( std::find( Circuit_output.begin(), Circuit_output.end(), gate ) != Circuit_output.end() )
        {
          nof_outputs++;
        }
        if ( current_output_set.find( gate ) != current_output_set.end() )
        {
          nof_outputs--;
        }
        if ( nof_outputs < best_nof_outputs )
        {
          best_gate = gate;
          best_nof_inputs = nof_inputs;
          best_nof_outputs = nof_outputs;
          outputs = gate_outputs;
          best_level = level;
        }
        else if ( nof_outputs == best_nof_outputs )
        {
          if ( nof_inputs < best_nof_inputs )
          {
            best_gate = gate;
            best_nof_inputs = nof_inputs;
            best_nof_outputs = nof_outputs;
            outputs = gate_outputs;
            best_level = level;
          }
          else if ( nof_inputs == best_nof_inputs && level < best_level )
          {
            best_gate = gate;
            best_nof_inputs = nof_inputs;
            best_nof_outputs = nof_outputs;
            outputs = gate_outputs;
            best_level = level;
          }
        }
      }
      selected_gates.insert( best_gate );
      potential_successors.erase( best_gate );
      current_output_set.insert( outputs.begin(), outputs.end() );
      for ( int x : subcircuitsyn.specification.getGateInputs( best_gate ) )
      {
        if ( std::find( Inputs.begin(), Inputs.end(), x ) == Inputs.end() && selected_gates.find( x ) == selected_gates.end() )
        {
          potential_successors.insert( x );
        }
      }
    }
    // The root gate shall be the first element of the list
    selected_gates.erase( root_gate_var );
    std::vector<int> result = { root_gate_var };
    result.insert( result.end(), selected_gates.begin(), selected_gates.end() );
    return result;
  }

  // BFS方式构造子电路
  std::vector<int> getSubcircuitBFS( int root_gate, int size, const std::unordered_set<int>& forbidden )
  {
    std::unordered_set<int> selected_gates;
    std::unordered_set<int> potential_successors = { root_gate }; // 潜在的候选节点
    std::unordered_set<int> next_level_to_consider;

    auto Circuit_output = subcircuitsyn.specification.getOutputs();
    auto Inputs = subcircuitsyn.specification.getInputs();

    // BFS构造逻辑
    while ( !potential_successors.empty() && selected_gates.size() < size )
    {
      int successor = *potential_successors.begin();              // 选取候选集中的第一个节点
      potential_successors.erase( potential_successors.begin() ); // 从候选集中移除
      if ( forbidden.find( successor ) != forbidden.end() )
      {
        continue; // 跳过被禁止的节点
      }

      selected_gates.insert( successor ); // 将节点加入选中的集合

      // 获取后继节点并加入到下一层的考虑集
      std::set<int> successors = subcircuitsyn.specification.getGateOutputs( successor );
      next_level_to_consider.insert( successors.begin(), successors.end() );

      // 当当前层候选节点为空时，切换到下一层节点
      if ( potential_successors.empty() )
      {
        potential_successors.swap( next_level_to_consider );
        for ( int gate : selected_gates )
        {
          potential_successors.erase( gate ); // 移除已选节点
        }
        for ( int gate : forbidden )
        {
          potential_successors.erase( gate ); // 移除禁止节点
        }
      }
    }

    // 清理并返回结果
    selected_gates.erase( root_gate );
    std::vector<int> result = { root_gate };
    result.insert( result.end(), selected_gates.begin(), selected_gates.end() );
    return result;
  }

  //带有随机性的类DFS方式构造子电路
  std::vector<int> getSubcircuitDFS( int root_gate, int size, const std::unordered_set<int>& forbidden )
  {
    std::unordered_set<int> selected_gates;
    std::unordered_set<int> potential_successors = { root_gate }; // 潜在的候选节点
    std::unordered_set<int> next_level_to_consider;

    auto Circuit_output = subcircuitsyn.specification.getOutputs();
    auto Inputs = subcircuitsyn.specification.getInputs();

    // 随机化
    while ( !potential_successors.empty() && selected_gates.size() < size )
    {
      // 随机选取候选集中的一个节点
      int successor = *potential_successors.begin();
      potential_successors.erase( potential_successors.begin() ); // 从候选集中移除
      if ( forbidden.find( successor ) != forbidden.end() )
      {
        continue; // 跳过被禁止的节点
      }

      selected_gates.insert( successor ); // 将节点加入选中的集合

      // 获取后继节点并根据概率边界插入下一层的候选集
      std::set<int> successors = subcircuitsyn.specification.getGateOutputs( successor );
      for ( int next_gate : successors )
      {
        std::uniform_real_distribution<> dist( 0.0, 1.0 );
        if ( dist( rng ) < 0.8 )
        { // 随机概率边界
          next_level_to_consider.insert( next_gate );
        }
      }

      // 当当前层候选节点为空时，切换到下一层节点，并应用概率筛选
      if ( potential_successors.empty() )
      {
        potential_successors.swap( next_level_to_consider );
        for ( int gate : selected_gates )
        {
          potential_successors.erase( gate ); // 移除已选节点
        }
        for ( int gate : forbidden )
        {
          potential_successors.erase( gate ); // 移除禁止节点
        }
      }
    }

    // 清理并返回结果
    selected_gates.erase( root_gate );
    std::vector<int> result = { root_gate };
    result.insert( result.end(), selected_gates.begin(), selected_gates.end() );
    return result;
  }

  std::string getAuxNodeAlias( int counter )
  {
    return "_aux_" + std::to_string( counter ) + "_";
  }

  std::unordered_map<int, int> setOutputs( std::ofstream& out, Specification& spec, const std::unordered_set<int>& outputs_in_both_polarities )
  {
    std::vector<std::string> out_str;
    std::vector<int> outputs = spec.getOutputs();
    std::unordered_set<int> inputs( spec.getInputs().begin(), spec.getInputs().end() );
    int max_var = spec.max_var;
    std::unordered_map<int, int> aux_var_dict;
    std::string str_to_append;

    for ( const int x : outputs_in_both_polarities )
    {
      aux_var_dict[x] = ++max_var;
    }

    for ( size_t idx = 0; idx < outputs.size(); ++idx )
    {
      int o = outputs[idx];
      if ( inputs.count( o ) )
      {
        if ( spec.isOutputNegated( idx ) )
        {
          ++max_var;
          str_to_append += ".names " + std::to_string( o ) + " " + std::to_string( max_var ) + "\n0 1\n";
          out_str.push_back( std::to_string( max_var ) );
        }
        else
        {
          out_str.push_back( std::to_string( o ) );
        }
      }
      else if ( outputs_in_both_polarities.count( o ) && spec.isOutputNegated( idx ) )
      {
        out_str.push_back( std::to_string( aux_var_dict[o] ) );
      }
      else
      {
        out_str.push_back( std::to_string( o ) );
      }
    }

    std::unordered_set<std::string> seen;
    int counter = 1;
    std::unordered_map<std::string, std::string> alias_association;
    for ( size_t idx = 0; idx < out_str.size(); ++idx )
    {
      const std::string& o = out_str[idx];
      if ( seen.count( o ) )
      {
        std::string newAlias = getAuxNodeAlias( counter );
        alias_association[newAlias] = o;
        out_str[idx] = newAlias;
        ++counter;
      }
      else
      {
        seen.insert( o );
      }
    }

    out << ".outputs " << out_str[0];
    for ( size_t i = 1; i < out_str.size(); ++i )
    {
      out << " " << out_str[i];
    }
    out << "\n"
        << str_to_append;

    for ( const auto& [new_alias, old_alias] : alias_association )
    {
      out << ".names " << old_alias << " " << new_alias << "\n1 1\n";
    }

    return aux_var_dict;
  }

  void writeSpecification( const std::string& fname, Specification& spec, const std::string& spec_name = "spec" )
  {
    std::ofstream file( fname );
    if ( file.is_open() )
    {
      writeSpecification2Stream( file, spec, spec_name );
    }
    else
    {
      std::cout << "Can't build new file " << std::endl;
    }
  }

  void writeSpecification2Stream( std::ofstream& out, Specification& spec, const std::string& spec_name = "spec" )
  {
    out << ".model " << spec_name << "\n";
    out << ".inputs ";
    for ( const auto& input : spec.getInputs() )
    {
      out << input << " ";
    }
    out << "\n";

    auto [negated_outputs, outputs_in_both_polarities] = spec.getOutputsToNegate();

    for ( const auto& pi : spec.getInputs() )
    {
      negated_outputs.erase( pi );
    }

    std::unordered_map<int, int> aux_var_dict = setOutputs( out, spec, outputs_in_both_polarities );

    for ( const auto& gate : spec.orderedGateTraversal() )
    {
      int alias = gate.getAlias();
      std::vector<bool> table = gate.table; // Assuming table is std::vector<bool>
      auto inputs = gate.inputs;
      if ( negated_outputs.find( alias ) != negated_outputs.end() )
      {
        table = negateTable( table );
        writeBlifGate( out, alias, table, inputs, negated_outputs );
      }
      else if ( outputs_in_both_polarities.find( alias ) != outputs_in_both_polarities.end() )
      {
        writeBlifGate( out, alias, table, inputs, negated_outputs );
        int alias_negated = aux_var_dict[alias];
        table = negateTable( table );
        std::vector<bool> table_const = { 1, 0 };
        std::vector<int> inputs_const = { alias };
        writeBlifGate( out, alias_negated, table_const, inputs_const, negated_outputs );
      }
      else
      {
        writeBlifGate( out, alias, table, inputs, negated_outputs );
      }
    }
    // 处理po直连常数
    std::vector<int> to_process;
    for ( const auto& x : spec.pos )
    {
      if ( x == 0 )
      {
        to_process.push_back( x );
      }
    }
    if ( !to_process.empty() )
    {
      out << ".names 1'b0\n";
      out << "0\n";

      int output = to_process.back();
      to_process.pop_back();

      if ( negated_outputs.find( output ) != negated_outputs.end() )
      {
        out << ".names 1'b0 " << output << "\n";
        out << "0 1\n";
      }
      else
      {
        out << ".names 1'b0 " << output << "\n";
        out << "0 0\n";
      }
    }

    out << ".end\n";
  }

  void writeBlifGate( std::ofstream& file, int alias, const std::vector<bool>& table, const std::vector<int>& inputs, const std::unordered_set<int>& negated_gates )
  {
    file << ".names ";
    for ( const auto& input : inputs )
    {
      file << input << " ";
    }
    file << alias << "\n";

    std::unordered_set<size_t> negated_inputs_indices;
    for ( size_t idx = 0; idx < inputs.size(); ++idx )
    {
      if ( negated_gates.find( inputs[idx] ) != negated_gates.end() )
      {
        negated_inputs_indices.insert( idx );
      }
    }

    for ( size_t idx = 0; idx < table.size(); ++idx )
    {
      if ( table[idx] )
      {
        auto binary_representation = getBitSeq( idx, inputs.size() );
        for ( size_t i = 0; i < inputs.size(); ++i )
        {
          if ( negated_inputs_indices.find( i ) != negated_inputs_indices.end() )
          {
            binary_representation[i] = !binary_representation[i];
          }
        }
        for ( const auto& bit : binary_representation )
        {
          file << bit;
        }
        file << " 1\n";
      }
    }
  }

  std::vector<int> getBitSeq( int value, size_t length )
  {
    std::vector<int> bit_seq( length );
    for ( size_t i = 0; i < length; ++i )
    {
      bit_seq[length - i - 1] = ( value >> i ) & 1;
    }
    return bit_seq;
  }

  std::vector<bool> negateTable( const std::vector<bool>& table )
  {
    std::vector<bool> negated_table( table.size() );
    std::transform( table.begin(), table.end(), negated_table.begin(), std::logical_not<bool>() );
    return negated_table;
  }

  aig_network convert_blif2Aig( const std::string& fname )
  {
    klut_network klut;
    if ( lorina::read_blif( fname, mockturtle::blif_reader( klut ) ) != lorina::return_code::success )
    {
      std::cout << "[w] parse error\n";
    }
    aig_network aig;
    aig = convert_klut_to_graph<aig_network>( klut );
    aig = cleanup_dangling( aig );
    return aig;
  }

  xag_network convert_blif2Xag( const std::string& fname )
  {
    klut_network klut;
    if ( lorina::read_blif( fname, mockturtle::blif_reader( klut ) ) != lorina::return_code::success )
    {
      std::cout << "[w] parse error\n";
    }
    xag_network xag;
    xag = convert_klut_to_graph<xag_network>( klut );
    xag = cleanup_dangling( xag );
    return xag;
  }
};

template<typename Ntk>
class SynthesisManager
{
private:
  Specification specification_origin;
  int initial_nof_gates;
  int initial_depth;
  unsigned int times_runs;
  unsigned int flag;
  int mode_root;
  int mode_sub;
  aig_network aig_new;
  xag_network xag_new;
  std::chrono::time_point<std::chrono::high_resolution_clock> start;

public:
  SynthesisManager()
  {
  }
  SynthesisManager( Ntk _ntk, unsigned int Flag )
  {
    times_runs = 1;
    flag = Flag;
    specification_origin = getSpecification( _ntk );
    initial_nof_gates = specification_origin.getNofGates();
    initial_depth = specification_origin.getDepth();
    start = std::chrono::high_resolution_clock::now();
  }
  void set_mode_root( int mode) { mode_root = mode; }
  void set_mode_sub( int mode) { mode_sub = mode; }
  aig_network getAig(){ return aig_new; }
  xag_network getXag(){return xag_new;}
  void reduce( std::pair<int, int> budget , bool useLocalCircuit ) // 输入是时间,是否分区
  {
    //std::cout<<"useLocalCircuit" <<useLocalCircuit<<std::endl;
    double total_abc_time = 0;
    int reduced_by_abc = 0;
    int subcircuit_size = 6;
    int gate_input_size = 2;
    for ( int i = 0; i < times_runs; ++i )
    {
      Specification synth = applyReduction( budget, subcircuit_size, gate_input_size, useLocalCircuit );
    }
    // total_time();
    // total_time_ms();
  }

  Specification applyReduction( std::pair<int, int> budget, int subcircuit_size, int gate_size, bool useLocalCircuit )
  {

    Synthesiser synthesiser( specification_origin, flag );
    // synthesiser.writeSpecification( "2.blif", specification_origin );测试用
    std::vector<int> mode_select = { mode_root, mode_sub };
    Specification spec_new = synthesiser.reduce( budget, subcircuit_size, gate_size, mode_select, useLocalCircuit );
    if ( flag == 1 )
    {
      aig_new = synthesiser.getAig();
    }
    else if ( flag == 2 )
    {
      xag_new = synthesiser.getXag();
    }
    return spec_new;
  }

  void total_time()
  {
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = end - start;
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>( elapsed );
    std::cout << "Elapsed time: " << elapsed_seconds.count() << " s" << std::endl;
    return;
  }

  void total_time_ms()
  {
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    std::cout << "Elapsed time: " << elapsed.count() << " ms" << std::endl;
    return;
  }
};

#endif // SYNTHESISRUN_H