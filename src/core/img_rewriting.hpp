/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file img_rewriting.hpp
 *
 * @brief implication logic network rewriting for area/depth
 * optimization
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef IMG_REWRITING_HPP
#define IMG_REWRITING_HPP

#include <array>
#include <mockturtle/mockturtle.hpp>
#include "../networks/img/img.hpp"
#include "../networks/img/img_compl_rw.hpp"

namespace mockturtle
{
  struct img_depth_rewriting_params
  {
    /*! \brief Rewriting strategy. */
    enum strategy_t
    {
      /*! \brief DFS rewriting strategy.
       *
       * Applies depth rewriting once to all output cones whose drivers have
       * maximum levels
       */
      dfs,
      /*! \brief Aggressive rewriting strategy.
       *
       * Applies depth reduction multiple times until the number of nodes, which
       * cannot be rewritten, matches the number of nodes, in the current
       * network; or the new network size is larger than the initial size w.r.t.
       * to an `overhead`.
       */
      aggressive,
      /*! \brief Selective rewriting strategy.
       *
       * Like `aggressive`, but only applies rewriting to nodes on critical paths
       * and without `overhead`.
       */
      selective
    } strategy = dfs;

    /*! \brief Overhead factor in aggressive rewriting strategy.
     *
     * When comparing to the initial size in aggressive depth rewriting, also the
     * number of dangling nodes are taken into account.
     */
    float overhead{2.0f};

    /*! \brief Allow area increase while optimizing depth. */
    bool allow_area_increase{true};

    bool verbose{false};
  };

  namespace detail
  {
    template<class Ntk>
    class img_depth_rewriting_impl
    {
      public:
        img_depth_rewriting_impl( Ntk& ntk, img_depth_rewriting_params const& ps )
          : ntk( ntk ), ps( ps )
        {
        }

        void run()
        {
          switch ( ps.strategy )
          {
            case img_depth_rewriting_params::dfs:
              run_dfs();
              break;

            case img_depth_rewriting_params::selective:
              run_selective();
              break;

            case img_depth_rewriting_params::aggressive:
              run_aggressive();
              break;

            default:
              assert( false );
              break;
          }
        }

      private:
        void run_dfs()
        {
          ntk.foreach_po( [this]( auto po ) {
              //const auto driver = ntk.get_node( po );
              //if ( ntk.level( driver ) < ntk.depth() )
              //return;
              topo_view topo{ntk, po};
              topo.foreach_node( [this]( auto n ) {
                  reduce_depth( n );
                  return true;
                  } );
              } );
        }

        void run_selective()
        {
          uint32_t counter{0};
          while ( true )
          {
            mark_critical_paths();

            topo_view topo{ntk};
            topo.foreach_node( [this, &counter]( auto n ) {
                if ( ntk.fanout_size( n ) == 0 || ntk.value( n ) == 0 )
                return;

                if ( reduce_depth( n ) )
                {
                  mark_critical_paths();
                }
                else
                {
                  ++counter;
                }
                } );

            if ( counter > ntk.size() )
              break;
          }
        }

        void run_aggressive()
        {
          uint32_t counter{0}, init_size{ntk.size()};
          while ( true )
          {
            topo_view topo{ntk};
            topo.foreach_node( [this, &counter]( auto n ) {
                if ( ntk.fanout_size( n ) == 0 )
                return;

                if ( !reduce_depth( n ) )
                {
                ++counter;
                }
                } );

            if ( ntk.size() > ps.overhead * init_size )
              break;

            if ( counter > ntk.size() )
              break;
          }
        }
        
        std::array<signal<Ntk>, 2> get_children( node<Ntk> const& n ) const
        {
          std::array<signal<Ntk>, 2> children;
          ntk.foreach_fanin( n, [&children]( auto const& f, auto i ) { children[i] = f; } );
          return children;
        }

        void mark_critical_path( node<Ntk> const& n )
        {
          if ( ntk.is_pi( n ) || ntk.is_constant( n ) || ntk.value( n ) )
            return;

          const auto level = ntk.level( n );
          ntk.set_value( n, 1 );
          ntk.foreach_fanin( n, [this, level]( auto const& f ) {
              if ( ntk.level( ntk.get_node( f ) ) == level - 1 )
              {
              mark_critical_path( ntk.get_node( f ) );
              }
              } );
        }

        void mark_critical_paths()
        {
          ntk.clear_values();
          ntk.foreach_po( [this]( auto const& f ) {
              if ( ntk.level( ntk.get_node( f ) ) == ntk.depth() )
              {
              mark_critical_path( ntk.get_node( f ) );
              }
              } );
        }

