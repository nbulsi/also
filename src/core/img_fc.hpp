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

#include "misc.hpp"

namespace also
{
  struct img_fc_rewriting_params
  {
    bool verbose{true};
    bool allow_area_increase{true};
    unsigned min_cand_cut_size{1u};
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
        
        void run_all_nodes_strategy()
        {
          /* init */
          auto interlock_pairs = get_interlock_pairs( img );
          for( auto const& p : interlock_pairs )
          {
            pairs.push_back( std::make_pair( p, false ) );
          }

          conflicts = img_fc_node_map( img );

          std::cout << fmt::format( "[i] #interlock pairs: {}, #fanouts_conflict: {}\n", num_interlock_pairs(), conflicts.size() );

          if( num_interlock_pairs() == 0u )
            return;

          img.foreach_node( [&]( auto const& n, auto index ) 
              {
              if ( index >= cuts.nodes_size() || img.is_constant( n ) || img.is_pi( n ) )
              return;
              
              auto t = img.node_to_index( n );
              /* cut enumeration */
              for( auto i = 0; i < cuts.cuts( t ).size(); i++ )
              {
              /* cut view*/
              cut_view<img_network> dcut( img, get_cut_leaves( t, i ), img.make_signal( n ) );

              std::vector<node_t> pis;
              std::vector<node_t> gates;
              std::vector<node_t> nodes;

              dcut.foreach_pi(   [&]( auto const& n2) { pis.push_back( n2 ); } );
              dcut.foreach_gate( [&]( auto const& n2) { gates.push_back( n2 ); } );
              dcut.foreach_node( [&]( auto const& n2) { nodes.push_back( n2 ); } );

              /* skip trivial cases */
              if( gates.size() < 2u )
                continue;

              if( ps.verbose )
              {
                /*std::cout << " cut " << i << " tt: " << get_cut_tt( t, i ) 
                  << " img: " << tt_to_img( t, i )
                  << " #leaves: " << get_cut_leaves( t, i ).size() << std::endl;
                std::cout << fmt::format( "There are {} pis, {} internal nodes in the cut.\n", 
                                           dcut.num_pis(), dcut.num_gates() );
                show_array( pis );
                show_array( gates );
                show_array( nodes );*/
              }

              /* search the interlock pairs and rewriting */
              for( auto & p : pairs )
              {
                auto p0 = p.first[0];
                auto p1 = p.first[1];
                auto solved = p.second;
                auto c  = img_get_children( img, p0 );
                auto c0 = img.get_node( c[0] );
                auto c1 = img.get_node( c[1] );

                if( is_element_in( nodes, c0 ) && is_element_in( nodes, c1 ) 
                    && is_element_in( nodes, p0 ) && is_element_in( nodes, p1 ) && !solved )
                {
                  rwdb_full.push_back( std::make_pair( n, i ) );
                  show_array( nodes ); 
                  std::cout << fmt::format( "Rewrite node {} cuts {} to solve pairs {} and {}\n", 
                                             n, i, p0, p1 );
                  p.second = true;
                  break;
                }

              }

              } } );
          
          /* rewrite the cut */
          run_rewrite();
          
          std::cout << fmt::format( "[i] # unsolved interlock pairs: {}\n", num_interlock_pairs() );
        }

