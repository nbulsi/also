/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019-2021 Ningbo University, Ningbo, China */

/**
 * @file utils.hpp
 *
 * @brief TODO
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef UTILS_HPP
#define UTILS_HPP

#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/networks/xag.hpp>

namespace also
{

  std::array<mockturtle::xmg_network::signal, 3> get_children( mockturtle::xmg_network const& xmg, mockturtle::xmg_network::node const& n );
  std::array<mockturtle::xag_network::signal, 2> get_xag_children( mockturtle::xag_network const& xag, mockturtle::xag_network::node const& n );
  void print_children( std::array<mockturtle::xmg_network::signal, 3> const& children );

}

#endif
