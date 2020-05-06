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
    class img_ff_rewriting_impl
    {
      public:
        img_ff_rewriting_impl( img_network& img, img_ff_rewriting_params const& ps )
          : img( img ), ps( ps )
        {
        }

        void run()
        {
          std::cout << "Begin fanout-free rewriting" << std::endl;

          std::cout << "#not gates: " << get_num_not_gates() << std::endl;
          std::cout << "#ff gates: "  << get_num_ff_gates() << std::endl;
          get_cuts();
        };

      private:
        std::array<img_network::signal, 2> get_children( img_network::node const& n ) const
        {
          std::array<img_network::signal, 2> children;
          img.foreach_fanin( n, [&children]( auto const& f, auto i ) { children[i] = f; } );
          return children;
        }

        unsigned get_num_not_gates()
        {
          unsigned num = 0u;

          topo_view topo{img};

          topo.foreach_node( [&]( auto n ){

              const auto& cs = get_children( n );

              if( img.get_node( cs[1] ) == 0 )
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

          topo_view topo{img};
          fanout_view fanout_img{topo};

          topo.foreach_node( [&]( auto n ) {
              if( img.fanout_size( n ) >= 2 && n != 0 )
              {
                num++;
                std::set<img_network::node> nodes;
                fanout_img.foreach_fanout( n, [&]( const auto& p )
                     { nodes.insert( p ); } );

                unsigned tmp = 0u;
                for( const auto& pc : nodes )
                {
                  const auto& cs = get_children( pc );
                  
                  if( img.get_node( cs[1] ) == n )
                  {
                    tmp++;
                  }
                }

                if( tmp >= 2u )
                {
                  num_ff++;
                  ff_node_map.insert( std::pair<unsigned, std::set<img_network::node>>( n, nodes ) );
                }
              }
              } );
          
          return ff_node_map.size();
        }

        void get_cuts()
        { 
          cut_enumeration_params ps;
          ps.cut_size = 3;
          const auto cuts = cut_enumeration<img_network, true>( img, ps );  
          const auto to_vector = []( auto const& cut ) {
            return std::vector<uint32_t>( cut.begin(), cut.end() );
          };

          /*const auto t = ntk.node_to_index( 60 );
          const auto t = img.node_to_index( 10 );

          std::cout << "There are " << cuts.cuts( t ).size() << " cuts for node 10" << std::endl;

          for( auto i = 0; i < cuts.cuts( t ).size(); i++ )
          {
            std::cout << " cut " << i << " tt: " << std::hex << cuts.truth_table( cuts.cuts( t )[i] )._bits[0] << std::endl;

            for( const auto& l : to_vector( cuts.cuts( t )[i] ) )
            {
              std::cout << " " << l;
            }
            std::cout << std::endl;
          }*/
        }
      
      private:
        img_network& img;
        img_ff_rewriting_params const& ps;
        std::map<unsigned,std::set<img_network::node>> ff_node_map;
    };
  }; /* namespace detail*/
  
  void img_ff_rewriting( img_network& img, img_ff_rewriting_params const& ps = {} )
  {
    detail::img_ff_rewriting_impl p( img, ps );
    p.run();
  }


}

#endif
