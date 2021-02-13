#include <algorithm>
#include <percy/percy.hpp>
#include <string>
#include <sstream>

#include "misc.hpp"

using namespace percy;
using namespace mockturtle;

namespace also
{

  std::vector<std::string> split_by_delim(const std::string &s, char delim)
  {
    std::vector<std::string> elems;
    split_by_delim(s, delim, std::back_inserter(elems));
    return elems;
  }

    template <typename Iterator>
      inline bool next_combination(const Iterator first, Iterator k, const Iterator last)
      {
        /* Credits: Thomas Draper */
        if ((first == last) || (first == k) || (last == k))
          return false;
        Iterator itr1 = first;
        Iterator itr2 = last;
        ++itr1;
        if (last == itr1)
          return false;
        itr1 = last;
        --itr1;
        itr1 = k;
        --itr2;
        while (first != itr1)
        {
          if (*--itr1 < *itr2)
          {
            Iterator j = k;
            while (!(*itr1 < *j)) ++j;
            std::iter_swap(itr1,j);
            ++itr1;
            ++j;
            itr2 = k;
            std::rotate(itr1,j,last);
            while (last != j)
            {
              ++j;
              ++itr2;
            }
            std::rotate(k,itr2,last);
            return true;
          }
        }
        std::rotate(first,k,last);
        return false;
      }

    std::vector<std::vector<unsigned>> get_all_combination_index( std::vector<unsigned>& vars,
                                                                  const unsigned& n,
                                                                  const unsigned& k )
    {
      std::vector<std::vector<unsigned>> index;
      std::vector<unsigned> tmp;

      do
      {
        tmp.clear();
        for ( unsigned i = 0; i < k; ++i )
        {
          tmp.push_back( vars[i] );
        }
        index.push_back( tmp );
      }
      while( next_combination( vars.begin(), vars.begin() + k, vars.end() ) );

      return index;
    }

    std::vector<std::vector<unsigned>> get_all_permutation( const std::vector<unsigned>& vars )
    {
      std::vector<std::vector<unsigned>> v;
      auto vec_copy = vars;

      std::sort( vec_copy.begin(), vec_copy.end() );

      do
      {
        v.push_back( vec_copy );
      }while( std::next_permutation( vec_copy.begin(), vec_copy.end() ) );

      return v;
    }

    void print_sat_clause(solver_wrapper* solver, pabc::lit* begin, pabc::lit* end)
    {
      printf("Add clause:  " );
      pabc::lit* i;
      for ( i = begin; i < end; i++ )
          printf( "%s%d ", (*i)&1 ? "!":"", (*i)>>1 );
      printf( "\n" );
    }

    int add_print_clause(std::vector<std::vector<int>>& clauses, pabc::lit* begin, pabc::lit* end)
    {
      std::vector<int> clause;
      while (begin != end) {
        clause.push_back(*begin);
        begin++;
      }
      clauses.push_back(clause);

      return 1;
    }

    std::vector<std::string> split(const std::string& s, char delimiter)
    {
      std::vector<std::string> tokens;
      std::string token;
      std::istringstream tokenStream(s);
      while (std::getline(tokenStream, token, delimiter))
      {
        tokens.push_back(token);
        std::cout << token << std::endl;
      }
      return tokens;
    }

    std::vector<std::string> split( const std::string& str, const std::string& sep )
    {
      std::vector<std::string> result;

      size_t last = 0;
      size_t next = 0;
      while ( ( next = str.find( sep, last ) ) != std::string::npos )
      {
        result.push_back( str.substr( last, next - last ) );
        last = next + 1;
      }
      result.push_back( str.substr( last ) );

      return result;
    }

    void to_dimacs(FILE* f, solver_wrapper* solver, std::vector<std::vector<int>>& clauses )
    {
      fprintf(f, "p cnf %d %d\n", solver->nr_vars(), clauses.size() );
      for (const auto& clause : clauses) {
        for (const auto lit : clause) {
          // NOTE: variable 0 does not exist in DIMACS format
          const auto var = pabc::Abc_Lit2Var(lit) + 1;
          const auto is_c = pabc::Abc_LitIsCompl(lit);
          fprintf(f, "%d ", is_c ? -var : var);
        }
        fprintf(f, "0\n");
      }
    }

}