        bool rewrite_constant( node<Ntk> const& n )
        {
          /* ( a -> b ) -> b = ( a -> 0 ) -> b */
          auto b1 = reduce_depth_rule_zero( n );
          
          /* ( ( a -> b ) -> c ) -> b = ( ( a -> 0 ) -> c ) -> b */
          auto b2 = reduce_depth_rule_eight( n );

          return b1 | b2;
        }

        bool reduce_size( node<Ntk> const& n )
        {
          /* a -> ( a -> b ) = a -> b */
          auto b1 = reduce_depth_rule_one( n );
          
          /* ( a -> b ) -> a = a */
          auto b2 = reduce_depth_rule_two( n );
          
          /* ( b -> 0 ) -> ( a -> 0 ) = a -> b */
          auto b3 = reduce_depth_rule_three( n );
          
          /* ( a -> 0 ) -> 0 = a */
          auto b4 = reduce_depth_rule_five( n );
        
          /* ( a -> b ) -> ( b -> c ) = ( b -> c ) */
          auto b5 = reduce_depth_rule_six( n );
         
          /* (a -> b) -> ( a -> 0 ) = a -> ( b -> 0) = b -> ( a -> 0 ) */
          auto b6 = reduce_depth_rule_eleven( n );

          return b1 | b2 | b3 | b4 | b5 | b6;
        }

        bool reduce_depth( node<Ntk> const& n )
        {
          /* 1, rewrite some signals into constants */
          auto b1 = rewrite_constant( n );

          /* 2, size optimization */
          auto b2 = reduce_size( n );

          /* 3, depth optimization */
          /* ( a -> 0 ) -> b = ( b -> 0 ) -> a */
          auto b3 = reduce_depth_rule_four( n );
          
          /* a -> ( b -> 0 ) = b -> ( a -> 0)*/
          auto b4 = reduce_depth_rule_tweleve( n );

          /* ( a -> b ) -> 0 -> c = ( b -> 0 ) -> ( a -> c ) */
          auto b5 = reduce_depth_rule_seven( n );

          return b1 | b2 | b3 | b4 | b5; 
        }

        /* ( a -> b ) -> 0 -> c = ( b -> 0 ) -> ( a -> c ) */
        bool reduce_depth_rule_seven( node<Ntk> const& n )
        {
          if( ntk.level( n ) < 3 )
            return false;

          const auto cs = get_children( n );

          if( ntk.level( ntk.get_node( cs[0] ) ) < 2 )
            return false;

          const auto gcs = get_children( ntk.get_node( cs[0] ) );
          if( ntk.level( ntk.get_node( gcs[0] ) ) < 1 )
            return false;

          if( ntk.get_node( gcs[1] ) != 0 )
            return false;

          const auto ggcs = get_children( ntk.get_node( gcs[0] ) );

          if( ggcs[1].index == 0 )
            return false;

          if( ntk.level( ntk.get_node( cs[1] ) ) < ntk.level( ntk.get_node( ggcs[0] ) ) + 1 )
          {
            if( ps.verbose )
            {
              std::cout << " rule seven " << std::endl;
            }
            auto opt = ntk.create_imp( ntk.create_not( ggcs[1] ), ntk.create_imp( ggcs[0], cs[1] ) );
            ntk.substitute_node( n, opt );
            ntk.update_levels();
            return true;
          }

          return false;
        }
        
        /* a -> ( a -> b ) = a -> b */
        bool reduce_depth_rule_one( node<Ntk> const& n )
        {
          if( ntk.level( n ) == 0 )
            return false;

          const auto cs = get_children( n );

          if( ntk.level( ntk.get_node( cs[1] ) ) == 0 )
            return false;
          
          /* child must have single fanout, if no area overhead is allowed */
          if ( !ps.allow_area_increase && ntk.fanout_size( ntk.get_node( cs[1] ) ) != 1 )
            return false;

          const auto gcs = get_children( ntk.get_node( cs[1] ) );

          if( ntk.get_node( cs[0] ) == ntk.get_node( gcs[0] ) )
          {
            if( ps.verbose )
            {
              std::cout << " rule one " << std::endl;
            }
            auto opt = cs[1];
            ntk.substitute_node( n, opt );
            ntk.update_levels();
            return true;
          }
          
          return true;
        }

