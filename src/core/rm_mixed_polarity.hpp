/* also: Advanced Logic Synthesis and Optimization tool*/
/* Copyright (C) 2022- Ningbo University, Ningbo, China */

/*!
  \file rm_mixed_polarity.hpp
  \brief RM logic optimization
  \author Hongwei Zhou
*/

#pragma once

#include <algorithm>
#include <cmath>
#include <iostream>
#include <mockturtle/mockturtle.hpp>
#include <string>
#include <vector>
using namespace std;

namespace mockturtle {

/*! \brief Parameters for rm_logic_optimization.
 *
 * The data structure `rm_logic_optimization` holds configurable
 * parameters with default arguments for `rm_logic_optimization`.
 */
struct rm_rewriting_params {
  /*! \brief Rewriting strategy. */
  enum strategy_t {
    /*! \brief Cut is used to divide the network. */
    cut,
    /*! \brief mffc is used to divide the network. */
    mffc,
  } strategy = cut;

  /*! \brief minimum multiplicative complexity in XAG. */
  bool multiplicative_complexity{false};
};

namespace detail {

template <class Ntk>
class rm_mixed_polarity_impl {
 public:
  using node_t = node<Ntk>;
  using signal_t = signal<Ntk>;

  rm_mixed_polarity_impl(Ntk& ntk, rm_rewriting_params const& ps_ntk)
      : ntk(ntk), ps_ntk(ps_ntk) {}

  void run() {
    switch (ps_ntk.strategy) {
      case rm_rewriting_params::cut:
        ntk_cut();
        break;
      case rm_rewriting_params::mffc:
        ntk_mffc();
        break;
    }
  }

