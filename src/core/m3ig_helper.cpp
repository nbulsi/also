/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file m3ig_helper.cpp
 *
 * @brief Helper for m3ig chain extraction and m3ig encoder
 * selection variables generation
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#include "m3ig_helper.hpp"

namespace also
{


  /******************************************************************************
   * Public functions by class comb and select                                  *
   ******************************************************************************/
  std::map<std::vector<int>, std::vector<int>> comput_input_and_set_map3( input_type3 type )
  {
    comb3 c( type );
    return c.get_on_set_map();
  }

  std::map<int, std::vector<unsigned>> comput_select_vars_map3( int nr_steps, int nr_in )
  {
    select3 s( nr_steps, nr_in );
    return s.get_sel_var_map();
  }

  int comput_select_vars_for_each_step3( int nr_steps, int nr_in, int step_idx )
  {
    assert( step_idx >= 0 && step_idx < nr_steps );
    select3 s( nr_steps, nr_in );
    return s.get_num_of_sel_vars_for_each_step( step_idx );
  }

  /* mig3 to expressions */
  std::string mig3_to_string( const spec& spec, const mig3& mig3 )
  {
    if( mig3.get_nr_steps() == 0 )
    {
      return "";
    }

    assert( mig3.get_nr_steps() >= 1 );

    std::stringstream ss;

    auto pol = spec.out_inv ? 1 : 0;
    ss << pol << " ";

    for(auto i = 0; i < spec.nr_steps; i++ )
    {
      ss << i + spec.nr_in + 1 << "-" << mig3.operators[i] << "-"
                                      << mig3.steps[i][0]
                                      << mig3.steps[i][1]
                                      << mig3.steps[i][2] << " ";
    }

    return ss.str();
  }

  std::string print_expr( const mig3& mig3, const int& step_idx )
  {
    std::stringstream ss;
    std::vector<char> inputs;

    for( auto i = 0; i < 3; i++ )
    {
      if( mig3.steps[step_idx][i] == 0 )
      {
        inputs.push_back( '0' );
      }
      else
      {
        inputs.push_back( char( 'a' + mig3.steps[step_idx][i] - 1 ) );
      }
    }

    switch( mig3.operators[ step_idx ] )
    {
      default:
        assert( false && "illegal operator id" );
        break;

      case 0:
        ss << "<" << inputs[0] << inputs[1] << inputs[2] << "> ";
        break;

      case 1:
        ss << "<!" << inputs[0] << inputs[1] << inputs[2] << "> ";
        break;

      case 2:
        ss << "<" << inputs[0] << "!" << inputs[1] << inputs[2] << "> ";
        break;

      case 3:
        ss << "<" << inputs[0] << inputs[1] << "!" << inputs[2] << "> ";
        break;
    }

    return ss.str();
  }

  std::string print_all_expr( const spec& spec, mig3& mig3 )
  {
    std::stringstream ss;

    char pol = ' ';
    for( auto i = 0; i < spec.get_nr_out(); i++ )
    {
        auto outvar = mig3.get_output(i) >> 1;
        if( outvar == spec.nr_steps - 1 && mig3.get_output(i) & 1 )
        {
            pol = '!';
            break;
        }
    }

    std::cout << "[i] " << spec.nr_steps << " steps are required " << std::endl;
    for(auto i = 0; i < spec.nr_steps; i++ )
    {
      if(  i == spec.nr_steps - 1 )
      {
        ss << pol;
        ss << char( i + spec.nr_in + 'a' ) << "=" << print_expr( mig3, i );
      }
      else
      {
        ss << char( i + spec.nr_in + 'a' ) << "=" << print_expr( mig3, i );
      }
    }

    std::cout << "[expressions] " << ss.str() << std::endl;
    if( spec.get_nr_out() > 1 )
    {
      std::cout << "[i] There are " << spec.get_nr_out() << " functions be synthesized\n\n";

      for( int i = 0; i < spec.get_nr_out(); i++ )
      {
        std::cout << "[i] Function id " << i << " is 0x" << kitty::to_hex( spec[i] ) << std::endl;
        auto outvar = mig3.get_output(i) >> 1;
        std::cout << "[i] PO " << i << " is " << char( outvar - 1 + 'a' );
        if( mig3.get_output(i) & 1 )
        {
          std::cout << " and inverted\n";
        }
        else
        {
          std::cout << std::endl;
        }

        std::cout << std::endl;
      }
    }
    return ss.str();
  }

  mig_network mig3_to_mig_network( const spec& spec, mig3& mig3 )
  {
    mig_network mig;

    std::vector<mig_network::signal> pis;

    pis.push_back( mig.get_constant( false ) );
    for( auto i = 0; i < spec.nr_in; i++ )
    {
      pis.push_back( mig.create_pi() );
    }

    for( auto i = 0; i < mig3.get_nr_steps(); i++ )
    {
      const auto& step = mig3.get_step_inputs( i );

      auto tmp_in0 = pis[step[0]];
      auto tmp_in1 = pis[step[1]];
      auto tmp_in2 = pis[step[2]];

      switch( mig3.get_op( i ) )
      {
        case 0:
          pis.push_back( mig.create_maj( tmp_in0, tmp_in1, tmp_in2 ) );
          break;

        case 1:
          pis.push_back( mig.create_maj( tmp_in0 ^ true, tmp_in1, tmp_in2 ) );
          break;

        case 2:
          pis.push_back( mig.create_maj( tmp_in0, tmp_in1 ^ true, tmp_in2 ) );
          break;

        case 3:
          pis.push_back( mig.create_maj( tmp_in0, tmp_in1, tmp_in2 ^ true ) );
          break;

        default:
          assert( false && "ops are not known" );
          break;
      }
    }

    const auto driver = pis[ pis.size() - 1 ] ^ ( spec.out_inv ? true : false );
    mig.create_po( driver );
    return mig;
  }

}

