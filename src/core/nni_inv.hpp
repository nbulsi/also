/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file nni_inv.hpp
 *
 * @brief TODO
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef NNI_INV_HPP
#define NNI_INV_HPP

#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/networks/klut.hpp>

namespace also
{

  mockturtle::klut_network nni_opt( mockturtle::xmg_network const& ntk );

}

#endif
