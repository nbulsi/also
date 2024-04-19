/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file rm3ig_helper.cpp
 *
 * @brief Helper for rm3ig chain extraction and rm3ig encoder
 * selection variables generation
 *
 */

#include "rm3ig_helper.hpp"

namespace also
{

/******************************************************************************
 * Public functions by class comb and select                                  *
 ******************************************************************************/
std::map<std::vector<int>, std::vector<int>> comput_input_and_set_maps( type_input type )
{
  comb4 c( type );
  return c.get_on_set_map();
}

std::map<int, std::vector<unsigned>> comput_select_vars_maps( int nr_steps, int nr_in )
{
  select4 s( nr_steps, nr_in );
  return s.get_sel_var_map();
}

int comput_select_vars_for_each_steps( int nr_steps, int nr_in, int step_idx )
{
  assert( step_idx >= 0 && step_idx < nr_steps );
  select4 s( nr_steps, nr_in );
  return s.get_num_of_sel_vars_for_each_step( step_idx );
}

/* rmig3 to expressions */
std::string rm3ig_to_string( const spec& spec, const rm3ig& rm3ig )
{
  if ( rm3ig.get_nr_steps() == 0 )
  {
    return "";
  }

  assert( rm3ig.get_nr_steps() >= 1 );

  std::stringstream ss;

  auto pol = spec.out_inv ? 1 : 0;
  ss << pol << " ";

  for ( auto i = 0; i < spec.nr_steps; i++ )
  {
    ss << i + spec.nr_in + 1 << "-" << rm3ig.operators[i] << "-"
       << rm3ig.steps[i][0]
       << rm3ig.steps[i][1]
       << rm3ig.steps[i][2] << " ";
  }

  return ss.str();
}

std::string print_expr( const rm3ig& rm3ig, const int& step_idx )
{
  std::stringstream ss;
  std::vector<char> inputs;

  for ( auto i = 0; i < 3; i++ )
  {
    if ( rm3ig.steps[step_idx][i] == 0 )
    {
      inputs.push_back( '0' );
    }
    else
    {
      inputs.push_back( char( 'a' + rm3ig.steps[step_idx][i] - 1 ) );
    }
  }

  switch ( rm3ig.operators[step_idx] )
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

std::string print_all_expr( const spec& spec, rm3ig& rm3ig )
{
  std::stringstream ss;

  char pol = ' ';
  for ( auto i = 0; i < spec.get_nr_out(); i++ )
  {
    auto outvar = rm3ig.get_output( i ) >> 1;
    if ( ( outvar == spec.nr_in + spec.nr_steps ) && rm3ig.get_output( i ) & 1 )
    {
      pol = '!';
      break;
    }
  }

  std::cout << "[i] " << spec.nr_steps << " steps are required " << std::endl;
  for ( auto i = 0; i < spec.nr_steps; i++ )
  {
    if ( i == spec.nr_steps - 1 )
    {
      ss << char( i + spec.nr_in + 'a' ) << "=" << pol << print_expr( rm3ig, i );
    }
    else
    {
      ss << char( i + spec.nr_in + 'a' ) << "=" << print_expr( rm3ig, i );
    }
  }

  std::cout << "[expressions] " << ss.str() << std::endl;
  if ( spec.get_nr_out() > 1 )
  {
    std::cout << "[i] There are " << spec.get_nr_out() << " functions be synthesized\n\n";

    for ( int i = 0; i < spec.get_nr_out(); i++ )
    {
      std::cout << "[i] Function id " << i << " is 0x" << kitty::to_hex( spec[i] ) << std::endl;
      auto outvar = rm3ig.get_output( i ) >> 1;
      std::cout << "[i] PO " << i << " is " << char( outvar - 1 + 'a' );
      if ( rm3ig.get_output( i ) & 1 )
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

rm3_network rm3ig_to_rm3ig_network( const spec& spec, rm3ig& rm3ig )
{
  rm3_network rm3;

  std::vector<rm3_network::signal> pis;

  pis.push_back( rm3.get_constant( false ) );
  for ( auto i = 0; i < spec.nr_in; i++ )
  {
    pis.push_back( rm3.create_pi() );
  }

  for ( auto i = 0; i < rm3ig.get_nr_steps(); i++ )
  {
    const auto& step = rm3ig.get_step_inputs( i );

    auto tmp_in0 = pis[step[0]];
    auto tmp_in1 = pis[step[1]];
    auto tmp_in2 = pis[step[2]];

    switch ( rm3ig.get_op( i ) )
    {
    case 0:
      pis.push_back( rm3.create_rm3( tmp_in0, tmp_in1, tmp_in2 ) );
      break;

    case 1:
      pis.push_back( rm3.create_rm3( tmp_in0 ^ true, tmp_in1, tmp_in2 ) );
      break;

    case 2:
      pis.push_back( rm3.create_rm3( tmp_in0, tmp_in1 ^ true, tmp_in2 ) );
      break;

    case 3:
      pis.push_back( rm3.create_rm3( tmp_in0, tmp_in1, tmp_in2 ^ true ) );
      break;

    default:
      assert( false && "ops are not known" );
      break;
    }
  }

  if ( spec.get_nr_out() > 1 )
  {
    for ( int i = 0; i < spec.get_nr_out(); i++ )
    {
      auto outvar = rm3ig.get_output( i ) >> 1;

      const auto driver = pis[outvar] ^ ( ( rm3ig.get_output( i ) & 1 ) ? true : false );
      rm3.create_po( driver );
    }
  }
  else
  {
    const auto driver = pis[pis.size() - 1] ^ ( spec.out_inv ? true : false );
    rm3.create_po( driver );
  }

  return rm3;
}

} // namespace also
