/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

#include "img_compl_rw.hpp"

using namespace mockturtle;

namespace also
{

/******************************************************************************
 * Types                                                                      *
 ******************************************************************************/
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

/******************************************************************************
 * Private functions                                                          *
 ******************************************************************************/
  void img_rewriting_po( img_network& img )
  {
    img.foreach_po( [&]( auto const& f, auto index ) {
        if( img.is_complemented( f ) )
        {
          std::cout << "rewriting IO is required." << std::endl;
        }
        } );

  }

/******************************************************************************
 * Public functions                                                           *
 ******************************************************************************/
  std::array<img_network::signal, 2> get_children( const img_network& img, img_network::node const& n ) 
  {
    std::array<img_network::signal, 2> children;
    img.foreach_fanin( n, [&children]( auto const& f, auto i ) { children[i] = f; } );
    return children;
  }

  img_network img_rewriting( img_network& img )
  {
    img_rewrite p( img );

    p.run();

    img = cleanup_dangling( img );

    img_rewriting_po( img );

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
    if( img.is_complemented( c[0] ) )
    {
      s << "(";
      step_to_expression( img, s, img.get_node( c[0] ) );
      s << "->0)";
    }
    else
    {
      step_to_expression( img, s, img.get_node( c[0] ) );
    }
    s << "->"; 
    if( img.is_complemented( c[1] ) )
    {
      s << "(";
      step_to_expression( img, s, img.get_node( c[1] ) );
      s << "->0)";
    }
    else
    {
      step_to_expression( img, s, img.get_node( c[1] ) );
    }
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