 private:
  /**************************************************************************************************************/
  /* for cost estimation we use reference counters initialized by the fanout
   * size. */
  void initialize_refs(Ntk& ntk1) {
    ntk1.clear_values();
    ntk1.foreach_node(
        [&](auto const& n) { ntk1.set_value(n, ntk1.fanout_size(n)); });
  }
  /**************************************************************************************************************/
  /* Count the number of and gates. */
  int count_node_num(vector<string>& minterm, int variate_num,
                     string polarity) {
    int count = 0;
    if (variate_num > 1) {
      for (int i = 0; i < minterm.size(); i++) {
        int count1 = 0;
        for (int j = 0; j < variate_num; j++) {
          if (minterm[i][j] == '1' || polarity[j] == '2') count1++;
        }
        if (count1 > 0) count = count + count1 - 1;
      }
    }

    return count;
  }
  /**************************************************************************************************************/
  /* Convert ternary to decimal. */
  int atoint(string s, int radix) {
    int ans = 0;
    for (int i = 0; i < s.size(); i++) {
      char t = s[i];
      if (t >= '0' && t <= '9')
        ans = ans * radix + t - '0';
      else
        ans = ans * radix + t - 'a' + 10;
    }
    return ans;
  }
  /**************************************************************************************************************/
  /* Convert decimal to ternary. */
  string intToA(int n, int radix) {
    string ans = "";
    do {
      int t = n % radix;
      if (t >= 0 && t <= 9)
        ans += t + '0';
      else
        ans += t - 10 + 'a';
      n /= radix;
    } while (n != 0);
    reverse(ans.begin(), ans.end());
    return ans;
  }
  /**************************************************************************************************************/
  /* create signal by expression. */
  signal_t create_ntk_from_str(std::string const& s,
                               std::vector<node_t> const& leaves) {
    std::vector<signal_t> pis;
    pis.clear();
    for (const auto& l : leaves) {
      pis.push_back(ntk.make_signal(l));
    }

    std::stack<signal_t> inputs;
    int flag = 0;
    int flag1 = 0;

    for (auto i = 0; i < s.size(); i++) {
      if (s[i] == '[') {
        continue;
      } else if (s[i] == '(') {
        continue;
      } else if (s[i] == '1') {
        flag1 = 1;
        continue;
      } else if (s[i] >= 'a') {
        if (flag == 1) {
          inputs.push(ntk.create_not(pis[s[i] - 'a']));
          flag = 0;
        } else
          inputs.push(pis[s[i] - 'a']);

      } else if (s[i] == ')') {
        auto x1 = inputs.top();
        inputs.pop();

        auto x2 = inputs.top();
        inputs.pop();

        inputs.push(ntk.create_and(x1, x2));
      } else if (s[i] == ']') {
        if (flag1 == 1) {
          auto x1 = inputs.top();
          inputs.pop();
          inputs.push(ntk.create_not(x1));
          flag1 = 0;
        } else {
          auto x1 = inputs.top();
          inputs.pop();

          auto x2 = inputs.top();
          inputs.pop();
          inputs.push(ntk.create_xor(x1, x2));
        }
      } else if (s[i] == '!') {
        flag = 1;
      }
    }
    return inputs.top();
  }
  /**************************************************************************************************************/
  /* List all possible variable values/List truth table. */
  vector<string> list_truth_table(int& variate_num) {
    int a = pow(2, variate_num);
    vector<int> b;
    for (int i = 0; i < a; i++) {
      b.push_back(i);
    }
    vector<string> binary;
    string binary1;
    for (int i = 0; i < a; i++) {
      binary1 = {""};
      for (int j = variate_num - 1; j >= 0; j--) {
        int d = ((b[i] >> j) & 1);
        binary1 += d + 48;
      }
      binary.push_back(binary1);
    }
    return binary;
  }
  /**************************************************************************************************************/
  /* List all possible polarity values. */
  vector<string> list_all_polarities(int& variate_num) {
    int a1 = pow(2, variate_num);
    vector<int> c;
    for (int i = 0; i < a1; i++) {
      c.push_back(i);
    }
    vector<string> polarity1;
    string polarity2;
    for (int i = 0; i < a1; i++) {
      polarity2 = {""};
      for (int j = variate_num - 1; j >= 0; j--) {
        int d = ((c[i] >> j) & 1);
        polarity2 += d + 48;
      }
      polarity1.push_back(polarity2);
    }

    /* Store all polarities. */
    vector<string> polarity;
    polarity.push_back(polarity1[0]);
    int a2 = pow(3, variate_num);
    string str1 = polarity1[0];
    string str2 = polarity1[1];
    for (int i = 0; i < a2 - 1; i++) {
      int num1 = atoint(str1, 3);
      int num2 = atoint(str2, 3);

      int sum = num1 + num2;
      string str3 = intToA(sum, 3);

      str1 = "0";
      if (str3.size() < variate_num) {
        for (int i = 0; i < variate_num - str3.size() - 1; i++) str1 += '0';

        str1 += str3;
      } else
        str1 = str3;

      polarity.push_back(str1);
    }
    return polarity;
  }
  /**************************************************************************************************************/
  /* Mixed polarity conversion algorithm based on list technique. */
  vector<string> polarity_conversion(int& variate_num, vector<string>& minterm,
                                     string& polarity) {
    vector<string> RM_product = minterm;
    string str;
    for (int i = 0; i < variate_num; i++) {
      for (int j = 0; j < RM_product.size(); j++) {
        if (polarity[i] == '0') {
          if (RM_product[j][i] == '0') {
            str = RM_product[j];
            str[i] = '1';
            RM_product.push_back(str);
          }

        } else if (polarity[i] == '1') {
          if (RM_product[j][i] == '1') {
            str = RM_product[j];
            str[i] = '0';
            RM_product.push_back(str);
          }
        }
      }
      if (polarity[i] == '0') {
        /* Delete the minimum number of even items. */
        if (RM_product.size() > 0) {
          sort(RM_product.begin(), RM_product.end());
          for (int m = 0; m < RM_product.size() - 1; m++) {
            if (RM_product[m] == RM_product[m + 1]) {
              RM_product[m] = "0";
              RM_product[m + 1] = "0";
            }
          }
        }
        /* Delete item with '0' in array. */
        vector<string> final_term;
        final_term.clear();
        for (int m = 0; m < RM_product.size(); m++) {
          if (RM_product[m] != "0") final_term.push_back(RM_product[m]);
        }
        RM_product.clear();
        RM_product = final_term;
      } else if (polarity[i] == '1') {
        /* Delete the minimum number of even items. */
        if (RM_product.size() > 0) {
          sort(RM_product.begin(), RM_product.end());
          for (int m = 0; m < RM_product.size() - 1; m++) {
            if (RM_product[m] == RM_product[m + 1]) {
              RM_product[m] = "0";
              RM_product[m + 1] = "0";
            }
          }
        }
        /* Delete item with '0' in array. */
        vector<string> final_term;
        final_term.clear();
        for (int m = 0; m < RM_product.size(); m++) {
          if (RM_product[m] != "0") {
            if (RM_product[m][i] == '0')
              RM_product[m][i] = '1';
            else if (RM_product[m][i] == '1')
              RM_product[m][i] = '0';
            final_term.push_back(RM_product[m]);
          }
        }
        RM_product.clear();
        RM_product = final_term;
      }
    }
    return RM_product;
  }
  /**************************************************************************************************************/
  /* Search for the optimal polarity and the corresponding product term. */
  void search_for_optimal_polarity(vector<string>& minterm,
                                   vector<string>& polarity, int& variate_num,
                                   vector<string>& RM_product,
                                   string& optimal_polarity) {
    vector<string> RM_product_initial;
    for (int l = 0; l < polarity.size(); l++) {
      RM_product_initial =
          polarity_conversion(variate_num, minterm, polarity[l]);

      int count1, count2;
      count1 = count_the_number_of_nodes(variate_num, RM_product_initial,
                                         polarity[l]);
      count2 =
          count_the_number_of_nodes(variate_num, RM_product, optimal_polarity);
      if (count1 < count2) {
        optimal_polarity = polarity[l];
        RM_product.clear();
        RM_product = RM_product_initial;
        RM_product_initial.clear();
      } else {
        RM_product_initial.clear();
      }
    }
  }
  /**************************************************************************************************************/
  /* Count the number of nodes in the new network. */
  int count_the_number_of_nodes(int& variate_num, vector<string>& RM_product,
                                string& optimal_polarity) {
    /* Count the number of nodes. */
    int node_num = 0;
    if (RM_product.size() > 1) node_num = RM_product.size() - 1;

    if (variate_num > 1) {
      for (int i = 0; i < RM_product.size(); i++) {
        int n = 0;
        for (int j = variate_num - 1; j >= 0; j--) {
          if (RM_product[i][j] == '1' || optimal_polarity[j] == '2') n++;
        }
        if (n > 0) node_num = node_num + n - 1;
      }
    }
    return node_num;
  }
  /**************************************************************************************************************/
  /* Create expression. */
  string create_expression(vector<string>& RM_product, int& variate_num,
                           string& optimal_polarity, vector<string>& binary) {
    string st = "abcdefghijklmnopqrstuvwxyz";
    string expression = "";
    if (RM_product.size() > 1)
      for (int i = 0; i < RM_product.size() - 1; i++) expression += '[';
    for (int i = 0; i < RM_product.size(); i++) {
      int k = 0;
      int l = 0;
      int m = 0;
      for (int j = variate_num - 1; j >= 0; j--) {
        if (RM_product[i][j] == '1' || optimal_polarity[j] == '2') l++;
      }
      for (int j = 0; j < l - 1; j++) expression += '(';
      for (int j = variate_num - 1; j >= 0; j--) {
        if (optimal_polarity[j] == '0' && RM_product[i][j] == '1') {
          expression += st[k];
          m++;
        } else if (optimal_polarity[j] == '1' && RM_product[i][j] == '1') {
          expression += '!';
          expression += st[k];
          m++;
        } else if (optimal_polarity[j] == '2' && RM_product[i][j] == '1') {
          expression += st[k];
          m++;
        } else if (optimal_polarity[j] == '2' && RM_product[i][j] == '0') {
          expression += '!';
          expression += st[k];
          m++;
        }

        if ((m > 1 && RM_product[i][j] == '1') ||
            (m > 1 && optimal_polarity[j] == '2'))
          expression += ')';
        k++;
      }
      if (RM_product[i] == binary[0]) {
        int flag = 0;
        for (int m = variate_num - 1; m >= 0; m--) {
          if (optimal_polarity[m] == '2') flag = 1;
        }
        if (flag == 0) expression += '1';
      }
      if (i > 0 && RM_product.size() > 1) expression += ']';
    }
    return expression;
  }
  /**************************************************************************************************************/
  /* Replace network. */
  void substitute_network_mffc(string& expression, std::vector<node_t>& leaves,
                               node_t& n) {
    auto opt = create_ntk_from_str(expression, leaves);
    ntk.substitute_node(n, opt);
    ntk.set_value(n, 0);
    ntk.set_value(ntk.get_node(opt), ntk.fanout_size(ntk.get_node(opt)));
    for (auto i = 0u; i < leaves.size(); i++) {
      ntk.set_value(leaves[i], ntk.fanout_size(leaves[i]));
    }
    ntk.update_levels();
  }
  /**************************************************************************************************************/
  /* Replace network. */
  void substitute_network_cut(string& expression, std::vector<node_t>& leaves,
                              node_t& n) {
    auto opt = create_ntk_from_str(expression, leaves);
    ntk.substitute_node(n, opt);
    ntk.update_levels();
  }
  /**************************************************************************************************************/
  /* get children of top node, ordered by node level (ascending). */
  vector<signal<Ntk>> ordered_children(node<Ntk> const& n) const {
    vector<signal<Ntk>> children;
    ntk.foreach_fanin(n, [&children](auto const& f) { children.push_back(f); });
    std::sort(
        children.begin(), children.end(),
        [this](auto const& c1, auto const& c2) {
          if (ntk.level(ntk.get_node(c1)) == ntk.level(ntk.get_node(c2))) {
            return c1.index < c2.index;
          } else {
            return ntk.level(ntk.get_node(c1)) < ntk.level(ntk.get_node(c2));
          }
        });
    return children;
  }
  /**************************************************************************************************************/
  /* Judge whether the leaf node is reached. */
  int is_existence(long int root, vector<signal_t> pis) {
    int flag = 0;
    for (int i = 0; i < pis.size(); i++) {
      if (root == ntk.get_node(pis[i])) {
        flag = 1;
        break;
      }
    }
    return flag;
  }
  /**************************************************************************************************************/
  /* Hierarchical ergodic statistics AND gates node cost function. */
  void LevelOrder(long int root, vector<signal_t> pis,
                   int& optimization_and_nodes) {
    int a = 0;
    int b = 0;
    if (is_existence(root, pis) == 1) return;
    /* Create a queue container. */
    queue<long int> deq;

    deq.push(root);
    while (!deq.empty()) {
      long int tr = deq.front();
      deq.pop();

      vector<signal<Ntk>> ocs;
      ocs.clear();
      /* get children of top node, ordered by node level (ascending). */
      ocs = ordered_children(tr);

      if (ocs.size() == 2 && is_existence(ntk.get_node(ocs[0]), pis) != 1) {
        deq.push(ntk.get_node(ocs[0]));
        if (ntk.is_and(ntk.get_node(ocs[0])) &&
            ntk.fanout_size(ntk.get_node(ocs[0])) != 1u && a == 0) {
          optimization_and_nodes = optimization_and_nodes - 1;
          a = 1;
        } else if (ntk.is_and(ntk.get_node(ocs[0])) && a == 1)
          optimization_and_nodes = optimization_and_nodes - 1;
      }

      if (ocs.size() == 2 && is_existence(ntk.get_node(ocs[1]), pis) != 1) {
        deq.push(ntk.get_node(ocs[1]));
        if (ntk.is_and(ntk.get_node(ocs[1])) &&
            ntk.fanout_size(ntk.get_node(ocs[1])) != 1u && b == 0) {
          optimization_and_nodes = optimization_and_nodes - 1;
          b = 1;
        } else if (ntk.is_and(ntk.get_node(ocs[1])) && b == 1)
          optimization_and_nodes = optimization_and_nodes - 1;
      }
    }
  }
  /**************************************************************************************************************/
  /* Cut is used to divide the network. */
  void ntk_cut() {
    /* for cost estimation we use reference counters initialized by the fanout
     * size. */
    initialize_refs(ntk);

    /* enumerate cuts */
    cut_enumeration_params ps;
    ps.cut_size = 6;
    ps.cut_limit = 8;
    ps.minimize_truth_table = true;
    /* true enables truth table computation */
    auto cuts = cut_enumeration<Ntk, true>(ntk, ps);

    /* iterate over all original nodes in the network */
    const auto size = ntk.size();
    ntk.foreach_node([&](auto n, auto index) {
      /* stop once all original nodes were visited */
      if (index >= size) return false;

      /* do not iterate over constants or PIs */
      if (ntk.is_constant(n) || ntk.is_pi(n)) return true;

      /* skip cuts with small MFFC */
      if (mffc_size(ntk, n) == 1) return true;

      /* foreach cut */
      for (auto& cut : cuts.cuts(ntk.node_to_index(n))) {
        /* skip trivial cuts */
        if (cut->size() < 2) continue;

        std::vector<node_t> leaves;
        for (auto leaf_index : *cut) {
          leaves.push_back(ntk.index_to_node(leaf_index));
        }
        cut_view<Ntk> dcut(ntk, leaves, ntk.make_signal(n));

        if (dcut.num_gates() > 14) continue;

        /* skip cuts with small MFFC */
        int mffc_num_nodes = mffc_size(dcut, n);
        if (mffc_num_nodes == 1) continue;

        string tt = to_binary(cuts.truth_table(*cut));
        string str = "1";

        int index1 = tt.find(str);
        if (index1 >= tt.length()) continue;

        int len = tt.length();
        /* Stores the number of input variables. */
        int variate_num = log(len) / log(2);

        /* List all possible variable values/List truth table. */
        vector<string> binary = list_truth_table(variate_num);

        /* Stores the output of the truth table. */
        vector<char> c_out;
        for (int i = len - 1; i >= 0; i--) {
          c_out.push_back(tt[i]);
        }

        vector<char> c_out1;
        /* Minimum item for storing binary. */
        vector<string> minterm;
        for (int i = 0; i < binary.size(); i++) {
          if (c_out[i] != '0') {
            minterm.push_back(binary[i]);
            c_out1.push_back(c_out[i]);
          }
        }

        /* Store all polarities. */
        vector<string> polarity = list_all_polarities(variate_num);

        /* Count the initial cost. */
        /* Storage optimal polarity. */
        string optimal_polarity = polarity[0];
        /* Mixed polarity conversion algorithm based on list technique. */
        vector<string> RM_product =
            polarity_conversion(variate_num, minterm, optimal_polarity);

        /* Search for the optimal polarity and the corresponding product term.
         */
        search_for_optimal_polarity(minterm, polarity, variate_num, RM_product,
                                    optimal_polarity);

        /* Count the number of nodes in the new network. */
        int node_num_new = count_the_number_of_nodes(variate_num, RM_product,
                                                     optimal_polarity);

        if (ps_ntk.multiplicative_complexity == true) {
          int optimization_nodes = dcut.num_gates() - node_num_new -
                                   (dcut.num_gates() - mffc_num_nodes);
          if (optimization_nodes > 0) {
            std::vector<signal_t> pis;
            pis.clear();
            for (const auto& l : leaves) {
              pis.push_back(ntk.make_signal(l));
            }

            int and_num = 0;
            /* Count the number of and nodes in the new network. */
            int and_num_new =
                count_node_num(RM_product, variate_num, optimal_polarity);

            dcut.foreach_gate([&](auto const& n1) {
              if (ntk.is_and(n1)) and_num++;
            });

            int optimization_and_nodes = and_num - and_num_new;

            /* Hierarchical ergodic statistics AND gates node cost function. */
            LevelOrder(n, pis, optimization_and_nodes);

            if (optimization_and_nodes >= 0) {
              /* Create expression. */
              string expression = create_expression(RM_product, variate_num,
                                                    optimal_polarity, binary);

              /* Replace network. */
              substitute_network_cut(expression, leaves, n);
            }
          }
        } else {
          int optimization_nodes = dcut.num_gates() - node_num_new -
                                   (dcut.num_gates() - mffc_num_nodes);

          if (optimization_nodes > 0) {
            /* Create expression. */
            string expression = create_expression(RM_product, variate_num,
                                                  optimal_polarity, binary);

            /* Replace network. */
            substitute_network_cut(expression, leaves, n);
          }
        }
      }

      return true;
    });
  }
  /**************************************************************************************************************/
  /* mffc is used to divide the network. */
  void ntk_mffc() {
    /* for cost estimation we use reference counters initialized by the fanout
     * size. */
    ntk.clear_visited();
    ntk.clear_values();
    ntk.foreach_node(
        [&](auto const& n) { ntk.set_value(n, ntk.fanout_size(n)); });

    /* iterate over all original nodes in the network */
    const auto size = ntk.size();
    ntk.foreach_node([&](auto n, auto index) {
      /* stop once all original nodes were visited */
      if (index >= size) return false;

      if (ntk.fanout_size(n) == 0u) {
        return true;
      }

      /* do not iterate over constants or PIs */
      if (ntk.is_constant(n) || ntk.is_pi(n)) return true;

      /* skip cuts with small MFFC */
      if (mffc_size(ntk, n) == 1) return true;

      mffc_view mffc{ntk, n};
      if (mffc.num_pos() == 0 || mffc.num_pis() > 6) {
        return true;
      }

      std::vector<node_t> leaves(mffc.num_pis());
      mffc.foreach_pi([&](auto const& m, auto j) { leaves[j] = m; });

      default_simulator<kitty::dynamic_truth_table> sim(mffc.num_pis());
      string tt = to_binary(simulate<kitty::dynamic_truth_table>(mffc, sim)[0]);
      string str = "1";

      int index1 = tt.find(str);
      if (index1 >= tt.length()) return true;

      int len = tt.length();
      /* Stores the number of input variables. */
      int variate_num = log(len) / log(2);

      /* List all possible variable values/List truth table. */
      vector<string> binary = list_truth_table(variate_num);

      /* Stores the output of the truth table. */
      vector<char> c_out;
      for (int i = len - 1; i >= 0; i--) {
        c_out.push_back(tt[i]);
      }

      vector<char> c_out1;
      /* Minimum item for storing binary. */
      vector<string> minterm;
      for (int i = 0; i < binary.size(); i++) {
        if (c_out[i] != '0') {
          minterm.push_back(binary[i]);
          c_out1.push_back(c_out[i]);
        }
      }

      /* Store all polarities. */
      vector<string> polarity = list_all_polarities(variate_num);

      /* Count the initial cost. */
      /* Storage optimal polarity. */
      string optimal_polarity = polarity[0];
      /* Mixed polarity conversion algorithm based on list technique. */
      vector<string> RM_product =
          polarity_conversion(variate_num, minterm, optimal_polarity);

      /* Search for the optimal polarity and the corresponding product term. */
      search_for_optimal_polarity(minterm, polarity, variate_num, RM_product,
                                  optimal_polarity);

      /* Count the number of nodes in the new network. */
      int node_num_new =
          count_the_number_of_nodes(variate_num, RM_product, optimal_polarity);

      if (ps_ntk.multiplicative_complexity == true) {
        /* Count the number of and nodes in the new network. */
        int and_num_new =
            count_node_num(RM_product, variate_num, optimal_polarity);
        int and_num = 0;
        mffc.foreach_gate([&](auto const& n1, auto i) {
          if (ntk.is_and(n1)) and_num++;
        });

        int optimization_nodes = mffc.num_gates() - node_num_new;
        int optimization_and_nodes = and_num - and_num_new;

        if (optimization_nodes > 0 && optimization_and_nodes >= 0) {
          /* Create expression. */
          string expression = create_expression(RM_product, variate_num,
                                                optimal_polarity, binary);

          /* Replace network. */
          substitute_network_mffc(expression, leaves, n);
        }
      } else {
        int optimization_nodes = mffc.num_gates() - node_num_new;
        if (optimization_nodes > 0) {
          /* Create expression. */
          string expression = create_expression(RM_product, variate_num,
                                                optimal_polarity, binary);

          /* Replace network. */
          substitute_network_mffc(expression, leaves, n);
        }
      }

      return true;
    });
  }

