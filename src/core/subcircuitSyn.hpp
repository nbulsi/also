#ifndef SUBCIRCUITSYN_HPP
#define SUBCIRCUITSYN_HPP

#include "specification.hpp"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <future>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <tuple>
#include <unistd.h>
#include <unordered_set>

using Circuit = std::tuple<std::vector<std::tuple<int, std::vector<int>, std::vector<bool>>>, std::unordered_map<int, int>, std::vector<int>, std::vector<int>>;

class EncoderCircuits
{
private:
  Specification& specification;
  std::vector<int> to_replace;
  int nof_gates;
  int nof_gate_inputs;

  std::unordered_set<int> pis_set;
  int last_used_variable; // 用于给每个约束一个编号
  bool allow_xors;
  bool writeComments;
  int DAG_Flag;
  int max_var_specification_representation; // 当前spec最大的变量使用表示（代表电路的节点或者线）
  // std::vector<int> partCircuitPo;

  std::vector<int> internal_gates;

  // 编码使用的变量
  std::vector<std::vector<int>> selection_variables;
  std::vector<std::vector<int>> gate_definition_variables;
  std::vector<std::vector<int>> gate_output_variables;

  std::vector<int> subcircuit_inputs;
  std::vector<int> subcircuit_outputs;
  std::vector<int> constraintGates;
  int output_var;
  // 约束 编码加速
  bool useTrivialRuleConstraint;
  bool useNoReapplicationConstraint;
  bool useAllStepsConstraint;
  bool useOrderedStepsConstraint;

  // 成环约束
  std::vector<std::pair<int, int>> forbidden;
  // 连接关系
  std::unordered_map<int, int> descendants_renaming;
  // 成环约束里的连接
  std::unordered_map<int, std::vector<int>> connection_variables;
  std::unordered_set<int> gates_to_copy;
  std::vector<int> inputs;

public:
  EncoderCircuits( Specification& specification, const std::vector<int>& gates_to_replace, int Flag )
      : specification( specification ),
        to_replace( gates_to_replace ),
        pis_set( specification.getInputs().begin(), specification.getInputs().end() ),
        last_used_variable( specification.max_var ),
        allow_xors( false ),
        writeComments( true ),
        DAG_Flag( Flag ),
        useTrivialRuleConstraint( true ),
        useNoReapplicationConstraint( true ),
        useAllStepsConstraint( true ),
        useOrderedStepsConstraint( true )
  {
    getSubcircuitIO();
    analyse_subcircuit();
    max_var_specification_representation = last_used_variable;
  }

public:
  void getSubcircuitIO()
  {
    auto input_set = specification.getSubcircuitInputs( to_replace );
    auto output_set = specification.getSubcircuitOutputs( to_replace );
    for ( const auto& elem : input_set )
    {
      subcircuit_inputs.push_back( elem ); // 保证两个容器之间的元素顺序一致性，以便于后续的提取/插入回原电路
    }
    for ( const auto& elem : output_set )
    {
      subcircuit_outputs.push_back( elem );
    }
    forbidden = specification.getPotentialCycles( input_set, output_set, to_replace );
    // for (auto &pair : forbidden)
    // {
    // 	std::cout << pair.first << "  " << pair.second <<std::endl;
    // }
  }
  void analyse_subcircuit()
  {
    descendants_renaming.clear();
    std::vector<int> to_analyse;
    std::unordered_set<int> seen( subcircuit_outputs.begin(), subcircuit_outputs.end() );

    for ( const auto& gate_var : subcircuit_outputs )
    {
      descendants_renaming[gate_var] = getNewVariable();
      auto gate_outputs = specification.getGateOutputs( gate_var );
      to_analyse.insert( to_analyse.end(), gate_outputs.begin(), gate_outputs.end() );
      seen.insert( gate_outputs.begin(), gate_outputs.end() );
    }

    gates_to_copy.clear();

    while ( !to_analyse.empty() )
    {
      int gate_var = to_analyse.back();
      to_analyse.pop_back();
      if ( std::find( to_replace.begin(), to_replace.end(), gate_var ) != to_replace.end() )
      {
        continue;
      }
      gates_to_copy.insert( gate_var );
      descendants_renaming[gate_var] = getNewVariable();
      auto gate_outputs = specification.getGateOutputs( gate_var );
      for ( const auto& output : gate_outputs )
      {
        if ( seen.find( output ) == seen.end() )
        {
          to_analyse.push_back( output );
          seen.insert( output );
        }
      }
    }
    if ( !forbidden.empty() )
    {
      std::unordered_set<int> inputs_to_handle;
      for ( auto& pair : forbidden )
      {
        inputs_to_handle.insert( pair.second );
      }

      inputs.clear();
      for ( const auto& input : subcircuit_inputs )
      {
        if ( inputs_to_handle.find( input ) != inputs_to_handle.end() )
        {
          inputs.push_back( descendants_renaming[input] );
        }
        else
        {
          inputs.push_back( input );
        }
      }

      for ( auto& pair : forbidden )
      {
        pair.second = descendants_renaming[pair.second];
      }
    }
    else
    {
      inputs.assign( subcircuit_inputs.begin(), subcircuit_inputs.end() );
    }
  }
  void printVector( const std::vector<int>& vec )
  {
    for ( const auto& value : vec )
    {
      std::cout << value << " ";
    }
    std::cout << std::endl;
  }
  int getNewVariable()
  {
    last_used_variable += 1;
    return last_used_variable;
  }
  const std::vector<std::vector<int>>& getSelectionVariables()
  {
    return selection_variables;
  }

  const std::vector<std::vector<int>>& getGateDefinitionVariables()
  {
    return gate_definition_variables;
  }
  const std::vector<std::vector<int>>& getGateOutputVariables()
  {
    return gate_output_variables;
  }
  const std::vector<int>& getSubcircuitInputs()
  {
    return subcircuit_inputs;
  }

  const std::vector<int>& getSubcircuitOutputs()
  {
    return subcircuit_outputs;
  }

  void resetEncoder()
  {
    last_used_variable = max_var_specification_representation;
    constraintGates.clear();
  }
  template<typename T>
  std::string join( const std::vector<T>& elements, const std::string& delimiter )
  {
    std::ostringstream oss;
    for ( size_t i = 0; i < elements.size(); ++i )
    {
      if ( i != 0 )
      {
        oss << delimiter;
      }
      oss << elements[i];
    }
    return oss.str();
  }
  void getEncoding( int nof_gates, int nof_gate_inputs, std::ofstream& file, int nof_and , bool useLocalCircuit)
  {
    bool synthesiseAig = false;
    bool synthesiseXag = false;
    // std::cout << DAG_Flag << std::endl;
    if ( DAG_Flag == 1 )
    {
      synthesiseAig = true;
    }
    else if ( DAG_Flag == 2 )
    {
      synthesiseXag = true;
    }
    assert( inputs.size() >= nof_gate_inputs && "The specification must have more inputs than the gates" );
    this->nof_gates = nof_gates;
    this->nof_gate_inputs = nof_gate_inputs;
    resetEncoder();

    std::unordered_set<int> unique_y;
    for ( const auto& pair : forbidden )
    {
      unique_y.insert( pair.second );
    }

    for ( const auto& y : unique_y )
    {
      std::vector<int> new_vars( nof_gates );
      for ( int i = 0; i < nof_gates; ++i )
      {
        new_vars[i] = getNewVariable();
      }
      connection_variables[y] = new_vars;
    }

    writePrefix( file );
    writeComment( file, "Specification" );
    writeSpecification( file );
    writeComment( file, "Specification Copy" );
    writeSpecificationCopy( file );
    if ( !forbidden.empty() )
    {
      // std::cout << "have cycle" << std::endl;
      writeComment( file, "Cycle Constraint" );
      writeCycleConstraint( file );
    }
    if ( synthesiseAig )
    {
      writeAigerConstraints( file );
    }
    else if ( synthesiseXag )
    {
      //writeXagerConstraints( file, nof_and );
      writeXagerConstraints(file);
    }
    writeEncoding( file );
    if(!useLocalCircuit)
    {
      writeEquivalenceConstraints( file );
      //std::cout << "writeEquivalenceConstraints" << std::endl;      
    }
    else if(useLocalCircuit)
    {
      writeConstraints( file );
      //std::cout << "writeConstraints" << std::endl;
    }

    writeAnd( file, output_var, constraintGates );
  }

