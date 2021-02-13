#pragma once

#include <mockturtle/mockturtle.hpp>
#include <percy/percy.hpp>

using namespace percy;
using namespace mockturtle;

namespace also
{
  template<typename Out>
    void split_by_delim(const std::string &s, char delim, Out result)
    {
      std::stringstream ss(s);
      std::string item;
      while (std::getline(ss, item, delim)) {
        *(result++) = item;
      }
    }

  std::vector<std::string> split_by_delim(const std::string &s, char delim);

  template <typename Iterator>
  inline bool next_combination(const Iterator first, Iterator k, const Iterator last);

  std::vector<std::vector<unsigned>> get_all_combination_index( std::vector<unsigned>& vars,
                                                                  const unsigned& n,
                                                                  const unsigned& k );

  std::vector<std::vector<unsigned>> get_all_permutation( const std::vector<unsigned>& vars );

  template <typename T>
  void show_array( const std::vector<T>& array )
  {
    std::cout << "Elements: ";
    for( const auto& x : array )
    {
      std::cout << " " << x;
    }
    std::cout << std::endl;
  }

  void print_sat_clause(solver_wrapper* solver, pabc::lit* begin, pabc::lit* end);
  int add_print_clause(std::vector<std::vector<int>>& clauses, pabc::lit* begin, pabc::lit* end);
  std::vector<std::string> split(const std::string& s, char delimiter);

  std::vector<std::string> split( const std::string& str, const std::string& sep );
  void to_dimacs(FILE* f, solver_wrapper* solver, std::vector<std::vector<int>>& clauses );

    template<class Ntk>
    void print_stats( const Ntk& ntk )
    {
      auto copy = ntk;
      depth_view depth_ntk{ copy };
      std::cout << fmt::format( "ntk   i/o = {}/{}   gates = {}   level = {}\n",
          ntk.num_pis(), ntk.num_pos(), ntk.num_gates(), depth_ntk.depth() );
    }
}

