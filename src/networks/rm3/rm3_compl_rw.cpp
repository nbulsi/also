#include "rm3_compl_rw.hpp"
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/views/topo_view.hpp>
#include <percy/percy.hpp>

namespace also
{
/******************************************************************************
 * Types                                                                      *
 ******************************************************************************/
  class rm3_rewrite
  {
    public:
      rm3_rewrite( rm3_network& rm3 )
        :rm3(rm3)
      {

      }

      void run()
      {
        rm3.foreach_po( [this]( auto po )
                        {
              topo_view topo{ rm3, po };
              
              topo.foreach_node( [this]( auto n )
                  {
                    rewrite_complement_edges_to_rm3( n );
                    return true;
                  }
                  ); } );
      }

      private:
        bool rewrite_complement_edges_to_rm3( rm3_network::node const& n )
        {
        if ( !rm3.is_rm3( n ) )
          return false;

        auto child = get_children( rm3, n );

        rm3_network::signal a, b, c;

        if ( rm3.is_complemented( child[0] ) )
        {
          child[0] = !child[0];
          a = rm3.create_not( child[0] );
        }
        else
        {
          a = child[0];
        }

        if ( rm3.is_complemented( child[1] ) )
        {
          child[1] = !child[1];
          b = rm3.create_not( child[1] );
        }
        else
        {
          b = child[1];
        }

        if ( rm3.is_complemented( child[2] ) )
        {
          child[2] = !child[2];
          c = rm3.create_not( child[2] );
        }
        else
        {
          c = child[2];
        }

        auto opt = rm3.create_rm3( a, b, c );

        rm3.substitute_node( n, opt );
        return true;
        }

      private:
        rm3_network& rm3;
  };

/******************************************************************************
* Private functions                                                          *
******************************************************************************/
  void rm3_rewriting_po( rm3_network& rm3 )
  {
    rm3.foreach_po( [&]( auto const& f, auto index ) {
        if( rm3.is_complemented( f ) )
        {
          std::cout << "rewriting IO is required." << std::endl;
        }
        } );
  }

/******************************************************************************
* Public functions                                                           *
******************************************************************************/
  std::array<rm3_network::signal, 3> get_children( const rm3_network& rm3, rm3_network::node const& n )
  {
    std::array<rm3_network::signal, 3> children;
    rm3.foreach_fanin( n, [&children]( auto const& f, auto i )
                       { children[i] = f; } );
    return children;
  }

  rm3_network rm3_rewriting( rm3_network& rm3 )
  {
    rm3_rewrite p( rm3 );

    p.run();

    rm3 = cleanup_dangling( rm3 );

    rm3_rewriting_po( rm3 );

    return cleanup_dangling( rm3 );
  }

  void step_to_expression( const rm3_network& rm3, std::ostream& s, int index )
  {
    auto num_pis = rm3.num_pis();

    //怎么同时定义1和0
    if ( index == 0 )
    {     
        s << "0";
        return;
    }
    // else if ( index == 1 )
    // {
    //     s << "1";
    //     return;
    // }

    else if ( index <= num_pis )
    {
        s << char( 'a' + index - 1 );
        return;
    }

    const auto child = get_children( rm3, index );
    s << "<";
    if ( rm3.is_complemented( child[0] ) )
    {
      if ( child[0].index == 0 )
      {
        s << "1";
        //return;
        }
      else
      {
        s << "<0!";
        step_to_expression( rm3, s, rm3.get_node( child[0] ) );
        s << "1>";
      }
    }
    else
    {
        step_to_expression( rm3, s, rm3.get_node( child[0] ) );
    }
    //s << "!";
    if ( rm3.is_complemented( child[1] ) )
    {
      if ( child[1].index == 0 )
      {
        s << "1";
        //return;
      }
      else
      {
        s << "<0!";
        step_to_expression( rm3, s, rm3.get_node( child[1] ) );
        s << "1>";
      }
        // s << "<0!";
        // step_to_expression( rm3, s, rm3.get_node( child[1] ) );
        // s << "1>";
    }
    else
    {
        step_to_expression( rm3, s, rm3.get_node( child[1] ) );
    }
    if ( rm3.is_complemented( child[2] ) )
    {
      if ( child[2].index == 0 )
      {
        s << "1";
        //return;
      }
      else
      {
        s << "<0!";
        step_to_expression( rm3, s, rm3.get_node( child[2] ) );
        s << "1>";
      }
    }
    else
    {
        step_to_expression( rm3, s, rm3.get_node( child[2] ) );
    }
    s << ">";

    //printf( "child_%d%d%d\n", child[0], child[1], child[2] );
  }

  // print the rm3 expression
  void rm3_to_expression( std::ostream& s, const rm3_network& rm3 )
  {
    rm3.foreach_po( [&]( auto po )
                    {
        //step_to_expression( rm3, s, rm3.get_node( po ) );
        if( rm3.is_complemented( po ) )
        {
          s << "<0!";
          step_to_expression( rm3, s, rm3.get_node( po ) );
          s << "1>";
        }
        else
        {
          step_to_expression( rm3, s, rm3.get_node( po ) );
        }
         } );
    s << "\n";
  }

}