/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

#include <vector>
#include <iostream>
#include <cmath>

#include <kitty/kitty.hpp>
#include <mockturtle/mockturtle.hpp>
#include "exact_m3ig_sto_encoder.hpp"

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
      bool validate( kitty::dynamic_truth_table const& tt );

    private:
      unsigned num_vars;
      unsigned m;
      unsigned n;
      std::vector<unsigned> vector;

      unsigned vec_sum = 0u;
      bool verbose = false;
      bool trivial = false;
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
    unsigned sum = 0u;
    for( auto const& v : vector )
    {
      sum += v;
    }

    return sum;
  }

  bool sto_syn_manager::validate( kitty::dynamic_truth_table const& tt )
  {
    unsigned j = 0u;
    
    for( j = 0u; j <= n; j++ )
    {
      if( verbose )
      {
        std::cout << "The " << j << "th element: " << vector[j] << std::endl;
        std::cout << "The " << "sum of entry equals to " << j << " is " << count_tt_sum_of_x( j, tt ) << std::endl;
      }

      if( vector[j] != count_tt_sum_of_x( j, tt ) )
      {
        return false;
      }
    }

    if( ( j - 1 ) == n )
    {
      return true;
    }

    assert( false );
  }

  /* check all primary inputs and its complements for trivial
   * cases */
  void sto_syn_manager::preprocess()
  {
    unsigned num_vars_m_plus_n = m + n;

    /* consts */
    if( vec_sum == 0u )
    {
      trivial = true;
      std::cout << "Const zero is a solution\n";
    }
    else if( vec_sum == pow( 2, num_vars_m_plus_n ) )
    {
      trivial = true;
      std::cout << "Const one is a solution\n";
    }
    else if( vec_sum > pow( 2, num_vars_m_plus_n ) )
    {
      assert( false && "Problem vector overflow\n" );
    }

    for( auto i = 0u; i < num_vars_m_plus_n; i++ )
    {
      kitty::dynamic_truth_table tt( num_vars_m_plus_n );
      kitty::create_nth_var( tt, i );

      auto num_ones = kitty::count_ones( tt );

      if( num_ones == vec_sum )
      {
        if( validate( tt ) ) 
        {
          trivial = true;
          kitty::print_binary( tt, std::cout );
          std::cout << " is a solution. The expression -->" << " f = " << static_cast<char>( 'a' + i ) << "\n";
        }
        else if( validate( ~tt ) )
        {
          trivial = true;
          kitty::print_binary( ~tt, std::cout );
          std::cout << " is a solution. The expression --> " << " f = !" << static_cast<char>( 'a' + i ) << "\n";
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

      for( auto i = 0u; i < n; i++ )
      {
        if( verbose )
        {
          std::cout << " bit on " << i + m << " is " << kitty::get_bit( tt_enum, i + m ) << "\n";
        }

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
    if( verbose )
    {
      std::cout << " num_vars : " << num_vars << " \n"
        << " m        : " << m << " \n"
        << " n        : " << n << " \n" 
        << " vec_sum  : " << vec_sum << " \n";
    }

    preprocess();

    if( trivial == false )
    {
      std::cout << "need further solve\n";
    }

    percy::spec spec;
    also::mig3 mig3;

    kitty::dynamic_truth_table tt( 3 );

    kitty::create_from_hex_string( tt, "e8" );
    spec[0] = tt;

    spec.initial_steps = 0;
    percy::bsat_wrapper solver;
    mig_three_sto_encoder encoder( solver );

    if( mig_three_sto_synthesize( spec, mig3, solver, encoder ) == percy::success )
    {
      print_all_expr( spec, mig3 );
    }
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
