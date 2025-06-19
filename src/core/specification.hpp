#ifndef SPECIFICATION_HPP
#define SPECIFICATION_HPP
#include <algorithm>

#include <bitset>
#include <cassert>
#include <cmath>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <stack>
#include <string>
#include <tuple>
#include <unordered_set>
#include <vector>

#include <alice/alice.hpp>
#include <kitty/kitty.hpp>
#include <lorina/diagnostics.hpp>
#include <lorina/genlib.hpp>

#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/mockturtle.hpp>
#include <mockturtle/views/names_view.hpp>

#pragma region Gate
class Gate
{
public:
  int gate_alias;
  std::vector<int> inputs;
  std::vector<bool> table;

public:
  Gate() {}
	~Gate() {}
  Gate( int gate_alias, const std::vector<int> inputs, std::vector<bool> table )
      : gate_alias( gate_alias ), inputs( inputs ), table( table )
  {
  }
  //输入连接关系 进行替换
  std::vector<int> substitute( const std::unordered_map<int, int>& renaming , std::vector<int>& inputs)
  {
    std::vector<size_t> removed;
    std::vector<int> renamedInputs;

    for ( size_t idx = 0; idx < inputs.size(); ++idx )
    {
      auto it = renaming.find( inputs[idx] );
      if ( it != renaming.end() && it->second == -1 )
      {
        removed.push_back( idx );
      }
    }
    // 重命名
    for ( auto x : inputs )
    {
      auto it = renaming.find( x );
      if ( it != renaming.end() && it->second != -1 )
      {
        renamedInputs.push_back( it->second );
      }
      else 
      {
        renamedInputs.push_back( x );
      }
    }

    if ( !removed.empty() )
    {
      reduceTable( removed );
    }

    if ( !inputs.empty() )
    {
      inputs = std::move( renamedInputs );
    }

    return inputs;
  }
   //输入连接关系 进行替换
  std::vector<int> substitute( const std::unordered_map<int, int>& renaming )
  {
    std::vector<size_t> removed;
    std::vector<int> renamedInputs;

    for ( size_t idx = 0; idx < inputs.size(); ++idx )
    {
      auto it = renaming.find( inputs[idx] );
      if ( it != renaming.end() && it->second == -1 )
      {
        removed.push_back( idx );
      }
    }
    // 重命名
    for ( auto x : inputs )
    {
      auto it = renaming.find( x );
      if ( it != renaming.end() && it->second != -1 )
      {
        renamedInputs.push_back( it->second );
      }
      else 
      {
        renamedInputs.push_back( x );
      }
    }

    if ( !removed.empty() )
    {
      reduceTable( removed );
    }

    if ( !inputs.empty() )
    {
      inputs = std::move( renamedInputs );
    }

    return inputs;
  }

  void reduceTable( const std::vector<size_t>& to_remove )
  {
    for ( auto idx : to_remove )
    {
      size_t reversed_idx = inputs.size() - 1 - idx;
      std::vector<bool> newTable;

      for ( size_t i = 0; i < table.size(); i++ )
      {
        if ( i % ( 2 * size_t( std::pow( 2, reversed_idx + 1 ) ) ) < size_t( std::pow( 2, reversed_idx ) ) )
        {
          newTable.push_back( table[i] );
        }
      }
      table = std::move( newTable );
    }

    bool constant_false = true;
    for ( auto bit : table ) // 检查是否全0
    {
      if ( bit )
      {
        constant_false = false;
        break;
      }
    }
    if ( constant_false )
    {
      inputs.clear();
      table = std::vector<bool>( 1, false );
    }
  }

  bool isConstant() const
  {
    return inputs.empty();
  }

  int getAlias() const
  {
    return gate_alias;
  }

  std::pair<bool, std::vector<std::vector<int>>> getQCIRGates( const std::vector<int>& names = std::vector<int>() ) const
  {
    assert( names.empty() || names.size() == inputs.size() );

    const auto& input_names = names.empty() ? inputs : names;

    // If the gate is false for the majority of the input combinations we
    // represent the gate by a disjunction of conjunctions, otherwise by
    // a conjunction of disjunctions.
    bool anded = std::count_if( table.begin(), table.end(), []( int x )
                                { return x > 0; } ) <= ( 1 << ( inputs.size() - 1 ) );
    int val = anded ? 1 : 0;
    std::vector<std::vector<int>> lines;

    for ( size_t idx = 0; idx < table.size(); ++idx )
    {
      if ( table[idx] == val )
      {
        std::vector<int> line;
        auto bit_seq = getBitSeq( idx, inputs.size() );
        for ( size_t i = 0; i < bit_seq.size(); ++i )
        {
          line.push_back( bit_seq[i] == val ? input_names[i] : -input_names[i] );
        }
        lines.push_back( line );
      }
    }

    return { anded, lines };
  }

  // 整数转为二进制
  std::vector<int> getBitSeq( int num, int len ) const
  {
    std::vector<int> seq;
    for ( int i = len - 1; i > -1; --i )
    {
      seq.push_back( ( num >> i ) & 1 );
    }
    return seq;
  }
  
};
#pragma endregion

class Specification
{
public:
  std::vector<int> pis;
  std::vector<int> pos;
  std::unordered_set<int> pos_set;
  int max_var;

  std::vector<bool> negated_pos;
  int constant_gate_alias = -1;
  std::map<int, Gate> alias2gate;
  std::map<int, std::set<int>> alias2outputs;
  std::map<int, int> alias2level;
  std::vector<int> topological_order;

public:
  Specification() {}
  ~Specification() {}
  Specification( std::vector<int> PIs, std::vector<int> POs )
      : pis( PIs ),
        pos( POs ),
        max_var( -1 )
  {
    for ( int value : pis )
    {
      if ( value > max_var )
      {
        max_var = value;
      }
    }
    this->pos_set = std::unordered_set<int>( pos.begin(), pos.end() );
    this->negated_pos = std::vector<bool>( pos.size(), false );
  }

