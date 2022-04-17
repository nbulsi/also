/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019-2021 Ningbo University, Ningbo, China */

/**
 * @file exact_bool_chain.hpp
 *
 * @brief TODO
 *
 * @author Zhufei
 * @since  0.1
 */

#ifndef EXACT_BOOL_CHAIN_HPP
#define EXACT_BOOL_CHAIN_HPP

#include "../core/exact_aoig.hpp"

namespace alice
{

  class exact_bool_chain_command: public command
  {
    public:
      explicit exact_bool_chain_command( const environment::ptr& env ) : command( env, "using exact synthesis to find optimal Boolean chain" )
      {
      }

      rules validity_rules() const
      {
        return { has_store_element<optimum_network>( env ) };
      }
    
    protected:
      void execute()
      {
        auto& opt = store<optimum_network>().current();
        auto copy = opt.function;

        assert( copy.num_vars() >= 2u );

        stopwatch<>::duration time{0};

        call_with_stopwatch( time, [&]()
            {
            also::tt2aoig( copy );
            });

        std::cout << fmt::format( "[time]: {:5.2f} seconds\n", to_seconds( time ) );
      }
  };

  ALICE_ADD_COMMAND( exact_bool_chain, "Exact synthesis" )
}

#endif
