/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file cog.hpp //compute optimal graph representations
 *
 * @brief given a file that contains multiple Boolean functions
 * represented in truth tables, compute its optimal m5ig or m3ig
 * or img
 * expressions
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef COG_HPP
#define COG_HPP

#include <mockturtle/mockturtle.hpp>

#include <fstream>
#include <sstream>
#include <string>

#include "../core/exact_m5ig_encoder.hpp"
#include "../core/m5ig_helper.hpp"
#include "../core/m3ig_helper.hpp"
#include "../core/misc.hpp"

namespace alice
{

  class cog_command: public command
  {
    public:
      explicit cog_command( const environment::ptr& env ) : command( env, "compute optimal m5ig" )
      {
        add_option( "filename, -f", filename, "the input txt file name" );
        add_flag(   "--m3ig, -m", "using m3ig as the underlying logic network" );
        add_flag(   "--img, -i",  "using img as the underlying logic network" );
      }

    protected:
      void execute()
      {
        std::ifstream infile( filename ); 

        std::ofstream outfile;

        outfile.open( "opt.txt" );

        if( !infile || !outfile ) 
        {
          std::cout << " Cannot open file " << std::endl;
          assert( false );
        }

        std::string line;
        while (std::getline(infile, line) ) 
        {
          std::string f = line;
          f.replace( f.begin(), f.begin() + 2, "" );
          
          const unsigned num_vars = ::log( f.size() * 4 ) / ::log( 2.0 );
          
          kitty::dynamic_truth_table t( num_vars );
          kitty::create_from_hex_string( t, f );

          spec spec;
          if( is_set( "m5ig" ) )
          {
            /* mig five synthesize */
            also::mig5 mig5;

            if( num_vars < 5 )
            {
              spec[0] = kitty::extend_to( t, 5 );
            }
            else
            {
              spec[0] = t;
            }

            if ( also::parallel_mig_five_fence_synthesize( spec, mig5 ) == success )
            {
              auto s = also::mig5_to_string( spec, mig5 );
              outfile << "0x" << f << " " << s << std::endl;
            }
          }
          else if( is_set( "img" ) )
          {
            also::img img;

            if( num_vars < 2 )
            {
              spec[0] = kitty::extend_to( t, 2 );
            }
            else
            {
              spec[0] = t;
            }

            bsat_wrapper solver;
            also::img_encoder encoder( solver );

            if ( also::img_cegar_synthesize( spec, img, solver, encoder ) == success )
            {
              auto s = also::img_to_string( spec, img );
              outfile << "0x" << f << " " << s << " size: " << spec.nr_steps << std::endl;
            }
          }
          else
          {
            /* mig three synthesize */
            also::mig3 mig3;

            if( num_vars < 3 )
            {
              spec[0] = kitty::extend_to( t, 3 );
            }
            else
            {
              spec[0] = t;
            }

            if ( also::parallel_mig_three_fence_synthesize( spec, mig3 ) == success )
            {
              auto s = also::mig3_to_string( spec, mig3 );
              outfile << "0x" << f << " " << s << std::endl;
            }
          }
        }

        infile.close();
        outfile.close();
      }

    private:
      std::string filename = "test.txt";
  };

  ALICE_ADD_COMMAND( cog, "Exact synthesis" )


}

#endif
