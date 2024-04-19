/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file exact_online_m5ig.hpp
 *
 * @brief Replace the network with exact m5ig result (computing online)
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef EXACT_ONLINE_M5IG_HPP
#define EXACT_ONLINE_M5IG_HPP

#include <mockturtle/mockturtle.hpp>

#include "../../core/exact_m5ig_encoder.hpp"
#include "../../core/m5ig_helper.hpp"

namespace mockturtle
{
  struct exact_m5ig_resynthesis_params
  {
    using cache_map_t = std::unordered_map<kitty::dynamic_truth_table, also::mig5, kitty::hash<kitty::dynamic_truth_table>>;
    using cache_t = std::shared_ptr<cache_map_t>;

    cache_t cache;

    bool add_alonce_clauses{true};
    bool add_colex_clauses{true};
    int conflict_limit{0};
  };
  
  template<class Ntk = m5ig_network>
    class exact_m5ig_resynthesis
    {
      public:
        explicit exact_m5ig_resynthesis( exact_m5ig_resynthesis_params const& ps = {} )
          : _ps( ps )
          {
          }

        template<typename LeavesIterator, typename Fn>
          void operator()( Ntk& ntk, 
                           kitty::dynamic_truth_table const& function, 
                           LeavesIterator begin, 
                           LeavesIterator end, 
                           Fn&& fn )
          {
            operator()( ntk, function, function.construct(), begin, end, fn );
          }

        template<typename LeavesIterator, typename Fn>
          void operator()( Ntk& ntk, 
                           kitty::dynamic_truth_table const& function, 
                           kitty::dynamic_truth_table const& dont_cares, 
                           LeavesIterator begin, 
                           LeavesIterator end, 
                           Fn&& fn )
          {
            // TODO: special case for small functions (up to 2 variables)?

            percy::spec spec;
            spec.fanin = 5;
            spec.verbosity = 0;
            spec.add_alonce_clauses = _ps.add_alonce_clauses;
            spec.add_colex_clauses = _ps.add_colex_clauses;
            spec.conflict_limit = _ps.conflict_limit;
            spec[0] = function;
            bool with_dont_cares{false};
            if ( !kitty::is_const0( dont_cares ) )
            {
              spec.set_dont_care( 0, dont_cares );
              with_dont_cares = true;
            }

            auto c = [&]() -> std::optional<also::mig5> {
              if ( !with_dont_cares && _ps.cache )
              {
                const auto it = _ps.cache->find( function );
                if ( it != _ps.cache->end() )
                {
                  return it->second;
                }
              }

              also::mig5 c;
              if ( const auto result = parallel_mig_five_fence_synthesize( spec, c ); result != percy::success )
              {
                return std::nullopt;
              }

              if ( !with_dont_cares && _ps.cache )
              {
                ( *_ps.cache )[function] = c;
              }
              return c;
            }();

            if ( !c )
            {
              return;
            }

            std::vector<signal<Ntk>> signals( begin, end );

            for( auto i = 0; i < c->get_nr_steps(); i++ )
            {
              auto array = c->get_step_inputs( i );
              auto c1 = signals[array[0]];
              auto c2 = signals[array[1]];
              auto c3 = signals[array[2]];
              auto c4 = signals[array[3]];
              auto c5 = signals[array[4]];

              switch( c->get_op( i ) )
              {
                default:
                  std::cerr << "[e] unsupported operation " << c->get_op(i) << "\n";
                  break;

                case 0:
                  signals.emplace_back( ntk.create_maj5( c1, c2, c3, c4, c5 ) );
                  break;

                case 1:
                  signals.emplace_back( ntk.create_maj5( !c1, c2, c3, c4, c5 ) );
                  break;

                case 2:
                  signals.emplace_back( ntk.create_maj5( c1, !c2, c3, c4, c5 ) );
                  break;

                case 3:
                  signals.emplace_back( ntk.create_maj5( c1, c2, !c3, c4, c5 ) );
                  break;

                case 4:
                  signals.emplace_back( ntk.create_maj5( c1, c2, c3, !c4, c5 ) );
                  break;

                case 5:
                  signals.emplace_back( ntk.create_maj5( c1, c2, c3, c4, !c5 ) );
                  break;

                case 6:
                  signals.emplace_back( ntk.create_maj5( !c1, !c2, c3, c4, c5 ) );
                  break;

                case 7:
                  signals.emplace_back( ntk.create_maj5( !c1, c2, !c3, c4, c5 ) );
                  break;

                case 8:
                  signals.emplace_back( ntk.create_maj5( !c1, c2, c3, !c4, c5 ) );
                  break;

                case 9:
                  signals.emplace_back( ntk.create_maj5( !c1, c2, c3, c4, !c5 ) );
                  break;

                case 10:
                  signals.emplace_back( ntk.create_maj5( c1, !c2, !c3, c4, c5 ) );
                  break;

                case 11:
                  signals.emplace_back( ntk.create_maj5( c1, !c2, c3, !c4, c5 ) );
                  break;

                case 12:
                  signals.emplace_back( ntk.create_maj5( c1, !c2, c3, c4, !c5 ) );
                  break;

                case 13:
                  signals.emplace_back( ntk.create_maj5( c1, c2, !c3, !c4, c5 ) );
                  break;

                case 14:
                  signals.emplace_back( ntk.create_maj5( c1, c2, !c3, c4, !c5 ) );
                  break;

                case 15:
                  signals.emplace_back( ntk.create_maj5( c1, c2, c3, !c4, !c5 ) );
                  break;
              }
              
              fn( spec.out_inv ? !signals.back() : signals.back() );
            }
            
          }

      private:
        exact_m5ig_resynthesis_params _ps;
    };

}

#endif