        /* ( a -> b ) -> a = a */
        bool reduce_depth_rule_two( node<Ntk> const& n )
        {
          if( ntk.level( n ) == 0 )
            return false;

          const auto cs = get_children( n );

          if( ntk.level( ntk.get_node( cs[0] ) ) == 0 )
            return false;
          
          /* child must have single fanout, if no area overhead is allowed */
          if ( !ps.allow_area_increase && ntk.fanout_size( ntk.get_node( cs[0] ) ) != 1 )
            return false;

          const auto gcs = get_children( ntk.get_node( cs[0] ) );

          if( ntk.get_node( cs[1] ) == ntk.get_node( gcs[0] ) )
          {
            if( ps.verbose )
            {
              std::cout << " rule two" << std::endl;
            }
            auto opt = cs[1];
            ntk.substitute_node( n, opt );
            ntk.update_levels();
            return true;
          }
          
          return true;
        }

        /* ( b -> 0 ) -> ( a -> 0 ) = a -> b */
        bool reduce_depth_rule_three( node<Ntk> const& n )
        {
          if( ntk.level( n ) == 0 )
            return false;

          const auto cs = get_children( n );

          if( ( ntk.level( ntk.get_node( cs[0] ) ) == 0 ) || ( ntk.level( ntk.get_node( cs[1] ) ) == 0 ) )
            return false;
          
          /* child must have single fanout, if no area overhead is allowed */
          if ( !ps.allow_area_increase && ntk.fanout_size( ntk.get_node( cs[0] ) ) != 1 )
            return false;
          
          if ( !ps.allow_area_increase && ntk.fanout_size( ntk.get_node( cs[1] ) ) != 1 )
            return false;

          const auto gcs0 = get_children( ntk.get_node( cs[0] ) );
          const auto gcs1 = get_children( ntk.get_node( cs[1] ) );

          if( ntk.get_node( gcs0[1] ) != 0 )
            return false;
          
          if( ntk.get_node( gcs1[1] ) != 0 )
            return false;

          if( ntk.get_node( gcs0[1] ) == ntk.get_node( gcs1[1] ) )
          {
            if( ps.verbose )
            {
              std::cout << " rule three" << std::endl;
              std::cout << " gcs0[0]: " << ntk.get_node( gcs0[0] ) << " gcs0[1]: " << ntk.get_node( gcs0[1] ) << std::endl;
              std::cout << " gcs1[0]: " << ntk.get_node( gcs1[0] ) << " gcs1[1]: " << ntk.get_node( gcs1[1] ) << std::endl;
            }
            
            auto opt = ntk.create_imp( gcs1[0], gcs0[0] );
            ntk.substitute_node( n, opt );
            ntk.update_levels();
            return true;
          }
          
          return true;
        }

        /* ( a -> 0 ) -> b = ( b -> 0 ) -> a, depth optimization */
        bool reduce_depth_rule_four( node<Ntk> const& n )
        {
          if( ntk.level( n ) == 0 )
            return false;
          
          const auto cs = get_children( n );

          if( ( ntk.level( ntk.get_node( cs[0] ) ) == 0 ) )
            return false;
          
          if( ( ntk.level( ntk.get_node( cs[1] ) ) == 0 ) )
            return false;

          if( ntk.get_node( cs[1] ) == 0 )
            return false;
          
          /* child must have single fanout */
          if ( !ps.allow_area_increase && ntk.fanout_size( ntk.get_node( cs[0] ) ) != 1 )
            return false;
          
          const auto gcs0 = get_children( ntk.get_node( cs[0] ) );
          
          if( ntk.get_node( gcs0[1] ) != 0 )
            return false;
          
          /* depth of the grandchild one must be higher than depth
           * of second child*/
          if( ntk.level( ntk.get_node( gcs0[0] ) ) < ntk.level( ntk.get_node( cs[1] ) ) + 1 )
            return false;
          
          if( ps.verbose )
          {
            std::cout << " rule four" << std::endl;
            std::cout << "a: " << ntk.get_node( gcs0[0] ) << " -> " << ntk.get_node( gcs0[1] ) 
                      << " -> " << ntk.get_node( cs[1] ) << std::endl;
          }

          auto opt = ntk.create_imp( ntk.create_not( cs[1] ), gcs0[0] );
          ntk.substitute_node( n, opt );
          ntk.update_levels();

          return true;
        }
        
