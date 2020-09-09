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
#include "build_xag_db.hpp"

namespace also
{
  /*! \brief Parameters for xag_dec */
  struct xag_dec_params
  {
    /*! \brief Apply NPN4. */
    bool with_npn4{true};
  };

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
      xag_dec_impl( xag_network& ntk, kitty::dynamic_truth_table const& func, std::vector<signal<xag_network>> const& children, 
                    std::unordered_map<std::string, std::string>& opt_xags, 
                    xag_dec_params const& ps )
        : _ntk( ntk ),
          _func( func ),
          pis( children ),
          opt_xags( opt_xags ),
          _ps( ps )
      {
      }

      signal<xag_network> decompose( kitty::dynamic_truth_table& remainder, std::vector<signal<xag_network>> const& children )
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
          if ( auto res = kitty::is_top_decomposable( remainder, var, &remainder, true );
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
            if ( auto res = kitty::is_bottom_decomposable( remainder, sup[i], sup[j], &remainder, true );
                res != kitty::bottom_decomposition::none ) /* allow XOR */
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

        if( sup.size() > 4u || !_ps.with_npn4 )
        {
          /* shannon decomposition */
          auto var = sup.front();
          auto c0 = kitty::cofactor0( remainder, var );
          auto c1 = kitty::cofactor1( remainder, var );

          const auto f0  = decompose( c0, children );  
          const auto f1  = decompose( c1, children );  

          return _ntk.create_ite( children[var], f1, f0 );
        }
        else
        {
          /* NPN transformation */
          return xag_from_npn( remainder, children );
        }

        assert( false );
      }

      signal<xag_network> xag_from_npn( kitty::dynamic_truth_table const& remainder, std::vector<signal<xag_network>> const& children )
      {
        /* get pi signals */
        auto sup = get_func_supports( remainder );
        std::vector<signal<xag_network>> small_pis;
        for( auto const i : sup )
        {
          small_pis.push_back( children[i] );
        }
        
        auto copy = remainder;
        const auto support = kitty::min_base_inplace( copy );
        const auto small_func = kitty::shrink_to( copy, static_cast<unsigned int>( support.size() ) );

        auto tt =  small_func;

        auto ttsup = get_func_supports( tt );
        
        const auto config = kitty::exact_npn_canonization( tt );

        auto func_str = "0x" + kitty::to_hex( std::get<0>( config ) );
        //std::cout << " process function " << func_str << std::endl;

        const auto it = opt_xags.find( func_str );
        assert( it != opt_xags.end() );

        std::vector<signal<xag_network>> pis( support.size(), _ntk.get_constant( false ) );
        std::copy( small_pis.begin(), small_pis.end(), pis.begin() );

        std::vector<signal<xag_network>> pis_perm( support.size() );
        auto perm = std::get<2>( config );
        for ( auto i = 0; i < support.size(); ++i )
        {
          pis_perm[i] = pis[perm[i]];
        }

        const auto& phase = std::get<1>( config );
        for ( auto i = 0; i < support.size(); ++i )
        {
          if ( ( phase >> perm[i] ) & 1 )
          {
            pis_perm[i] = !pis_perm[i];
          }
        }

        auto res = create_xag_from_str( it->second, pis_perm );

        return ( ( phase >> support.size() ) & 1 ) ? !res : res;
      }
      
      signal<xag_network> create_xag_from_str( const std::string& str, const std::vector<signal<xag_network>>& pis_perm )
      {
        std::stack<int> polar;
        std::stack<signal<xag_network>> inputs;

        for ( auto i = 0ul; i < str.size(); i++ )
        {
          // operators polarity
          if ( str[i] == '[' || str[i] == '(' || str[i] == '{' )
          {
            polar.push( (i > 0 && str[i - 1] == '!') ? 1 : 0 );
          }

          //input signals
          if ( str[i] >= 'a' && str[i] <= 'd' )
          {
            inputs.push( pis_perm[str[i] - 'a'] );

            polar.push( ( i > 0 && str[i - 1] == '!' ) ? 1 : 0 );
          }

          //create signals
          if ( str[i] == ']' )
          {
            assert( inputs.size() >= 2u );
            auto x1 = inputs.top();
            inputs.pop();
            auto x2 = inputs.top();
            inputs.pop();

            assert( polar.size() >= 3u );
            auto p1 = polar.top();
            polar.pop();
            auto p2 = polar.top();
            polar.pop();

            auto p3 = polar.top();
            polar.pop();

            inputs.push( _ntk.create_xor( x1 ^ p1, x2 ^ p2 ) ^ p3 );
            polar.push( 0 );
          }

          if ( str[i] == ')' )
          {
            assert( inputs.size() >= 2u );
            auto x1 = inputs.top();
            inputs.pop();
            auto x2 = inputs.top();
            inputs.pop();

            assert( polar.size() >= 3u );
            auto p1 = polar.top();
            polar.pop();
            auto p2 = polar.top();
            polar.pop();

            auto p3 = polar.top();
            polar.pop();

            inputs.push( _ntk.create_and( x1 ^ p1, x2 ^ p2 ) ^ p3 );
            polar.push( 0 );
          }

          if ( str[i] == '}' )
          {
            assert( inputs.size() >= 2u );
            auto x1 = inputs.top();
            inputs.pop();
            auto x2 = inputs.top();
            inputs.pop();

            assert( polar.size() >= 3u );
            auto p1 = polar.top();
            polar.pop();
            auto p2 = polar.top();
            polar.pop();

            auto p3 = polar.top();
            polar.pop();

            inputs.push( _ntk.create_or( x1 ^ p1, x2 ^ p2 ) ^ p3 );
            polar.push( 0 );
          }
        }

        assert( !polar.empty() );
        auto po = polar.top();
        polar.pop();
        return inputs.top() ^ po;
      }

      signal<xag_network> run()
      {
        return decompose( _func, pis );
      }

    private:
    xag_network& _ntk;
    kitty::dynamic_truth_table _func;
    std::vector<signal<xag_network>> pis;
    std::unordered_map<std::string, std::string>& opt_xags;
    xag_dec_params const& _ps;
  };

    signal<xag_network> xag_dec( xag_network& ntk, kitty::dynamic_truth_table const& func, std::vector<signal<xag_network>> const& children,
                                 std::unordered_map<std::string, std::string>& opt_xags,
                                   xag_dec_params const& ps = {} )
    {
      xag_dec_impl impl( ntk, func, children, opt_xags, ps );
      return impl.run();
    }


} //end of namespace

#endif