        void run_candidate_strategy()
        {
          std::cout << "IMG_FC_REWRITING" << std::endl;
          //std::cout << "#invs: " << img_num_inverters( img ) << std::endl; 
          //std::cout << "#fcs:  " << img_fc_node_map( img ).size() << std::endl; 

          /* interlock conflict pairs */
          auto pairs = get_interlock_pairs( img );
          std::cout << "[i] # conflict pairs [before]: " << pairs.size() << std::endl;

          /* begin rewrite interlock conflict pairs */ 
          for( auto const& p : pairs )
          {
            auto cand = find_node_candidates( p[0], p[1] );

            if( ps.verbose )
            {
              std::cout << fmt::format( "{} and {} are conflict pairs\n", p[0], p[1] ); 
              std::cout << "cand: " << cand << std::endl;
            }

            if( !rewrite_conflict_pairs( cand, p[0], p[1] ) )
            {
              std::cout << fmt::format( "rewriting {} and {} conflict pairs failed\n", p[0], p[1] ); 
              assert( false );
            }
          }
            
          /* rewrite the cut */
          run_rewrite();
          
          //print_cut_info();

          /* fanout pairs */
          auto m = img_fc_node_map( img );
          print_fc_node_map( m );
          

          //print_cut_info( img.index_to_node( 80 ) );
          //print_cut_info();

          //rewrite_cut( 14, 1 );
          //rewrite_cut( 18, 3 );

          img.foreach_po( [this]( auto po ) {
              topo_view topo{img, po};
              topo.foreach_node( [this]( auto n ) {
                  rewrite_constant( n );
                  } );
              } );
        }

      private:
        unsigned num_interlock_pairs()
        {
          unsigned count = 0u;

          for( auto const& p : pairs )
          {
            count += ( p.second ? 0 : 1 );
          }
          return count;
        }

        std::optional<node_t> get_reconvergent_node( node_t const& n, node_t const& n1, node_t const& n2 )
        {
          /* set all nodes are not visited */
          img.clear_visited();

          std::vector<node_t> cand_nodes;

          auto fout = get_fanout_set( img, n );
          if( !fout.empty() )
          {
            for( auto const& e : fout )
            {
              if( e == n1 || e == n2 )
              {
                dfs_util( e, cand_nodes );
              }
            }
          }

          if( !cand_nodes.empty() )
          {
            auto r = std::min_element( cand_nodes.begin(), cand_nodes.end() );
            return *r;
          }
          else
          {
            return std::nullopt;
          }
        }

        void dfs_util( node_t const& n, std::vector<node_t>& s )
        {
          img.set_visited( n, 1 ); 

          auto fout = get_fanout_set( img, n );
          for( auto const& e : fout )
          {
            if( img.visited( e ) != 1 )
            {
              dfs_util( e, s );
            }
            else
            {
              s.push_back( e );
            }
          }
        }


        bool is_element_in( std::vector<node_t> const& v, node_t const& key )
        {
          if( v.empty() ) return false;
          return std::count(v.begin(), v.end(), key) ? true : false;
        }
        
        unsigned get_element_index( std::vector<node_t> const& v, node_t const& key )
        {
          auto itr = std::find( v.begin(), v.end(), key );

          if( itr != v.cend() )
          {
            /* a = 1, b = 2, .... */
            return std::distance( v.begin(), itr ) + 1;
          }

          assert( false && "elements are not in the list" );
          return 0;
        }

