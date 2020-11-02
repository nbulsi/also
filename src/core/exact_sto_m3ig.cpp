/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

#include <vector>
#include <iostream>
#include <cmath>

#include <kitty/kitty.hpp>

namespace also
{

/******************************************************************************
 * Types                                                                      *
 ******************************************************************************/
  class sto_syn_manager
  {
    public:
      sto_syn_manager( unsigned const& num_vars, unsigned const& m, unsigned const& n, std::vector<unsigned> const& vector );
      void run();
      void preprocess();
      unsigned sum_of_vector();
      unsigned count_tt_sum_of_x( unsigned const& spec_num, kitty::dynamic_truth_table const& tt_solution );

    private:
      unsigned num_vars;
      unsigned m;
      unsigned n;
      std::vector<unsigned> vector;

      unsigned vec_sum;
  };

/******************************************************************************
 * Private functions                                                          *
 ******************************************************************************/
  sto_syn_manager::sto_syn_manager( unsigned const& num_vars, unsigned const& m, unsigned const& n, std::vector<unsigned> const& vector )
    : num_vars( num_vars ), m( m ), n( n ), vector( vector )
  {
    vec_sum = sum_of_vector();
  }

  unsigned sto_syn_manager::sum_of_vector()
  {
    unsigned sum{ 0u };
    for( auto const& v : vector )
    {
      sum += v;
    }

    return sum;
  }

  /* check all primary inputs and its complements for trivial
   * cases */
  void sto_syn_manager::preprocess()
  {
    unsigned num_vars_m_plus_n = m + n;

    for( auto i = 0u; i < num_vars_m_plus_n; i++ )
    {
      kitty::dynamic_truth_table tt( num_vars_m_plus_n );
      kitty::create_nth_var( tt, i );

      kitty::print_binary( ~tt, std::cout );
      std::cout << "\n";
      auto num_ones = kitty::count_ones( tt );
      std::cout << "num_ones: " << num_ones << std::endl;

      if( num_ones == vec_sum )
      {
        /* for further check */
        for( auto j = 0u; j <= n; j++ )
        {
          std::cout << "The " << j << "th element: " << vector[j] << std::endl;
          std::cout << "The " << "sum of entry equals to " << j << " is " << count_tt_sum_of_x( j, tt ) << std::endl;
        }
      }
    }
  }

  /* find the number of tt entry whose sum is spec number 
   * spec_num is in range [0,n]
   * */
  unsigned sto_syn_manager::count_tt_sum_of_x( unsigned const& spec_num, kitty::dynamic_truth_table const& tt_solution )
  {
    unsigned total = 0u; 
    auto var = (unsigned)ceil( log2( m + n ) );
    kitty::dynamic_truth_table tt_enum( var );

    auto num_loop = 0u;
    do
    {
      auto sum = 0u;
      //kitty::print_binary( tt_enum, std::cout );
      //std::cout << std::endl;

      for( auto i = 0u; i < n; i++ )
      {
        //std::cout << " bit on " << i + m << " is " << kitty::get_bit( tt_enum, i + m ) << "\n";
        sum += kitty::get_bit( tt_enum, i + m );
      }

      if( ( sum == spec_num ) && kitty::get_bit( tt_solution, num_loop ) ) 
      { 
        total++; 
      }

      kitty::next_inplace( tt_enum );
      num_loop++;
    }while( !kitty::is_const0( tt_enum ) && num_loop < pow( 2, m + n ) );

    return total;
  }

  void sto_syn_manager::run()
  {
    std::cout << " num_vars : " << num_vars << " \n"
              << " m        : " << m << " \n"
              << " n        : " << n << " \n" 
              << " vec_sum  : " << vec_sum << " \n";

    preprocess();
  }

/******************************************************************************
 * Public functions                                                           *
 ******************************************************************************/
  void stochastic_synthesis( unsigned const& num_vars, unsigned const& m, unsigned const& n, std::vector<unsigned> const& vector )
  {
    sto_syn_manager mgr( num_vars, m, n, vector );
    mgr.run();
  }

}