  void writePrefix( std::ofstream& out )
  {
    out << "#QCIR-G14\n";
    auto outermost_existentials = setupCircuitVariables();
    for ( auto& [y, vec] : connection_variables ) // 成环
    {
      outermost_existentials.insert( outermost_existentials.end(), vec.begin(), vec.end() );
    }
    out << "exists(" << join( outermost_existentials, ", " ) << ")\n";
    out << "forall(" << getUniversallyQuantifiedVariables() << ")\n";
    internal_gates.resize( nof_gates );
    for ( auto& gate : internal_gates )
    {
      gate = getNewVariable();
    }
    out << "exists(" << join( internal_gates, ", " ) << ")\n";
    std::vector<int> subcircuit_output_variables;
    for ( const auto& output : subcircuit_outputs )
    {
      subcircuit_output_variables.push_back( descendants_renaming.at( output ) );
    }
    out << "exists(" << join( subcircuit_output_variables, ", " ) << ")\n";
    // if (use_gate_input_variables)   //用于加速
    // {
    // 	gate_input_variables.resize(nof_gates, std::vector<int>(nof_gate_inputs));
    // 	for (auto &gate_inputs : gate_input_variables)
    // 	{
    // 		for (auto &input : gate_inputs)
    // 		{
    // 			input = getNewVariable();
    // 		}
    // 	}
    // 	out << "exists(" << join(gate_input_variables, ", ") << ")\n";
    // }
    output_var = getNewVariable();
    out << "output(" << output_var << ")\n";
  }
  std::string getUniversallyQuantifiedVariables() const
  {
    std::ostringstream oss;
    for ( const auto& input : specification.getInputs() )
    {
      if ( &input != &specification.getInputs().front() )
      {
        oss << ", ";
      }
      oss << input;
    }
    return oss.str();
  }
  std::vector<int> setupCircuitVariables()
  {
    std::vector<int> variables;
    selection_variables.resize( nof_gates );

    for ( int y = 0; y < nof_gates; ++y )
    {
      selection_variables[y].resize( inputs.size() + y );
      for ( auto& var : selection_variables[y] )
      {
        var = getNewVariable();
      }
      variables.insert( variables.end(), selection_variables[y].begin(), selection_variables[y].end() );
    }

    int offset = 1; // We use normal gates
    gate_definition_variables.resize( nof_gates );
    for ( auto& gate_def : gate_definition_variables )
    {
      gate_def.resize( ( 1 << nof_gate_inputs ) - offset );
      for ( auto& var : gate_def )
      {
        var = getNewVariable();
      }
      variables.insert( variables.end(), gate_def.begin(), gate_def.end() );
    }

    gate_output_variables.resize( nof_gates );
    for ( auto& gate_output : gate_output_variables )
    {
      gate_output.resize( subcircuit_outputs.size() );
      for ( auto& var : gate_output )
      {
        var = getNewVariable();
      }
      variables.insert( variables.end(), gate_output.begin(), gate_output.end() );
    }

    return variables;
  }
  void writeSpecification( std::ofstream& out )
  {
    for ( const auto& gate : specification.orderedGateTraversal() )
    {
      int alias = gate.getAlias();
      bool anded;
      std::vector<std::vector<int>> lines;
      std::tie( anded, lines ) = gate.getQCIRGates();
      writeGate( out, alias, anded, lines );
    }
  }

  void writeSpecificationCopy( std::ofstream& out )
  {
    for ( const auto& gate : specification.orderedGateTraversal() )
    {
      if ( gates_to_copy.find( gate.getAlias() ) != gates_to_copy.end() )
      {
        assert( !gate.isConstant() && "A constant gate cannot be the successor of a replaced gate" );
        int renamed_alias = descendants_renaming[gate.getAlias()];
        std::vector<int> inputs;
        for ( const auto& input : gate.inputs )
        {
          if ( descendants_renaming.find( input ) != descendants_renaming.end() )
          {
            inputs.push_back( descendants_renaming[input] );
          }
          else
          {
            inputs.push_back( input );
          }
        }
        bool anded;
        std::vector<std::vector<int>> lines;
        std::tie( anded, lines ) = gate.getQCIRGates( inputs );
        writeGate( out, renamed_alias, anded, lines );
      }
    }
  }
  void writeGate( std::ofstream& out, int alias, bool anded, const std::vector<std::vector<int>>& lines )
  {
    if ( anded )
    {
      if ( lines.empty() )
      {
        writeAnd( out, alias, {} );
      }
      else if ( lines.size() == 1 )
      {
        writeAnd( out, alias, lines[0] );
      }
      else
      {
        std::vector<int> aux_gate_aliases;
        for ( const auto& line : lines )
        {
          int aux_gate_alias = getNewVariable();
          aux_gate_aliases.push_back( aux_gate_alias );
          writeAnd( out, aux_gate_alias, line );
        }
        writeOr( out, alias, aux_gate_aliases );
      }
    }
    else
    {
      if ( lines.empty() )
      {
        writeOr( out, alias, {} );
      }
      else if ( lines.size() == 1 )
      {
        writeOr( out, alias, lines[0] );
      }
      else
      {
        std::vector<int> aux_gate_aliases;
        for ( const auto& line : lines )
        {
          int aux_gate_alias = getNewVariable();
          aux_gate_aliases.push_back( aux_gate_alias );
          writeOr( out, aux_gate_alias, line );
        }
        writeAnd( out, alias, aux_gate_aliases );
      }
    }
  }

  void writeAnd( std::ofstream& out, int out_var, const std::vector<int>& inputs )
  {
    std::ostringstream oss;
    for ( size_t i = 0; i < inputs.size(); ++i )
    {
      if ( i > 0 )
      {
        oss << ", ";
      }
      oss << inputs[i];
    }
    out << out_var << " = and(" << oss.str() << ")\n";
  }

  void writeOr( std::ofstream& out, int out_var, const std::vector<int>& inputs )
  {
    std::ostringstream oss;
    for ( size_t i = 0; i < inputs.size(); ++i )
    {
      if ( i > 0 )
      {
        oss << ", ";
      }
      oss << inputs[i];
    }
    out << out_var << " = or(" << oss.str() << ")\n";
  }

  void writeXor( std::ofstream& out, int out_var, int in1, int in2 )
  {
    if ( allow_xors )
    {
      out << out_var << " = xor(" << in1 << ", " << in2 << ")\n";
    }
    else
    {
      int or1 = getNewVariable();
      writeOr( out, or1, { in1, in2 } );
      int or2 = getNewVariable();
      writeOr( out, or2, { -in1, -in2 } );
      writeAnd( out, out_var, { or1, or2 } );
    }
  }

  void writeEquivalence( std::ofstream& out, int out_var, int in1, int in2 )
  {
    writeXor( out, out_var, -in1, in2 );
  }

  void writeConditionalEquivalence( std::ofstream& out, int out_var, int cond_var, int in1, int in2 )
  {
    int or1 = getNewVariable();
    writeOr( out, or1, { -cond_var, -in1, in2 } );
    int or2 = getNewVariable();
    writeOr( out, or2, { -cond_var, in1, -in2 } );
    writeAnd( out, out_var, { or1, or2 } );
  }

  void writeComment( std::ofstream& out, const std::string& val )
  {
    if ( writeComments )
    {
      out << "# " << val << "\n";
    }
  }

  void writeCycleConstraint( std::ofstream& out )
  {
    assert( !forbidden.empty() && "Cycle rules shall only be written if there are forbidden pairs" );
    writeComment( out, "Connection Variables Base" );
    setupConnectionvariables( out );

    if ( forbidden.size() > 1 )
    {
      writeComment( out, "Multiple Forbidden" );
      writeCombinedCycleRule( out );
    }

    writeComment( out, "Cycle Restrictions" );
    writeGateOutputCycleConstraints( out );
  }

