/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file img_fc.hpp
 *
 * @brief img rewriting to solve conflict fanout
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef IMG_FC_HPP
#define IMG_FC_HPP

#include <mockturtle/mockturtle.hpp>
#include <fmt/format.h>

#include "../networks/img/img.hpp"
#include "../networks/img/img_utils.hpp"

namespace also
{
  struct img_fc_rewriting_params
  {
    bool verbose{true};
    bool allow_area_increase{true};
  };

  namespace detail
  {

    class img_mffc_impl
    {
      public:
        img_mffc_impl( img_network& img, node<img_network> n )
          : img( img ), n( n )
        {
          init_img_refs( img );
        }

        void print_mffc_info()
        {
           mffc_view mffc( img, n ); 

           std::cout << fmt::format( "mffc of node {}:\n size : {}, #pis : {}, #pos : {}, #gates : {}\n",
                                        n, mffc.size(), mffc.num_pis(), mffc.num_pos(), mffc.num_gates() );
              
           mffc.foreach_pi( [&]( auto const& nc ) { std::cout << " pi : " << nc; } );     std::cout << "\n"; 
           mffc.foreach_gate( [&]( auto const& nc ) { std::cout << " gate : " << nc; } ); std::cout << "\n\n";
        }

      private:
        img_network& img;
        node<img_network> n;
    };

    class img_fc_rewriting_impl
    {
      public:
        img_fc_rewriting_impl( img_network& img, img_fc_rewriting_params const& ps )
          : img( img ), ps( ps ), img_depth( depth_view<img_network> {img} )
        {
        }
        

        void run()
        {
          std::cout << "IMG_FC_REWRITING" << std::endl;
          std::cout << "#invs: " << img_num_inverters( img ) << std::endl; 
          std::cout << "#fcs:  " << img_fc_node_map( img ).size() << std::endl; 
          
          auto m = img_fc_node_map( img );
          print_fc_node_map( m );

          img.foreach_po( [this]( auto po ) {
              //const auto driver = ntk.get_node( po );
              //if ( ntk.level( driver ) < ntk.depth() )
              //return;
              
              img_mffc_impl p( img, img.get_node( po ) );
              p.print_mffc_info();

              topo_view topo{img, po};
              topo.foreach_node( [this]( auto n ) {
                  rewrite_constant( n );
                  return true;
                  } );
              } );

           img.foreach_gate( [this]( auto n ) { 
               img_mffc_impl p( img, n );
               p.print_mffc_info();
              } );
              
        }

      private:
        bool rewrite_constant( node<img_network> const& n )
        {
          auto b0 = rule_zero( n );

          return b0;
        }

        /* ( a -> b ) -> b = ( a -> 0 ) -> b = ( b -> 0 ) -> a */
        bool rule_zero( node<img_network> const& n )
        {
          if( img_depth.level( n ) == 0 )
            return false;

          const auto& cs = img_get_children( img, n );
          
          if( img_depth.level( img.get_node( cs[0] ) ) == 0 )
            return false;

          if( img.get_node( cs[1] ) == 0 )
            return false;

          const auto& gcs = img_get_children( img, img.get_node( cs[0] ) );

          if( img.get_node( gcs[1] ) != img.get_node( cs[1] ) )
            return false;
          
          /* child must have single fanout */
          if ( !ps.allow_area_increase && img.fanout_size( img.get_node( cs[0] ) ) != 1 )
            return false;
          
          if( ps.verbose )
          {
            std::cout << " rule zero" << std::endl;
          }
          
          auto opt = img.create_imp( img.create_not( gcs[0] ), cs[1] );
          img.substitute_node( n, opt );
          img_depth.update_levels();

          return true;
        }

        void print_fc_node_map( std::map<unsigned, std::set<node<img_network>>> const& m )
        {
          auto print = [](const node<img_network>& n) { std::cout << " " << n; };
          
          for( const auto& mc : m )
          {
            std::cout << fmt::format( "node {} has {} fanouts;\n", mc.first, mc.second.size() );
            std::for_each( mc.second.begin(), mc.second.end(), print );
            std::cout << "\n";
          }
        }

      private:
        img_network& img;
        img_fc_rewriting_params const& ps;
        depth_view<img_network> img_depth;
    };

  } /* namespace detail*/

  /* public function */
  void img_fc_rewriting( img_network& img, img_fc_rewriting_params const& ps = {} )
  {
    detail::img_fc_rewriting_impl p( img, ps );
    p.run();
  }

} /* namespace also */

#endif
