/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019-2021 Ningbo University, Ningbo, China */

#include "exact_m3ig_app_encoder.hpp"
#include "exact_app_m3ig.hpp"

namespace also
{

/******************************************************************************
 * Types                                                                      *
 ******************************************************************************/

/******************************************************************************
 * Private functions                                                          *
 ******************************************************************************/

/******************************************************************************
 * Public functions                                                           *
 ******************************************************************************/
    mig_network approximate_synthesis( percy::spec& spec, const unsigned& dist, const unsigned& min, const bool& allow )
    {
      mig_network mig;
      also::mig3 mig3;

      percy::bsat_wrapper solver;
      mig_three_app_encoder encoder( solver, dist, min, allow );

      if( mig_three_app_synthesize( spec, mig3, solver, encoder ) == percy::success )
      {
        print_all_expr( spec, mig3 );
        mig = mig3_to_mig_network( spec, mig3 );
      }

      return mig;
    }

    void enumerate_app_m3ig( percy::spec& spec, const unsigned& dist, const unsigned& min, const bool& allow )
    {
        also::mig3 mig3;
        percy::bsat_wrapper solver;
        mig_three_app_encoder encoder( solver, dist, min, allow );

        unsigned num_solutions = 0;

        while( also::next_solution( spec, mig3, solver, encoder ) == percy::success )
        {
          print_all_expr( spec, mig3 );
          num_solutions++;
        }

        std::cout << "There are " << num_solutions << " solutions found." << std::endl;
    }

}
