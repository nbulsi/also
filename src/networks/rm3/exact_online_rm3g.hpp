// /* also: Advanced Logic Synthesis and Optimization tool
//  * Copyright (C) 2019- Ningbo University, Ningbo, China */

// /**
//  * @file exact_online_rm3g.hpp
//  *
//  * @brief Replace the network with exact rm3g result (computing online)
//  *
//  * @author Zhufei Chu
//  * @since  0.1
//  */

// #ifndef EXACT_ONLINE_RM3G_HPP
// #define EXACT_ONLINE_RM3G_HPP

// #include <mockturtle/mockturtle.hpp>

// #include "../../core/exact_rm3_encoder.hpp"
// #include "../../core/rm3_help.hpp"
// using namespace percy;
// namespace mockturtle
// {
// struct exact_rm3_resynthesis_params
// {
//   using cache_map_t = std::unordered_map<kitty::dynamic_truth_table, also::rm3, kitty::hash<kitty::dynamic_truth_table>>;
//   using cache_t = std::shared_ptr<cache_map_t>;

//   cache_t cache;

//   bool add_alonce_clauses{ true };
//   bool add_colex_clauses{ true };
//   bool add_lex_clauses{ false };
//   bool add_lex_func_clauses{ false };
//   bool add_symvar_clauses{ true };
//   int conflict_limit{ 0 };
// };

// template<class Ntk = rm3_network>
// class exact_rm3_resynthesis
// {
// public:
//   explicit exact_rm3_resynthesis( exact_rm3_resynthesis_params const& ps = {} )
//       : _ps( ps )
//   {
//   }

//   template<typename LeavesIterator, typename Fn>
//   void operator()( Ntk& ntk,
//                    kitty::dynamic_truth_table const& function,
//                    LeavesIterator begin,
//                    LeavesIterator end,
//                    Fn&& fn )
//   {
//     operator()( ntk, function, function.construct(), begin, end, fn );
//   }

//   template<typename LeavesIterator, typename Fn>
//   void operator()( Ntk& ntk,
//                    kitty::dynamic_truth_table const& function,
//                    kitty::dynamic_truth_table const& dont_cares,
//                    LeavesIterator begin,
//                    LeavesIterator end,
//                    Fn&& fn )
//   {
//     // TODO: special case for small functions (up to 2 variables)?

//     percy::spec spec;
//     spec.fanin = 3;
//     spec.verbosity = 0;
//     spec.add_alonce_clauses = _ps.add_alonce_clauses;
//     spec.add_colex_clauses = _ps.add_colex_clauses;
//     spec.add_lex_clauses = _ps.add_lex_clauses;
//     spec.add_lex_func_clauses = _ps.add_lex_func_clauses;
//     spec.add_symvar_clauses = _ps.add_symvar_clauses;
//     spec.conflict_limit = _ps.conflict_limit;
//     spec[0] = function;
//     bool with_dont_cares{ false };
//     if ( !kitty::is_const0( dont_cares ) )
//     {
//       spec.set_dont_care( 0, dont_cares );
//       with_dont_cares = true;
//     }

//     auto c = [&]() -> std::optional<also::rm3>
//     {
//       if ( !with_dont_cares && _ps.cache )
//       {
//         const auto it = _ps.cache->find( function );
//         if ( it != _ps.cache->end() )
//         {
//           return it->second;
//         }
//       }

//       also::rm3 c;
//       percy::bsat_wrapper solver;
//       also::rm_three_encoder encoder( solver );
//       if ( const auto result = rm_three_synthesize( spec & spec, c & c, solver_wrapper & solver, rm_three_encoder & encoder ); result != percy::success )
//       {
//         return std::nullopt;
//       }

//       if ( !with_dont_cares && _ps.cache )
//       {
//         ( *_ps.cache )[function] = c;
//       }
//       return c;
//     }();

//     if ( !c )
//     {
//       return;
//     }

//     std::vector<signal<Ntk>> signals( begin, end );