  /**************************************************************************************************************/
 private:
  Ntk& ntk;
  rm_rewriting_params const& ps_ntk;
};
}  // namespace detail

/*! \brief RM logic optimization.
 *
 * This algorithm divides the network and then attempts to rewrite the subnetwork by RM logic
 * in terms of gates of the same network.  The rewritten structures are added
 * to the network, and if they lead to area improvement, will be used as new
 * parts of the logic.
 *
 * **Required network functions:**
 * - `get_node`
 * - `level`
 * - `update_levels`
 * - `create_and`
 * - `create_not`
 * - `create_xor`
 * - `substitute_node`
 * - `foreach_node`
 * - `foreach_po`
 * - `foreach_fanin`
 * - `is_and`
 * - `clear_values`
 * - `set_value`
 * - `value`
 * - `fanout_size`
 *
   \verbatim embed:rst

  .. note::

   \endverbatim
 */
template <class Ntk>
void rm_mixed_polarity(Ntk& ntk, rm_rewriting_params const& ps_ntk = {}) {
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_level_v<Ntk>, "Ntk does not implement the level method" );
  static_assert( has_create_maj_v<Ntk>, "Ntk does not implement the create_maj method" );
  static_assert( has_create_xor_v<Ntk>, "Ntk does not implement the create_maj method" );
  static_assert( has_substitute_node_v<Ntk>, "Ntk does not implement the substitute_node method" );
  static_assert( has_update_levels_v<Ntk>, "Ntk does not implement the update_levels method" );
  static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
  static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_is_and_v<Ntk>, "Ntk does not implement the is_and method" );
  static_assert( has_is_xor_v<Ntk>, "Ntk does not implement the is_xor method" );
  static_assert( has_clear_values_v<Ntk>, "Ntk does not implement the clear_values method" );
  static_assert( has_set_value_v<Ntk>, "Ntk does not implement the set_value method" );
  static_assert( has_value_v<Ntk>, "Ntk does not implement the value method" );
  static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );

  detail::rm_mixed_polarity_impl p(ntk, ps_ntk);
  p.run();
}

} /* namespace mockturtle */
