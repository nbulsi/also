/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file stp_circuit.hpp
 *
 * @brief Basic Semi-Tensor Product calculation model
 *
 * @author Hongyang Pan
 * @since  0.1
 */

#ifndef STP_CIRCUIT_HPP
#define STP_CIRCUIT_HPP

#include <Eigen/Eigen>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
using Eigen::MatrixXi;

namespace also
{
    void stp_calc(string &expression, vector<string> &tt, vector<MatrixXi> &mtxvec);
}

#endif
