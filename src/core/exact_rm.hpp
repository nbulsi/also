#ifndef EXACT_RM_HPP
#define EXACT_RM_HPP

#include <percy/percy.hpp>
#include <mockturtle/utils/stopwatch.hpp>
#include <mockturtle/properties/migcost.hpp>

#include "../networks/rm3/RM3.hpp"
#include "rm3_help.hpp"
#include "../networks/rm3/rm3_compl_rw.hpp"

using namespace percy;

namespace also
{
/******************************************************************************
* Types                                                                      *
******************************************************************************/

/******************************************************************************
* Private functions                                                          *
******************************************************************************/

/******************************************************************************
* Public functions                                                           *
******************************************************************************/

/* AIG-based exact synthesis */
rm3_network rm3_from_aig_chain(chain &c)
{
    rm3_network rm3;
    int nr_in = c.get_nr_inputs();
    std::vector < rm3_network::signal> sigs;
    sigs.push_back( rm3.get_constant( false ) );
    //sigs.push_back( rm3.get_constant( true ) );
    for (int i = 0; i < nr_in; i++)
    {
        sigs.push_back(rm3.create_pi());
    }

    for (size_t i = 0; i < c.get_nr_steps(); i++)
    {
        const auto &step = c.get_step(i);
        const auto idx = nr_in + i + 1;
        //const auto idx = nr_in + i + 2;
        const auto op = c.get_operator(i);

        // sigs.push_back( rm3.create_rm3( sigs[step[0] + 1],
        //                                 sigs[step[1] + 1],
        //                                 sigs[step[2] + 1] ) );

        auto ops = kitty::to_binary(op);
        if (ops == "0010")
        {
          sigs.push_back( rm3.create_op1( sigs[step[0] + 1],
                                          sigs[step[1] + 1] ) );
        }
        else if (ops == "1110")
        {
            sigs.push_back(rm3.create_op2(sigs[step[0] + 1],
                                          sigs[step[1] + 1]));
        }
        else if (ops == "1000")
        {
            
            sigs.push_back(rm3.create_op3(sigs[step[0] + 1],
                                          sigs[step[1] + 1]));
        }
        else if (ops == "0100")
        {
            sigs.push_back(rm3.create_op4(sigs[step[0] + 1],
                                          sigs[step[1] + 1]));
        }
        else if ( ops == "1011" )
        {
            sigs.push_back( rm3.create_op5( sigs[step[0] + 1],
                                            sigs[step[1] + 1] ) );
        }
        else if ( ops == "1101" )
        {
            sigs.push_back( rm3.create_op6( sigs[step[0] + 1],
                                            sigs[step[1] + 1] ) );
        }
        else
        {
            assert(false && "UNKNOWN OPS");
        }
    }

    // for ( const auto& sig : sigs )
    // {
    //   std::cout << sig.index<< std::endl;
    // }

    auto top = c.is_output_inverted(0) ? !sigs[sigs.size() - 1] : sigs[sigs.size() - 1];

    rm3.create_po(top);
    return rm3;
    }

void rm3_from_aig(const kitty::dynamic_truth_table &tt, const bool &verbose, int &min_gates)
    {
        chain c;
        spec spec;
        spec.verbosity = 0;

        bsat_wrapper solver;
        ssv_encoder encoder(solver);

        spec.fanin = 2; // each node has at most 2 fanins, while it equals 3 for MAJ synthesis
        spec.set_primitive(AIG);
        spec[0] = tt;

        if (verbose)
        {
            std::cout << "Synthesizing function " << kitty::to_hex(spec[0]) << std::endl;
        }

        mockturtle::stopwatch<>::duration time{0};
        int nr_solutions = 0;

        {
            mockturtle::stopwatch t(time);

            // synthesis
            int min_num_gates = INT_MAX;
            int current_num_gates = 0;
            rm3_network best_rm3;

            while (next_solution(spec, c, solver, encoder) == success)
            {
                c.print_expression();
                auto tts = c.simulate();
                assert(tts[0] == spec[0]);
                assert(c.is_aig());
                nr_solutions++;

                auto r = c;
                auto rm3 = rm3_from_aig_chain(r);

                current_num_gates = rm3.num_gates() + mockturtle::num_inverters(rm3);

                int x;
                x = mockturtle::num_inverters( rm3 );
                std::cout <<"反相器个数为"<< x << std::endl;

                if ( current_num_gates < min_num_gates )
                {
                    min_num_gates = current_num_gates;
                    best_rm3 = rm3;
                }

                if (verbose)
                {
                    printf("NEW RM3 size: %d\n", current_num_gates);
                    rm3_to_expression(std::cout, rm3);
                }
            }

            if (nr_solutions != 0)
            {
                printf("[i]: found rm3_network with minimum number of gates: %d\n", min_num_gates);
                printf("[i]: expression: ");
                rm3_to_expression(std::cout, best_rm3);
            }

            std::cout << "\nThere are " << nr_solutions << " solutions found." << std::endl;

            min_gates = min_num_gates;
        }

        if (verbose)
        {
            std::cout << fmt::format("[i]: {:5.2f} seconds passed to enumerate {} solutions\n",
                                     mockturtle::to_seconds(time), nr_solutions);
        }
    }
}

#endif