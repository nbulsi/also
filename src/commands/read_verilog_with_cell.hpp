/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file read_verilog_with_cell.hpp
 *
 * @brief Read mapped verilog (ASIC netlist) into klut network
 * 
 * @author Chengyu Ma
 */

#ifndef READ_VERILOG_WITH_CELL_HPP
#define READ_VERILOG_WITH_CELL_HPP

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <lorina/verilog.hpp>
#include <mockturtle/io/genlib_reader.hpp>
#include <mockturtle/mockturtle.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/utils/standard_cell.hpp>

namespace alice
{

class read_verilog_with_cell_command : public command
{
public:
  explicit read_verilog_with_cell_command( const environment::ptr& env )
      : command( env, "Read mapped verilog (cell instances) into block network" )
  {
    add_option( "--filename,-f", filename, "input mapped verilog file" )->required();
    add_flag( "--verbose,-v", "verbose diagnostics" );
  }

  rules validity_rules() const
  {
    return { has_store_element<std::vector<mockturtle::gate>>( env ) };
  }

protected:
  void execute()
  {
    if ( store<std::vector<mockturtle::gate>>().empty() )
    {
      env->err() << "[e] no genlib in the store\n";
      return;
    }

    auto const& gates = store<std::vector<mockturtle::gate>>().current();
    auto const cells = mockturtle::get_standard_cells( gates );

    auto const to_lower = []( std::string s ) {
      std::transform( s.begin(), s.end(), s.begin(), []( unsigned char c ) {
        return static_cast<char>( std::tolower( c ) );
      } );
      return s;
    };

    std::unordered_map<std::string, mockturtle::standard_cell const*> cell_by_name;
    for ( auto const& c : cells )
    {
      cell_by_name[c.name] = &c;
      cell_by_name[to_lower( c.name )] = &c;
    }

    class mapped_cell_verilog_reader : public lorina::verilog_reader
    {
    public:
      mapped_cell_verilog_reader( mockturtle::klut_network& ntk,
                                  std::unordered_map<std::string, mockturtle::standard_cell const*> const& cell_map,
                                  bool verbose )
          : ntk( ntk ), cell_map( cell_map ), verbose( verbose )
      {
        signals["0"] = ntk.get_constant( false );
        signals["1"] = ntk.get_constant( true );
        signals["1'b0"] = ntk.get_constant( false );
        signals["1'b1"] = ntk.get_constant( true );
      }

      void on_inputs( const std::vector<std::string>& names, std::string const& size = "" ) const override
      {
        (void)size;
        for ( auto const& name : names )
        {
          signals[name] = ntk.create_pi();
        }
      }

      void on_outputs( const std::vector<std::string>& names, std::string const& size = "" ) const override
      {
        (void)size;
        outputs.insert( outputs.end(), names.begin(), names.end() );
      }

      void on_assign( const std::string& lhs, const std::pair<std::string, bool>& rhs ) const override
      {
        auto const it = signals.find( rhs.first );
        if ( it == signals.end() )
        {
          std::cerr << fmt::format( "[w] undefined signal {} in assign\n", rhs.first );
          return;
        }
        signals[lhs] = rhs.second ? ntk.create_not( it->second ) : it->second;
      }

      void on_module_instantiation( std::string const& module_name,
                                    std::vector<std::string> const& params,
                                    std::string const& inst_name,
                                    std::vector<std::pair<std::string, std::string>> const& args ) const override
      {
        (void)params;
        (void)inst_name;

        auto c_it = cell_map.find( module_name );
        if ( c_it == cell_map.end() )
        {
          c_it = cell_map.find( to_lower_copy( module_name ) );
        }
        if ( c_it == cell_map.end() )
        {
          if ( verbose )
          {
            std::cerr << fmt::format( "[w] skip unknown cell {}\n", module_name );
          }
          return;
        }

        auto const* cell = c_it->second;
        std::unordered_map<std::string, std::string> pin_to_sig;
        for ( auto const& [pin, sig] : args )
        {
          pin_to_sig[pin] = sig;
          pin_to_sig[to_lower_copy( pin )] = sig;
        }

        for ( uint32_t i = 0u; i < cell->gates.size(); ++i )
        {
          auto const& gate = cell->gates[i];
          std::vector<mockturtle::signal<mockturtle::klut_network>> children;
          children.reserve( gate.pins.size() );

          for ( auto const& pin : gate.pins )
          {
            auto p_it = pin_to_sig.find( pin.name );
            if ( p_it == pin_to_sig.end() )
            {
              p_it = pin_to_sig.find( to_lower_copy( pin.name ) );
            }
            if ( p_it == pin_to_sig.end() )
            {
              if ( verbose )
              {
                std::cerr << fmt::format( "[w] missing input pin {} for cell {}\n", pin.name, module_name );
              }
              return;
            }
            auto const s_it = signals.find( p_it->second );
            if ( s_it == signals.end() )
            {
              if ( verbose )
              {
                std::cerr << fmt::format( "[w] undefined driver {} for pin {}\n", p_it->second, pin.name );
              }
              return;
            }
            children.push_back( s_it->second );
          }

          auto const& out_pin_name = cell->gates[i].output_name;
          auto p_it = pin_to_sig.find( out_pin_name );
          if ( p_it == pin_to_sig.end() )
          {
            p_it = pin_to_sig.find( to_lower_copy( out_pin_name ) );
          }
          if ( p_it == pin_to_sig.end() )
          {
            if ( verbose )
            {
              std::cerr << fmt::format( "[w] missing output pin {} for cell {}\n", out_pin_name, module_name );
            }
            continue;
          }

          signals[p_it->second] = ntk.create_node( children, gate.function );
        }
      }

      void finalize_pos() const
      {
        for ( auto const& o : outputs )
        {
          auto const it = signals.find( o );
          if ( it == signals.end() )
          {
            if ( verbose )
            {
              std::cerr << fmt::format( "[w] output {} has no driver, connect 0\n", o );
            }
            ntk.create_po( ntk.get_constant( false ) );
          }
          else
          {
            ntk.create_po( it->second );
          }
        }
      }

    private:
      static std::string to_lower_copy( std::string s )
      {
        std::transform( s.begin(), s.end(), s.begin(), []( unsigned char c ) {
          return static_cast<char>( std::tolower( c ) );
        } );
        return s;
      }

      mockturtle::klut_network& ntk;
      std::unordered_map<std::string, mockturtle::standard_cell const*> const& cell_map;
      bool verbose{ false };

      mutable std::map<std::string, mockturtle::signal<mockturtle::klut_network>> signals;
      mutable std::vector<std::string> outputs;
    };

    auto const sanitize_for_lorina = [&]( std::string const& input_file ) -> std::optional<std::string>
    {
      std::ifstream fin( input_file );
      if ( !fin.is_open() )
      {
        return std::nullopt;
      }

      std::string text( ( std::istreambuf_iterator<char>( fin ) ), std::istreambuf_iterator<char>() );
      std::unordered_map<std::string, std::string> escaped_id_map;
      std::string sanitized_id_text;
      sanitized_id_text.reserve( text.size() );

      uint32_t next_id = 0u;
      bool changed = false;

      for ( size_t i = 0u; i < text.size(); )
      {
        if ( text[i] == '\\' )
        {
          size_t j = i + 1u;
          while ( j < text.size() && !std::isspace( static_cast<unsigned char>( text[j] ) ) )
          {
            ++j;
          }

          auto const escaped = text.substr( i, j - i );
          auto it = escaped_id_map.find( escaped );
          if ( it == escaped_id_map.end() )
          {
            auto const safe_name = fmt::format( "esc_{}", next_id++ );
            it = escaped_id_map.emplace( escaped, safe_name ).first;
          }

          sanitized_id_text.append( it->second );
          i = j;
          changed = true;
          continue;
        }

        sanitized_id_text.push_back( text[i] );
        ++i;
      }

      auto const trim = []( std::string s ) {
        auto not_space = []( unsigned char ch ) { return !std::isspace( ch ); };
        s.erase( s.begin(), std::find_if( s.begin(), s.end(), not_space ) );
        s.erase( std::find_if( s.rbegin(), s.rend(), not_space ).base(), s.end() );
        return s;
      };

      auto const split_csv = [&]( std::string const& csv ) {
        std::vector<std::string> names;
        std::stringstream ss( csv );
        std::string tok;
        while ( std::getline( ss, tok, ',' ) )
        {
          tok = trim( tok );
          if ( !tok.empty() )
          {
            names.push_back( tok );
          }
        }
        return names;
      };

      auto const emit_decl = [&]( std::string const& keyword, std::vector<std::string> const& names, std::string& out ) {
        constexpr size_t chunk = 32u;
        for ( size_t i = 0u; i < names.size(); i += chunk )
        {
          auto const end = std::min( i + chunk, names.size() );
          out.append( "  " + keyword + " " );
          for ( size_t j = i; j < end; ++j )
          {
            if ( j != i )
            {
              out.append( " , " );
            }
            out.append( names[j] );
          }
          out.append( " ;\n" );
        }
      };

      std::stringstream in_lines( sanitized_id_text );
      std::string normalized;
      std::string line;
      while ( std::getline( in_lines, line ) )
      {
        auto const t = trim( line );

        if ( t.rfind( "module ", 0 ) == 0u && t.find( '(' ) != std::string::npos && t.find( ");" ) != std::string::npos )
        {
          auto const module_name_end = t.find( '(' );
          auto const module_kw = t.substr( 0u, module_name_end );
          auto const l = t.find( '(' );
          auto const r = t.rfind( ")" );
          auto const ports_csv = t.substr( l + 1u, r - l - 1u );
          auto const ports = split_csv( ports_csv );

          normalized.append( module_kw );
          normalized.append( "( " );
          for ( size_t i = 0u; i < ports.size(); ++i )
          {
            if ( i != 0u )
            {
              normalized.append( " , " );
            }
            normalized.append( ports[i] );
          }
          normalized.append( " );\n" );
          if ( ports.size() > 32u )
          {
            changed = true;
          }
          continue;
        }

        auto handle_decl = [&]( char const* keyword ) {
          auto const kw = std::string( keyword );
          if ( t.rfind( kw + " ", 0 ) == 0u && t.back() == ';' )
          {
            auto const body = t.substr( kw.size() + 1u, t.size() - kw.size() - 2u );
            auto names = split_csv( body );
            if ( names.size() > 32u )
            {
              changed = true;
            }
            emit_decl( kw, names, normalized );
            return true;
          }
          return false;
        };

        if ( handle_decl( "input" ) || handle_decl( "output" ) || handle_decl( "wire" ) )
        {
          continue;
        }

        normalized.append( line );
        normalized.push_back( '\n' );
      }

      if ( !changed )
      {
        return std::nullopt;
      }

      auto const unique_suffix = std::chrono::steady_clock::now().time_since_epoch().count();
      auto const temp_path = std::filesystem::temp_directory_path() /
                             fmt::format( "also_read_with_cell_{}.v", unique_suffix );

      std::ofstream fout( temp_path );
      if ( !fout.is_open() )
      {
        return std::nullopt;
      }
      fout << normalized;

      return temp_path.string();
    };

    auto const parse_into_klut = [&]( std::string const& input_file,
                                      mockturtle::klut_network& out_klut ) -> lorina::return_code
    {
      mapped_cell_verilog_reader reader( out_klut, cell_by_name, is_set( "verbose" ) );
      auto const rc = lorina::read_verilog( input_file, reader );
      if ( rc == lorina::return_code::success )
      {
        reader.finalize_pos();
      }
      return rc;
    };

    auto const parse_into_klut_manual = [&]( std::string const& input_file,
                                             mockturtle::klut_network& out_klut ) -> bool
    {
      std::ifstream fin( input_file );
      if ( !fin.is_open() )
      {
        return false;
      }

      auto trim = []( std::string s ) {
        auto not_space = []( unsigned char ch ) { return !std::isspace( ch ); };
        s.erase( s.begin(), std::find_if( s.begin(), s.end(), not_space ) );
        s.erase( std::find_if( s.rbegin(), s.rend(), not_space ).base(), s.end() );
        return s;
      };

      auto split_csv = [&]( std::string const& csv ) {
        std::vector<std::string> toks;
        std::stringstream ss( csv );
        std::string tok;
        while ( std::getline( ss, tok, ',' ) )
        {
          tok = trim( tok );
          if ( !tok.empty() )
          {
            toks.push_back( tok );
          }
        }
        return toks;
      };

      std::map<std::string, mockturtle::signal<mockturtle::klut_network>> signals;
      std::vector<std::string> outputs;
      struct parsed_instance
      {
        mockturtle::standard_cell const* cell{ nullptr };
        std::unordered_map<std::string, std::string> pin_to_sig;
      };
      std::vector<parsed_instance> instances;
      uint64_t matched_instances{ 0u };
      uint64_t unknown_cell_instances{ 0u };
      uint64_t known_cell_instances{ 0u };
      uint64_t missing_pin_instances{ 0u };
      uint64_t created_nodes{ 0u };
      signals["0"] = out_klut.get_constant( false );
      signals["1"] = out_klut.get_constant( true );
      signals["1'b0"] = out_klut.get_constant( false );
      signals["1'b1"] = out_klut.get_constant( true );

      std::regex inst_re( R"(^\s*([^\s\(]+)\s+([^\s\(]+)\s*\((.*)\)\s*;\s*$)" );
      std::regex arg_re( R"(\.\s*([A-Za-z_][A-Za-z0-9_$]*)\s*\(\s*([^\)]+)\s*\))" );
      std::regex assign_re( R"(^\s*assign\s+([^\s=]+)\s*=\s*([^;]+)\s*;\s*$)" );

      auto const resolve_rhs_signal = [&]( std::string rhs ) -> std::optional<std::pair<std::string, bool>>
      {
        rhs = trim( rhs );
        bool invert = false;

        while ( !rhs.empty() && ( rhs.front() == '(' || rhs.front() == '+' ) )
        {
          rhs.erase( rhs.begin() );
          rhs = trim( rhs );
        }
        while ( !rhs.empty() && rhs.back() == ')' )
        {
          rhs.pop_back();
          rhs = trim( rhs );
        }

        if ( !rhs.empty() && ( rhs.front() == '~' || rhs.front() == '!' ) )
        {
          invert ^= true;
          rhs.erase( rhs.begin() );
          rhs = trim( rhs );
        }

        if ( rhs == "1'b0" || rhs == "0" || rhs == "'0" )
        {
          return std::make_pair( std::string( "1'b0" ), invert );
        }
        if ( rhs == "1'b1" || rhs == "1" || rhs == "'1" )
        {
          return std::make_pair( std::string( "1'b1" ), invert );
        }

        if ( rhs.empty() )
        {
          return std::nullopt;
        }

        return std::make_pair( rhs, invert );
      };

      std::string line;
      while ( std::getline( fin, line ) )
      {
        auto t = trim( line );
        if ( t.empty() || t.rfind( "//", 0 ) == 0u )
        {
          continue;
        }

        auto parse_decl = [&]( char const* kw, std::vector<std::string>& target ) {
          auto prefix = std::string( kw ) + " ";
          if ( t.rfind( prefix, 0 ) != 0u )
          {
            return false;
          }

          std::string body = t.substr( prefix.size() );
          while ( body.find( ';' ) == std::string::npos )
          {
            std::string next;
            if ( !std::getline( fin, next ) )
            {
              break;
            }
            body += " " + trim( next );
          }

          auto semi = body.find( ';' );
          if ( semi != std::string::npos )
          {
            body = body.substr( 0u, semi );
          }
          auto names = split_csv( body );
          target.insert( target.end(), names.begin(), names.end() );
          return true;
        };

        std::vector<std::string> inputs;
        if ( parse_decl( "input", inputs ) )
        {
          for ( auto const& n : inputs )
          {
            if ( signals.find( n ) == signals.end() )
            {
              signals[n] = out_klut.create_pi();
            }
          }
          continue;
        }
        if ( parse_decl( "output", outputs ) )
        {
          continue;
        }
        std::vector<std::string> wires;
        if ( parse_decl( "wire", wires ) )
        {
          continue;
        }

        std::smatch assign_match;
        if ( std::regex_match( t, assign_match, assign_re ) )
        {
          auto const lhs = trim( assign_match[1].str() );
          auto const rhs = assign_match[2].str();
          auto rhs_sig = resolve_rhs_signal( rhs );
          if ( !rhs_sig.has_value() )
          {
            if ( is_set( "verbose" ) )
            {
              env->out() << fmt::format( "[w] skip unsupported assign {}\n", t );
            }
            continue;
          }

          auto const rhs_it = signals.find( rhs_sig->first );
          if ( rhs_it == signals.end() )
          {
            if ( is_set( "verbose" ) )
            {
              env->out() << fmt::format( "[w] undefined signal {} in assign\n", rhs_sig->first );
            }
            continue;
          }

          signals[lhs] = rhs_sig->second ? out_klut.create_not( rhs_it->second ) : rhs_it->second;
          continue;
        }

        std::smatch m;
        if ( !std::regex_match( t, m, inst_re ) )
        {
          continue;
        }
        ++matched_instances;

        auto const module_name = m[1].str();
        auto const args_text = m[3].str();
        auto c_it = cell_by_name.find( module_name );
        if ( c_it == cell_by_name.end() )
        {
          c_it = cell_by_name.find( to_lower( module_name ) );
        }
        if ( c_it == cell_by_name.end() )
        {
          ++unknown_cell_instances;
          continue;
        }
        ++known_cell_instances;
        auto const* cell = c_it->second;

        std::unordered_map<std::string, std::string> pin_to_sig;
        for ( std::sregex_iterator it( args_text.begin(), args_text.end(), arg_re ), end; it != end; ++it )
        {
          auto const pin_name = (*it)[1].str();
          auto const signal_name = trim( (*it)[2].str() );
          pin_to_sig[pin_name] = signal_name;
          pin_to_sig[to_lower( pin_name )] = signal_name;
        }

        instances.push_back( { cell, std::move( pin_to_sig ) } );
      }

      std::vector<bool> resolved( instances.size(), false );
      uint64_t remaining = static_cast<uint64_t>( instances.size() );
      bool progress = true;
      uint32_t unresolved_debug_printed{ 0u };

      auto const lookup_pin = [&]( std::unordered_map<std::string, std::string> const& pin_to_sig,
                                   std::string const& pin_name ) -> std::optional<std::string>
      {
        auto it = pin_to_sig.find( pin_name );
        if ( it != pin_to_sig.end() )
        {
          return it->second;
        }

        auto const lower = to_lower( pin_name );
        it = pin_to_sig.find( lower );
        if ( it != pin_to_sig.end() )
        {
          return it->second;
        }

        return std::nullopt;
      };

      auto const lookup_output_pin = [&]( std::unordered_map<std::string, std::string> const& pin_to_sig,
                                          std::string const& pin_name ) -> std::optional<std::string>
      {
        if ( auto s = lookup_pin( pin_to_sig, pin_name ); s.has_value() )
        {
          return s;
        }

        std::vector<std::string> aliases;
        if ( pin_name == "X" || pin_name == "x" )
        {
          aliases = { "Y", "y", "ZN", "zn", "Q", "q" };
        }
        else if ( pin_name == "Y" || pin_name == "y" )
        {
          aliases = { "X", "x", "ZN", "zn", "Q", "q" };
        }
        else
        {
          aliases = { "Y", "y", "X", "x", "ZN", "zn", "Q", "q" };
        }

        for ( auto const& a : aliases )
        {
          auto it = pin_to_sig.find( a );
          if ( it != pin_to_sig.end() )
          {
            return it->second;
          }
        }

        return std::nullopt;
      };

      while ( progress && remaining > 0u )
      {
        progress = false;

        for ( size_t idx = 0u; idx < instances.size(); ++idx )
        {
          if ( resolved[idx] )
          {
            continue;
          }

          auto const& inst = instances[idx];
          auto const& pin_to_sig = inst.pin_to_sig;

          bool ready = true;
          std::string fail_reason;
          std::vector<std::pair<std::string, std::vector<mockturtle::signal<mockturtle::klut_network>>>> gate_data;
          gate_data.reserve( inst.cell->gates.size() );

          for ( auto const& gate : inst.cell->gates )
          {
            std::vector<mockturtle::signal<mockturtle::klut_network>> children;
            children.reserve( gate.pins.size() );

            for ( auto const& pin : gate.pins )
            {
              auto pin_sig = lookup_pin( pin_to_sig, pin.name );
              if ( !pin_sig.has_value() )
              {
                ready = false;
                fail_reason = fmt::format( "missing pin {}", pin.name );
                break;
              }
              auto const s_it = signals.find( *pin_sig );
              if ( s_it == signals.end() )
              {
                ready = false;
                fail_reason = fmt::format( "undriven net {}", *pin_sig );
                break;
              }
              children.push_back( s_it->second );
            }

            if ( !ready )
            {
              break;
            }

            auto out_sig = lookup_output_pin( pin_to_sig, gate.output_name );
            if ( !out_sig.has_value() )
            {
              ready = false;
              fail_reason = fmt::format( "missing output pin {}", gate.output_name );
              break;
            }

            gate_data.emplace_back( *out_sig, std::move( children ) );
          }

          if ( !ready )
          {
            if ( is_set( "verbose" ) && unresolved_debug_printed < 5u && !fail_reason.empty() )
            {
              env->out() << fmt::format( "[i] unresolved instance {}: {}\n", idx, fail_reason );
              ++unresolved_debug_printed;
            }
            continue;
          }

          for ( size_t k = 0u; k < inst.cell->gates.size(); ++k )
          {
            auto const& gate = inst.cell->gates[k];
            signals[gate_data[k].first] = out_klut.create_node( gate_data[k].second, gate.function );
            ++created_nodes;
          }

          resolved[idx] = true;
          --remaining;
          progress = true;
        }
      }

      missing_pin_instances = remaining;

      for ( auto const& o : outputs )
      {
        auto it = signals.find( o );
        out_klut.create_po( it == signals.end() ? out_klut.get_constant( false ) : it->second );
      }

      if ( is_set( "verbose" ) )
      {
        env->out() << fmt::format( "[i] manual parser stats: inst={} known_cell={} unknown_cell={} unresolved_inst={} created_nodes={} outputs={}\n",
                                   matched_instances, known_cell_instances, unknown_cell_instances,
                                   missing_pin_instances, created_nodes, outputs.size() );
      }

      return out_klut.num_pos() > 0u;
    };

    mockturtle::klut_network klut;
    auto ret = parse_into_klut( filename, klut );

    auto const should_retry_with_fallback = [&]( mockturtle::klut_network const& ntk ) {
      return ntk.num_gates() == 0u && ( ntk.num_pis() > 0u || ntk.num_pos() > 0u );
    };

    if ( ret == lorina::return_code::success && should_retry_with_fallback( klut ) )
    {
      if ( is_set( "verbose" ) )
      {
        env->out() << "[i] direct parse produced empty logic, retry with fallback parser\n";
      }

      mockturtle::klut_network fallback_klut;
      if ( parse_into_klut_manual( filename, fallback_klut ) && fallback_klut.num_gates() > 0u )
      {
        klut = std::move( fallback_klut );
      }
    }

    if ( ret != lorina::return_code::success )
    {
      if ( is_set( "verbose" ) )
      {
        env->out() << "[i] direct parse failed, retry with sanitized escaped identifiers\n";
      }

      auto const sanitized_file = sanitize_for_lorina( filename );
      if ( sanitized_file.has_value() )
      {
        mockturtle::klut_network retry_klut;
        ret = parse_into_klut( *sanitized_file, retry_klut );
        if ( ret == lorina::return_code::success )
        {
          klut = std::move( retry_klut );
        }
      }

      if ( ret != lorina::return_code::success )
      {
        if ( is_set( "verbose" ) )
        {
          env->out() << "[i] fallback to manual mapped-verilog parser\n";
        }
        mockturtle::klut_network fallback_klut;
        if ( parse_into_klut_manual( filename, fallback_klut ) )
        {
          klut = std::move( fallback_klut );
          ret = lorina::return_code::success;
        }
      }
    }

    if ( ret != lorina::return_code::success )
    {
      env->err() << "[e] parse error\n";
      return;
    }

    store<mockturtle::klut_network>().extend();
    store<mockturtle::klut_network>().current() = klut;

    if ( is_set( "verbose" ) )
    {
      mockturtle::depth_view depth{ klut };
      env->out() << fmt::format( "[i] klut i/o = {}/{} gates = {} level = {}\n",
                                 klut.num_pis(), klut.num_pos(), klut.num_gates(), depth.depth() );
    }
  }

private:
  std::string filename;
};

ALICE_ADD_COMMAND( read_verilog_with_cell, "I/O" )

} // namespace alice

#endif