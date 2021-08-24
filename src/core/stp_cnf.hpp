/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file stp_cnf.hpp
 *
 * @brief Semi-Tensor Product based SAT for CNF
 *
 * @author Hongyang Pan
 * @since  0.1
 */

#ifndef STP_CNF_HPP
#define STP_CNF_HPP

#include <Eigen/Eigen>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <math.h>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using Eigen::MatrixXi;

namespace also
{
    void stp_cnf(vector<int> &expression, vector<string> &tt, vector<MatrixXi> &mtxvec);
}

#endif
