/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file exact_online_m3ig.hpp
 *
 * @brief Replace the network with exact m3ig result (computing online)
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef EXACT_ONLINE_M3IG_HPP
#define EXACT_ONLINE_M3IG_HPP

#include <mockturtle/mockturtle.hpp>

#include "../../core/exact_m3ig_encoder.hpp"
#include "../../core/m3ig_helper.hpp"

namespace also
{
  struct exact_mig_resynthesis_params
  {
    using cache_map_t = std::unordered_map<kitty::dynamic_truth_table, mig3, kitty::hash<kitty::dynamic_truth_table>>;
    using cache_t = std::shared_ptr<cache_map_t>;

    cache_t cache;

    bool add_alonce_clauses{true};
    bool add_colex_clauses{true};
    bool add_lex_clauses{false};
    bool add_lex_func_clauses{true};
    bool add_symvar_clauses{true};
    int conflict_limit{0};
  };
  
  template<class Ntk = mig_network>
    class exact_mig_resynthesis
    {
      public:
        explicit exact_mig_resynthesis( exact_mig_resynthesis_params const& ps = {} )
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
            spec.fanin = 3;
            spec.verbosity = 0;
            spec.add_alonce_clauses = _ps.add_alonce_clauses;
            spec.add_colex_clauses = _ps.add_colex_clauses;
            spec.add_lex_clauses = _ps.add_lex_clauses;
            spec.add_lex_func_clauses = _ps.add_lex_func_clauses;
            spec.add_symvar_clauses = _ps.add_symvar_clauses;
            spec.conflict_limit = _ps.conflict_limit;
            spec[0] = function;
            bool with_dont_cares{false};
            if ( !kitty::is_const0( dont_cares ) )
            {
              spec.set_dont_care( 0, dont_cares );
              with_dont_cares = true;
            }

            auto c = [&]() -> std::optional<mig3> {
              if ( !with_dont_cares && _ps.cache )
              {
                const auto it = _ps.cache->find( function );
                if ( it != _ps.cache->end() )
                {
                  return it->second;
                }
              }

              mig3 c;
              if ( const auto result = parallel_mig_three_fence_synthesize( spec, c ); result != percy::success )
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

              switch( c->get_op( i ) )
              {
                default:
                  std::cerr << "[e] unsupported operation " << c->get_op(i) << "\n";
                  break;

                case 0:
                  signals.emplace_back( ntk.create_maj( c1, c2, c3 ) );
                  break;

                case 1:
                  signals.emplace_back( ntk.create_maj( !c1, c2, c3 ) );
                  break;

                case 2:
                  signals.emplace_back( ntk.create_maj( c1, !c2, c3 ) );
                  break;

                case 3:
                  signals.emplace_back( ntk.create_maj( c1, c2, !c3 ) );
                  break;
              }
              
              fn( spec.out_inv ? !signals.back() : signals.back() );
            }
            
          }

      private:
        exact_mig_resynthesis_params _ps;
    };

}

#endif