  void setupConnectionvariables( std::ofstream& out )
  {
    std::vector<int> constraint_vars;
    for ( const auto& [inp, connection_vars] : connection_variables )
    {
      auto input_it = std::find( inputs.begin(), inputs.end(), inp );
      auto input_idx = std::distance( inputs.begin(), input_it );

      for ( int i = 0; i < nof_gates; ++i )
      {
        int constraint_var = getNewVariable();
        constraint_vars.push_back( constraint_var );
        writeOr( out, constraint_var, { -selection_variables[i][input_idx], connection_vars[i] } );
        for ( int j = 0; j < i; ++j )
        {
          int constraint_var = getNewVariable();
          constraint_vars.push_back( constraint_var );
          writeOr( out, constraint_var, { -selection_variables[i][getSelectionVariableIndex( j )], -connection_vars[j], connection_vars[i] } );
        }
      }
    }
    int constraint_var = getNewVariable();
    writeAnd( out, constraint_var, constraint_vars );
    constraintGates.push_back( constraint_var );
  }

  void writeCombinedCycleRule( std::ofstream& out )
  {
    assert( forbidden.size() > 1 && "Cycle rules shall only be written if there are forbidden pairs" );
    std::vector<int> constraint_vars;
    std::unordered_map<int, std::unordered_set<int>> input_in_pair;
    std::unordered_map<int, std::unordered_set<int>> not_a_pair;

    for ( const auto& [a, y] : forbidden )
    {
      input_in_pair[a].insert( y );
    }
    for ( const auto& [x, y_set] : input_in_pair )
    {
      for ( const auto& [_, b] : forbidden )
      {
        if ( y_set.find( b ) == y_set.end() )
        {
          not_a_pair[x].insert( b );
        }
      }
    }

    for ( const auto& [outp, inputs] : not_a_pair )
    {
      auto output_it = std::find( subcircuit_outputs.begin(), subcircuit_outputs.end(), outp );
      auto output_idx = std::distance( subcircuit_outputs.begin(), output_it );
      for ( const auto& inp : inputs )
      {
        const auto& connection_vars = connection_variables[inp];
        for ( int i = 0; i < nof_gates; ++i )
        {
          for ( const auto& k : input_in_pair[outp] )
          {
            auto input_it = std::find( inputs.begin(), inputs.end(), k );
            auto input_idx = std::distance( inputs.begin(), input_it );
            int condition_var = getNewVariable();
            writeAnd( out, condition_var, { gate_output_variables[i][output_idx], connection_vars[i] } );
            for ( int j = 0; j < nof_gates; ++j )
            {
              int constraint_var = getNewVariable();
              constraint_vars.push_back( constraint_var );
              writeOr( out, constraint_var, { -condition_var, -selection_variables[j][input_idx], connection_vars[j] } );
            }
          }
        }
      }
    }
    int constraint_var = getNewVariable();
    writeAnd( out, constraint_var, constraint_vars );
    constraintGates.push_back( constraint_var );
  }

  void writeGateOutputCycleConstraints( std::ofstream& out )
  {
    std::vector<int> constraint_vars;
    for ( const auto& [outp, inp] : forbidden )
    {
      auto output_it = std::find( subcircuit_outputs.begin(), subcircuit_outputs.end(), outp );
      auto output_idx = std::distance( subcircuit_outputs.begin(), output_it );
      const auto& connection_vars = connection_variables[inp];
      for ( int i = 0; i < nof_gates; ++i )
      {
        int constraint_var = getNewVariable();
        constraint_vars.push_back( constraint_var );
        writeOr( out, constraint_var, { -gate_output_variables[i][output_idx], -connection_vars[i] } );
      }
    }

    int constraint_var = getNewVariable();
    writeAnd( out, constraint_var, constraint_vars );
    constraintGates.push_back( constraint_var );
  }

  void writeAigerConstraints( std::ofstream& out )
  {
    assert( nof_gate_inputs == 2 && "An AIG gate must have two inputs" );
    assert( useTrivialRuleConstraint );
    writeComment( out, "AIGER Constraints" );
    for ( int i = 0; i < nof_gates; ++i )
    {
      int constraint_var = getNewVariable();
      writeOr( out, constraint_var, { -gate_definition_variables[i][0], -gate_definition_variables[i][1], gate_definition_variables[i][2] } );
      constraintGates.push_back( constraint_var );
    }
  }

  void writeXagerConstraints( std::ofstream& out )
  {
    assert( nof_gate_inputs == 2 && "An gate must have two inputs" );
    assert( useTrivialRuleConstraint );
    writeComment( out, "XAGER Constraints" );
    for ( int i = 0; i < nof_gates; ++i )
    {
      int constraint_var_and = getNewVariable();
      writeOr( out, constraint_var_and, { -gate_definition_variables[i][0], -gate_definition_variables[i][1], gate_definition_variables[i][2] } );
      int constraint_var1 = getNewVariable();
      writeOr( out, constraint_var1, { gate_definition_variables[i][0], gate_definition_variables[i][1], -gate_definition_variables[i][2] } );
      int constraint_var2 = getNewVariable();
      writeOr( out, constraint_var2, { gate_definition_variables[i][0], -gate_definition_variables[i][1], gate_definition_variables[i][2] } );
      int constraint_var3 = getNewVariable();
      writeOr( out, constraint_var3, { -gate_definition_variables[i][0], gate_definition_variables[i][1], gate_definition_variables[i][2] } );
      int constraint_var4 = getNewVariable();
      writeOr( out, constraint_var4, { -gate_definition_variables[i][0], -gate_definition_variables[i][1], -gate_definition_variables[i][2] } );
      int constraint_var_xor = getNewVariable();
      writeAnd( out, constraint_var_xor, { constraint_var1, constraint_var2, constraint_var3, constraint_var4 } ); // xor
      int constraint_var = getNewVariable();
      writeXor( out, constraint_var, constraint_var_xor, constraint_var_and );
      constraintGates.push_back( constraint_var );
    }
  }

  void writeXagerConstraints( std::ofstream& out, int NofAND )
  {
    assert( nof_gate_inputs == 2 && "An gate must have two inputs" );
    assert( useTrivialRuleConstraint );
    writeComment( out, "XAGER Constraints" );

    // 新增逻辑：记录所有 AND 变量
    std::vector<int> constraint_vars;

    for ( int i = 0; i < nof_gates; ++i )
    {
      int constraint_var_and = getNewVariable();
      writeOr( out, constraint_var_and,
               { -gate_definition_variables[i][0], -gate_definition_variables[i][1], gate_definition_variables[i][2] } );

      int constraint_var1 = getNewVariable();
      writeOr( out, constraint_var1,
               { gate_definition_variables[i][0], gate_definition_variables[i][1], -gate_definition_variables[i][2] } );

      int constraint_var2 = getNewVariable();
      writeOr( out, constraint_var2,
               { gate_definition_variables[i][0], -gate_definition_variables[i][1], gate_definition_variables[i][2] } );

      int constraint_var3 = getNewVariable();
      writeOr( out, constraint_var3,
               { -gate_definition_variables[i][0], gate_definition_variables[i][1], gate_definition_variables[i][2] } );

      int constraint_var4 = getNewVariable();
      writeOr( out, constraint_var4,
               { -gate_definition_variables[i][0], -gate_definition_variables[i][1], -gate_definition_variables[i][2] } );

      int constraint_var_xor = getNewVariable();
      writeAnd( out, constraint_var_xor, { constraint_var1, constraint_var2, constraint_var3, constraint_var4 } ); // xor

      int constraint_var = getNewVariable();
      writeXor( out, constraint_var, constraint_var_xor, constraint_var_and );

      constraintGates.push_back( constraint_var );

      constraint_vars.push_back( constraint_var_and );
    }

    // 约束AND数量
    if ( !constraint_vars.empty() )
    {
      std::vector<std::vector<int>> valid_combinations;

      // 构造所有不超过 AND个数的 个1 的组合
      int size = constraint_vars.size();
      for ( int count = 0; count <= NofAND; ++count )
      {
        // 生成所有大小为 count 的组合
        std::vector<int> indices( count );
        std::iota( indices.begin(), indices.end(), 0 ); // 初始化前 count 个元素
        do
        {
          std::vector<int> combination( size, -1 ); // 初始全为 -1（表示取 neg）
          for ( int index : indices )
          {
            combination[index] = 1; // 表示取 pos
          }
          valid_combinations.push_back( combination );

        } while ( std::next_permutation( indices.begin(), indices.end() ) );
      }

      // 将组合转换为布尔逻辑表达式
      std::vector<int> global_constraints;
      for ( const auto& combination : valid_combinations )
      {
        std::vector<int> clause;
        for ( size_t i = 0; i < combination.size(); ++i )
        {
          if ( combination[i] == 1 )
            clause.push_back( constraint_vars[i] );
          else
            clause.push_back( -constraint_vars[i] );
        }

        // 添加每个组合的逻辑“与”
        int clause_var = getNewVariable();
        writeAnd( out, clause_var, clause );
        global_constraints.push_back( clause_var );
      }

      // 将所有子约束用“或”连接起来
      int final_constraint = getNewVariable();
      writeOr( out, final_constraint, global_constraints );
      writeComment( out, "Adding global constraint for no-more-than-n logic" );
    }
  }