  void init( bool ordered_gate = true )
  {
    if ( !ordered_gate )
    {
      setGateOutputs();
    }
    removeConstantGates();
    setGateLevels();
		//printAlias2Gate( alias2gate );
  }

  std::vector<Gate> orderedGateTraversal() const
  {
    std::vector<Gate> traversal;
    for ( const auto& x : topological_order )
    {
      traversal.push_back( alias2gate.at( x ) );
    }
    return traversal;
  }
  void toggleOutputNegation( int alias )
  {
    std::vector<size_t> indices = getAllIndices( pos, alias );
    for ( size_t i : indices )
    {
      negated_pos[i] = !negated_pos[i];
    }
  }

  std::vector<size_t> getAllIndices( const std::vector<int>& lst, int val )
  {
    std::vector<size_t> indices;
    for ( size_t i = 0; i < lst.size(); ++i )
    {
      if ( lst[i] == val )
      {
        indices.push_back( i );
      }
    }
    return indices;
  }

  void addGate( int gate_alias, const std::vector<int> inputs, std::vector<bool> table )
  {
    assert( table.size() == static_cast<size_t>( 1 ) << inputs.size() );
    assert( isNormalised( table ) ); // 平凡函数

    max_var = std::max( max_var, gate_alias );
    for ( auto x : inputs )
    {
      alias2outputs[x].insert( gate_alias );
    }
    alias2gate[gate_alias] = Gate( gate_alias, inputs, table );
    alias2outputs[gate_alias] = std::set<int>(); // 初始化
    alias2level[gate_alias] = -1;
  }
  void addGateUnsorted( int gate_alias, const std::vector<int>& inputs, std::vector<bool>& table )
  {
    assert( table.size() == static_cast<size_t>( 1 ) << inputs.size() );
    assert( isNormalised( table ) ); // 平凡函数

    max_var = std::max( max_var, gate_alias );
    alias2gate[gate_alias] = Gate( gate_alias, inputs, table );

    alias2outputs[gate_alias] = std::set<int>(); // 初始化
    alias2level[gate_alias] = -1;
  }
  //测试是否正确存储
  void printAlias2Gate( const std::map<int, Gate>& alias2gate )
  {
    for ( const auto& pair : alias2gate )
    {
      std::cout << "Alias: " << pair.first << std::endl;
      std::cout << "Gate alias: " << pair.second.gate_alias << std::endl;
      std::cout << "Inputs: ";
      for ( int input : pair.second.inputs )
      {
        std::cout << input << " ";
      }
      std::cout << std::endl;

      std::cout << "Table: ";
      for ( bool value : pair.second.table )
      {
        std::cout << ( value ? "1" : "0" ) << " ";
      }
      std::cout << std::endl;
      std::cout << "-----------------------" << std::endl;
    }
  }
  
  void printStats()
  {
    std::cout << std::endl << std::endl;
    printAlias2Gate( alias2gate );
  }
  
  
  bool isNormalised( const std::vector<bool> table ) { return table[0] == 0; }

  void setGateOutputs()
  {
    std::unordered_set<int> pis_set( pis.begin(), pis.end() );
    // A PI may be an PO or constant
    std::vector<int> to_process;
    for ( auto& x : pos )
    {
      if ( pis_set.count( x ) == 0 && x != 0 )
      {
        to_process.push_back( x );
      }
    }
    std::set<int> seen( to_process.begin(), to_process.end() );

    while ( !to_process.empty() )
    {
      int alias = to_process.back();
      to_process.pop_back();
      Gate& gate = getGate( alias );
      for ( auto& x : gate.inputs )
      {
        alias2outputs[x].insert( alias );
        if ( seen.count( x ) == 0 && pis_set.count( x ) == 0 )
        {
          seen.insert( x );
          to_process.push_back( x );
        }
      }
    }
  }
  void setGateLevels()
  {
    for ( auto& x : pis )
    {
      alias2level[x] = 0;
    }
    if ( constant_gate_alias != -1 ) // -1代表没有常量门
    {
      alias2level[constant_gate_alias] = 0;
    }
    getTopologicalOrder();
    for ( auto& x : topological_order )
    {
      auto inputs = getGateInputs( x );
      if ( !inputs.empty() )
      {
        int max_level = 0;
        for ( auto& y : inputs )
        {
          max_level = std::max( max_level, alias2level[y] );
        }
        alias2level[x] = 1 + max_level;
      }
      // 否则门是一个常量门，其级别为0
    }
  }
  void setGateLevels(Specification spec )
  {
    for ( auto& x : pis )
    {
      alias2level[x] = 0;
    }
    if ( constant_gate_alias != -1 ) // -1代表没有常量门
    {
      alias2level[constant_gate_alias] = 0;
    }
    getTopologicalOrder(spec);
    for ( auto& x : topological_order )
    {
      auto inputs = getGateInputs( x );
      if ( !inputs.empty() )
      {
        int max_level = 0;
        for ( auto& y : inputs )
        {
          max_level = std::max( max_level, alias2level[y] );
        }
        alias2level[x] = 1 + max_level;
      }
      // 否则门是一个常量门，其级别为0
    }
  }

 
  void removeConstantGates() // 合并所有的常量节点
  {
    std::set<int> constant_gates;
    for ( auto& x : getGateAliases() )
    {
      if ( getGateInputs( x ).empty() )
      {
        constant_gates.insert( x );
      }
    }
    std::unordered_map<int, int> substitution;
    for ( auto& x : constant_gates )
    {
      substitution[x] = -1;
    }
    while ( !constant_gates.empty() )
    {
      auto alias = *constant_gates.begin();
      constant_gates.erase( constant_gates.begin() );
      auto outputs = getGateOutputs( alias );
      for ( auto& x : outputs )
      {
        auto& gate = getGate( x );
        gate.substitute( substitution );

        if ( gate.isConstant() ) // 如果输出门变成常量门，将其添加到常量门集合中
        {
          constant_gates.insert( x );
          substitution[x] = -1;
        }
      }
      removeGate( alias );
      if ( isPO( alias ) )
      {
        auto out_indices = getAllIndices( pos, alias ); // 在pos中查找等于alias的元素索引，
        for ( auto out_idx : out_indices )
        {
          pos[out_idx] = getConstantAlias( alias );
          pos_set = std::unordered_set<int>( pos.begin(), pos.end() );
        }
      }
    }
  }

