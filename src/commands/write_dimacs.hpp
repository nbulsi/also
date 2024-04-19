/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019-2021 Ningbo University, Ningbo, China */

/**
 * @file write_dimacs.hpp
 *
 * @brief write logic network into CNF DIMACS format
 *
 * @author Zhufei
 * @since  0.1
 */

#ifndef WRITE_DIMACS_HPP
#define WRITE_DIMACS_HPP

#include <iostream>
#include <sstream>

#include <mockturtle/io/write_dimacs.hpp>
#include <fmt/format.h>

#include "../core/utils.hpp"

namespace alice
{

  class write_dimacs_command : public command
  {
    public:
      explicit write_dimacs_command( const environment::ptr& env ) : command( env, "write logic network into dimacs" )
      {
        add_flag( "--xag, -g",   "using xag as source logic network" );
        add_flag( "--aig, -a",   "using aig as source logic network" );
        add_flag( "--xmg, -x",   "using xmg as source logic network" );
        add_flag( "--mig, -m",   "using mig as source logic network" );
        add_flag( "--klut, -l",  "using klut as source logic network" );
        add_flag( "--xor-cnf, -c", "write into XOR-CNF format, only if --xmg is enabled" );
        add_option( "--filename, -f", filename, "specify the filename, default = /tmp/tmp.cnf" );
      }

    private:
      void write_dimacs_xor_cnf( xmg_network const& xmg, std::ostream& out = std::cout )
      {
        std::stringstream clauses;
        uint32_t num_clauses = 0u;

        xmg.foreach_gate( [&]( auto n)
            {
              if( xmg.is_xor3( n ) )
              {
                clauses << fmt::format( "x " );

                xmg.foreach_fanin( n, [&]( auto const& f )
                    {
                      auto child = xmg.get_node( f );
                      if( child != 0 ) //child1 = 0 for xor2
                      {
                        clauses << fmt::format( "{}{} ", xmg.is_complemented( f ) ? "-" : "", child );
                      }
                    }
                   );

                clauses << fmt::format( "-{} 0\n", n );
                ++num_clauses;
              }
              else if( xmg.is_maj( n ) )
              {
                auto children = also::get_children( xmg, n );

                auto c1 = xmg.get_node( children[0] );
                auto c2 = xmg.get_node( children[1] );
                auto c3 = xmg.get_node( children[2] );

                auto p1 = xmg.is_complemented( children[0] );
                auto p2 = xmg.is_complemented( children[1] );
                auto p3 = xmg.is_complemented( children[2] );

                if( c1 == 0 ) // and/or
                {
                  if( p1 ) // c2 | c3
                  {
                    clauses << fmt::format( "{}{} {} 0\n", p2 ? "" : "-", c2, n );
                    clauses << fmt::format( "{}{} {} 0\n", p3 ? "" : "-", c3, n );
                    clauses << fmt::format( "{}{} {}{} -{} 0\n", p2 ? "-" : "", c2, 
                                                                p3 ? "-" : "", c3, n );
                    num_clauses += 3u;
                  }
                  else //and c2 & c3
                  {
                    clauses << fmt::format( "{}{} -{} 0\n", p2 ? "-" : "", c2, n );
                    clauses << fmt::format( "{}{} -{} 0\n", p3 ? "-" : "", c3, n );
                    clauses << fmt::format( "{}{} {}{} {} 0\n", p2 ? "" : "-", c2, 
                                                                p3 ? "" : "-", c3, n );
                    num_clauses += 3u;
                  }
                }
                else // maj( c1, c2, c3 )
                {
                    clauses << fmt::format( "{}{} {}{} {} 0\n", p1 ? "" : "-", c1, 
                                                                p2 ? "" : "-", c2, n );
                    clauses << fmt::format( "{}{} {}{} {} 0\n", p1 ? "" : "-", c1, 
                                                                p3 ? "" : "-", c3, n );
                    clauses << fmt::format( "{}{} {}{} {} 0\n", p2 ? "" : "-", c2, 
                                                                p3 ? "" : "-", c3, n );
                    
                    clauses << fmt::format( "{}{} {}{} -{} 0\n", p1 ? "-" : "", c1, 
                                                                 p2 ? "-" : "", c2, n );
                    clauses << fmt::format( "{}{} {}{} -{} 0\n", p1 ? "-" : "", c1, 
                                                                 p3 ? "-" : "", c3, n );
                    clauses << fmt::format( "{}{} {}{} -{} 0\n", p2 ? "-" : "", c2, 
                                                                 p3 ? "-" : "", c3, n );
                    
                    num_clauses += 6u;
                }
              }
              else
              {
                assert( false && "unknown gates in XMG" );
              }

            }
            );

            xmg.foreach_po( [&]( auto const& f ) 
                {
                  auto po = xmg.get_node( f );

                  clauses << fmt::format( "{}{} 0\n", xmg.is_complemented( f ) ? "-" : "", po );
                  ++num_clauses;
                }
                );

            out << fmt::format( "p cnf {} {}\n{}", xmg.size() - 1u, num_clauses, clauses.str() );
      }

