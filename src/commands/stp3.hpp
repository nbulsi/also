/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file stp3.hpp
 *
 * @brief Semi-Tensor Product based SAT for CNF
 *
 * @author Hongyang Pan
 * @since  0.1
 */

#ifndef STP3_HPP
#define STP3_HPP

#include "../core/stp_cnf.hpp"
#include <alice/alice.hpp>
#include <mockturtle/mockturtle.hpp>

using namespace std;
using namespace Eigen;
using Eigen::MatrixXi;

namespace alice
{
    class stp_cnf_command : public command
    {
    public:
        explicit stp_cnf_command(const environment::ptr &env) : command(env, " Semi-Tensor Product Calculation Result ")
        {
            add_option("filename,-f", filename, "the input txt file name");
        }

    protected:
        void execute()
        {
            ifstream fin(filename);
            stringstream buffer;
            buffer << fin.rdbuf();
            string str(buffer.str());
            string expression;
            vector<int> expre;
            for (int i = 6; i < (str.size() - 1); i++)
            {
                expression += str[i];
                if ((str[i] == ' ') || (str[i] == '\n'))
                {
                    expression.pop_back();
                    int intstr = atoi(expression.c_str());
                    expre.push_back(intstr);
                    expression.clear();
                }
            }
            vector<int> &exp = expre;
            vector<string> &t = tt;
            vector<MatrixXi> &mtxvec = vec;
            int count = 0;
            stopwatch<>::duration time{0};
            call_with_stopwatch(time, [&]()
                                { stp_cnf(exp, t, mtxvec); });
            if (t.size() > 0)
            {
                cout << "Semi-Tensor Product Result : " << endl;
                vector<string>::iterator ite_t = t.begin();
                for (; ite_t != t.end(); ite_t++)
                {
                    cout << *ite_t << " ";
                    count += 1;
                    if (count == 10)
                    {
                        cout << endl;
                        count = 0;
                    }
                }
                cout << endl;
                cout << "SATISFIABLE" << endl;
            }
            else
            {
                cout << "UNSATISFIABLE" << endl;
            }
            cout << fmt::format("[CPU time]: {:5.3f} seconds\n", to_seconds(time));
            fin.close();
        }

    private:
        string filename;
        string tmp;
        vector<int> expre;
        vector<string> tt;
        vector<MatrixXi> vec;
    };

    ALICE_ADD_COMMAND(stp_cnf, "Exact synthesis")

}

#endif