        /* ( a -> 0 ) -> 0 = a = ( a -> 0 ) -> a */
        bool reduce_depth_rule_five( node<Ntk> const& n )
        {
          if( ntk.level( n ) == 0 )
            return false;

          const auto& cs = get_children( n );
          
          if( ntk.level( ntk.get_node( cs[0] ) ) == 0 )
            return false;

          if( ntk.get_node( cs[1] ) != 0 )
            return false;

          const auto& gcs = get_children( ntk.get_node( cs[0] ) );

          if( ntk.get_node( gcs[1] ) != 0 && ntk.get_node( gcs[0] ) != ntk.get_node( cs[1] ) )
            return false;
          
          /* child must have single fanout */
          if ( !ps.allow_area_increase && ntk.fanout_size( ntk.get_node( cs[0] ) ) != 1 )
            return false;
          
          if( ps.verbose )
          {
            std::cout << " rule five" << std::endl;
          }

          auto opt = gcs[0];
          ntk.substitute_node( n, opt );
          ntk.update_levels();

          return true;
        }
        
        /* ( a -> b ) -> b = ( a -> 0 ) -> b = ( b -> 0 ) -> a, depth optimization */
        bool reduce_depth_rule_zero( node<Ntk> const& n )
        {
          if( ntk.level( n ) == 0 )
            return false;

          const auto& cs = get_children( n );
          
          if( ntk.level( ntk.get_node( cs[0] ) ) == 0 )
            return false;

          if( ntk.get_node( cs[1] ) == 0 )
            return false;

          const auto& gcs = get_children( ntk.get_node( cs[0] ) );

          if( ntk.get_node( gcs[1] ) != ntk.get_node( cs[1] ) )
            return false;
          
          /* child must have single fanout */
          if ( !ps.allow_area_increase && ntk.fanout_size( ntk.get_node( cs[0] ) ) != 1 )
            return false;
          
          if( ps.verbose )
          {
            std::cout << " rule zero" << std::endl;
          }
          
          auto opt = ntk.create_imp( ntk.create_not( gcs[0] ), cs[1] );
          ntk.substitute_node( n, opt );
          ntk.update_levels();

          return true;
        }

        /* ( a -> b ) -> ( b -> c ) = ( b -> c ) */
        bool reduce_depth_rule_six( node<Ntk> const& n )
        {
          if( ntk.level( n ) == 0 )
            return false;
          
          const auto& cs = get_children( n );
          
          if( ntk.level( ntk.get_node( cs[0] ) ) == 0 )
            return false;
          
          if( ntk.level( ntk.get_node( cs[1] ) ) == 0 )
            return false;
          
          if ( !ps.allow_area_increase && ntk.fanout_size( ntk.get_node( cs[0] ) ) != 1 )
            return false;
          
          if ( !ps.allow_area_increase && ntk.fanout_size( ntk.get_node( cs[1] ) ) != 1 )
            return false;
          
          const auto& gcs0 = get_children( ntk.get_node( cs[0] ) );
          const auto& gcs1 = get_children( ntk.get_node( cs[1] ) );
          
          if( ntk.get_node( gcs0[1] ) == 0 )
            return false;
      
          if( ntk.get_node( gcs0[1] ) != ntk.get_node( gcs1[0] ) )
            return false;
          
          if( ps.verbose )
          {
            std::cout << " rule six" << std::endl;
          }

          auto opt = cs[1];
          ntk.substitute_node( n, opt );
          ntk.update_levels();

          return true;
        }

        /* ( ( a -> b ) -> c ) -> b = ( ( a -> 0 ) -> c ) -> b */
        bool reduce_depth_rule_eight( node<Ntk> const& n )
        {
          if( ntk.level( n ) < 3 )
            return false;

          const auto& cs = get_children( n );

          if( ntk.get_node( cs[1] ) == 0 )
            return false;

          if( ntk.level( ntk.get_node( cs[0] ) ) < 2 )
            return false;

          const auto& gcs = get_children( ntk.get_node( cs[0] ) );
          if( ntk.get_node( gcs[1] ) == 0 )
            return false;

          const auto& ggcs = get_children( ntk.get_node( gcs[0] ) );
          
          if ( !ps.allow_area_increase && ntk.fanout_size( ntk.get_node( cs[0] ) ) != 1 )
            return false;
          
          if ( !ps.allow_area_increase && ntk.fanout_size( ntk.get_node( gcs[0] ) ) != 1 )
            return false;

          if( ntk.get_node( ggcs[1] ) == ntk.get_node( cs[1] ) )
          {
            if( ps.verbose )
            {
              std::cout << " rule eight" << std::endl;
            }
            auto opt = ntk.create_imp( ntk.create_imp( 
                                       ntk.create_not( ggcs[0] ), 
                                       gcs[1] ), cs[1] );
            
            ntk.substitute_node( n, opt );
            ntk.update_levels();

            return true;
          }

          return false;
        }

