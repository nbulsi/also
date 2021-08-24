/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file stp_circuit.hpp
 *
 * @brief Basic Semi-Tensor Product calculation model
 *
 * @author Hongyang Pan
 * @since  0.1
 */

#ifndef STP_HPP
#define STP_HPP

#include "../core/stp_circuit.hpp"
#include <alice/alice.hpp>
#include <fstream>
#include <mockturtle/mockturtle.hpp>

using namespace std;
using namespace Eigen;
using Eigen::MatrixXi;

namespace alice
{
    class stp_circuit_command : public command
    {
    public:
        explicit stp_circuit_command(const environment::ptr &env) : command(env, " Semi-Tensor Product Calculation Result ")
        {
            add_option("filename,-f", filename, "the input txt file name");
            add_flag("--cinput,-c", "Manual input test");
        }

    protected:
        void execute()
        {
            if (is_set("cinput"))
            {
                cin >> expression;

                vector<string> &t = tt;
                vector<MatrixXi> &mtxvec = vec;
                string &exp = expression;

                stopwatch<>::duration time{0};
                call_with_stopwatch(time, [&]()
                                    {
                                        stp_calc(exp, t, mtxvec);
                                        cout << mtxvec[0] << endl;
                                        int n = mtxvec[0].cols();
                                        MatrixXi r = mtxvec[0].topRows(1);

                                        cout << "stp_result: " << endl;
                                        int count = 0;
                                        for (int p = 0; p < n; p++)
                                        {
                                            if (r(p) == 1)
                                            {
                                                int i = p + 1;
                                                int a[1000];
                                                int b = 0;
                                                int tmp = n - i;
                                                while (tmp)
                                                {
                                                    a[b] = tmp % 2;
                                                    tmp /= 2;
                                                    b++;
                                                }
                                                for (int q = b - 1; q >= 0; q--)
                                                    cout << a[q];
                                                cout << " ";
                                                count += 1;
                                                if (count == 6)
                                                {
                                                    cout << endl;
                                                    count = 0;
                                                }
                                            }
                                            else
                                            {
                                                continue;
                                            }
                                        }
                                        cout << endl;
                                    });
                cout << fmt::format("[time]: {:5.2f} seconds\n", to_seconds(time));
                cout << "success" << endl;
            }
            else
            {
                ifstream myfile(filename);
                if (myfile.is_open())
                {
                    while (!myfile.eof())
                    {
                        myfile >> tmp;
                        expression += tmp;
                    }
                    myfile.close();

                    vector<string> &t = tt;
                    vector<MatrixXi> &mtxvec = vec;
                    string &exp = expression;

                    stopwatch<>::duration time{0};
                    call_with_stopwatch(time, [&]()
                                        {
                                            stp_calc(exp, t, mtxvec);
                                            int n = mtxvec[0].cols();
                                            MatrixXi r = mtxvec[0].topRows(1);

                                            cout << "stp_result: " << endl;
                                            int count = 0;
                                            for (int p = 0; p < n; p++)
                                            {
                                                if (r(p) == 1)
                                                {
                                                    int i = p + 1;
                                                    int a[1000];
                                                    int b = 0;
                                                    int tmp = n - i;
                                                    while (tmp)
                                                    {
                                                        a[b] = tmp % 2;
                                                        tmp /= 2;
                                                        b++;
                                                    }
                                                    for (int q = b - 1; q >= 0; q--)
                                                        cout << a[q];
                                                    cout << " ";
                                                    count += 1;
                                                    if (count == 6)
                                                    {
                                                        cout << endl;
                                                        count = 0;
                                                    }
                                                }
                                                else
                                                {
                                                    continue;
                                                }
                                            }
                                            cout << endl;
                                        });
                    cout << fmt::format("[time]: {:5.2f} seconds\n", to_seconds(time));
                    cout << "success" << endl;
                }
                else
                {
                    cerr << "Cannot open input file" << endl;
                }
            }
        }

    private:
        string filename = "cnf.txt";
        string tmp;
        string expression;
        vector<string> tt;
        vector<MatrixXi> vec;
    };

    ALICE_ADD_COMMAND(stp_circuit, "Exact synthesis")

}

#endif
