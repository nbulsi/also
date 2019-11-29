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
#include <unordered_set>

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
        add_flag(   "--pclassfication, -p",  "using p classfication" );
      }

    protected:
      void execute()
      {
        spec spec;

        if( is_set( "img" ) )
        {
          if( is_set( "pclassfication" ) )
          {
            kitty::dynamic_truth_table tt4s( 4 );
            int count = 0;

            std::unordered_set<std::string> myset;
            
            /* enumerate all 4-input Boolean function */
            do
            {
              count++;

              auto res = kitty::exact_p_canonization( tt4s );
              
              myset.insert( kitty::to_hex( std::get<0>( res ) ) );

              kitty::next_inplace( tt4s );
            }while( !kitty::is_const0( tt4s ) );

            std::cout << "There are " << count << " functions. " << " The distinct representive tt size: " << myset.size() << std::endl;
            
            std::ofstream outfile;
            outfile.open( "p4.txt" );

            if( !outfile ) 
            {
              std::cout << " Cannot open file " << std::endl;
              assert( false );
            }
            

            for( const auto& s : myset )
            {
              outfile << "0x" << s << " ";
              kitty::static_truth_table<4> tt;

              kitty::create_from_hex_string( tt, s );

              spec[0] = tt;

              also::img img;
              bsat_wrapper solver;
              also::img_encoder encoder( solver );

              //solver.set_time_limit( 60 * 10 );
              //solver.restart();
              
              auto res = also::implication_syn_by_img_encoder( spec, img, solver, encoder );

              if( res == success )
              {
                auto expr = img.img_to_expression();
                outfile << expr << std::endl;
              }
              else if( res == timeout )
              {
                outfile << "TIMEOUT" << std::endl;
              }
              else
              {
                assert( false );
              }
            }
            
            outfile.close();
          }
          else
          {
            kitty::dynamic_truth_table tt2s( 2 );
            kitty::dynamic_truth_table tt3s( 3 );
            also::img img;
            bsat_wrapper solver;
            also::img_encoder encoder( solver );

            std::ofstream outfile;
            outfile.open( "opt_img.txt" );

            if( !outfile ) 
            {
              std::cout << " Cannot open file " << std::endl;
              assert( false );
            }

            /* enumerate all 2-input Boolean function */
            do
            {
              spec[0] = tt2s;
              if ( also::implication_syn_by_img_encoder( spec, img, solver, encoder ) == success )
              {
                //auto s = also::img_to_string( spec, img );
                auto s = img.img_to_expression();
                outfile << "0x" << kitty::to_hex( tt2s ) << " " << s << std::endl;
              }

              kitty::next_inplace( tt2s );
            }while( !kitty::is_const0( tt2s ) );

            /* enumerate all 3-input Boolean function */
            do
            {
              spec[0] = tt3s;
              if ( also::implication_syn_by_img_encoder( spec, img, solver, encoder ) == success )
              {
                //auto s = also::img_to_string( spec, img );
                auto s = img.img_to_expression();
                outfile << "0x" << kitty::to_hex( tt3s ) << " " << s << std::endl;
              }

              kitty::next_inplace( tt3s );
            }while( !kitty::is_const0( tt3s ) );

            outfile.close();
          }
        }
        else
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
      }

    private:
      std::string filename = "test.txt";
  };

  ALICE_ADD_COMMAND( cog, "Exact synthesis" )


}

#endif
