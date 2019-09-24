/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file imgrw.hpp
 *
 * @brief img_network rewriting
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef IMGRW_HPP
#define IMGRW_HPP

#include "img.hpp"
#include <mockturtle/mockturtle.hpp>

using namespace mockturtle;

namespace also
{
  img_network img_rewriting( img_network& img );
  void step_to_expression( const img_network& img, std::ostream& s, int index );
  void img_to_expression( std::ostream& s, const img_network& img );
  std::array<img_network::signal, 2> get_children( const img_network& img, img_network::node const& n );
}

#endif