  int getConstantAlias( int candidate )
  {
    if ( constant_gate_alias == -1 )
    {
      constant_gate_alias = candidate;
      alias2level[candidate] = 0;
      alias2outputs[candidate] = std::set<int>();

      std::vector<bool> table( 1, false );
      Gate gate( candidate, std::vector<int>{}, table );
      alias2gate[candidate] = gate;
    }
    return constant_gate_alias;
  }

  std::vector<std::pair<int, int>> getConnected( int alias, const std::unordered_set<int>& gates, const std::vector<int>& internal_gates ) const
  {
    int level = std::numeric_limits<int>::max();
    for ( const auto& gate : gates )
    {
      level = std::min( level, alias2level.at( gate ) );
    }
    std::vector<std::pair<int, int>> connected_pairs;
    if ( level >= alias2level.at( alias ) )
    {
      return connected_pairs;
    }
    std::vector<int> to_check = { alias };
    std::unordered_set<int> seen( internal_gates.begin(), internal_gates.end() );
    while ( !to_check.empty() )
    {
      int current_gate = to_check.back();
      to_check.pop_back();
      seen.insert( current_gate );
      for ( const auto& inp : getGateInputs( current_gate ) )
      {
        if ( std::find( gates.begin(), gates.end(), inp ) != gates.end() )
        {
          connected_pairs.emplace_back( inp, alias );
        }
        else if ( seen.find( inp ) == seen.end() )
        {
          seen.insert( inp );
          int inp_level = alias2level.at( inp );
          if ( inp_level > level )
          {
            to_check.push_back( inp );
          }
        }
      }
    }
    return connected_pairs;
  }

  std::vector<std::pair<int, int>> getPotentialCycles( const std::set<int>& inputs, const std::unordered_set<int>& outputs, const std::vector<int>& internal_gates ) const
  {
    std::vector<std::pair<int, int>> cycle_candidates;
    if ( inputs.empty() )
    {
      std::cerr << "Warning -- getPotentialCycles: inputs empty" << std::endl;
    }
    if ( outputs.empty() )
    {
      std::cerr << "Warning -- getPotentialCycles: outputs empty" << std::endl;
      return cycle_candidates;
    }
    for ( const auto& inp : inputs )
    {
      auto connected = getConnected( inp, outputs, internal_gates );
      cycle_candidates.insert( cycle_candidates.end(), connected.begin(), connected.end() );
    }
    return cycle_candidates;
  }

  void removeGate( int alias )
  {
    auto inputs = getGateInputs( alias );
    removeGateAux( alias, inputs );
  }

  void removeGateAux( int alias, const std::vector<int>& inputs )
  {
    assert( alias2gate.find( alias ) != alias2gate.end() );

    for ( const auto& x : inputs ) //移除输入
    {
      if ( alias2outputs.find( x ) != alias2outputs.end() )
      {
        alias2outputs[x].erase( alias );
      }
    }
    alias2gate.erase( alias );
    alias2level.erase( alias );
    alias2outputs.erase( alias );
  }

  void updatePos( std::unordered_map<int, int>& output_assoc )
  {
    for ( auto& x : pos )
    {
      auto it = output_assoc.find( x );
      if ( it != output_assoc.end() )
      {
        if ( it->second == -1 )
        {
          x = getConstantAlias( x );
        }
        else
        {
          x = it->second;
        }
      }
    }
    pos_set = std::unordered_set<int>( pos.begin(), pos.end() );
  }

  void getTopologicalOrder()
  {
    std::set<int> expanded;
    std::set<int> visited;
		topological_order.resize(alias2gate.size(), -1);
    int order_index = topological_order.size() - 1;

    // The pis shall be treated differently as the gates
    // Thus, we do not put them into the stack and handle them all at once
    for ( int pi : pis )
    {
      std::stack<std::pair<int, bool>> to_process_stack;
      for ( int x : alias2outputs[pi] )
      {
        if ( expanded.find( x ) == expanded.end() )
        {
          to_process_stack.push( { x, false } );
        }
      }
      while ( !to_process_stack.empty() )
      {
        auto [alias, children_processed] = to_process_stack.top();
        to_process_stack.pop();
        if ( expanded.find( alias ) != expanded.end() )
        {
          assert( !children_processed );
          continue;
        }
        if ( children_processed ) // 所有子门已经处理完毕，将其添加到拓扑排序结果中，并标记为已处理。
        {
          topological_order[order_index] = alias;
          order_index--;
          expanded.insert( alias );
        }
        else // 尚未处理完毕，将其重新添加到栈中，并将其子门添加到栈中。
        {
          // We try to expand a gate that is already on the current DFS path.
          if ( visited.find( alias ) != visited.end() )
          {
            std::cout << "gate: " << alias << std::endl;
            
            assert( false && "Cycle detected" );
          }
          // Will get processed as soon as all outputs are processed
          to_process_stack.push( { alias, true } );
          visited.insert( alias );
          for ( int x : alias2outputs[alias] )
          {
            if ( expanded.find( x ) == expanded.end() )
            {
              to_process_stack.push( { x, false } );
            }
          }
        }
      }
    }

    // The constant gate is not connected to the pis. Thus it needs to be handled separately.
    if ( expanded.size() != alias2gate.size() )
    {
      assert( expanded.size() == alias2gate.size() - 1 ); // 存在一个常量节点
      assert( constant_gate_alias != -1 );
      topological_order[0] = constant_gate_alias;
    }
  }
  