  void writeEncoding( std::ofstream& out )
  {
    restrictSelectionVariables( out );
    // for each output exactly one gateoutputvariable shall be true
    writeComment( out, "Output Vars" );
    for ( size_t i = 0; i < subcircuit_outputs.size(); ++i )
    {
      std::vector<int> vars;
      for ( const auto& gate_output : gate_output_variables )
      {
        vars.push_back( gate_output[i] );
      }
      writeComment( out, "Constraints for output " + std::to_string( i ) );
      addCardinalityConstraint( vars, 1, out );
    }
    writeComment( out, "Output Vars end" );

    restrictGates( out );
    addSymmetryBreakingConstraints( out ); // 对称破缺
  }

  void writeEquivalenceConstraints( std::ofstream& out )
  {
    setupSubcircuitOutputVariables( out );
    writeComment( out, "Establish equivalence between specification and copy" );
    for ( const auto& spec_out : specification.getOutputs() )
    {
      if ( descendants_renaming.find( spec_out ) != descendants_renaming.end() )
      {
        int spec_out_copy = descendants_renaming[spec_out];
        int c1 = getNewVariable();
        writeEquivalence( out, c1, spec_out, spec_out_copy );
        constraintGates.push_back( c1 );
      }
    }
  }

  void writeConstraints( std::ofstream& out )
  {
    setupSubcircuitOutputVariables( out );
    writeComment( out, "Establish equivalence between specification and copy" );

    for ( const auto& spec_out : subcircuit_outputs )
    {
      if ( descendants_renaming.find( spec_out ) != descendants_renaming.end() )
      {
        int spec_out_copy = descendants_renaming[spec_out];
        int c1 = getNewVariable();
        writeEquivalence( out, c1, spec_out, spec_out_copy );
        constraintGates.push_back( c1 );
      }
    }
  }

  void restrictSelectionVariables( std::ofstream& out )
  {
    writeComment( out, "Selection Vars" );
    for ( int i = 0; i < nof_gates; ++i )
    {
      writeComment( out, "Constraints for selection variables at gate " + std::to_string( i ) );
      // printVector(selection_variables[i]);
      addCardinalityConstraint( selection_variables[i], nof_gate_inputs, out );
    }
  }

  void restrictGates( std::ofstream& out )
  {
    for ( int i = 0; i < nof_gates; ++i )
    {
      writeComment( out, "Rules for gate " + std::to_string( i ) );
      addGateConstraint( i, out );
    }
  }

  void setupSubcircuitOutputVariables( std::ofstream& out )
  {
    writeComment( out, "Subcircuit output equivalences" );
    for ( size_t idx = 0; idx < subcircuit_outputs.size(); ++idx )
    {
      int sub_circ_out = descendants_renaming[subcircuit_outputs[idx]];
      for ( int i = 0; i < nof_gates; ++i )
      {
        int in_node1 = getGate( i );
        int c = getNewVariable();
        writeConditionalEquivalence( out, c, gate_output_variables[i][idx], in_node1, sub_circ_out );
        constraintGates.push_back( c );
      }
    }
  }
  void addGateConstraint( int gate_index, std::ofstream& out )
  {
    auto idx_tuple = initIndexTuple( nof_gate_inputs );
    auto end_idx_tuple = getMaxIdxTuple( nof_gate_inputs, gate_index + inputs.size() );

    int out_node = getGate( gate_index );
    while ( true )
    {
      std::vector<int> selection_vars;
      for ( const auto& x : idx_tuple )
      {
        selection_vars.push_back( -selection_variables[gate_index][x] );
      }

      // in normalised chains the outputs has to be false if all inputs are false
      std::vector<int> normalised_input_condition_vars;
      for ( size_t j = 0; j < idx_tuple.size(); ++j )
      {
        int in_node = getNode( idx_tuple[j] );
        int cond = getCondition( in_node, 0 );
        normalised_input_condition_vars.push_back( cond );
      }

      int normalised_output_condition = getCondition( out_node, 1 ); // this means that the output has to be false
      int constraint_var = getNewVariable();
      auto merged_vars = mergeVectors( { normalised_output_condition }, normalised_input_condition_vars, selection_vars );
      writeOr( out, constraint_var, merged_vars );
      constraintGates.push_back( constraint_var );

      for ( int i = 1; i < ( 1 << nof_gate_inputs ); ++i )
      {
        auto polarities = getBits( i, nof_gate_inputs );
        std::vector<int> aux_vars;
        for ( size_t j = 0; j < idx_tuple.size(); ++j )
        {
          int in_node = getNode( idx_tuple[j] );
          int cond = getCondition( in_node, polarities[j] );
          aux_vars.push_back( cond );
        }

        for ( int c = 0; c < 2; ++c )
        {
          int c1 = getCondition( getGateDefinitionVariable( gate_index, i ), c );
          int c2 = getCondition( out_node, c ^ 1 ); // c^1 negate c
          int constraint_var = getNewVariable();
          auto merged_vars = mergeVectors( { c1, c2 }, aux_vars, selection_vars );
          writeOr( out, constraint_var, merged_vars );
          constraintGates.push_back( constraint_var );
        }
      }

      if ( idx_tuple == end_idx_tuple )
      {
        break;
      }
      incrementIndexTuple( idx_tuple );
    }
  }

  void addCardinalityConstraint( const std::vector<int>& vars, int cardinality, std::ofstream& out )
  {
    if ( vars.size() == cardinality )
    {
      int gate_var = getNewVariable();
      writeAnd( out, gate_var, vars );
      constraintGates.push_back( gate_var );
      return;
    }
    SequentialCounterCardinalityConstraint( vars, cardinality, out );
  }

  void SequentialCounterCardinalityConstraint( const std::vector<int>& vars, int cardinality, std::ofstream& out )
  {
    assert( vars.size() > cardinality );
    std::vector<int> carries;
    std::vector<int> aux = { vars[0] };
    for ( size_t idx = 1; idx < vars.size(); ++idx )
    {
      bool last_counter = ( idx == vars.size() - 1 );
      auto [outputs, carry] = cardinalitySubCircuit( vars[idx], aux, cardinality, last_counter, out );
      aux = outputs;
      if ( carry != -1 )
      {
        carries.push_back( -carry );
      }
    }

    int constraint_var = getNewVariable();
    carries.push_back( aux.back() );
    writeAnd( out, constraint_var, carries );
    constraintGates.push_back( constraint_var );
  }

  std::pair<std::vector<int>, int> cardinalitySubCircuit( int in1, const std::vector<int>& inputs, int cardinality, bool last_counter, std::ofstream& out )
  {
    if ( last_counter )
    {
      int carry = getNewVariable();
      writeAnd( out, carry, { in1, inputs.back() } );
      int gate_var_or = getNewVariable();
      if ( inputs.size() == 1 )
      {
        writeOr( out, gate_var_or, { in1, inputs.back() } );
      }
      else
      {
        int gate_var_and = getNewVariable();
        writeAnd( out, gate_var_and, { in1, inputs[inputs.size() - 2] } );
        writeOr( out, gate_var_or, { gate_var_and, inputs.back() } );
      }
      return { { gate_var_or }, carry };
    }

    int n = std::min( static_cast<int>( inputs.size() ), cardinality );
    std::vector<int> outputs;
    int or_in = in1;
    for ( const auto& input : inputs )
    {
      int gate_var_or = getNewVariable();
      writeOr( out, gate_var_or, { or_in, input } );
      outputs.push_back( gate_var_or );

      int gate_var_and = getNewVariable();
      writeAnd( out, gate_var_and, { in1, input } );
      or_in = gate_var_and;
    }

    int carry = ( outputs.size() == cardinality ) ? or_in : -1;
    if ( outputs.size() != cardinality )
    {
      outputs.push_back( or_in );
    }
    return { outputs, carry };
  }