        bool rewrite_conflict_pairs( node_t const& cand, node_t const& p0, node_t const& p1 )
        {
          unsigned index = 0u;
          auto c  = img_get_children( img, p0 );
          auto c0 = img.get_node( c[0] );
          auto c1 = img.get_node( c[1] );

          /* enumerate all cuts, in a cut view,
           * p0 and/or p1 are/is internal gates, c0 and/or c1
           * are/is primary input 
           * */
          if( ps.verbose )
          {
            std::cout << fmt::format( "find cuts of node {} to solve conflict pairs: {} and {}, the children inputs are {} and {}\n",
                                      cand, p0, p1, c0, c1 );
          }
          
          auto t = img.node_to_index( cand );
          /* cut enumeration */
          for( auto i = 0; i < cuts.cuts( t ).size(); i++ )
          {
            /* cut view*/
            cut_view<img_network> dcut( img, get_cut_leaves( t, i ), img.make_signal( cand ) );

            std::vector<node_t> pis;
            std::vector<node_t> gates;
            std::vector<node_t> nodes;

            dcut.foreach_pi(   [&]( auto const& n2) { pis.push_back( n2 ); } );
            dcut.foreach_gate( [&]( auto const& n2) { gates.push_back( n2 ); } );
            dcut.foreach_node( [&]( auto const& n2) { nodes.push_back( n2 ); } );

            if( ps.verbose )
            {
              std::cout << " cut " << i << " tt: " << get_cut_tt( t, i ) 
                        << " img: " << tt_to_img( t, i )
                        << " #leaves: " << get_cut_leaves( t, i ).size() << std::endl;
              std::cout << fmt::format( "There are {} pis, {} internal nodes in the cut\n", dcut.num_pis(), dcut.num_gates() );
              //show_array( pis );
              //show_array( gates );
              show_array( nodes );
            }

            /* conditions */
            /* 1, at least one children inputs are in pis */
            /* 2, at least one conflict pair nodes are in gates */
            //[TODO] research the conditions
            if( is_element_in( nodes, c0 ) && is_element_in( nodes, c1 ) 
                && is_element_in( nodes, p0 ) && is_element_in( nodes, p1 ) )
            {
                rwdb_full.push_back( std::make_pair( cand, i ) );
                return true;
            }
            /*if( is_element_in( pis, c0) && is_element_in( pis, c1 ) )
            {
              if( is_element_in( gates, p0 ) || is_element_in( gates, p1 ) ) 
              { 
                rwdb_full.push_back( std::make_pair( cand, i ) );
                return true;
              }
            }
            else if( is_element_in( pis, c0 ) )
            {
              auto pi_index = get_element_index( pis, c0 );
              
              if( ps.verbose )
              {
                std::cout << fmt::format( "Partial rewriting: the index of {} is {}\n", c0, pi_index );
              }
              
              rwdb_partial.push_back( std::make_tuple( cand, i, pi_index ) );
              return true;
            }
            else if( is_element_in( pis, c1 ) )
            {
              auto pi_index = get_element_index( pis, c1 );
              if( ps.verbose )
              {
                std::cout << fmt::format( "Partial rewriting: the index of {} is {}\n", c1, pi_index );
              }

              rwdb_partial.push_back( std::make_tuple( cand, i, pi_index ) );
              return true;
            }*/
            else
            {
              continue;
            }
          }

          return false;
        }

        void run_rewrite()
        {
          for( auto const& data : rwdb_full )
          {
            rewrite_cut( data.first, data.second );
          }

          for( auto const& data : rwdb_partial )
          {
            rewrite_cut( std::get<0>( data ), 
                         std::get<1>( data ),
                         std::get<2>( data ) );
          }
        }

        img_network create_test_img()
        {
          img_network test;

          auto a = test.create_pi();
          auto b = test.create_pi();

          auto f1 = test.create_imp( a, b );
          auto f2 = test.create_imp( b, a );
          auto f3 = test.create_imp( f1, f2 );

          test.create_po( f3 );
          return test;
        }

        inline node_t find_node_candidates( node_t const& n1, node_t const& n2 )
        {
          auto c  = img_get_children( img, n1 );
          auto c0 = img.get_node( c[0] );
          auto c1 = img.get_node( c[1] );

          auto cand = get_reconvergent_node( c0, n1, n2 );
          if( cand != std::nullopt )
          {
            return *cand;
          }
          else
          {
            return c0;
          }

#if 0 
          auto pa = get_fanout_set( img, n1 ); 
          auto pb = get_fanout_set( img, n2 );
          
          /* corner case that both n1 and n2 are primary outputs
           * without fanouts */
          if( is_po_driver( img, n1) && is_po_driver( img, n2 ) 
              && pa.empty() && pb.empty() )
          {
            return n1;
          }
          
          /* get the intersection */
          std::set<node_t> intersect;
          std::set_intersection( pa.begin(), pa.end(), pb.begin(), pb.end(),
                                 std::inserter( intersect, intersect.begin() ) );

          /* if common parent node exist, return, 
           * otherwise, find a parent node that has single fanout and return */
          if( !intersect.empty() ) 
          {
            return *intersect.begin();
          }
          else
          {
            auto fanouts_a = ( img_depth.level( n1 ) > img_depth.level( n2 ) ? pa : pb );
            auto fanouts_b = ( img_depth.level( n1 ) > img_depth.level( n2 ) ? pb : pa );

            for( auto const& f : fanouts_a )
            {
              if( img.fanout_size( f ) == 1u )
              {
                intersect.insert( f );
                break;
              }
            }

            if( intersect.empty() )
            {
              for( auto const& f : fanouts_b )
              {
                if( img.fanout_size( f ) == 1u )
                {
                  intersect.insert( f );
                  break;
                }
              }
            }

            if( intersect.empty() && !pa.empty() )
            {
              intersect.insert( *pa.begin() );
            }
            
            if( intersect.empty() && !pb.empty() )
            {
              intersect.insert( *pb.begin() );
            }
          }

          assert( !intersect.empty() );
          return *intersect.begin();
#endif
        }

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
        