  std::unordered_set<int> getReconvergent()
  {
    std::unordered_set<int> joining_gates;
    std::unordered_set<int> expanded2;
    std::unordered_set<int> successors_current_pi;

    for ( const auto& pi : pis )
    {
      std::stack<std::pair<int, bool>> to_process_stack;
      
      successors_current_pi.clear();
      for ( const auto& x : alias2outputs[pi] )
      {
        if ( expanded2.find( x ) == expanded2.end() )
        {
          to_process_stack.push( { x, false } );
        }
      }
      while ( !to_process_stack.empty() )
      {
        auto [alias, children_processed] = to_process_stack.top();
        to_process_stack.pop();

        successors_current_pi.insert( alias );

        if ( expanded2.find( alias ) != expanded2.end() )
        {
          if ( !children_processed )  continue;
        }
        if ( children_processed )
        {
          expanded2.insert( alias );
        }
        else
        {
          to_process_stack.push( { alias, true } );
          for ( const auto& x : alias2outputs[alias] )
          {
            if ( expanded2.find( x ) == expanded2.end() )
            {
              to_process_stack.push( { x, false } );
            }
            if (  successors_current_pi.find( x ) != successors_current_pi.end() )
            {
              joining_gates.insert( x );
            }
          }
        }
      }
    }
    return joining_gates;
  }



