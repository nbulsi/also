/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

#include <cmath>
#include <iostream>
#include <vector>

#include "exact_m3ig_sto_encoder.hpp"
#include "exact_sto_m3ig.hpp"
#include <kitty/kitty.hpp>

namespace also {

/******************************************************************************
 * Types                                                                      *
 ******************************************************************************/
class sto_syn_manager {
public:
  sto_syn_manager(unsigned const &num_vars, unsigned const &m,
                  unsigned const &n, std::vector<unsigned> const &vector,
                  std::vector<unsigned> const &preoccupy,
                  unsigned const &time_sec_limit );
  std::optional<mig_network> run();
  std::optional<mig_network> preprocess();
  unsigned sum_of_vector();
  unsigned count_tt_sum_of_x(unsigned const &spec_num,
                             kitty::dynamic_truth_table const &tt_solution);
  bool validate(kitty::dynamic_truth_table const &tt);

private:
  unsigned num_vars;
  unsigned m;
  unsigned n;
  std::vector<unsigned> vector;
  std::vector<unsigned> preoccupy;
  unsigned time_sec_limit; //solver time limit

  unsigned vec_sum = 0u;
  bool verbose = false;
  bool trivial = false;
};

/******************************************************************************
 * Private functions                                                          *
 ******************************************************************************/
sto_syn_manager::sto_syn_manager(unsigned const &num_vars, unsigned const &m,
                                 unsigned const &n,
                                 std::vector<unsigned> const &vector,
                                 std::vector<unsigned> const &preoccupy,
                                 unsigned const &time_sec_limit )
    : num_vars(num_vars), m(m), n(n), vector(vector), preoccupy(preoccupy), time_sec_limit( time_sec_limit )
{
  vec_sum = sum_of_vector();
}

unsigned sto_syn_manager::sum_of_vector() {
  unsigned sum = 0u;
  for (auto const &v : vector) {
    sum += v;
  }

  return sum;
}

bool sto_syn_manager::validate(kitty::dynamic_truth_table const &tt) {
  unsigned j = 0u;

  for (j = 0u; j <= n; j++) {
    if (verbose) {
      std::cout << "The " << j << "th element: " << vector[j] << std::endl;
      std::cout << "The "
                << "sum of entry equals to " << j << " is "
                << count_tt_sum_of_x(j, tt) << std::endl;
    }

    if (vector[j] != count_tt_sum_of_x(j, tt)) {
      return false;
    }
  }

  if ((j - 1) == n) {
    return true;
  }

  assert(false);
}

/* check all primary inputs and its complements for trivial
 * cases */
std::optional<mig_network> sto_syn_manager::preprocess() {
  unsigned num_vars_m_plus_n = m + n;
  mig_network mig;
  /* consts */
  if (vec_sum == 0u) {
    trivial = true;
    std::cout << "Const zero is a solution\n";

    const auto c0 = mig.get_constant(false);
    mig.create_po(c0);
    return mig;
  } else if (vec_sum == pow(2, num_vars_m_plus_n)) {
    trivial = true;
    std::cout << "Const one is a solution\n";

    const auto c0 = mig.get_constant(true);
    mig.create_po(c0);
    return mig;
  } else if (vec_sum > pow(2, num_vars_m_plus_n)) {
    assert(false && "Problem vector overflow\n");
  }

  for (auto i = 0u; i < num_vars_m_plus_n; i++) {
    kitty::dynamic_truth_table tt(num_vars_m_plus_n);
    kitty::create_nth_var(tt, i);

    auto num_ones = kitty::count_ones(tt);

    if (num_ones == vec_sum) {
      if (validate(tt)) {
        trivial = true;
        //kitty::print_binary(tt, std::cout);
        // std::cout << " is a solution. The expression -->" << " f = " <<
        // static_cast<char>( 'a' + i ) << "\n";

        for (auto id = 0; id <= i; id++) {
          auto pi = mig.create_pi();

          if (id == i) {
            mig.create_po(pi);
            return mig;
          }
        }
      } else if (validate(~tt)) {
        trivial = true;
        //kitty::print_binary(~tt, std::cout);
        // std::cout << " is a solution. The expression --> " << " f = !" <<
        // static_cast<char>( 'a' + i ) << "\n";

        for (auto id = 0; id <= i; id++) {
          auto pi = mig.create_pi();

          if (id == i) {
            mig.create_po(!pi);
            return mig;
          }
        }
      }
    }
  }

  return std::nullopt;
}

/* find the number of tt entry whose sum is spec number
 * spec_num is in range [0,n]
 * */
unsigned sto_syn_manager::count_tt_sum_of_x(
    unsigned const &spec_num, kitty::dynamic_truth_table const &tt_solution) {
  unsigned total = 0u;
  auto var = (unsigned)ceil(log2(m + n));
  kitty::dynamic_truth_table tt_enum(var);

  auto num_loop = 0u;
  do {
    auto sum = 0u;

    for (auto i = 0u; i < n; i++) {
      if (verbose) {
        std::cout << " bit on " << i + m << " is "
                  << kitty::get_bit(tt_enum, i + m) << "\n";
      }

      sum += kitty::get_bit(tt_enum, i + m);
    }

    if ((sum == spec_num) && kitty::get_bit(tt_solution, num_loop)) {
      total++;
    }

    kitty::next_inplace(tt_enum);
    num_loop++;
  } while (!kitty::is_const0(tt_enum) && num_loop < pow(2, m + n));

  return total;
}

std::optional<mig_network> sto_syn_manager::run() {
  mig_network mig;
  if (verbose) {
    std::cout << " num_vars : " << num_vars << " \n"
              << " m        : " << m << " \n"
              << " n        : " << n << " \n"
              << " vec_sum  : " << vec_sum << " \n";
  }

  auto ret = preprocess();

  if (trivial == false && !ret.has_value()) {
    std::cout << "[i] Not trivial case, need further solve.\n";

    percy::spec spec;
    also::mig3 mig3;

    spec.verbosity = 0;

    // stochastic problem vector
    Problem_Vector_t instance;
    instance.num_vars = num_vars;
    instance.m = m;
    instance.n = n;
    instance.v = vector;
    instance.pre_occupy_position_idxs = preoccupy;

    percy::bsat_wrapper solver;
    mig_three_sto_encoder encoder(solver, instance);

    //set time limit for sat
    solver.set_time_limit( time_sec_limit );

    auto result = mig_three_sto_synthesize( spec, mig3, solver, encoder );
    if( result == percy::success )
    {
      print_all_expr(spec, mig3);
      mig = mig3_to_mig_network(spec, mig3);
      return mig;
    }
    else if( result == percy::timeout )
    {
        return std::nullopt;
    }
    else
    {
        assert( false && "exact synthesis failed\n" );
    }
  }

  return ret.value();
}

/******************************************************************************
 * Public functions                                                           *
 ******************************************************************************/
std::optional<mig_network> stochastic_synthesis(unsigned const &num_vars, unsigned const &m,
                                 unsigned const &n,
                                 std::vector<unsigned> const &vector,
                                 std::vector<unsigned> const &preoccupy,
                                 unsigned const &time_sec_limit ) {
  sto_syn_manager mgr( num_vars, m, n, vector, preoccupy, time_sec_limit );
  return mgr.run();
}

} // namespace also