      void write_dimacs_xor_cnf( xmg_network const& xmg, std::string const& fname )
      {
        std::ofstream os( fname.c_str(), std::ofstream::out );
        write_dimacs_xor_cnf( xmg, os );
        os.close();
      }

      void write_dimacs_xag_xor_cnf( xag_network const& xag, std::ostream& out = std::cout )
      {
        std::stringstream clauses;
        uint32_t num_clauses = 0u;

        xag.foreach_gate( [&]( auto n)
            {
              if( xag.is_xor( n ) )
              {
                clauses << fmt::format( "x " );

                xag.foreach_fanin( n, [&]( auto const& f )
                    {
                      auto child = xag.get_node( f );
                      clauses << fmt::format( "{}{} ", xag.is_complemented( f ) ? "-" : "", child );
                    }
                   );

                clauses << fmt::format( "-{} 0\n", n );
                ++num_clauses;
              }
              else if( xag.is_and( n ) )
              {
                auto children = also::get_xag_children( xag, n );

                auto c1 = xag.get_node( children[0] );
                auto c2 = xag.get_node( children[1] );

                auto p1 = xag.is_complemented( children[0] );
                auto p2 = xag.is_complemented( children[1] );
                
                clauses << fmt::format( "{}{} -{} 0\n", p1 ? "-" : "", c1, n );
                clauses << fmt::format( "{}{} -{} 0\n", p2 ? "-" : "", c2, n );
                clauses << fmt::format( "{}{} {}{} {} 0\n", p1 ? "" : "-", c1, 
                                                            p2 ? "" : "-", c2, n );
                num_clauses += 3u;
              }
              else
              {
                assert( false && "unknown gates in XAG" );
              }

            }
            );

            xag.foreach_po( [&]( auto const& f ) 
                {
                  auto po = xag.get_node( f );

                  clauses << fmt::format( "{}{} 0\n", xag.is_complemented( f ) ? "-" : "", po );
                  ++num_clauses;
                }
                );

            out << fmt::format( "p cnf {} {}\n{}", xag.size() - 1u, num_clauses, clauses.str() );
      }
      
      void write_dimacs_xag_xor_cnf( xag_network const& xag, std::string const& fname )
      {
        std::ofstream os( fname.c_str(), std::ofstream::out );
        write_dimacs_xag_xor_cnf( xag, os );
        os.close();
      }

    protected:
      void execute()
      {
        /* parameters */
        if( is_set( "xmg" ) )
        {
          if( is_set( "xor-cnf" ) )
          {
            xmg_network xmg = store<xmg_network>().current();
            write_dimacs_xor_cnf( xmg, filename );
          }
          else
          {
            xmg_network xmg = store<xmg_network>().current();
            write_dimacs( xmg, filename );
          }
        }
        else if( is_set( "xag" ) )
        {
          if( is_set( "xor-cnf" ) )
          {
            xag_network xag = store<xag_network>().current();
            write_dimacs_xag_xor_cnf( xag, filename );
          }
          else
          {
            xag_network xag = store<xag_network>().current();
            write_dimacs( xag, filename );
          }
        }
        else if( is_set( "mig" ) )
        {
          mig_network mig = store<mig_network>().current();
          write_dimacs( mig, filename );
        }
        else if( is_set( "aig" ) )
        {
          aig_network aig = store<aig_network>().current();
          write_dimacs( aig, filename );
        }
        else if( is_set( "klut" ) )
        {
          klut_network klut = store<klut_network>().current();
          write_dimacs( klut, filename );
        }
        else
        {
          std::cout << "At least one store should be spcified, see write_dimacs -h for more info.\n"; 
        }
      }

    private:
      std::string filename = "/tmp/tmp.cnf";
  };


  ALICE_ADD_COMMAND( write_dimacs, "I/O" )
}

#endif