//     for ( auto i = 0; i < c->get_nr_steps(); ++i )
//     {
//       // auto array = c->get_step_inputs( i );
//       // auto c1 = signals[array[0]];
//       // auto c2 = signals[array[1]];
//       // auto c3 = signals[array[2]];

//       auto const c1 = signals[c->get_step( i )[0]];
//       auto const c2 = signals[c->get_step( i )[1]];
//       auto const c3 = signals[c->get_step( i )[2]];

//       switch ( c->get_operator( i )._bits[0] )
//       {
//       default:
//         std::cerr << "[e] unsupported operation " << kitty::to_hex( c->get_operator( i ) ) << "\n";
//         break;
//       case 0x00:
//         signals.emplace_back( ntk.get_constant( false ) );
//         break;
//       case 0xaa:
//         signals.emplace_back( ntk.create_rm3( get_constant( false ), c1, get_constant( true ) ) );
//         break;
//       case 0xe8:
//         signals.emplace_back( ntk.create_rm3( c1, !c2, c3 ) );
//         break;
//       case 0xd4:
//         signals.emplace_back( ntk.create_rm3( c2, c1, c3 ) );
//         break;
//       case 0xb2:
//         signals.emplace_back( ntk.create_rm3( c1, c2, c3 ) );
//         break;
//       case 0x8e:
//         signals.emplace_back( ntk.create_rm3( c1, c3, c2 ) );
//         break;
//       // case 0xc0:
//       //   signals.emplace_back( ntk.create_rm3( ntk.get_constant( false ), c2, c3 ) ); // c0
//       //   break;
//       // case 0xfc:
//       //   signals.emplace_back( ntk.create_rm3( !ntk.get_constant( false ), c2, c3 ) ); // fc
//       //   break;
//       // case 0x30:
//       //   signals.emplace_back( ntk.create_rm3( ntk.get_constant( false ), !c2, c3 ) ); // 30
//       //   break;
//       // case 0x0c:
//       //   signals.emplace_back( ntk.create_rm3( ntk.get_constant( false ), c2, !c3 ) ); // 0c
//       //   break;
//       // case 0xa0:
//       //   signals.emplace_back( ntk.create_rm3( c1, ntk.get_constant( false ), c3 ) ); // 0a
//       //   break;
//       // case 0x50:
//       //   signals.emplace_back( ntk.create_rm3( !c1, ntk.get_constant( false ), c3 ) ); // 50
//       //   break;
//       // case 0xfa:
//       //   signals.emplace_back( ntk.create_rm3( c1, !ntk.get_constant( false ), c3 ) ); // fa
//       //   break;
//       // case 0x0a:
//       //   signals.emplace_back( ntk.create_rm3( c1, ntk.get_constant( false ), !c3 ) ); // 0a
//       //   break;
//       case 0x88:
//         signals.emplace_back( ntk.create_rm3( c1, ntk.get_constant( true ), c2 ) ); // 88
//         break;
//       case 0xee:
//         signals.emplace_back( ntk.create_rm3( c1, ntk.get_constant( false ), c2 ) ); // ee
//         break;
//       case 0x44:
//         signals.emplace_back( ntk.create_rm3( c2, c1, ntk.get_constant( false ) ) ); // 44
//         break;
//       case 0x22:
//         signals.emplace_back( ntk.create_rm3( c1, c2, ntk.get_constant( false ) ) ); // 22
//         break;
//       case 0xdd:
//         signals.emplace_back( ntk.create_rm3( c2, c1, ntk.get_constant( true ) ) ); // 44
//         break;
//       case 0xbb:
//         signals.emplace_back( ntk.create_rm3( c1, c2, ntk.get_constant( true ) ) ); // 22
//         break;
//       }

//       fn( spec.out_inv ? !signals.back() : signals.back() );
//     }
//   }

// private:
//   exact_rm3_resynthesis_params _ps;
// };

// } // namespace mockturtle

// #endif