  void getTopologicalOrder(Specification spec)
  {
    std::set<int> expanded;
    std::set<int> visited;
		topological_order.resize(alias2gate.size(), -1);
    int order_index = topological_order.size() - 1;

    // The pis shall be treated differently as the gates
    // Thus, we do not put them into the stack and handle them all at once
    for ( int pi : pis )
    {
      std::stack<std::pair<int, bool>> to_process_stack;
      for ( int x : alias2outputs[pi] )
      {
        if ( expanded.find( x ) == expanded.end() )
        {
          to_process_stack.push( { x, false } );
        }
      }
      while ( !to_process_stack.empty() )
      {
        auto [alias, children_processed] = to_process_stack.top();
        to_process_stack.pop();
        if ( expanded.find( alias ) != expanded.end() )
        {
          assert( !children_processed );
          continue;
        }
        if ( children_processed ) // 所有子门已经处理完毕，将其添加到拓扑排序结果中，并标记为已处理。
        {
          topological_order[order_index] = alias;
          order_index--;
          expanded.insert( alias );
        }
        else // 尚未处理完毕，将其重新添加到栈中，并将其子门添加到栈中。
        {
          // We try to expand a gate that is already on the current DFS path.
          if ( visited.find( alias ) != visited.end() )
          {
            std::cout << "gate: " << alias << std::endl;
            writeSpecification("error.blif", spec);
            assert( false && "Cycle detected" );
          }
          // Will get processed as soon as all outputs are processed
          to_process_stack.push( { alias, true } );
          visited.insert( alias );
          for ( int x : alias2outputs[alias] )
          {
            if ( expanded.find( x ) == expanded.end() )
            {
              to_process_stack.push( { x, false } );
            }
          }
        }
      }
    }

    // The constant gate is not connected to the pis. Thus it needs to be handled separately.
    if ( expanded.size() != alias2gate.size() )
    {
      assert( expanded.size() == alias2gate.size() - 1 ); // 存在一个常量节点
      assert( constant_gate_alias != -1 );
      topological_order[0] = constant_gate_alias;
    }
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

    auto outputs = spec.getOutputs();
    int max_var = spec.max_var;
    auto [negated_outputs, outputs_in_both_polarities] = spec.getOutputsToNegate();
    std::unordered_map<int, int> aux_var_dict;
    int idx = 0;
    for ( auto x : outputs_in_both_polarities )
    {
      aux_var_dict[x] = max_var + idx + 1;
      ++idx;
    }
    std::vector<int> blif_outputs;
    for ( size_t idx = 0; idx < outputs.size(); ++idx )
    {
      int x = outputs[idx];
      if ( outputs_in_both_polarities.find( x ) != outputs_in_both_polarities.end() && spec.isOutputNegated( idx ) )
      {
        blif_outputs.push_back( aux_var_dict[x] );
      }
      else
      {
        blif_outputs.push_back( x );
      }
    }

    out << ".outputs ";
    for ( const auto& output : blif_outputs )
    {
      out << output << " ";
    }
    out << "\n";

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
        writeBlifGate( out, alias_negated, table, inputs, negated_outputs );
      }
      else
      {
        writeBlifGate( out, alias, table, inputs, negated_outputs );
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

  void printTopologicalOrder() const
  {
    for ( int alias : topological_order )
    {
      std::cout << alias << " ";
    }
    std::cout << std::endl;
  }
#pragma region circuit information
  std::vector<int> getGateAliases()
  {
    std::vector<int> aliases;
    for ( const auto& kv : alias2gate )
    {
      aliases.push_back( kv.first );
    }
    return aliases;
  }

  std::set<int> getGateAliasesSet()
  {
    std::set<int> aliases;
    for ( const auto& pair : alias2gate )
    {
      aliases.insert( pair.first );
    }
    return aliases;
  }

  size_t getNofGates()
  {
    return alias2gate.size();
  }

  Gate& getGate( int alias )
  {
    return alias2gate[alias];
  }

  const std::vector<int> getGateInputs( int alias ) const
  {
    return alias2gate.at( alias ).inputs;
  }

  const std::set<int> getGateOutputs( int alias ) const
  {
    return alias2outputs.at( alias );
  }

  int getGateLevel( int alias )
  {
    return alias2level[alias];
  }

  const std::vector<int>& getInputs()
  {
    return pis;
  }

  const std::vector<int>& getOutputs()
  {
    return pos;
  }

  int getMaxAlias()
  {
    return max_var;
  }

  bool isOutputNegated( size_t idx )
  {
    return negated_pos[idx];
  }

  bool isPO( int alias ) const
  {
    return pos_set.find( alias ) != pos_set.end();
  }

  int getDepth()
  {
    int maxDepth = 0;
    for ( const auto& alias : pos )
    {
      auto it = alias2level.find( alias ); // 使用 find 寻找键
      if ( it != alias2level.end() )
      {
        // 如果找到，就使用找到的键的值进行比较
        maxDepth = std::max( maxDepth, it->second );
      }
    }
		return maxDepth;
	}

  std::vector<bool> getGateTable(int alias)
  {
    return alias2gate[alias].table;
  }

  bool isGateXor(int alias)
  {
		bool flag = true;
    std::vector<bool> ttXor = {false, true, true, false}; 
		std::vector<bool> tt = getGateTable(alias);
    for (size_t i = 0; i < tt.size(); ++i) {
      if (tt[i] != ttXor[i]) flag = false;
    } 
    return flag;
  }

  bool isGateAnd(int alias)
  {
    std::vector<bool> tt = getGateTable(alias);
    std::vector<std::vector<bool>> truthTables = {
      {false, true, true, true},  
      {false, true, false, false},   
      {false, false, true, false},   
      {false, false, false, true}     
    }; 
     for (const auto& ttAnd : truthTables) {
        if (tt == ttAnd) {
            return true; // 一旦匹配，返回 true
        }
    }
    return false;
  }





#pragma endregion

#pragma region Subcircuit

    std::set<int> getSubcircuitInputs( const std::vector<int> aliases ) const
    {
      std::set<int> input_set;
      for ( auto& y : aliases )
      {
        auto gate_inputs = getGateInputs( y );
        input_set.insert( gate_inputs.begin(), gate_inputs.end() );
      }
      for ( const auto& alias : aliases )
      {
        input_set.erase( alias );
      }
      return input_set;
    }

    std::set<int> getDirectSuccessors( const std::vector<int> aliases ) const
    {
      std::set<int> successor_set;
      for ( const auto& y : aliases )
      {
        auto gate_outputs = getGateOutputs( y );
        successor_set.insert( gate_outputs.begin(), gate_outputs.end() );
      }
      for ( const auto& alias : aliases )
      {
        successor_set.erase( alias );
      }
      return successor_set;
    }

    std::unordered_set<int> getSubcircuitOutputs( const std::vector<int> aliases ) const
    {
      std::unordered_set<int> alias_set( aliases.begin(), aliases.end() );
      std::unordered_set<int> output_set;
      for ( const auto& x : aliases )
      {
        if ( isPO( x ) || !std::all_of( getGateOutputs( x ).begin(), getGateOutputs( x ).end(), [&alias_set]( int output )
                                        { return alias_set.find( output ) != alias_set.end(); } ) )
        {
          output_set.insert( x );
        }
      }
      return output_set;
    }
#pragma endregion

    // 将优化后电路插回原网络中
    std::unordered_set<int> replaceSubcircuit( const std::vector<int>& to_remove, const std::vector<std::tuple<int, std::vector<int>, std::vector<bool>>>& new_gates, std::unordered_map<int, int>& output_assoc ,Specification spec)
    {
      std::vector<int> old_gate_aliases( to_remove.begin(), to_remove.end() );
      auto successors_to_update = getDirectSuccessors( old_gate_aliases ); //获取后续输出与子电路连接的节点
      auto unused_gate_candidates = getSubcircuitInputs( old_gate_aliases );  //子电路输入
      auto subcircuit_output_dict = getOutputsDict( to_remove, output_assoc );
      std::unordered_set<int> redundant;

      for ( const auto& x : to_remove )
      {
        removeGate( x );
      }

      insertGates( new_gates );
      incorporateOutputs( subcircuit_output_dict );

      while ( !successors_to_update.empty() )
      {
        int alias_to_process = *successors_to_update.begin();
        successors_to_update.erase( successors_to_update.begin() );
        Gate& gate = getGate( alias_to_process );
        
        auto old_inputs = gate.substitute( output_assoc);
        
        if ( gate.isConstant() )
        {
          output_assoc[alias_to_process] = -1; // 使用-1表示None
          auto gate_outputs = getGateOutputs( alias_to_process );
          successors_to_update.insert( gate_outputs.begin(), gate_outputs.end() );
          redundant.insert( alias_to_process );
          removeGateAux( alias_to_process, old_inputs );
          unused_gate_candidates.insert( old_inputs.begin(), old_inputs.end() );
        }
      }

      for ( int pi : pis )
      {
        unused_gate_candidates.erase( pi );
      }
      auto unused = removeUnusedGates( unused_gate_candidates );
      unused.insert( redundant.begin(), redundant.end() );
      updatePos( output_assoc );
      setGateLevels();

      return unused;
    }

    std::unordered_map<int, std::unordered_set<int>> getOutputsDict( const std::vector<int>& to_remove, const std::unordered_map<int, int>& output_assoc ) const
    {
      auto subcircuit_outputs = getSubcircuitOutputs( to_remove );
      std::unordered_set<int> to_remove_set( to_remove.begin(), to_remove.end() );
      std::unordered_map<int, std::unordered_set<int>> log;

      for ( const auto& x : subcircuit_outputs )
      {
        auto outputs_set = getGateOutputs( x );
        std::unordered_set<int> outputs( outputs_set.begin(), outputs_set.end() );
        for ( const auto& remove : to_remove_set )
        {
          outputs.erase( remove );
        }

        if ( output_assoc.at( x ) == -1 ) // 使用-1表示None
        {
          continue;
        }
      //   for (auto out : outputs){
      //   std::cout << "节点: " << output_assoc.at( x ) << "  节点连接的外部输出:" << out << std::endl;
      // }
        if ( log.find( output_assoc.at( x ) ) != log.end() )
        {
          log[output_assoc.at( x )].insert( outputs.begin(), outputs.end() );
        }
        else
        {
          log[output_assoc.at( x )] = outputs;
        }
      }

      return log;
    }

    void insertGates( const std::vector<std::tuple<int, std::vector<int>, std::vector<bool>>>& new_gates )
    {
      for ( const auto& g : new_gates )
      {
        int alias = std::get<0>( g );
        alias2outputs[alias] = {};
      }

      for ( const auto& g : new_gates )
      {
        int alias = std::get<0>( g );
        const auto& inputs = std::get<1>( g );
        const auto& table = std::get<2>( g );
        addGate( alias, inputs, table );
      }
    }
    void incorporateOutputs( const std::unordered_map<int, std::unordered_set<int>>& output_log )
    {
      for ( const auto& [alias, outputs] : output_log )
      {
        alias2outputs[alias].insert( outputs.begin(), outputs.end() );
      }
    }

    std::unordered_set<int> removeUnusedGates( std::set<int> & aliases_to_check )
    {
      std::unordered_set<int> unused;

      while ( !aliases_to_check.empty() )
      {
        int x = *aliases_to_check.begin();
        aliases_to_check.erase( aliases_to_check.begin() );

        if ( !isPO( x ) && getGateOutputs( x ).empty() )
        {
          auto inputs = getGateInputs( x );
          aliases_to_check.insert( inputs.begin(), inputs.end() );
          for ( const auto& pi : pis )
          {
            aliases_to_check.erase( pi );
          }
          removeGate( x );
          unused.insert( x );
        }
      }
      return unused;
    }

    std::pair<std::unordered_set<int>, std::unordered_set<int>> getOutputsToNegate() const
    {
      std::unordered_set<int> positive_outputs;
      std::unordered_set<int> negative_outputs;

      for ( size_t idx = 0; idx < pos.size(); ++idx )
      {
        int out = pos[idx];
        if ( negated_pos[idx] )
        {
          negative_outputs.insert( out );
        }
        else
        {
          positive_outputs.insert( out );
        }
      }

      std::unordered_set<int> outputs_in_both_polarities;
      for ( const auto& out : positive_outputs )
      {
        if ( negative_outputs.find( out ) != negative_outputs.end() )
        {
          outputs_in_both_polarities.insert( out );
        }
      }

      for ( const auto& out : outputs_in_both_polarities )
      {
        negative_outputs.erase( out );
      }

      return { negative_outputs, outputs_in_both_polarities };
    }
  };



namespace alice {
#pragma region Network
template<typename Ntk>
class CircuitInterface
{
public:
  CircuitInterface();
  ~CircuitInterface();
  CircuitInterface( Ntk net );

