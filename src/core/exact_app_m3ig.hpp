/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019-2021 Ningbo University, Ningbo, China */

/**
 * @file exact_app_m3ig.hpp
 *
 * @brief TODO
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef EXACT_APP_M3IG_HPP
#define EXACT_APP_M3IG_HPP

namespace also
{
    mig_network approximate_synthesis( percy::spec& spec, const unsigned& dist );
    void enumerate_app_m3ig( percy::spec& spec, const unsigned& dist );
}

#endif
