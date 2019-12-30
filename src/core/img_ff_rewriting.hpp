/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file img_ff_rewriting.hpp
 *
 * @brief imgplication logic network rewriting for fanout-conflict
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef IMG_FF_REWRITING_HPP
#define IMG_FF_REWRITING_HPP

#include <mockturtle/mockturtle.hpp>
#include "../networks/img/img.hpp"

namespace also
{

  struct img_ff_rewriting_params
  {
    bool allow_area_increase{true};
    bool verbose{false};
  };

  namespace detail
  {
    template<class Ntk>
    class img_ff_rewriting_impl
    {
      public:
        img_ff_rewriting_impl( Ntk& ntk, img_ff_rewriting_params const& ps )
          : ntk( ntk ), ps( ps )
        {
        }

        void run()
        {
          std::cout << "Begin fanout-free rewriting" << std::endl;

          std::cout << "#not gates: " << get_num_not_gates() << std::endl;
          std::cout << "#ff gates: "  << get_num_ff_gates() << std::endl;
        };

      private:
        std::array<signal<Ntk>, 2> get_children( node<Ntk> const& n ) const
        {
          std::array<signal<Ntk>, 2> children;
          ntk.foreach_fanin( n, [&children]( auto const& f, auto i ) { children[i] = f; } );
          return children;
        }

        unsigned get_num_not_gates()
        {
          unsigned num = 0u;

          topo_view topo{ntk};

          topo.foreach_node( [&]( auto n ){

              const auto& cs = get_children( n );

              if( ntk.get_node( cs[1] ) == 0 )
              {
                num++;
              }
              } );
          
          return num;
        }
        
        /* get the number of fanout-conflict gates */
        unsigned get_num_ff_gates()
        {
          unsigned num = 0u;
          unsigned num_ff = 0u; //2 or more fanout are connecting the working memristor

          topo_view topo{ntk};
          fanout_view fanout_ntk{topo};

          topo.foreach_node( [&]( auto n ){
              if( ntk.fanout_size( n ) >= 2 )
              {
                num++;
                std::set<node<Ntk>> nodes;
                fanout_ntk.foreach_fanout( n, [&]( const auto& p ){ std::cout << " " << p; nodes.insert( p ); } );
                std::cout << std::endl;

                unsigned tmp = 0u;
                for( const auto& pc : nodes )
                {
                  const auto& cs = get_children( pc );
                  
                  if( ntk.get_node( cs[1] ) == n )
                  {
                    tmp++;
                  }
                }

                if( tmp >= 2u )
                {
                  num_ff++;
                  std::cout << " node " << n << " has " << ntk.fanout_size( n ) << " fanouts. " << std::endl;
                }
              }
              } );
          
          return num_ff;
        }
      
      private:
        Ntk& ntk;
        img_ff_rewriting_params const& ps;
    };
  }; /* namespace detail*/
  
  template<class Ntk>
  void img_ff_rewriting( Ntk& ntk, img_ff_rewriting_params const& ps = {} )
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

    detail::img_ff_rewriting_impl<Ntk> p( ntk, ps );
    p.run();
  }


}

#endif
