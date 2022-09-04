/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

#pragma once
#ifndef FORMAT_CONVERT_H
#define FORMAT_CONVERT_H

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

namespace also
{

  void write_module( const vector<string>& inputs, const vector<string>& outputs, const string& module_name, std::ostream& out )
  {
    int line_feed = 0;
    out << module_name;
    for ( auto m_inputs : inputs )
    {
      if ( m_inputs[0] != '[' )
      {
        out << m_inputs << ",";
        continue;
      }
      vector<string> idx( 2 );
      string name;
      int i = 0;
      for ( auto m_char : m_inputs )
      {
        if ( m_char >= '0' && m_char <= '9' )
        {
          idx[i] += m_char;
        }
        else if ( m_char == ':' )
        {
          i++;
        }
        else if ( m_char == '[' || m_char == ']' || m_char == ',' )
          continue;
        else
        {
          name += m_char;
        }
      }
      for ( int i = stoi( idx[1] ); i < stoi( idx[0] ) + 1; ++i )
      {
        out << name << i << ",";
        ++line_feed;
        if ( line_feed % 30 == 0 )
        {
          line_feed = 0;
          out << endl;
        }
      }
    }
    for ( auto m_outputs : outputs )
    {
      if ( m_outputs[0] != '[' )
      {
        out << m_outputs << ",";
        continue;
      }
      vector<string> idx( 2 );
      string name;
      int i = 0;
      for ( auto m_char : m_outputs )
      {
        if ( m_char >= '0' && m_char <= '9' )
        {
          idx[i] += m_char;
        }
        else if ( m_char == ':' )
        {
          i++;
        }
        else if ( m_char == '[' || m_char == ']' || m_char == ',' )
          continue;
        else
        {
          name += m_char;
        }
      }
      for ( int i = stoi( idx[1] ); i < stoi( idx[0] ) + 1; ++i )
      {
        if ( line_feed % 30 == 0 )
        {
          line_feed = 0;
          out << endl;
        }
        out << name << i;
        line_feed++;
        if ( i < stoi( idx[0] ) )
          out << ",";
      }
    }
    out << ");" << endl
      << endl;
  }
  bool change( std::istream& in, std::ostream& out )
  {

    string module_name;
    vector<string> inputs;
    vector<string> outputs;
    vector<string> wires;
    vector<string> assign;
    vector<string> result;
    for ( string line; getline( in, line, '\n' ); )
    {
      if ( line == "" )
        continue;
      int idx;
      idx = line.find( "endmodule" );
      if ( idx != std::string::npos )
      {
        break;
      }
      idx = line.find( "module" );
      if ( idx != std::string::npos )
      {
        module_name = line;
        continue;
      }
      idx = line.find( "input" );
      if ( idx != std::string::npos )
      {
        line.erase( line.begin() + idx, line.begin() + idx + 5 );
        for ( unsigned int i = 0; i < line.size(); )
        {
          if ( line[i] == ' ' || line[i] == ',' )
          {
            line.erase( line.begin() + i );
          }
          else
            ++i;
        }
        inputs.push_back( line );
      }
      idx = line.find( "output" );
      if ( idx != std::string::npos )
      {
        line.erase( line.begin() + idx, line.begin() + idx + 6 );
        for ( unsigned int i = 0; i < line.size(); )
        {
          if ( line[i] == ' ' || line[i] == ',' )
          {
            line.erase( line.begin() + i );
          }
          else
            ++i;
        }
        outputs.push_back( line );
      }

      idx = line.find( "wire" );
      if ( idx != std::string::npos )
      {
        wires.push_back( line );
      }

      idx = line.find( "assign" );
      if ( idx != std::string::npos )
      {
        line.erase( line.begin(), line.begin() + idx );
        for ( unsigned int i = 0; i < line.size(); )
        {
          if ( line[i] == '[' || line[i] == ']' )
          {
            line.erase( line.begin() + i );
          }
          else
            ++i;
        }
        assign.push_back( line );
      }
    }
    // write verilog
    // write module
    write_module( inputs, outputs, module_name, out );
    // write inputs
    for ( auto m_inputs : inputs )
    {
      if ( m_inputs[0] != '[' )
      {
        out << "input ";
        out << m_inputs << ";" << endl;
        continue;
      }
      vector<string> idx( 2 );
      string name;
      int i = 0;
      for ( auto m_char : m_inputs )
      {
        if ( m_char >= '0' && m_char <= '9' )
        {
          idx[i] += m_char;
        }
        else if ( m_char == ':' )
        {
          i++;
        }
        else if ( m_char == '[' || m_char == ']' || m_char == ',' )
          continue;
        else
        {
          name += m_char;
        }
      }
      int j;
      int line_feed = 0;
      for ( j = stoi( idx[1] ); j < stoi( idx[0] ); ++j )
      {
        if ( line_feed == 0 )
        {
          out << "input ";
          out << name << j << ",";
          line_feed++;
        }
        else if ( line_feed % 29 == 0 && line_feed > 0 )
        {
          line_feed = 0;
          out << name << j << ";" << endl;
        }
        else
        {
          out << name << j << ",";
          line_feed++;
        }
      }
      if ( line_feed )
        out << name << j << ";" << endl;
    }
    // write outputs
    for ( auto m_outputs : outputs )
    {
      if ( m_outputs[0] != '[' )
      {
        out << "output ";
        out << m_outputs << ";";
        continue;
      }
      vector<string> idx( 2 );
      string name;
      int i = 0;
      for ( auto m_char : m_outputs )
      {
        if ( m_char >= '0' && m_char <= '9' )
        {
          idx[i] += m_char;
        }
        else if ( m_char == ':' )
        {
          i++;
        }
        else if ( m_char == '[' || m_char == ']' || m_char == ',' )
          continue;
        else
        {
          name += m_char;
        }
      }
      int j;
      int line_feed = 0;
      for ( j = stoi( idx[1] ); j < stoi( idx[0] ); ++j )
      {
        if ( line_feed == 0 )
        {
          out << "output ";
          out << name << j << ",";
          line_feed++;
        }
        else if ( line_feed % 29 == 0 && line_feed > 0 )
        {
          line_feed = 0;
          out << name << j << ";" << endl;
        }
        else
        {
          out << name << j << ",";
          line_feed++;
        }
      }
      if ( line_feed )
        out << name << j << ";" << endl;
    }
    // write wire
    for ( auto m_wire : wires )
    {
      out << m_wire << endl;
    }
    out << endl;
    // write assign
    for ( auto m_assign : assign )
    {
      out << m_assign << endl;
    }
    out << endl;
    out << "endmodule" << endl;
    return true;
  }

  void mag_convert( const string& infile, const string& outfile )
  {
    std::ifstream in( infile );
    std::ofstream out( outfile );
    change( in, out );
  }

} // namespace also

#endif
