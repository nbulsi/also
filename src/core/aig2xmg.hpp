/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file aig2xmg.hpp
 *
 * @brief Apply aig to xmg by one-to-one mapping
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef AIG2XMG_HPP
#define AIG2XMG_HPP

#include <mockturtle/mockturtle.hpp>
using namespace mockturtle;

namespace also
{
  xmg_network xmg_from_aig( const aig_network& aig );
}

#endif