        /*
         * (a -> b) -> ( a -> 0 ) = a -> ( b -> 0) = b -> ( a -> 0 )
         * */
        bool reduce_depth_rule_eleven( node<Ntk> const& n )
        {
          if( ntk.level( n ) < 2 )
            return false;

          const auto& cs = get_children( n );
          
          if( ntk.level( ntk.get_node( cs[0] ) ) == 0 )
            return false;
          
          if( ntk.level( ntk.get_node( cs[1] ) ) == 0 )
            return false;

          const auto& gcs0 = get_children( ntk.get_node( cs[0] ) );
          const auto& gcs1 = get_children( ntk.get_node( cs[1] ) );
          
          if ( !ps.allow_area_increase && ntk.fanout_size( ntk.get_node( cs[0] ) ) != 1 )
            return false;

          if( ntk.get_node( gcs0[0] ) == ntk.get_node( gcs1[0] ) && ntk.get_node( gcs1[1] ) == 0 ) 
          {
            if( ps.verbose )
            {
              std::cout << " rule eleven" << std::endl;
            }

            auto opt = ntk.level( ntk.get_node( gcs0[0] ) ) > ntk.level( ntk.get_node( gcs0[1] ) ) ?
                       ntk.create_imp( gcs0[0], ntk.create_not( gcs0[1] ) ) :
                       ntk.create_imp( gcs0[1], ntk.create_not( gcs0[0] ) );
            
            ntk.substitute_node( n, opt );
            ntk.update_levels();

            return true;
          }

          return false;
        }
        
        /*
         * a -> ( b -> 0 ) = b -> ( a -> 0)
         * */
        bool reduce_depth_rule_tweleve( node<Ntk> const& n )
        {
          if( ntk.level( n ) < 2 )
            return false;

          const auto& cs = get_children( n );
          if( ntk.level( ntk.get_node( cs[1] ) ) == 0 )
            return false;

          const auto& gcs = get_children( ntk.get_node( cs[1] ) );

          if ( !ps.allow_area_increase && ntk.fanout_size( ntk.get_node( cs[1] ) ) != 1 )
            return false;
          
          if( ntk.get_node( gcs[1] ) == 0 && 
              ntk.level( ntk.get_node( gcs[0] ) ) > ntk.level( ntk.get_node( cs[0] ) ) + 1 )
          {
            if( ps.verbose )
            {
              std::cout << " rule tweleve " << " a: " << ntk.get_node( cs[0] ) << " level: " << ntk.level( ntk.get_node( cs[0] ) ) 
                        << " b: " << ntk.get_node( gcs[0] ) << " level: " << ntk.level( ntk.get_node( gcs[0] ) ) << std::endl;
            }

            auto opt = ntk.create_imp( gcs[0], ntk.create_not( cs[0] ) );
            
            ntk.substitute_node( n, opt );
            ntk.update_levels();

            return true;
          }

          return false;
        }

      private:
        Ntk& ntk;
        img_depth_rewriting_params const& ps;
    };

  }; /* namespace detail*/


/*! \brief IMG algebraic depth rewriting.
 *
 * This algorithm tries to rewrite a network with 2-input
 * implication gates for depth
 * optimization using the  identities in
 * implication logic.  
 *
 * **Required network functions:**
 * - `get_node`
 * - `level`
 * - `update_levels`
 * - `create_imp`
 * - `substitute_node`
 * - `foreach_node`
 * - `foreach_po`
 * - `foreach_fanin`
 * - `is_imp`
 * - `clear_values`
 * - `set_value`
 * - `value`
 * - `fanout_size`
 *
   \verbatim embed:rst

   \endverbatim
 */
  template<class Ntk>
  void img_depth_rewriting( Ntk& ntk, img_depth_rewriting_params const& ps = {} )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
    static_assert( has_level_v<Ntk>, "Ntk does not implement the level method" );
    static_assert( has_substitute_node_v<Ntk>, "Ntk does not implement the substitute_node method" );
    static_assert( has_update_levels_v<Ntk>, "Ntk does not implement the update_levels method" );
    static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
    static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
    static_assert( has_clear_values_v<Ntk>, "Ntk does not implement the clear_values method" );
    static_assert( has_set_value_v<Ntk>, "Ntk does not implement the set_value method" );
    static_assert( has_value_v<Ntk>, "Ntk does not implement the value method" );
    static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );

    detail::img_depth_rewriting_impl<Ntk> p( ntk, ps );
    p.run();
  }

} /* namespace also */

#endif
