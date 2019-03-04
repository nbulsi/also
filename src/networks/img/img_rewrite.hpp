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

/**
 * @file img_rewrite.hpp
 *
 * @brief rewriting IMG
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef IMG_REWRITE_HPP
#define IMG_REWRITE_HPP

#include "img.hpp"
#include <mockturtle/algorithms/cleanup.hpp>

namespace mockturtle
{
  class img_rewrite
  {
    public:
      img_rewrite( img_network& img )
        : img( img )
      {
      }

      void run()
      {
        img.foreach_po( [this]( auto po )
            {
              const auto driver = img.get_node( po );

              topo_view topo{ img, po };

              topo.foreach_node( [this]( auto n )
                  {
                    rewrite_complement_edges_to_imply( n );
                    return true;
                  }
                  );
            }
            );
      }

      private:
      
      bool rewrite_complement_edges_to_imply( img_network::node const& n )
      {
        if( !img.is_imp( n ) )
          return false;

        auto c = get_children( img, n );

        img_network::signal a, b;

        if( img.is_complemented( c[0] ) )
        {
          c[0] = !c[0];
          a = img.create_not( c[0] );
        }
        else
        {
          a = c[0];
        }
        
        if( img.is_complemented( c[1] ) )
        {
          c[1] = !c[1];
          b = img.create_not( c[1] );
        }
        else
        {
          b = c[1];
        }

        auto opt = img.create_imp( a, b );

        img.substitute_node( n, opt );
        img.update();
        return true;
      }

    private:
      img_network& img;
  };

  img_network img_rewriting( img_network& img )
  {
    img_rewrite p( img );

    p.run();

    return cleanup_dangling( img );
  }


  void step_to_expression( const img_network& img, std::ostream& s, int index )
  {
    auto num_pis = img.num_pis();

    if( index == 0 )
    {
      s << "0";
      return;
    }
    else if( index <= num_pis )
    {
      s << char( 'a' + index - 1 );
      return;
    }

    const auto c = get_children( img, index );
    s << "(";
    step_to_expression( img, s, img.get_node( c[0] ) );
    s << "->"; 
    step_to_expression( img, s, img.get_node( c[1] ) );
    s << ")";
  }

  //print the IMPLY expression
  void img_to_expression( std::ostream& s, const img_network& img )
  {
    img.foreach_po( [&]( auto po ) 
        {
        step_to_expression( img, s, img.get_node( po ) );
        if( img.is_complemented( po ) )
        {
          s << "->0";
        }
        }
        );
    s << "\n";
  }
  
}

#endif