  void verilog_read( const std::string& fname );
  void aiger_read( const std::string& fname );
  std::vector<int> getInputs() const;
  std::vector<int> getOutputs() const;
  int getNofGates() const;
  std::tuple<int, int, int> getGate( int idx ) const;
  bool is_and( int idx ) const;
  bool is_xor( int idx ) const;

  int getConstantTrue() const;
  int getConstantFalse() const;

private:
  Ntk circuit;
};

template<typename Ntk>
CircuitInterface<Ntk>::CircuitInterface() {}

template<typename Ntk>
CircuitInterface<Ntk>::~CircuitInterface() {}

template<typename Ntk>
CircuitInterface<Ntk>::CircuitInterface( Ntk net )
{
  circuit = net; // 获取当前的网络
}

template<typename Ntk>
int CircuitInterface<Ntk>::getConstantTrue() const
{
  return ( (int)circuit.get_constant( true ).index );
}

template<typename Ntk>
int CircuitInterface<Ntk>::getConstantFalse() const
{
  return ( (int)circuit.get_constant( false ).index );
}

template<typename Ntk>
void CircuitInterface<Ntk>::verilog_read( const std::string& fname )
{
  if ( lorina::read_verilog( fname, mockturtle::verilog_reader( circuit ) ) != lorina::return_code::success )
  {
    std::cout << "[w] parse error\n";
  }
}

template<typename Ntk>
void CircuitInterface<Ntk>::aiger_read( const std::string& fname )
{
  if ( lorina::read_aiger( fname, mockturtle::aiger_reader( circuit ) ) != lorina::return_code::success )
  {
    std::cout << "[w] parse error\n";
  }
}

template<typename Ntk>
std::vector<int> CircuitInterface<Ntk>::getInputs() const
{
  std::vector<int> inputs;
  inputs.reserve( circuit.num_pis() );
  for ( int i = 0; i < circuit.num_pis(); i++ )
  {
    inputs.push_back( i + 1 );
  }
  return inputs;
}

template<typename Ntk>
std::vector<int> CircuitInterface<Ntk>::getOutputs() const
{
  std::vector<int> outputs;
  outputs.reserve( circuit.num_pos() );
  circuit.foreach_po( [&]( auto node_po )
                      {
      const auto driver = circuit.get_node( node_po );

      // if ( circuit.is_constant(driver) )
      // {
      //   if(circuit.is_complemented(node_po)) outputs.push_back(1);
      //   else  outputs.push_back(0);
      // }
      // else 
      if(circuit.is_complemented(node_po))
           outputs.push_back(circuit.node_to_index(driver)*2+1);  //配合后面的补边提取
      else outputs.push_back(circuit.node_to_index(driver)*2); } );
  return outputs;
}

template<typename Ntk>
int CircuitInterface<Ntk>::getNofGates() const
{
  return (int)circuit.num_gates();
}


template<typename Ntk>
std::tuple<int, int, int> CircuitInterface<Ntk>::getGate( int idx ) const
{
  int count = 0;
  static int l_hs, r_hs0, r_hs1 = 0;
  std::vector<int> inputs;
  circuit.foreach_gate( [&]( auto n )
                        {                
      if(count == idx) {
        circuit.foreach_fanin(n, [&](auto fi)
                              {
        const auto driver = circuit.get_node( fi );
        if(circuit.is_complemented(fi))
          {
            inputs.push_back(circuit.node_to_index(driver)*2+1);  //配合后面的补边提取
          }  
        else  inputs.push_back(circuit.node_to_index(driver)*2); 
       });
        auto lhs = idx + circuit.num_pis() + 1;   
        l_hs = (int)lhs;
      }
      count++; } );
  assert( inputs.size() == 2 );
  return std::make_tuple( l_hs, inputs[0], inputs[1] );
}

template<typename Ntk>
bool CircuitInterface<Ntk>::is_and( int idx ) const
{
  int count = 0;
  bool isAnd;
  circuit.foreach_gate( [&]( auto n )
                        {                
      if(count == idx) {
       isAnd =  circuit.is_and(n);
      }
      count++; } );
  return isAnd;
}

template<typename Ntk>
bool CircuitInterface<Ntk>::is_xor( int idx ) const
{
  int count = 0;
  bool isXor;
  circuit.foreach_gate( [&]( auto n )
                        {                
      if(count == idx) {
       isXor = circuit.is_xor(n);
      }
      count++; } );
  return isXor;
}
#pragma endregion

#pragma region Spec
// 函数声明
int processVariable(int var);
bool isNegatedLiteral(int lit);
int negateLiteral(int lit);

template<typename Ntk>
void addGate(Specification &specification, CircuitInterface<Ntk> &circuitInterface, int gate_idx, std::set<int> &negated_gates,std::unordered_set<int> &pos_set);
template<typename Ntk>
void addGate2(Specification &specification, CircuitInterface<Ntk> &circuitInterface, int gate_idx, std::set<int> &negated_gates,std::unordered_set<int> &pos_set);
template<typename... Bools>
std::vector<bool> convert_to_vector(Bools... bools)
{
    return std::vector<bool>{bools...};
}

void printVector(const std::vector<int>& vec) {
    for (const auto& value : vec) {
        std::cout << value << " ";
    }
    std::cout << std::endl;
}

Specification getSpecification(mockturtle::aig_network aig)
{
  CircuitInterface<mockturtle::aig_network> aigerInterface(aig);  //从store中读取当前aig

  std::vector<int> pis, pos;
  std::unordered_set<int> pos_set;

  for (const auto &x : aigerInterface.getInputs())
  {
    pis.push_back(x);
  }
  for (const auto &x : aigerInterface.getOutputs())
  {
    int var = processVariable(x);
    pos.push_back(var);
    pos_set.insert(var);
  }
  Specification specification(pis, pos);
  // 处理输出取反
  unsigned int idx = 0;
  for ( const auto& out : aigerInterface.getOutputs() )
  {
    if ( idx >= specification.negated_pos.size() )
    {
      specification.negated_pos.resize( idx + 1, false ); // 使用 false 作为新元素的默认值
    }
    if ( isNegatedLiteral( out ) )
    {
      specification.negated_pos[idx] = true;
    }
    idx++;
  }
  // 添加门到spec
  std::set<int> negated_gates;
  for (int i = 0; i < aigerInterface.getNofGates(); i++)
  {
    addGate<mockturtle::aig_network>(specification, aigerInterface, i, negated_gates, pos_set);
  }

  specification.init(false);
  return specification;
}

Specification getSpecification( mockturtle::xag_network xag )
{
  alice::CircuitInterface<mockturtle::xag_network> xagerInterface( xag );

  std::vector<int> pis, pos;
  std::unordered_set<int> pos_set;

  for ( const auto& x : xagerInterface.getInputs() )
  {
    pis.push_back( x );
  }
  for ( const auto& x : xagerInterface.getOutputs() )
  {
    int var = processVariable( x );
    pos.push_back( var );
    pos_set.insert( var );
  }
  Specification specification( pis, pos );

  // 处理输出取反
  unsigned int idx = 0;
  for ( const auto& out : xagerInterface.getOutputs() )
  {
    if ( idx >= specification.negated_pos.size() )
    {
      specification.negated_pos.resize( idx + 1, false ); // 使用 false 作为新元素的默认值
    }
    if ( isNegatedLiteral( out ) )
    {
      specification.negated_pos[idx] = true;
    }
    idx++;
  }
  // 添加门到spec
  int cont1 = 0;
  int cont2 =0;
  std::set<int> negated_gates;
  for ( int i = 0; i < xagerInterface.getNofGates(); i++ )
  {
    if ( xagerInterface.is_and( i ) )
    {
       cont1++;
      addGate<mockturtle::xag_network>( specification, xagerInterface, i, negated_gates, pos_set );
    }
    else if ( xagerInterface.is_xor( i ) )
    {
       cont2++;
      addGate2<mockturtle::xag_network>( specification, xagerInterface, i, negated_gates, pos_set );
    }
  }
  
  std::cout << "AND: " << cont1 << "  XOR:  " << cont2 << std::endl;
  specification.init( false );
  return specification;
}


template<typename Ntk>
void addGate(Specification &specification, CircuitInterface<Ntk> &CircuitInterface, int gate_idx, std::set<int> &negated_gates, std::unordered_set<int> &pos_set)
{
  int lhs, rhs1, rhs2;
  std::tie(lhs, rhs1, rhs2) = CircuitInterface.getGate(gate_idx);
  int alias = lhs;
  const int constantFalse = CircuitInterface.getConstantFalse();
  const int constantTrue = CircuitInterface.getConstantTrue();
  //检查 rhs 是否在 negated_gates 集合中，如果在，则将 rhs 取反
  auto handleNegation = [&](int &rhs)   
  {
    if (negated_gates.find(processVariable(rhs)) != negated_gates.end())
    {
      rhs = negateLiteral(rhs);
    }
  };

  std::vector<int> inputs;
  std::vector<bool> truthTable;

  if (rhs1 == constantFalse || rhs2 == constantFalse)
  {
    truthTable = convert_to_vector(false);
  }
  else if (rhs1 == constantTrue)
  {
    if (rhs2 == constantTrue)
    {
      truthTable = convert_to_vector(false);
      negated_gates.insert(alias);
      if (pos_set.find(alias) != pos_set.end())
      {
        specification.toggleOutputNegation(alias);
      }
    }
    else
    {
      handleNegation(rhs2);
      inputs.push_back(processVariable(rhs2));
      truthTable = convert_to_vector(false, true);
      if (isNegatedLiteral(rhs2))
      {
        negated_gates.insert(alias);
        if (pos_set.find(alias) != pos_set.end())
        {
          specification.toggleOutputNegation(alias);
        }
      }
    }
  }
  else if (rhs2 == constantTrue)
  {
    handleNegation(rhs1);
    inputs.push_back(processVariable(rhs1));
    truthTable = convert_to_vector(false, true);
    if (isNegatedLiteral(rhs1))
    {
      negated_gates.insert(alias);
      if (pos_set.find(alias) != pos_set.end())
      {
        specification.toggleOutputNegation(alias);
      }
    }
  }
  else
  {
    handleNegation(rhs1);
    handleNegation(rhs2);
    inputs = {processVariable(rhs1), processVariable(rhs2)};
    if (isNegatedLiteral(rhs1) && isNegatedLiteral(rhs2))
    {
      negated_gates.insert(alias);
      if (pos_set.find(alias) != pos_set.end())
      {
        specification.toggleOutputNegation(alias);
      }
      truthTable = convert_to_vector(false, true, true, true);
    }
    else if (isNegatedLiteral(rhs1))
    {
      truthTable = convert_to_vector(false, true, false, false);
    }
    else if (isNegatedLiteral(rhs2))
    {
      truthTable = convert_to_vector(false, false, true, false);
    }
    else
    {
      truthTable = convert_to_vector(false, false, false, true);
    }
  }

  specification.addGateUnsorted(alias, inputs, truthTable); //将节点的输入以及对应的状态添加入网络
}

template<typename Ntk>
void addGate2( Specification& specification, CircuitInterface<Ntk>& circuitInterface, int gate_idx, std::set<int>& negated_gates, std::unordered_set<int>& pos_set )
{
  int lhs, rhs1, rhs2;
  std::tie( lhs, rhs1, rhs2 ) = circuitInterface.getGate( gate_idx );
  int alias = lhs;
  const int constantFalse = circuitInterface.getConstantFalse();
  const int constantTrue = circuitInterface.getConstantTrue();

  auto handleNegation = [&]( int& rhs )
  {
    if ( negated_gates.find( processVariable( rhs ) ) != negated_gates.end() )
    {
      rhs = negateLiteral( rhs );
    }
  };

  std::vector<int> inputs;
  std::vector<bool> truthTable;

  if ( rhs1 == constantFalse && rhs2 == constantFalse )
  {
    truthTable = convert_to_vector( false );
  }
  else if ( rhs1 == constantTrue && rhs2 == constantTrue )
  {
    truthTable = convert_to_vector( false );
  }
  else if ( rhs1 == constantFalse )
  {
    if ( rhs2 == constantTrue )
    {
      truthTable = convert_to_vector( false );
      negated_gates.insert( alias );
      if ( pos_set.find( alias ) != pos_set.end() )
      {
        specification.toggleOutputNegation( alias );
      }
    }
    else
    {
      handleNegation( rhs2 );
      inputs.push_back( processVariable( rhs2 ) );
      truthTable = convert_to_vector( false, true );
      if ( isNegatedLiteral( rhs2 ) )
      {
        negated_gates.insert( alias );
        if ( pos_set.find( alias ) != pos_set.end() )
        {
          specification.toggleOutputNegation( alias );
        }
      }
    }
  }
  else if ( rhs1 == constantTrue )
  {
    if ( rhs2 == constantFalse )
    {
      truthTable = convert_to_vector( false );
      negated_gates.insert( alias );
      if ( pos_set.find( alias ) != pos_set.end() )
      {
        specification.toggleOutputNegation( alias );
      }
    }
    else
    {
      handleNegation( rhs2 );
      inputs.push_back( processVariable( rhs2 ) );
      truthTable = convert_to_vector( true, false );
      if ( isNegatedLiteral( rhs2 ) )
      {
        negated_gates.insert( alias );
        if ( pos_set.find( alias ) != pos_set.end() )
        {
          specification.toggleOutputNegation( alias );
        }
      }
    }
  }
  else if ( rhs2 == constantTrue )
  {
    handleNegation( rhs1 );
    inputs.push_back( processVariable( rhs1 ) );
    truthTable = convert_to_vector( true, false );
    if ( isNegatedLiteral( rhs1 ) )
    {
      negated_gates.insert( alias );
      if ( pos_set.find( alias ) != pos_set.end() )
      {
        specification.toggleOutputNegation( alias );
      }
    }
  }
  else if ( rhs2 == constantFalse )
  {
    handleNegation( rhs1 );
    inputs.push_back( processVariable( rhs1 ) );
    truthTable = convert_to_vector( false, true );
    if ( isNegatedLiteral( rhs1 ) )
    {
      negated_gates.insert( alias );
      if ( pos_set.find( alias ) != pos_set.end() )
      {
        specification.toggleOutputNegation( alias );
      }
    }
  }
  else
  {
    handleNegation( rhs1 );
    handleNegation( rhs2 );
    inputs = { processVariable( rhs1 ), processVariable( rhs2 ) };
    if ( isNegatedLiteral( rhs1 ) && isNegatedLiteral( rhs2 ) )
    {

      truthTable = convert_to_vector( false, true, true, false );
    }
    else if ( isNegatedLiteral( rhs1 ) )
    {
      negated_gates.insert( alias );
      if ( pos_set.find( alias ) != pos_set.end() )
      {
        specification.toggleOutputNegation( alias );
      }
      truthTable = convert_to_vector( false, true, true, false );
    }
    else if ( isNegatedLiteral( rhs2 ) )
    {
      negated_gates.insert( alias );
      if ( pos_set.find( alias ) != pos_set.end() )
      {
        specification.toggleOutputNegation( alias );
      }
      truthTable = convert_to_vector( false, true, true, false );
    }
    else
    {
      truthTable = convert_to_vector( false, true, true, false );
    }
  }

  specification.addGateUnsorted( alias, inputs, truthTable );
}

int processVariable(int var)
{
  return var / 2;
}

bool isNegatedLiteral(int lit)
{
  return lit & 1;
}

int negateLiteral(int lit)
{
  return lit ^ 1;
}
#pragma endregion
}



#endif
