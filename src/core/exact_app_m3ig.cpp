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
    mig_network approximate_synthesis( percy::spec& spec )
    {
      mig_network mig;
      also::mig3 mig3;

      spec.verbosity = 0;

      percy::bsat_wrapper solver;
      mig_three_app_encoder encoder( solver );

      if( mig_three_app_synthesize( spec, mig3, solver, encoder ) == percy::success )
      {
        print_all_expr( spec, mig3 );
        mig = mig3_to_mig_network( spec, mig3 );
      }

      return mig;
    }

}
