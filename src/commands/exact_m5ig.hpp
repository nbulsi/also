/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file exact_m5ig.hpp
 *
 * @brief exact synthesis using m5ig as the underlying data
 * structure
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef EXACT_M5IG_HPP
#define EXACT_M5IG_HPP

#include <alice/alice.hpp>
#include <mockturtle/mockturtle.hpp>

#include "../store.hpp"
#include "../core/exact_m5ig_encoder.hpp"

namespace alice
{

  class exact_m5ig_command: public command
  {
    public:
      explicit exact_m5ig_command( const environment::ptr& env ) : command( env, "using exact synthesis to find optimal m5igs" )
      {
        add_flag( "--verbose, -v",  "print the information" );
      }

      rules validity_rules() const
      {
        return { has_store_element<optimum_network>( env ) };
      }

    private:
  std::string nbu_mig_five_encoder_test( const kitty::dynamic_truth_table& tt )
  {
    std::stringstream ss;
    bsat_wrapper solver;
    spec spec;
    also::mig5 mig5;

    auto copy = tt;
    if( copy.num_vars()  < 5 )
    {
      spec[0] = kitty::extend_to( copy, 5 );
    }
    else
    {
      spec[0] = tt;
    }
    
    also::mig_five_encoder encoder( solver );

    if ( also::mig_five_synthesize( spec, mig5, solver, encoder ) == success )
    {
      auto pol = spec.out_inv ? 1 : 0;
      ss << pol << " ";
      for(auto i = 0; i < spec.nr_steps; i++ )
      {
        ss << i + spec.nr_in + 1 << "-" << mig5.operators[i] << "-" << mig5.steps[i][0] 
           << mig5.steps[i][1] << mig5.steps[i][2] << mig5.steps[i][3] << mig5.steps[i][4] << " ";
      }

      std::cout << "[result: inverter_output ? gate_id-operation_id-five inputs] " << ss.str() << std::endl;
      return ss.str();
    }
    else
    {
      return " fail ";
    }
  }
  
  std::string nbu_mig_five_encoder_cegar_test( const kitty::dynamic_truth_table& tt )
  {
    std::stringstream ss;
    bsat_wrapper solver;
    spec spec;
    also::mig5 mig5;

    auto copy = tt;
    if( copy.num_vars()  < 5 )
    {
      spec[0] = kitty::extend_to( copy, 5 );
    }
    else
    {
      spec[0] = tt;
    }
    
    also::mig_five_encoder encoder( solver );

    if ( also::mig_five_cegar_synthesize( spec, mig5, solver, encoder ) == success )
    {
      auto pol = spec.out_inv ? 1 : 0;
      ss << pol << " ";
      for(auto i = 0; i < spec.nr_steps; i++ )
      {
        ss << i + spec.nr_in + 1 << "-" << mig5.operators[i] << "-" << mig5.steps[i][0] 
           << mig5.steps[i][1] << mig5.steps[i][2] << mig5.steps[i][3] << mig5.steps[i][4] << " ";
      }

      std::cout << "[result: inverter_output ? gate_id-operation_id-five inputs] " << ss.str() << std::endl;
      return ss.str();
    }
    else
    {
      return " fail ";
    }
  }

    protected:
      void execute()
      {
        bool verb = false;

        if( is_set( "verbose" ) )
        {
          verb = true;
        }

        auto& opt = store<optimum_network>().current();
        const auto config = kitty::exact_npn_canonization( opt.function );
        std::cout << kitty::to_hex( opt.function ) << " npn : " << kitty::to_hex( std::get<0>( config ) ) << std::endl;

        nbu_mig_five_encoder_cegar_test( opt.function );
      }

  };

  ALICE_ADD_COMMAND( exact_m5ig, "Exact synthesis" )

}

#endif
