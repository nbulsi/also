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
#include "../core/exact_rm3ig_encoder.hpp"
#include "../core/m5ig_helper.hpp"
#include "../core/m3ig_helper.hpp"
#include "../core/misc.hpp"
#include "../core/exact_aoig.hpp"
#include "../commands/exact_rm3.hpp"

namespace alice
{

  class cog_command: public command
  {
    public:
      explicit cog_command( const environment::ptr& env ) : command( env, "compute optimal m5ig" )
      {
        add_option( "filename, -f", filename, "the input txt file name" );
        add_option( "tt_size, -s", tt_size,   "the input size of tt" );
        add_flag(   "--m3ig, -m", "using m3ig as the underlying logic network" );
        add_flag( "--rm3, -r", "using rm3ig as the underlying logic network" );
        add_flag(   "--img, -i",  "using img as the underlying logic network" );
        add_flag(   "--aoig, -a",  "using aoig as the underlying logic network" );
        add_flag(   "--fanout_free, -g",  "add fanout clauses to realize a fanout-free img" );
        add_flag(   "--pclassfication, -p",  "using p classfication" );
      }

    protected:
      void execute()
      {
        spec spec;

        bool enable_fanout_clauses = false;
        //bool verb = false;
       // auto& opt = store<optimum_network>().current();

        if( is_set( "fanout_free" ) )
        {
          enable_fanout_clauses = true;
          spec.add_fanout_clauses = true;
        }

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
            

            unsigned count_top_down{0};
            unsigned count_bottom_up{0};
            unsigned count_bi{0};

            for( const auto& s : myset )
            {
              outfile << "0x" << s << " ";
              kitty::dynamic_truth_table tt( 4 );

              kitty::create_from_hex_string( tt, s );

              /*
               * try top decomposition
               * */

              kitty::dynamic_truth_table expr( 4 );
              bool flag = false;

              for( auto i = 0u; i < 4u; i++ )
              {
                auto r = kitty::is_top_decomposable( tt, i, &expr );

                if( r != kitty::top_decomposition::none )
                {
                  std::cout << s << " top decomposition is possibile at input index " << i << std::endl;
                  count_top_down++;
                  flag = true;
                  break;
                }
              }

              if( !flag )
              {
                bool flag_found = false;

                for( auto i = 0u; i < 3u && !flag_found; i++ )
                {
                  for( auto j = i + 1; j < 4u && !flag_found; j++ )
                  {
                    auto r = kitty::is_bottom_decomposable( tt, i, j, &expr, true );

                    if( r != kitty::bottom_decomposition::none )
                    {
                      std::cout << s << " bottom decomposition is possibile at input index " << i << " and " << j << std::endl;
                      count_bottom_up++;
                      flag_found = true;
                      flag = true;
                    }
                  }
                }
              }

              if( !flag )
              {
                auto r = kitty::is_bi_decomposable( tt, expr );
                if ( std::get<1>( r ) != kitty::bi_decomposition::none )
                {
                  std::cout << s << " bidecomposition is possibile " << std::endl;
                  count_bi++;
                  flag = true;
                }
              }

              outfile << also::nbu_cog( tt, enable_fanout_clauses ) << std::endl;
            }
            
            std::cout << "There are " << count_top_down  << " functions can be top down decomposed " << std::endl;
            std::cout << "There are " << count_bottom_up << " functions can be bottom up decomposed " << std::endl;
            std::cout << "There are " << count_bi << " functions can be bi-decomposed " << std::endl;
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
        else if( is_set( "aoig" ) )
        {
            kitty::dynamic_truth_table tt4s( tt_size );
            int count = 0;

            std::unordered_set<std::string> myset;
            
            /* enumerate all 4-input Boolean function */
            do
            {
              count++;

              auto res = kitty::exact_npn_canonization( tt4s );
              
              myset.insert( kitty::to_hex( std::get<0>( res ) ) );

              kitty::next_inplace( tt4s );
            }while( !kitty::is_const0( tt4s ) );

            std::cout << "There are " << count << " functions. " << " The distinct representive tt size: " << myset.size() << std::endl;
            
            std::ofstream outfile;
            outfile.open( "opt_aoig.txt" );

            if( !outfile ) 
            {
              std::cout << " Cannot open file " << std::endl;
              assert( false );
            }
            

            for( const auto& s : myset )
            {
              outfile << "0x" << s << " ";
              kitty::dynamic_truth_table tt( tt_size );

              kitty::create_from_hex_string( tt, s );

              also::tt2aoig( tt );

              //outfile << also::nbu_cog( tt, enable_fanout_clauses ) << std::endl;
              outfile << "s" << std::endl;
            }
            
            outfile.close();
        }
        else if ( is_set( "rm3" ) )
        {
          std::ifstream infile( filename );

          std::ofstream outfile;

          outfile.open( "opt_rm3.txt" );

          kitty::dynamic_truth_table tt4s( 4 );

          if ( !infile || !outfile )
          {
            std::cout << " Cannot open file " << std::endl;
            assert( false );
          }

          std::string line;
          while ( std::getline( infile, line ) )
          {
            std::string f = line;
            f.replace( f.begin(), f.begin() + 2, "" );

            const unsigned num_vars = ::log( f.size() * 4 ) / ::log( 2.0 );

            kitty::dynamic_truth_table t( num_vars );
            kitty::create_from_hex_string( t, f );

            /* rm three synthesize */
            also::rm3ig rm3ig;
            bsat_wrapper solver;
            also::rm_three_encoder encoder( solver );

            if ( num_vars < 3 )
            {
              spec[0] = kitty::extend_to( t, 3 );
            }
            else
            {
              spec[0] = t;
            }

            if ( also::rm_three_synthesize( spec, rm3ig, solver, encoder ) == success )
            {
              auto s = also::rm3ig_to_string( spec, rm3ig );
              outfile << "0x" << f << " " << s << std::endl;
            }
            // int min_gates = 0;

            // also::best_rm3ig( opt.function, verb, min_gates );
            // auto s = also::rm3ig_to_string( spec, rm3ig );
            // outfile << "0x" << f << " " << s << std::endl;
            
          }
          
          infile.close();
          outfile.close();
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
      unsigned tt_size = 4u;
  };

  ALICE_ADD_COMMAND( cog, "Exact synthesis" )


}

#endif
