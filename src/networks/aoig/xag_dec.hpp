/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file xag_dec.hpp
 *
 * @brief Deompose a truth table into an XAG signal by combined
 * decompostion methods (DSD, Shannon, and NPN )
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef XAG_DEC_HPP
#define XAG_DEC_HPP

#include "../../core/misc.hpp"

namespace also
{
  
  template<class Ntk>
  class xag_dec_impl
  {
    private:
      std::vector<uint8_t> get_func_supports( kitty::dynamic_truth_table const& spec )
      {
        std::vector<uint8_t> supports;

        for( auto i = 0u; i < spec.num_vars(); ++i )
        {
          if( kitty::has_var( spec, i ) )
          {
            supports.push_back( i );
          }
        }

        return supports;
      }

    public:
      xag_dec_impl( Ntk& ntk, kitty::dynamic_truth_table const& func, std::vector<signal<Ntk>> const& children, dsd_decomposition_params const& ps )
        : _ntk( ntk ),
          _func( func ),
          pis( children ),
          _ps( ps )
      {
      }

      signal<Ntk> decompose( kitty::dynamic_truth_table& remainder, std::vector<signal<Ntk>> const& children )
      {
        auto sup = get_func_supports( remainder );
        
        /* check constants */
        if ( kitty::is_const0( remainder ) )
        {
          return _ntk.get_constant( false );
        }
        if ( kitty::is_const0( ~remainder ) )
        {
          return _ntk.get_constant( true );
        }
        
        /* check primary inputs */
        if( sup.size() == 1u )
        {
          auto var = remainder.construct();

          kitty::create_nth_var( var, sup.front() );
          if( remainder == var )
          {
            return children[sup.front()];
          }
          else
          {
            assert( remainder == ~var );
            return _ntk.create_not( children[sup.front()] );
          }
          assert( false && "ERROR for primary inputs" );
        }

        /* top decomposition */
        for( auto var : sup )
        {
          if ( auto res = kitty::is_top_decomposable( remainder, var, &remainder, _ps.with_xor );
              res != kitty::top_decomposition::none )
          {
            const auto right = decompose( remainder, children );

            switch ( res )
            {
              default:
                assert( false );
              case kitty::top_decomposition::and_:
                return _ntk.create_and( children[var], right );
              case kitty::top_decomposition::or_:
                return _ntk.create_or( children[var], right );
              case kitty::top_decomposition::lt_:
                return _ntk.create_lt( children[var], right );
              case kitty::top_decomposition::le_:
                return _ntk.create_le( children[var], right );
              case kitty::top_decomposition::xor_:
                return _ntk.create_xor( children[var], right );
            }
          }
        }
        
        /* bottom decomposition */
        for ( auto j = 1u; j < sup.size(); ++j )
        {
          for ( auto i = 0u; i < j; ++i )
          {
            if ( auto res = kitty::is_bottom_decomposable( remainder, sup[i], sup[j], &remainder, _ps.with_xor );
                res != kitty::bottom_decomposition::none )
            {
              auto copy = children;
              switch ( res )
              {
                default:
                  assert( false );
                case kitty::bottom_decomposition::and_:
                  copy[sup[i]] = _ntk.create_and( copy[sup[i]], copy[sup[j]] );
                  break;
                case kitty::bottom_decomposition::or_:
                  copy[sup[i]] = _ntk.create_or( copy[sup[i]], copy[sup[j]] );
                  break;
                case kitty::bottom_decomposition::lt_:
                  copy[sup[i]] = _ntk.create_lt( copy[sup[i]], copy[sup[j]] );
                  break;
                case kitty::bottom_decomposition::le_:
                  copy[sup[i]] = _ntk.create_le( copy[sup[i]], copy[sup[j]] );
                  break;
                case kitty::bottom_decomposition::xor_:
                  copy[sup[i]] = _ntk.create_xor( copy[sup[i]], copy[sup[j]] );
                  break;
              }

              return decompose( remainder, copy ); 
            }
          }
        }

        /* shannon decomposition */
        auto var = sup.front();
        auto c0 = kitty::cofactor0( remainder, var );
        auto c1 = kitty::cofactor1( remainder, var );

        const auto f0  = decompose( c0, children );  
        const auto f1  = decompose( c1, children );  

        return _ntk.create_ite( children[var], f1, f0 );

        assert( false );
      }

      signal<Ntk> run()
      {
        return decompose( _func, pis );
      }

    private:
    Ntk& _ntk;
    kitty::dynamic_truth_table _func;
    std::vector<signal<Ntk>> pis;
    dsd_decomposition_params const& _ps;
  };

    template<class Ntk>
    signal<Ntk> xag_dec( Ntk& ntk, kitty::dynamic_truth_table const& func, std::vector<signal<Ntk>> const& children, 
                                   dsd_decomposition_params const& ps = {} )
    {
      xag_dec_impl<Ntk> impl( ntk, func, children, ps );
      return impl.run();
    }


} //end of namespace

#endif
