/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2022- Ningbo University, Ningbo, China */
/**
 * @file rm_mp2.hpp
 *
 * @brief Multilevel RM logic optimization
 *
 * @author Hongwei Zhou
 *
 */

#ifndef RM_MP2_HPP
#define RM_MP2_HPP

#include <time.h>

#include <mockturtle/mockturtle.hpp>
#include "../core/rm_mixed_polarity2.hpp"

namespace alice {

class rm2_command : public command {
 public:
  explicit rm2_command(const environment::ptr& env)
      : command(env, "Performs multi-level RM logic optimization") {
    add_option("strategy, -s", strategy, "cut = 0, mffc = 1");
    add_flag("--minimum_and_gates, -m", "minimum multiplicative complexity in XAG");
    add_flag("--xag, -g", "RM logic optimization for xag network");
    add_flag("--xmg, -x", "RM logic optimization for xmg network");
    add_flag("--cec,-c", "apply equivalence checking in rewriting");
  }

  rules validity_rules() const {
    if (is_set("xmg")) {
      return {has_store_element<xmg_network>(env),
              {[this]() { return (strategy <= 1 && strategy >= 0); },
               "strategy must in [0,1] "}};
    }

    return {has_store_element<xag_network>(env),
            {[this]() { return (strategy <= 1 && strategy >= 0); },
             "strategy must in [0,1] "}};
  }

 protected:
  void execute() {
    if (is_set("xag")) {
     //clock_t start, end;
      //start = clock();
      mockturtle::xag_network xag = store<xag_network>().current();
      /* parameters */
      ps_ntk.multiplicative_complexity = is_set("minimum_and_gates");

      if (strategy == 0)
        ps_ntk.strategy = rm_rewriting_params2::cut;
      else if (strategy == 1)
        ps_ntk.strategy = rm_rewriting_params2::mffc;
      else
        assert(false);

      xag_network xag1, xag2;
      xag1 = xag;
      
      //depth_view<mockturtle::xag_network,unit_cost<mockturtle::xag_network>> depth_xag1(xag);
      cut_rewriting_params ps;
      ps.cut_enumeration_ps.cut_size =6;
      ps.cut_enumeration_ps.cut_limit=12;
      ps.min_cand_cut_size=2;
      ps.allow_zero_gain=true;
      ps.progress = true;
      rm_mixed_polarity2(xag, ps_ntk,ps);
      xag = cleanup_dangling( xag );

      xag2 = xag;

      if (is_set("cec")) {
        /* equivalence checking */
        const auto miter_xag = *miter<xag_network>(xag1, xag2);
        equivalence_checking_stats eq_st;
        const auto result = equivalence_checking(miter_xag, {}, &eq_st);
        assert(*result);
      }

      //end = clock();
      // std::cout << "run time: " << (double)(end - start) / CLOCKS_PER_SEC
      //         << std::endl;
      std::cout << "[rm_mixed_polarity] ";
      also::print_stats(xag);

     store<xag_network>().extend();
     store<xag_network>().current() = xag;

    } else if (is_set("xmg")) {
      //clock_t start, end;
      //start = clock();
      mockturtle::xmg_network xmg = store<xmg_network>().current();
      /* parameters */
      if (strategy == 0)
        ps_ntk.strategy = rm_rewriting_params2::cut;
      else if (strategy == 1)
        ps_ntk.strategy = rm_rewriting_params2::mffc;
      else
        assert(false);

      xmg_network xmg1, xmg2;
      xmg1 = xmg;

      cut_rewriting_params ps;
      ps.cut_enumeration_ps.cut_size =6;
      ps.cut_enumeration_ps.cut_limit=12;
      ps.min_cand_cut_size=2;
      ps.allow_zero_gain=true;
      ps.progress = true;
      rm_mixed_polarity2(xmg, ps_ntk,ps);
      xmg = cleanup_dangling(xmg);

      xmg2 = xmg;

      if (is_set("cec")) {
        /* equivalence checking */
        const auto miter_xmg = *miter<xmg_network>(xmg1, xmg2);
        equivalence_checking_stats eq_st;
        const auto result = equivalence_checking(miter_xmg, {}, &eq_st);
        assert(*result);
      }
      
      //end = clock();
      // std::cout << "run time: " << (double)(end - start) / CLOCKS_PER_SEC
      //          << std::endl;
      std::cout << "[rm_mixed_polarity] ";
      also::print_stats(xmg);
      
      store<xmg_network>().extend();
      store<xmg_network>().current() = xmg;
    }
  }

 private:
  int strategy = 0;
  rm_rewriting_params2 ps_ntk;
};

ALICE_ADD_COMMAND(rm2, "Rewriting")

}  // namespace alice

#endif