        inline std::string tt_to_img( node_t const& n, unsigned const& cut_index, unsigned const& pi_index )
        {
          return nbu_cog( cuts.truth_table( cuts.cuts( n )[cut_index] ), true, true, pi_index );
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

        /*
         * create signal by exact_imply synthesis
         * */
        signal_t create_img_from_str( std::string const& s, std::vector<node_t> const&  leaves )
        {
          std::vector<signal_t> pis;

          pis.push_back( img.get_constant( false ) );
          for( const auto& l : leaves )
          {
            pis.push_back( img.make_signal( l ) );
          }

          std::stack<signal_t> inputs;

          for ( auto i = 0ul; i < s.size(); i++ )
          {
            if( s[i] == '(' )
            {
              continue;
            }
            else if( s[i] >= 'a' )
            {
              inputs.push( pis[ s[i] - 'a' + 1] );
            }
            else if( s[i] == '0' )
            {
              inputs.push( pis[0] );
            }
            else if( s[i] == ')' )
            {
              auto x1 = inputs.top();
              inputs.pop();
              
              auto x2 = inputs.top();
              inputs.pop();
              
              if( img.get_node( x1 ) == 0 )
              {
                inputs.push( img.create_not( x2 ) );
              }
              else
              {
                inputs.push( img.create_imp( x2, x1 ) );
              }
            }
          }

          assert( inputs.size() == 1u );
          return inputs.top();
        }

        void rewrite_cut( node_t const& n, unsigned const& cut_index )
        {
          auto opt = create_img_from_str( tt_to_img( n, cut_index), 
                                          get_cut_leaves( n, cut_index ) );
          img.substitute_node( n, opt );
          img_depth.update_levels();
        }
        
        void rewrite_cut( node_t const& n, unsigned const& cut_index, unsigned const& pi_index )
        {
          auto opt = create_img_from_str( tt_to_img( n, cut_index, pi_index ), 
                                          get_cut_leaves( n, cut_index ) );
          img.substitute_node( n, opt );
          img_depth.update_levels();
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
           img.foreach_node( [&]( auto node ) 
               {
                 print_cut_info( node );
               }
                );
        }

      private:
        img_network& img;
        img_fc_rewriting_params const& ps;
        cut_enumeration_params  const& cut_ps;
        depth_view<img_network> img_depth;
        network_cuts<img_network, true, empty_cut_data> cuts;
        /* record rewriting data */
        using mypair  = std::pair<node_t, unsigned>;
        using mytuple = std::tuple<node_t, unsigned, unsigned>;
        std::vector<mypair>  rwdb_full;
        std::vector<mytuple> rwdb_partial;

        /* interlock pairs */
        using myarray = std::array<node_t,2>;
        //std::set<myarray> pairs;
        std::vector<std::pair<myarray, bool>> pairs;

        /* fanout conflicts */
        std::map<unsigned, std::set<node_t>> conflicts;
    };

  } /* namespace detail*/

  /* public function */
  void img_fc_rewriting( img_network& img, img_fc_rewriting_params const& ps = {}, cut_enumeration_params const& cut_ps = {} )
  {
    detail::img_fc_rewriting_impl p( img, ps, cut_ps );
    //p.run_candidate_strategy();
    p.run_all_nodes_strategy();
  }

} /* namespace also */

#endif
