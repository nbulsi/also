/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file exact_sto_m3ig.hpp
 *
 * @brief stochastic circuit synthesis using m3ig as the underlying logic
 * representation
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef EXACT_STO_M3IG_HPP
#define EXACT_STO_M3IG_HPP

using namespace mockturtle;
namespace also
{
  typedef struct Problem_Vector_t_ Problem_Vector_t;

  /*
   * 1 : number of primary output
   * 2 : n
   * 2 : m
   * 0 3 0 : problem_vector
   * 4 5 6 : pre_occupy_position_idxs, the entries of these positions are assigned 1
   * dc \ ba
   *      0   0   1   1
   *      0   1   0   1
   *  +---+---+---+---+
   * 00 | x  | 0  | 1  | 2  |
   * +---+---+---+---+
   * 01 | 3  | 4  | 5  | 6  |
   * +---+---+---+---+
   * 10 | 7  | 8  | 9  | 10  |
   * +---+---+---+---+
   * 11 | 11  | 12 | 13 | 14 |
   * +---+---+---+---+
   * */
  struct Problem_Vector_t_
  {
    unsigned num_vars;
    unsigned m;
    unsigned n;
    std::vector<unsigned> v;
    std::vector<unsigned> pre_occupy_position_idxs;
  };

  mig_network stochastic_synthesis( unsigned const& num_vars, unsigned const& m, unsigned const& n,
                                    std::vector<unsigned> const& vector,
                                    std::vector<unsigned> const& preoccupy );

}

#endif
