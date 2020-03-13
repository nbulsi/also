/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file exact_aoig.hpp
 *
 * @brief Given a truth table, return an optimal expression from
 * exact synthesis in percy
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef EXACT_AOIG_HPP
#define EXACT_AOIG_HPP

using namespace percy;

namespace also
{

  void tt2aoig( const kitty::dynamic_truth_table& tt )
  {
    chain c;
    spec spec;
    spec.verbosity = 0; 
    spec.add_alonce_clauses = true;
    spec.add_colex_clauses = true;
    spec.add_lex_func_clauses = true;
    spec.add_nontriv_clauses = true;
    spec.add_noreapply_clauses = true;
    spec.add_symvar_clauses = true; 
    spec.conflict_limit = 0;

    bsat_wrapper solver;
    ssv_encoder encoder( solver );

    spec.fanin = 2; 
    spec[0] = tt;

    spec.preprocess();

    std::cout << "0x" << kitty::to_hex( tt ) << " ";

    if( spec.nr_triv == spec.get_nr_out() )
    {
      std::cout << "trivial function" << std::endl;
      return;
    }

    const auto result = synthesize( spec, c, solver, encoder );

    if( result == success )
    {
      encoder.extract_chain( spec, c );
      c.print_expression();
      std::cout << std::endl;
    }
  }


}

#endif