  void addSymmetryBreakingConstraints( std::ofstream& out )
  {
    writeComment( out, "Non-trivial constraints" );
    addNonTrivialConstraint( out ); // 非平凡常量门约束
    writeComment( out, "Use all steps constraints" );
    addUseAllStepsConstraint( out );
    writeComment( out, "No reapplication constraints" );
    addNoReapplicationConstraint( out );
    writeComment( out, "Ordered steps constraints" );
    addOrderedStepsConstraint( out );
  }

  void addNonTrivialConstraint( std::ofstream& out )
  {
    for ( int i = 0; i < nof_gates; ++i )
    {
      // We exclude gates representing constant values
      int constraint_constant_false = getNewVariable();
      writeOr( out, constraint_constant_false, gate_definition_variables[i] );
      // Normalised gates cannot represent true
      // int constraint_constant_true = getNewVariable();
      // writeOr(out, constraint_constant_true, negateVector(gate_definition_variables[i]));
      // std::vector<int> constraints = {constraint_constant_false, constraint_constant_true};
      std::vector<int> constraints = { constraint_constant_false };

      // We exclude gates representing the projection of one of its inputs
      for ( int j = 0; j < nof_gate_inputs; ++j )
      {
        int start = 1 << ( nof_gate_inputs - j );
        int block_length = 1 << ( nof_gate_inputs - j - 1 );
        std::vector<int> vars1;
        std::vector<int> vars2;
        for ( int k = 0; k < ( 1 << j ); ++k )
        {
          vars1.insert( vars1.end(), gate_definition_variables[i].begin() + k * start - ( k == 0 ? 0 : 1 ), gate_definition_variables[i].begin() + k * start + block_length - 1 );
          vars2.insert( vars2.end(), gate_definition_variables[i].begin() + k * start + block_length - 1, gate_definition_variables[i].begin() + ( k + 1 ) * start - 1 );
        }
        for ( auto& var : vars2 )
        {
          var = -var;
        }
        int constraint_projection = getNewVariable();
        vars1.insert( vars1.end(), vars2.begin(), vars2.end() );
        writeOr( out, constraint_projection, vars1 );
        constraints.push_back( constraint_projection );
      }
      constraintGates.insert( constraintGates.end(), constraints.begin(), constraints.end() );
    }
  }

  void addUseAllStepsConstraint( std::ofstream& out )
  {
    for ( int i = 0; i < nof_gates; ++i )
    {
      std::vector<int> disjuncts( gate_output_variables[i].begin(), gate_output_variables[i].end() );
      for ( int j = i + 1; j < nof_gates; ++j )
      {
        disjuncts.push_back( selection_variables[j][getSelectionVariableIndex( i )] );
      }
      int constraint_var = getNewVariable();
      writeOr( out, constraint_var, disjuncts );
      constraintGates.push_back( constraint_var );
    }
  }

  void addNoReapplicationConstraint( std::ofstream& out )
  {
    std::vector<int> constraints;
    for ( int i = 0; i < nof_gates; ++i )
    {
      auto idx_tuple = initIndexTuple( nof_gate_inputs );
      auto end_idx_tuple = getMaxIdxTuple( nof_gate_inputs, getSelectionVariableIndex( i ) );
      while ( true )
      {
        std::vector<int> sel_vars;
        for ( const auto& idx : idx_tuple )
        {
          sel_vars.push_back( -selection_variables[i][idx] );
        }
        for ( int j = i + 1; j < nof_gates; ++j )
        {
          int selector = selection_variables[j][getSelectionVariableIndex( i )];
          std::vector<int> disjuncts = sel_vars;
          disjuncts.push_back( -selector );
          const auto& selection_vars_j = selection_variables[j];
          for ( size_t idx = 0; idx < selection_vars_j.size(); ++idx )
          {
            if ( std::find( idx_tuple.begin(), idx_tuple.end(), idx ) == idx_tuple.end() && selection_vars_j[idx] != selector )
            {
              disjuncts.push_back( selection_vars_j[idx] );
            }
          }

          int or_var = getNewVariable();
          writeOr( out, or_var, disjuncts );
          constraints.push_back( or_var );
        }

        if ( idx_tuple == end_idx_tuple )
        {
          break;
        }
        incrementIndexTupleSimple( idx_tuple );
      }
    }
    constraintGates.insert( constraintGates.end(), constraints.begin(), constraints.end() );
  }

  void addOrderedStepsConstraint( std::ofstream& out )
  {
    std::vector<int> constraints;
    for ( int i = 0; i < nof_gates - 1; ++i )
    {
      for ( int j = 0; j < i; ++j )
      {
        int selector = selection_variables[i][getSelectionVariableIndex( j )];
        std::vector<int> vars;
        for ( int k = j; k <= i; ++k )
        {
          vars.push_back( selection_variables[i + 1][getSelectionVariableIndex( k )] );
        }
        int constraint_var = getNewVariable();
        std::vector<int> disjuncts = { -selector };
        disjuncts.insert( disjuncts.end(), vars.begin(), vars.end() );
        writeOr( out, constraint_var, disjuncts );
        constraints.push_back( constraint_var );
      }
    }
    constraintGates.insert( constraintGates.end(), constraints.begin(), constraints.end() );
  }

  int getGate( int idx )
  {
    return internal_gates[idx];
  }
  int getNode( int idx )
  {
    if ( idx < static_cast<int>( inputs.size() ) )
    {
      return inputs[idx];
    }
    else
    {
      return internal_gates[idx - inputs.size()];
    }
  }
  std::vector<int> getBits( int n, int nof_bits )
  {
    std::vector<int> bits( nof_bits );
    for ( int i = 0; i < nof_bits; ++i )
    {
      bits[nof_bits - 1 - i] = ( n >> i ) & 1;
    }
    return bits;
  }
  std::vector<int> initIndexTuple( int size )
  {
    std::vector<int> idx_tuple( size );
    for ( int i = 0; i < size; ++i )
    {
      idx_tuple[i] = i;
    }
    return idx_tuple;
  }

  std::vector<int> getMaxIdxTuple( int size, int max_idx )
  {
    std::vector<int> idx_tuple( size );
    for ( int i = 0; i < size; ++i )
    {
      idx_tuple[i] = max_idx - size + i;
    }
    return idx_tuple;
  }

  int getCondition( int variable, int constant )
  {
    if ( constant == 1 )
    { // true
      return -variable;
    }
    else
    {
      return variable;
    }
  }
  int getGateDefinitionVariable( int gate_idx, int idx )
  {
    int offset = 1; // We use normal gates
    return gate_definition_variables[gate_idx][idx - offset];
  }
  int getSelectionVariableIndex( int gate_idx ) const
  {
    return inputs.size() + gate_idx;
  }
  void resetIndexSubTuple( std::vector<int>& val, int idx )
  {
    for ( int i = 0; i < idx; ++i )
    {
      val[i] = i;
    }
  }
  void incrementIndexTupleSimple( std::vector<int>& val ) const
  {
    for ( size_t i = 0; i < val.size(); ++i )
    {
      if ( i == val.size() - 1 )
      {
        val[i] += 1;
      }
      else if ( val[i] < val[i + 1] - 1 )
      {
        val[i] += 1;
        break;
      }
    }
  }
  std::vector<int> incrementIndexTuple( std::vector<int>& val )
  {
    for ( size_t i = 0; i < val.size(); ++i )
    {
      if ( i == val.size() - 1 )
      {
        val[i] += 1;
        resetIndexSubTuple( val, i );
      }
      else if ( val[i] < val[i + 1] - 1 )
      {
        val[i] += 1;
        resetIndexSubTuple( val, i );
        break;
      }
    }
    return val;
  }

