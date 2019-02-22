/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China 
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
#include <percy/percy.hpp>

#include "exact_img.hpp"
#include "../networks/img.hpp"
#include "../networks/img_rewrite.hpp"

using namespace percy; 
using namespace mockturtle; 

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
  img_network img_from_chain( chain& c )
  {
    img_network img;
    int nr_in = c.get_nr_inputs();

    std::vector<img_network::signal> sigs;

    sigs.push_back( img.get_constant( false ) );

    for( int i = 0; i < nr_in; i++ )
    {
      sigs.push_back( img.create_pi() );
    }

    for( size_t i = 0; i < c.get_nr_steps(); i++ )
    {
      const auto& step = c.get_step( i );
      const auto  idx  = nr_in + i + 1;
      const auto op = c.get_operator( i );
      //std::cout << "node " << idx << " inputs: " << step[0] + 1 << "  " << step[1] + 1 << " op: " << kitty::to_binary( op ) << std::endl;

      auto ops = kitty::to_binary( op );
      if( ops == "0010" )
      {
        sigs.push_back( img.create_op1( sigs[ step[0] + 1 ], 
                                        sigs[ step[1] + 1] ) );
      }
      else if( ops == "1110" )
      {
        sigs.push_back( img.create_op2( sigs[ step[0] + 1 ], 
                                        sigs[ step[1] + 1] ) );
      }
      else if( ops == "1000" )
      {
        sigs.push_back( img.create_op3( sigs[ step[0] + 1 ], 
                                        sigs[ step[1] + 1] ) );
      }
      else if( ops == "0100" )
      {
        sigs.push_back( img.create_op4( sigs[ step[0] + 1 ], 
                                        sigs[ step[1] + 1] ) );
      }
      else
      {
        assert( false && "UNKNOWN OPS" );
      }
    }

    auto top = c.is_output_inverted( 0 ) ? !sigs[ sigs.size() - 1 ] : sigs[ sigs.size() - 1 ];

    img.create_po( top );
    return img;
  }

  void img_syn( const kitty::dynamic_truth_table& tt, const bool& verbose )
  {
    chain c;
    spec spec;
    spec.verbosity = 0;

    bsat_wrapper solver;
    ssv_encoder encoder( solver );

    spec.fanin = 2; //each node has at most 2 fanins, while it equals 3 for MAJ synthesis
    spec.set_primitive( AIG );
    spec[0] = tt;

    if( verbose )
    {
      std::cout << "Synthesizing function " << kitty::to_hex( spec[0] ) << std::endl;
    }

    //synthesis 
    int nr_solutions = 0;
    while (next_solution(spec, c, solver, encoder) == success) 
    {
      printf("\ngot solution: ");
      c.print_expression();
      auto tts = c.simulate();
      assert( tts[0] == spec[0] );
      assert( c.is_aig() );
      printf("\n");
      nr_solutions++;

      auto r = c;
      auto img = img_from_chain( r );
      printf( "IMG size: %d\n", img.num_gates() );

      auto img_new = img_rewriting( img );
      printf( "NEW IMG size: %d\n", img_new.num_gates() );
      img_to_expression( std::cout, img_new);

    }
    printf("\nfound %d solutions\n", nr_solutions);

  }

}
