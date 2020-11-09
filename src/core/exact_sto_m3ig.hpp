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

namespace also
{
  typedef struct Problem_Vector_t_ Problem_Vector_t;

  struct Problem_Vector_t_
  {
    unsigned num_vars;
    unsigned m;
    unsigned n;
    std::vector<unsigned> v;
  };

  void stochastic_synthesis( unsigned const& num_vars, unsigned const& m, unsigned const& n, std::vector<unsigned> const& vector );

}

#endif
