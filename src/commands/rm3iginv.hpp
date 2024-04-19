/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file rm3iginv.hpp
 *
 * @brief inversion optimization of RM3IGs
 *
 */

#ifndef RM3IGINV_HPP
#define RM3IGINV_HPP

#include "../core/rm3ig_inv.hpp"
#include <mockturtle/mockturtle.hpp>
#include <mockturtle/utils/stopwatch.hpp>

namespace alice
{

class rm3iginv_command : public command
{
public:
  explicit rm3iginv_command( const environment::ptr& env )
      : command( env, "inversion optimization of rm3ig" )
  {
    add_flag( "--verbose, -v", "print the information" );
  }

  rules validity_rules() const
  {
    return { has_store_element<rm3_network>( env ) };
  }

protected:
  void execute()
  {
    /* derive some RM3IG */
    rm3_network rm3ig = store<rm3_network>().current();
    rm3_network rm3ig_opt;

    //rm3ig_opt = also::rm3ig_inv_optimization( rm3ig );

    stopwatch<>::duration time{ 0 };
    call_with_stopwatch( time, [&]()
                         { rm3ig_opt = also::rm3ig_inv_optimization( rm3ig ); 
                          store<rm3_network>().extend();
                          store<rm3_network>().current() = rm3ig_opt; } );

    //store<rm3_network>().extend();
    //store<rm3_network>().current() = rm3ig_opt;
    std::cout << fmt::format( "[time]: {:5.2f} seconds\n", to_seconds( time ) );
  }
};

ALICE_ADD_COMMAND( rm3iginv, "Optimization" )

} // namespace alice

#endif
