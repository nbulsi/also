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

#include <mockturtle/utils/node_map.hpp>
#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/mig.hpp>

namespace also
{
  mockturtle::xmg_network xmg_from_aig( const mockturtle::aig_network& aig );
  mockturtle::xmg_network xmg_from_mig( const mockturtle::mig_network& mig );
}

#endif
