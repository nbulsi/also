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

      signal<Ntk> decompose( kitty::dynamic_truth_table& remainder, std::vector<uint8_t> const& inputs )
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
          kitty::create_nth_var( var, inputs[0] );

          if( remainder == var )
          {
            return pis[inputs[0]];
          }
          else if( remainder == ~var )
          {
            return _ntk.create_not( pis[inputs[0]] );
          }
          else
          {
            /* do nothing */
            assert( false && "ERROR for primary inputs" );
          }
        }

        /* check top decomposition */
        for( auto var : inputs )
        {
          if ( auto res = kitty::is_top_decomposable( remainder, var, &remainder, _ps.with_xor );
              res != kitty::top_decomposition::none )
          {
            /* remove support */
            auto update_inputs = inputs;
            update_inputs.erase( std::remove( update_inputs.begin(), update_inputs.end(), var ), update_inputs.end() );
            
            const auto right = decompose( remainder, update_inputs );

            switch ( res )
            {
              default:
                assert( false );
              case kitty::top_decomposition::and_:
                return _ntk.create_and( pis[var], right );
              case kitty::top_decomposition::or_:
                return _ntk.create_or( pis[var], right );
              case kitty::top_decomposition::lt_:
                return _ntk.create_lt( pis[var], right );
              case kitty::top_decomposition::le_:
                return _ntk.create_le( pis[var], right );
              case kitty::top_decomposition::xor_:
                return _ntk.create_xor( pis[var], right );
            }
          }
        }
        
        /* step 4: check bottom disjoint decomposition */
        for ( auto j = 1u; j < inputs.size(); ++j )
        {
          for ( auto i = 0u; i < j; ++i )
          {
            if ( auto res = kitty::is_bottom_decomposable( remainder, inputs[i], inputs[j], &remainder, _ps.with_xor );
                res != kitty::bottom_decomposition::none )
            {
              switch ( res )
              {
                default:
                  assert( false );
                case kitty::bottom_decomposition::and_:
                  pis[inputs[i]] = _ntk.create_and( pis[inputs[i]], pis[inputs[j]] );
                  break;
                case kitty::bottom_decomposition::or_:
                  pis[inputs[i]] = _ntk.create_or( pis[inputs[i]], pis[inputs[j]] );
                  break;
                case kitty::bottom_decomposition::lt_:
                  pis[inputs[i]] = _ntk.create_lt( pis[inputs[i]], pis[inputs[j]] );
                  break;
                case kitty::bottom_decomposition::le_:
                  pis[inputs[i]] = _ntk.create_le( pis[inputs[i]], pis[inputs[j]] );
                  break;
                case kitty::bottom_decomposition::xor_:
                  pis[inputs[i]] = _ntk.create_xor( pis[inputs[i]], pis[inputs[j]] );
                  break;
              }

              auto update_inputs = inputs;
              update_inputs.erase( update_inputs.begin() + j );

              return decompose( remainder, update_inputs ); 
            }
          }
        }

        /* shannon decomposition */
        auto var = inputs.front();
        /* remove support */
        auto update_inputs = inputs;
        update_inputs.erase( std::remove( update_inputs.begin(), update_inputs.end(), var ), update_inputs.end() );

        auto c0 = kitty::cofactor0( remainder, var );
        auto c1 = kitty::cofactor1( remainder, var );

        auto f1  = decompose( c1, update_inputs );  
        auto f0  = decompose( c0, update_inputs );  

        return _ntk.create_ite( pis[var], f1, f0 );

        assert( false );
      }
      

      signal<Ntk> run()
      {
        return decompose( _func, get_func_supports( _func ) );
      }

    private:
    Ntk& _ntk;
    kitty::dynamic_truth_table _func;
    std::vector<signal<Ntk>> pis;
    dsd_decomposition_params const& _ps;

  };

  template<class Ntk>
    signal<Ntk> xag_dec( Ntk& ntk, kitty::dynamic_truth_table const& func, std::vector<signal<Ntk>> const& children, dsd_decomposition_params const& ps = {} )
    {
      xag_dec_impl<Ntk> impl( ntk, func, children, ps );
      return impl.run();
    }


} //end of namespace

#endif
