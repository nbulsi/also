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
#include <mockturtle/utils/cuts.hpp>
#include <mockturtle/algorithms/cut_enumeration.hpp>
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

    using node_t   = node<img_network>;
    using signal_t = signal<img_network>;
    
    /*
     * img node mffc view
     * */
    class img_mffc_details
    {
      public:
        img_mffc_details( img_network& img, node_t n )
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
        node_t n;
    };

    class img_fc_rewriting_impl
    {
      public:
        img_fc_rewriting_impl( img_network& img, img_fc_rewriting_params const& ps, cut_enumeration_params const& cut_ps )
          : img( img ), ps( ps ), cut_ps( cut_ps), 
            img_depth( depth_view<img_network> {img} ), cuts( cut_enumeration<img_network, true>( img, cut_ps ) )
        {
        }
        

        void run()
        {
          std::cout << "IMG_FC_REWRITING" << std::endl;
          std::cout << "#invs: " << img_num_inverters( img ) << std::endl; 
          std::cout << "#fcs:  " << img_fc_node_map( img ).size() << std::endl; 

          auto m = img_fc_node_map( img );
          print_fc_node_map( m );

          //print_cut_info( img.index_to_node( 80 ) );
          print_cut_info();

          img.foreach_po( [this]( auto po ) {
              topo_view topo{img, po};
              topo.foreach_node( [this]( auto n ) {
                  rewrite_constant( n );
                  } );
              } );

           auto pairs = get_interlock_pairs( img );
        }

      private:
        bool rewrite_constant( node_t const& n )
        {
          auto b0 = rule_zero( n );
          auto b1 = rule_one( n );
          auto b2 = rule_two( n );
          auto b3 = rule_three( n );

          return b0 | b1 | b2 | b3;
        }

        /* ( a -> b ) -> b = ( a -> 0 ) -> b = ( b -> 0 ) -> a */
        bool rule_zero( node_t const& n )
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
            std::cout << fmt::format( " root node {}, a = {}, b = {} satisfy rule zero.\n", 
                                        n, img.get_node( gcs[0] ), img.get_node( cs[1] ) );
          }
          
          auto opt = img.create_imp( img.create_not( gcs[0] ), cs[1] );
          img.substitute_node( n, opt );
          img_depth.update_levels();

          return true;
        }

        /* ( ( b -> ( a -> c) ) -> c ) = ( ( b -> ( a -> 0 ) ) -> c )*/
        bool rule_one( node_t const& n )
        {
          if( img_depth.level( n ) < 3 )
            return false;

          const auto& cs = img_get_children( img, n );

          if( img_depth.level( img.get_node( cs[0] ) ) < 2 )
            return false;

          if( img.get_node( cs[1] ) == 0 )
            return false;

          const auto& gcs = img_get_children( img, img.get_node( cs[0] ) );
          
          if( img_depth.level( img.get_node( gcs[1] ) ) < 1 )
            return false;

          const auto& ggcs = img_get_children( img, img.get_node( gcs[1] ) );

          if( img.get_node( ggcs[1] ) != img.get_node( cs[1] ) )
            return false;
          
          /* child must have single fanout */
          if ( !ps.allow_area_increase && img.fanout_size( img.get_node( cs[0] ) ) != 1 )
            return false;
          
          if ( !ps.allow_area_increase && img.fanout_size( img.get_node( gcs[1] ) ) != 1 )
            return false;
          
          if( ps.verbose )
          {
            std::cout << fmt::format( " root node {}, a = {}, b = {}, c = {} satisfy rule two. \n", 
                                       n, img.get_node( ggcs[0] ), img.get_node( gcs[0]), img.get_node( cs[1] ) );
          }
          
          auto opt = img.create_imp( img.create_imp( gcs[0], img.create_not( ggcs[0] ) ), cs[1] );
          img.substitute_node( n, opt );
          img_depth.update_levels();

          return true;
        }
        
        /* ( ( ( a -> c ) -> b ) -> c ) = ( ( ( a -> 0 ) -> b ) -> c ) */
        bool rule_two( node_t const& n )
        {
          if( img_depth.level( n ) < 3 )
            return false;

          const auto& cs = img_get_children( img, n );

          if( img_depth.level( img.get_node( cs[0] ) ) < 2 )
            return false;

          if( img.get_node( cs[1] ) == 0 )
            return false;

          const auto& gcs = img_get_children( img, img.get_node( cs[0] ) );
          
          if( img_depth.level( img.get_node( gcs[0] ) ) < 1 )
            return false;

          const auto& ggcs = img_get_children( img, img.get_node( gcs[0] ) );

          if( img.get_node( ggcs[1] ) != img.get_node( cs[1] ) )
            return false;
          
          /* child must have single fanout */
          if ( !ps.allow_area_increase && img.fanout_size( img.get_node( cs[0] ) ) != 1 )
            return false;
          
          if ( !ps.allow_area_increase && img.fanout_size( img.get_node( gcs[0] ) ) != 1 )
            return false;
          
          if( ps.verbose )
          {
            std::cout << fmt::format( " root node {}, a = {}, b = {}, c = {} satisfy rule two. \n", 
                                       n, img.get_node( ggcs[0] ), img.get_node( gcs[1]), img.get_node( cs[1] ) );
          }
          
          auto opt = img.create_imp( img.create_imp( img.create_not( ggcs[0] ), gcs[1] ), cs[1] );
          img.substitute_node( n, opt );
          img_depth.update_levels();

          return true;
        }
        
        /*  ( ( a -> b ) -> ( c -> b ) ) =  ( ( a -> 0 ) -> ( c -> b ) ) */
        bool rule_three( node_t const& n )
        {
          if( img_depth.level( n ) < 2 )
            return false;

          const auto& cs = img_get_children( img, n );

          if( img_depth.level( img.get_node( cs[0] ) ) < 1 )
            return false;
          
          if( img_depth.level( img.get_node( cs[1] ) ) < 1 )
            return false;

          const auto& gcs1 = img_get_children( img, img.get_node( cs[0] ) );
          const auto& gcs2 = img_get_children( img, img.get_node( cs[1] ) );

          if( img.get_node( gcs1[1] ) == 0 ||
              img.get_node( gcs2[1] ) == 0 )
            return false;
          
          if( img.get_node( gcs1[1] ) != img.get_node( gcs2[1] ) )
            return false;
          
          /* child must have single fanout */
          if ( !ps.allow_area_increase && img.fanout_size( img.get_node( cs[0] ) ) != 1 )
            return false;
          
          if ( !ps.allow_area_increase && img.fanout_size( img.get_node( cs[1] ) ) != 1 )
            return false;
          
          if( ps.verbose )
          {
            std::cout << fmt::format( " root node {}, a = {}, b = {}, c = {} satisfy rule three. \n", 
                                       n, img.get_node( gcs1[0] ), img.get_node( gcs1[1]), img.get_node( gcs2[0] ) );
          }
          
          auto opt = img.create_imp( img.create_not( gcs1[0] ), img.create_imp( gcs2[0], gcs2[1] ) );
          img.substitute_node( n, opt );
          img_depth.update_levels();

          return true;
        }

        void print_fc_node_map( std::map<unsigned, std::set<node_t>> const& m )
        {
          auto print = [](const node_t& n) { std::cout << " " << n; };
          
          for( const auto& mc : m )
          {
            std::cout << fmt::format( "node {} has {} fanouts;\n", mc.first, mc.second.size() );
            std::for_each( mc.second.begin(), mc.second.end(), print );
            std::cout << "\n";
          }
        }

        inline std::string get_cut_tt( node_t const& n, unsigned const& cut_index )
        {
          assert( cut_index < cuts.cuts( n ).size() );
          return kitty::to_hex( cuts.truth_table( cuts.cuts( n )[cut_index] ) );
        }

        inline std::string tt_to_img( node_t const& n, unsigned const& cut_index )
        {
          return nbu_cog( cuts.truth_table( cuts.cuts( n )[cut_index] ), true );
        }

        inline std::vector<node_t> get_cut_leaves( node_t const& n, unsigned const& cut_index )
        {
          std::vector<node_t> leaves;

          for( auto leaf_index : cuts.cuts( n )[cut_index] )
          {
            leaves.push_back( img.index_to_node( leaf_index ) );
          }

          return leaves;
        }

        void print_cut_info( node_t const& n )
        {
          auto t = img.node_to_index( n );
          std::cout << cuts.cuts( t ) << "\n";
          for( auto i = 0; i < cuts.cuts( t ).size(); i++ )
          {
            std::cout << " cut " << i << " tt: " << get_cut_tt( t, i ) 
                      << " img: " << tt_to_img( t, i )
                      << " #leaves: " << get_cut_leaves( t, i ).size() << std::endl;

            /* cut view*/
            cut_view<img_network> dcut( img, get_cut_leaves( t, i ), img.make_signal( n ) );

            dcut.foreach_node( [&]( auto const& n2) 
                {
                std::cout << " node: " << n2;
                }
                );
            std::cout << "\n";
          }
        }

        void print_cut_info()
        {
           img.foreach_node( [&]( auto node ) {
              print_cut_info( node );
               }
                );
        }

      private:
        img_network& img;
        img_fc_rewriting_params const& ps;
        cut_enumeration_params const& cut_ps;
        depth_view<img_network> img_depth;
        network_cuts<img_network, true, empty_cut_data> cuts;
    };

  } /* namespace detail*/

  /* public function */
  void img_fc_rewriting( img_network& img, img_fc_rewriting_params const& ps = {}, cut_enumeration_params const& cut_ps = {} )
  {
    detail::img_fc_rewriting_impl p( img, ps, cut_ps );
    p.run();
  }

} /* namespace also */

#endif
