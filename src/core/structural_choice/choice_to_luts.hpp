#ifndef CHOICE_TO_LUTS_HPP
#define CHOICE_TO_LUTS_HPP
#pragma once

#include <mockturtle/mockturtle.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/utils/node_map.hpp>
#include <optional>
#include <unordered_map>
using namespace mockturtle;
namespace lsmap {
template <class NtkDest, class Ntk>
class collapse_mapped_choice_impl {
 public:
  using node_t = typename Ntk::node;
  collapse_mapped_choice_impl(Ntk const& ntk) : ntk(ntk) {}

  void run(NtkDest& dest) {
    node_map<signal, Ntk> node_to_signal(ntk);

    /* special map for output drivers to perform some optimizations */
    enum class driver_type { none, pos, neg, mixed };
    node_map<driver_type, Ntk> node_driver_type(ntk, driver_type::none);

    /* opposites are filled for nodes with mixed driver types, since they have
       two nodes in the network. */
    std::unordered_map<node<Ntk>, signal> opposites;

    /* initial driver types */
    ntk.foreach_po([&](auto const& f) {
      switch (node_driver_type[f]) {
        case driver_type::none:
          node_driver_type[f] =
              ntk.is_complemented(f) ? driver_type::neg : driver_type::pos;
          break;
        case driver_type::pos:
          node_driver_type[f] =
              ntk.is_complemented(f) ? driver_type::mixed : driver_type::pos;
          break;
        case driver_type::neg:
          node_driver_type[f] =
              ntk.is_complemented(f) ? driver_type::neg : driver_type::mixed;
          break;
        case driver_type::mixed:
        default:
          break;
      }
    });

    /* it could be that internal nodes also point to an output driver node */
    ntk.foreach_node([&](auto const n) {
      if (ntk.is_constant(n) || ntk.is_pi(n) || !ntk.is_cell_root(n)) return;

      ntk.foreach_cell_fanin(n, [&](auto fanin) {
        if (node_driver_type[fanin] == driver_type::neg) {
          node_driver_type[fanin] = driver_type::mixed;
        }
      });
    });

    /* constants */
    auto add_constant_to_map = [&](bool value) {
      const auto n = ntk.get_node(ntk.get_constant(value));
      switch (node_driver_type[n]) {
        default:
        case driver_type::none:
        case driver_type::pos:
          node_to_signal[n] = dest.get_constant(value);
          break;

        case driver_type::neg:
          node_to_signal[n] = dest.get_constant(!value);
          break;

        case driver_type::mixed:
          node_to_signal[n] = dest.get_constant(value);
          opposites[n] = dest.get_constant(!value);
          break;
      }
    };

    add_constant_to_map(false);
    if (ntk.get_node(ntk.get_constant(false)) !=
        ntk.get_node(ntk.get_constant(true))) {
      add_constant_to_map(true);
    }

    /* primary inputs */
    ntk.foreach_pi([&](auto n) {
      signal dest_signal;
      switch (node_driver_type[n]) {
        default:
        case driver_type::none:
        case driver_type::pos:
          dest_signal = dest.create_pi();
          node_to_signal[n] = dest_signal;
          break;

        case driver_type::neg:
          dest_signal = dest.create_pi();
          node_to_signal[n] = dest.create_not(dest_signal);
          break;

        case driver_type::mixed:
          dest_signal = dest.create_pi();
          node_to_signal[n] = dest_signal;
          opposites[n] = dest.create_not(node_to_signal[n]);
          break;
      }

      if constexpr (has_has_name_v<Ntk> && has_get_name_v<Ntk> &&
                    has_set_name_v<NtkDest>) {
        if (ntk.has_name(ntk.make_signal(n)))
          dest.set_name(dest_signal, ntk.get_name(ntk.make_signal(n)));
      }
    });

    /* nodes */

    ntk.foreach_gate([&](auto const& n) {
      if (ntk.is_constant(n) || ntk.is_pi(n) || !ntk.is_cell_root(n)) return;

      std::vector<signal> children;
      ntk.foreach_cell_fanin(
          n, [&](auto fanin) { children.push_back(node_to_signal[fanin]); });

      switch (node_driver_type[n]) {
        default:
        case driver_type::none:
        case driver_type::pos:
          node_to_signal[n] = dest.create_node(children, ntk.cell_function(n));
          break;

        case driver_type::neg:
          node_to_signal[n] = dest.create_node(children, ~ntk.cell_function(n));
          break;

        case driver_type::mixed:
          node_to_signal[n] = dest.create_node(children, ntk.cell_function(n));
          opposites[n] = dest.create_node(children, ~ntk.cell_function(n));
          break;
      }
    });

    /* outputs */
    ntk.foreach_po([&](auto const& f, auto index) {
      (void)index;

      if (ntk.is_complemented(f) && node_driver_type[f] == driver_type::mixed) {
        dest.create_po(opposites[ntk.get_node(f)]);
      } else {
        dest.create_po(node_to_signal[f]);
      }

      if constexpr (has_has_output_name_v<Ntk> &&
                    has_get_output_name_v<Ntk> &&
                    has_set_output_name_v<NtkDest>) {
        if (ntk.has_output_name(index)) {
          dest.set_output_name(index, ntk.get_output_name(index));
        }
      }
    });
  }

 private:
  Ntk const& ntk;
};  // end class collapse_mapped_choice_impl
template <class NtkDest, class Ntk>
std::optional<NtkDest> choice_to_luts(Ntk const& ntk) {
  if (!ntk.has_mapping() && ntk.num_gates() > 0) {
    return std::nullopt;
  } else {
    lsmap::collapse_mapped_choice_impl<NtkDest, Ntk> p(ntk);
    NtkDest dest;
    p.run(dest);
    return dest;
  }
}

template <class NtkDest, class Ntk>
bool choice_to_luts(NtkDest& dest, Ntk const& ntk) {
  if (!ntk.has_mapping() && ntk.num_gates() > 0) {
    return false;
  } else {
    lsmap::collapse_mapped_choice_impl<NtkDest, Ntk> p(ntk);
    p.run(dest);
    return true;
  }
}

}  // namespace lsmap
#endif