  std::vector<int> mergeVectors( const std::vector<int>& vec1, const std::vector<int>& vec2 )
  {
    std::vector<int> result;
    result.reserve( vec1.size() + vec2.size() );
    result.insert( result.end(), vec1.begin(), vec1.end() );
    result.insert( result.end(), vec2.begin(), vec2.end() );
    return result;
  }
  std::vector<int> mergeVectors( const std::vector<int>& vec1, const std::vector<int>& vec2, const std::vector<int>& vec3 )
  {
    std::vector<int> result;
    result.reserve( vec1.size() + vec2.size() + vec3.size() );
    result.insert( result.end(), vec1.begin(), vec1.end() );
    result.insert( result.end(), vec2.begin(), vec2.end() );
    result.insert( result.end(), vec3.begin(), vec3.end() );
    return result;
  }
};

class SubcircuitSynthesiser
{
public:
  Specification specification;

  std::map<int, int> nof_replacements_per_size;
  std::map<int, int> total_nof_checks_per_size;
  std::map<int, std::string> subcir_equiv_dir; // 假设键为int，值为等效的子电路名称
  int subcircuit_counter;
  int DAG_Flag;

public:
  SubcircuitSynthesiser( Specification spec, int Flag )
      : specification( spec ),
        subcircuit_counter( 0 ),
        DAG_Flag( Flag )
  {
  }

  // 返回值是可行性；电路信息；时间超时
  auto reduceSubcircuit( const std::vector<int>& to_replace, int nof_gate_inputs, bool require_reduction, bool useLocalCircuit )
  {
    auto start = std::chrono::high_resolution_clock::now();
    bool realisable;
    int size;
    std::vector<bool> ttXor = { false, true, true, false };

    std::vector<std::tuple<int, std::vector<int>, std::vector<bool>>> gates;
    std::unordered_map<int, int> output_association;
    std::vector<int> subcircuit_inputs;
    std::vector<int> gate_names;

    /* Circuit是一个宏定义，里面包含的子电路的约束 ：
                        gates： 门的编号，输入节点 , 真值表
                    output_association：输出之间的连接关系
                        subcircuit_inputs：
                        gate_names ：
                */
    Circuit circuit;
    bool timeout;
    std::tie( realisable, size, circuit, timeout ) = synthesiseQBF( to_replace, nof_gate_inputs, require_reduction, useLocalCircuit);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    double total_time = 0;
    total_time += elapsed.count();

    if ( realisable )
    {
      unsigned int NofAnd = 0;
      unsigned int NofXor = 0;

      std::tie( gates, output_association, subcircuit_inputs, gate_names ) = circuit;

      for ( int gate : to_replace )
      {
        if ( specification.isGateAnd( gate ) )
          NofAnd += 1;
      }
      for ( auto gate : gates )
      {
        bool flag = true;
        std::vector<bool> ttGate = std::get<2>( gate );

        if ( ttGate == ttXor )
          NofXor += 1;
      }
      if ( gate_names.size() - NofXor <= NofAnd ) // 确保ANd门的减少
      {
        auto unused = specification.replaceSubcircuit( to_replace, gates, output_association, specification );
        return std::make_tuple( realisable, std::make_tuple( gate_names, output_association, unused ), false );
      }
      return std::make_tuple( false, std::tuple<std::vector<int>, std::unordered_map<int, int>, std::unordered_set<int>>(), timeout );
    }
    else
    {
      return std::make_tuple( realisable, std::tuple<std::vector<int>, std::unordered_map<int, int>, std::unordered_set<int>>(), timeout );
    }
  }

  void printGate( const std::vector<std::tuple<int, std::vector<int>, std::vector<bool>>>& gates )
  {
    for ( const auto& gate : gates )
    {
      int id = std::get<0>( gate );
      const std::vector<int>& inputs = std::get<1>( gate );
      const std::vector<bool>& states = std::get<2>( gate );

      std::cout << "Gate ID: " << id << std::endl;
      std::cout << "Inputs: ";
      for ( int input : inputs )
      {
        std::cout << input << " ";
      }
      std::cout << std::endl;

      std::cout << "States: ";
      for ( bool state : states )
      {
        std::cout << ( state ? "true" : "false" ) << " ";
      }
      std::cout << std::endl
                << std::endl;
    }
  }
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
    // printAlias2Gate(specification.alias2gate);
    // std::cout << "NofGates: " << specification.getNofGates() << "  "<< "level: " << specification.getDepth() << std::endl;
  }
  // 实现qbf的重综合
  std::tuple<bool, int, Circuit, bool> synthesiseQBF( const std::vector<int>& to_replace, int nof_gate_inputs, bool require_reduction, bool useLocalCircuit )
  {
    if ( to_replace.empty() )
    {
      std::cerr << "No gates to replace" << std::endl;
      return std::make_tuple( false, 0, Circuit(), false );
    }
    if ( nof_gate_inputs != 2 )
    {
      std::cerr << "Only two-input gates are supported" << std::endl;
      return std::make_tuple( false, 0, Circuit(), false );
    }

  //   // 编码电路
  //   return synthesise( to_replace, nof_gate_inputs, require_reduction, useLocalCircuit );
  // }
  // {
    EncoderCircuits encoder( specification, to_replace, DAG_Flag );
    // encoder.useGateInputVariables(useGateInputVariables); // 为每个门的每个输入使用一个单独的变量

    if ( encoder.getSubcircuitInputs().size() < nof_gate_inputs )
    {
      return std::make_tuple( false, 0, Circuit(), false );
    }
    if ( encoder.getSubcircuitOutputs().empty() )
    {
      std::cerr << "Subcircuit with no outputs detected" << std::endl;
      std::cerr << "To replace: ";
      for ( const auto& gate : to_replace )
      {
        std::cerr << gate << " ";
      }
      std::cerr << std::endl;
      return std::make_tuple( true, 0, Circuit(), false );
    }

    return synthesise( encoder, to_replace, nof_gate_inputs, require_reduction, useLocalCircuit );
  }

  // 获取替换后的子电路
  std::tuple<bool, int, Circuit, bool> synthesise( EncoderCircuits& encoder, const std::vector<int>& to_replace, int nof_gate_inputs, bool require_reduction, bool useLocalCircuit  )
  {
    subcircuit_counter++;
    int NofAnd = 0;
    bool realisable = false;
    int max_size = require_reduction ? ( to_replace.size() - 1 ) : to_replace.size();

    Circuit subcir_candidate;
    int smallest_representation = to_replace.size();

    for ( int gate : to_replace )
    {
      if ( specification.isGateAnd( gate ) )
        NofAnd += 1;
    }
    // 等面积替换搜寻
    if ( !require_reduction )
    {
      std::tie( realisable, subcir_candidate ) = analyseOriginalSize( encoder, to_replace, nof_gate_inputs, NofAnd, useLocalCircuit );
      if ( !realisable )
      {
        return std::make_tuple( realisable, 0, Circuit(), false );
      }
    }
    // 自顶向下搜寻，每次-1
    int bound = to_replace.size() - 1;

    for ( int nof_gates = bound; nof_gates > 0; --nof_gates )
    {
      bool current_realisable;
      Circuit subcir;
      std::tie( current_realisable, subcir ) = checkEncoding( encoder, to_replace, nof_gates, nof_gate_inputs, NofAnd, useLocalCircuit );
      if ( current_realisable )
      {
        realisable = true;
        smallest_representation = nof_gates;
        subcir_candidate = subcir;
      }
      else
      {
        break;
      }
    }
    //   std::vector<std::tuple<int, std::vector<int>, std::vector<bool>>> gates;
    // 	std::unordered_map<int, int> output_association;
    // 	std::vector<int> subcircuit_inputs;
    // 	std::vector<int> gate_names;
    // 	std::tie(gates, output_association, subcircuit_inputs, gate_names) = subcir_candidate;
    //  for ( const auto& pair : output_association )
    //  {
    //    std::cout << "*: " << pair.first << " : " << pair.second << std::endl;
    //  }

    if ( !realisable )
    {
      return std::make_tuple( realisable, 0, Circuit(), false );
    }
    return std::make_tuple( realisable, smallest_representation, subcir_candidate, false );
  }

  // 测试等面积替换
  std::tuple<bool, Circuit> analyseOriginalSize( EncoderCircuits& encoder, const std::vector<int>& to_replace, int nof_gate_inputs, int nof_and, bool useLocalCircuit)
  {
    int nof_gates = to_replace.size();
    bool realisable;
    Circuit subcir_candidate;
    std::tie( realisable, subcir_candidate ) = checkEncoding( encoder, to_replace, nof_gates, nof_gate_inputs, nof_and, useLocalCircuit );

    if ( !realisable )
    {
      // std::cerr << "Warning: Cannot be replaced by circuit of initial size" << std::endl;
      return std::make_tuple( false, Circuit() );
    }

    return std::make_tuple( realisable, subcir_candidate );
  }
#pragma region main

  void writeEncoding( std::ofstream& file, EncoderCircuits& encoder, int nof_gates, int nof_gate_inputs, int nof_and, bool useLocalCircuit ) const
  {
    auto start = std::chrono::high_resolution_clock::now();
    encoder.getEncoding( nof_gates, nof_gate_inputs, file, nof_and , useLocalCircuit);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    return;
  }

  // 编码，运行，解码
  std::tuple<bool, Circuit> checkEncoding( EncoderCircuits& encoder, const std::vector<int>& to_replace, int nof_gates, int nof_gate_inputs, int nof_and, bool useLocalCircuit)
  {
    int timeout = 90;
    std::string encoding_suffix = ".qcir";
    std::string fname;
    bool realisable = false;
    std::unordered_map<int, bool> assignment; // 返回的结果
    double used_time = 0.0;
    bool valid = false;

    char tmpname[] = "/tmp/tempfile.XXXXXX";
    int fd = mkstemp( tmpname );
    close( fd );

    std::ofstream tmp( tmpname );
    writeEncoding( tmp, encoder, nof_gates, nof_gate_inputs, nof_and, useLocalCircuit );
    tmp.close();

    std::tie( realisable, assignment, used_time, valid ) = runSolverAndGetAssignment( tmpname, timeout );
    cleanup_zombie_processes();
    // if ( valid ) //测试
    // {
    //   std::ifstream temp_input( tmpname, std::ios::binary );
    //   std::ofstream output_file( "/home/zj/also/test.qcir", std::ios::binary );
    //   output_file << temp_input.rdbuf(); // 将临时文件内容写入新文件
    //   temp_input.close();
    //   output_file.close();
    // }
    std::remove( tmpname );
    if ( !valid )
    {
      std::cerr << "QBF yielded invalid results -- error in encoding" << std::endl;
      assert( false );
    }

    if ( realisable )
    {
      auto subcircuit_data = extractGatesFromAssignment( to_replace, encoder, nof_gates, nof_gate_inputs, assignment );

      return std::make_tuple( realisable, subcircuit_data );
    }
    else
    {
      return std::make_tuple( realisable, Circuit() );
    }
  }

  Circuit extractGatesFromAssignment( const std::vector<int>& to_replace, EncoderCircuits& encoder, int nof_gates, int nof_gate_inputs, const std::unordered_map<int, bool>& assignment ) const
  {
    std::vector<int> gate_names;
    if ( nof_gates <= to_replace.size() )
    {
      gate_names.assign( to_replace.begin(), to_replace.begin() + nof_gates );
    }
    else
    {
      gate_names.assign( to_replace.begin(), to_replace.end() );
      for ( int i = 0; i < nof_gates - to_replace.size(); ++i )
      {
        gate_names.push_back( specification.max_var + i + 1 );
      }
    }

    auto gate_definition_variables = encoder.getGateDefinitionVariables();
    auto selection_variables = encoder.getSelectionVariables();
    auto gate_output_variables = encoder.getGateOutputVariables();
    auto subcircuit_inputs = encoder.getSubcircuitInputs();
    auto subcircuit_outputs = encoder.getSubcircuitOutputs();

    std::vector<std::tuple<int, std::vector<int>, std::vector<bool>>> gates;
    std::unordered_map<int, int> output_association; // associates the outputs of the subcircuits with replaced gates

    for ( int i = 0; i < nof_gates; ++i )
    {
      std::vector<int> inputs;
      for ( size_t j = 0; j < selection_variables[i].size(); ++j )
      {
        if ( assignment.at( selection_variables[i][j] ) )
        {
          if ( j < subcircuit_inputs.size() )
          {
            inputs.push_back( subcircuit_inputs[j] );
          }
          else
          {
            inputs.push_back( gate_names[j - subcircuit_inputs.size()] );
          }
        }
      }

      std::vector<bool> gate_definitions( 1 << nof_gate_inputs, false );
      int offset = 1; // We use normal gates
      for ( int j = offset; j < ( 1 << nof_gate_inputs ); ++j )
      {
        if ( assignment.at( gate_definition_variables[i][j - offset] ) )
        {
          gate_definitions[j] = true;
        }
      }

      gates.emplace_back( gate_names[i], inputs, gate_definitions );

      for ( size_t j = 0; j < gate_output_variables[i].size(); ++j )
      {
        if ( assignment.at( gate_output_variables[i][j] ) )
        {
          output_association[subcircuit_outputs[j]] = gate_names[i];
        }
      }
    }

    return std::make_tuple( gates, output_association, subcircuit_inputs, gate_names );
  }

  /*如果返回码为 10，表示求解成功，调用 getAssignment 方法解析输出并返回结果。
          如果返回码为 20，表示求解失败，返回空的赋值结果。 */
  std::tuple<bool, std::unordered_map<int, bool>, double, bool> runSolverAndGetAssignment( const std::string& input )
  {
    std::string qfun_path = "qfun";
    std::string solver_cmd = qfun_path + " " + input;
    std::string output_pattern = "\\nv\\s*(.*)\\n*";
    auto start = std::chrono::high_resolution_clock::now();

    FILE* pipe = popen( solver_cmd.c_str(), "r" );
    if ( !pipe )
    {
      std::cerr << "Failed to run solver command." << std::endl;
      return std::make_tuple( false, std::unordered_map<int, bool>(), 0.0, false );
    }

    std::array<char, 128> buffer;
    std::string result;
    while ( fgets( buffer.data(), buffer.size(), pipe ) != nullptr )
    {
      result += buffer.data();
    }
    std::cout << result << std::endl;
    int return_code = pclose( pipe );
    // std::cout << "return_code" << return_code <<std::endl;
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    double solving_time = elapsed.count();

    if ( return_code == 10 )
    {
      auto assignment = getAssignment( result, output_pattern );
      return std::make_tuple( true, assignment, solving_time, true );
    }
    else if ( return_code == 20 )
    {
      return std::make_tuple( false, std::unordered_map<int, bool>(), solving_time, true );
    }
    else
    {
      std::cerr << "Solver message:" << std::endl;
      std::cerr << result << std::endl;
      return std::make_tuple( false, std::unordered_map<int, bool>(), 0.0, false );
    }
  }

  // std::tuple<bool, std::unordered_map<int, bool>, double, bool> runSolverAndGetAssignment(const std::string &input, int timeout) {
  //     std::string solver_cmd;
  //     std::string output_pattern;
  // 		std::string qfun_path = "qfun";

  //     // 根据不同的 QBF solver 设置不同的命令和输出模式
  //       solver_cmd = qfun_path + " " + input;
  //       output_pattern = "\\nv\\s*(.*)\\n*";

  //     auto start = std::chrono::high_resolution_clock::now();

  //     // 创建临时文件
  //     char temp_filename[] = "/tmp/tempfile_ZZZZZZ";

  //     //char temp_filename[] = "/tmp/tempfile_XXXXXX";
  //     // int temp_fd = mkstemp( temp_filename );

  //     // if ( temp_fd == -1 )
  //     // {
  //     //   std::cerr << "Failed to create temporary file." << std::endl;
  //     //   return std::make_tuple( false, std::unordered_map<int, bool>(), 0.0, false );
  //     // }

  //     // close( temp_fd );
  //     // close( temp_fd );
  //     // close( temp_fd );

  //     // 将命令的输出重定向到临时文件
  //     pid_t pid = fork();
  //     if ( pid == -1 )
  //     {
  //       std::cerr << "Failed to fork process." << std::endl;
  //       return std::make_tuple( false, std::unordered_map<int, bool>(), 0.0, false );
  //     }
  //     else if ( pid == 0 )
  //     {
  //       // 子进程
  //       // 将命令的输出重定向到临时文件
  //       FILE *fp = freopen( temp_filename, "w", stdout );
  //       if ( fp == NULL )
  //       {
  //         std::cerr << "Failed to redirect stdout to temporary file1." << std::endl;
  //         _exit( 127 );
  //       }
  //       fp = freopen( temp_filename, "a", stderr );
  //       if ( fp == NULL )
  //       {
  //         std::cerr << "Failed to redirect stdout to temporary file2." << std::endl;
  //         _exit( 127 );
  //       }

  //       execl( "/bin/sh", "sh", "-c", solver_cmd.c_str(), (char*)NULL );
  //       _exit( 127 ); // execl only returns on error
  //     }
  //     else
  //     {
  //       // 父进程
  //       int return_code;
  //       auto future = std::async( std::launch::async, [&]()
  //                                 {
  //             waitpid(pid, &return_code, 0);
  //             return return_code; } );

  //       if ( future.wait_for( std::chrono::seconds( timeout ) ) == std::future_status::timeout )
  //       {
  //         // 超时，终止子进程
  //         kill( pid, SIGKILL );
  //         waitpid( pid, &return_code, 0 ); // Wait for the process to actually terminate
  //         std::cerr << "Solver timed out." << std::endl;
  // 		//remove(temp_filename); // 确保删除临时文件
  //         return std::make_tuple( false, std::unordered_map<int, bool>(), 0.0, true );
  //       }

  //       return_code = future.get();

  //       // 读取临时文件的内容
  //       std::ifstream temp_file( temp_filename );
  // 	  //remove(temp_filename);
  //       if ( !temp_file.is_open() )
  //       {
  //         std::cerr << "Failed to open temporary file." << std::endl;
  //         return std::make_tuple(false, std::unordered_map<int, bool>(), 0.0, false);
  //       }

  //     std::stringstream buffer;
  //     buffer << temp_file.rdbuf();
  //     std::string result = buffer.str();

  //     // 删除临时文件
  //     temp_file.close();
  //     //remove(temp_filename);

  //     auto end = std::chrono::high_resolution_clock::now();
  //     std::chrono::duration<double> elapsed = end - start;
  //     double solving_time = elapsed.count();

  //     // 检查返回代码的高8位是否为0
  //     if (WIFEXITED(return_code)) {
  //         return_code = WEXITSTATUS(return_code);
  //     } else {
  //         std::cerr << "Solver terminated abnormally." << std::endl;
  //         return std::make_tuple(false, std::unordered_map<int, bool>(), 0.0, false);
  //     }

  //     if (return_code == 10) {
  //         auto assignment = getAssignment(result, output_pattern);
  //         return std::make_tuple(true, assignment, solving_time, true);
  //     } else if (return_code == 20) {
  //         return std::make_tuple(false, std::unordered_map<int, bool>(), solving_time, true);
  //     } else {
  //         std::cerr << "Solver message:" << std::endl;
  //         std::cerr << result << std::endl;
  //         return std::make_tuple(false, std::unordered_map<int, bool>(), 0.0, false);
  //       }
  //   }
  // }

  std::tuple<bool, std::unordered_map<int, bool>, double, bool> runSolverAndGetAssignment( const std::string& input, int timeout )
  {
    std::string solver_cmd;
    std::string output_pattern;
    std::string qfun_path = "qfun";

    solver_cmd = qfun_path + " " + input;
    output_pattern = "\\nv\\s*(.*)\\n*";

    // 创建管道
    int pipefd[2];
    if ( pipe( pipefd ) == -1 )
    {
      std::cerr << "Failed to create pipe. Errno: " << errno << " (" << strerror( errno ) << ")" << std::endl;
      return std::make_tuple( false, std::unordered_map<int, bool>(), 0.0, false );
    }

    auto start = std::chrono::high_resolution_clock::now();

    pid_t pid = fork();
    if ( pid == -1 )
    {
      std::cerr << "Failed to fork process." << std::endl;
      close( pipefd[0] ); // 关闭管道读取端
      close( pipefd[1] ); // 关闭管道写入端
      return std::make_tuple( false, std::unordered_map<int, bool>(), 0.0, false );
    }
    else if ( pid == 0 )
    {
      // 子进程：将标准输出和标准错误重定向到管道的写入端
      close( pipefd[0] );               // 关闭读取端，子进程只需要写入
      dup2( pipefd[1], STDOUT_FILENO ); // 将标准输出重定向到管道写入端
      dup2( pipefd[1], STDERR_FILENO ); // 将标准错误重定向到管道写入端
      close( pipefd[1] );               // 关闭写入端（已重定向）

      execl( "/bin/sh", "sh", "-c", solver_cmd.c_str(), (char*)NULL );
      _exit( 127 ); // 子进程出错退出
    }
    else
    {
      // 父进程：关闭写入端，父进程只需要读取
      close( pipefd[1] );

      // 等待子进程的结果
      int return_code;
      auto future = std::async( std::launch::async, [&]()
                                {
            waitpid(pid, &return_code, 0);  // 回收子进程资源
            return return_code; } );

      // 如果超时则杀死子进程
      if ( future.wait_for( std::chrono::seconds( timeout ) ) == std::future_status::timeout )
      {
        kill( pid, SIGKILL );
        waitpid( pid, &return_code, 0 ); // 确保杀死进程后及时回收
        close( pipefd[0] );              // 关闭读取端
        std::cerr << "Solver timed out." << std::endl;
        return std::make_tuple( false, std::unordered_map<int, bool>(), 0.0, true );
      }

      // 从管道中读取输出
      std::stringstream buffer;
      char buf[4096];
      ssize_t n;
      while ( ( n = read( pipefd[0], buf, sizeof( buf ) ) ) > 0 )
      {
        buffer.write( buf, n );
      }

      close( pipefd[0] ); // 读取完成后关闭读取端

      // 获取子进程的返回状态
      return_code = future.get();
      auto end = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> elapsed = end - start;
      double solving_time = elapsed.count();

      if ( WIFEXITED( return_code ) )
      {
        return_code = WEXITSTATUS( return_code );
      }
      else
      {
        std::cerr << "Solver terminated abnormally." << std::endl;
        return std::make_tuple( false, std::unordered_map<int, bool>(), 0.0, false );
      }

      std::string result = buffer.str();

      if ( return_code == 10 )
      {
        auto assignment = getAssignment( result, output_pattern );
        return std::make_tuple( true, assignment, solving_time, true );
      }
      else if ( return_code == 20 )
      {
        return std::make_tuple( false, std::unordered_map<int, bool>(), solving_time, true );
      }
      else
      {
        std::cerr << "Solver message:" << std::endl;
        std::cerr << result << std::endl;
        return std::make_tuple( false, std::unordered_map<int, bool>(), 0.0, false );
      }
    }
  }
  void cleanup_zombie_processes()
  {
    // 通过循环调用 waitpid 来回收所有可能未被回收的子进程资源
    while ( waitpid( -1, nullptr, WNOHANG ) > 0 )
      ;
  }

  /*使用正则表达式匹配求解器输出中的赋值部分
              将赋值字符串解析为字典
              返回解析后的赋值结果*/
  std::unordered_map<int, bool> getAssignment( const std::string& output, const std::string& output_pattern )
  {
    std::unordered_map<int, bool> assignment;
    std::smatch match;
    std::regex re( output_pattern );

    // 尝试在output中查找符合output_pattern的子串
    if ( std::regex_search( output, match, re ) )
    {
      assert( !match.empty() ); // 确保找到了匹配项，类似于Python中的断言

      // 抽取匹配到的字符串部分
      std::string assignment_str = match[1].str();
      std::istringstream iss( assignment_str );
      std::string lit_str;

      // 分割字符串并转换为整数
      while ( iss >> lit_str )
      {
        int lit = std::stoi( lit_str );
        if ( lit == 0 )
        {
          continue;
        }
        assignment[std::abs( lit )] = lit > 0;
      }
    }
    else
    {
      // 如果没有找到匹配项，请根据您的需要处理这种情况
      assert( false ); // 或其他错误处理方式
    }

    return assignment;
  }

#pragma endregion
};

#endif