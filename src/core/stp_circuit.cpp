/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file stp_circuit.cpp
 *
 * @brief Basic Semi-Tensor Product calculation model
 *
 * @author Hongyang Pan
 * @since  0.1
 */

#include "stp_circuit.hpp"

using namespace std;
using namespace Eigen;
using Eigen::MatrixXi;

namespace also
{
    class stp_calc_impl
    {
    public:
        stp_calc_impl(string &expression, vector<string> &tt, vector<MatrixXi> &mtxvec)
            : expression(expression), tt(tt), mtxvec(mtxvec)
        {
        }

        void run()
        {
            parser_from_expression(tt, expression);
            matrix_mapping(tt, mtxvec);
            stp_exchange_judge(tt, mtxvec);
            stp_product_judge(tt, mtxvec);
        }

    private:
        void parser_from_expression(vector<string> &tt, string &expression)
        {
            for (int i = 0; i < expression.size(); i++)
            {

                if ((expression[i] >= 'A' && expression[i] <= 'Z') || (expression[i] >= 'a' && expression[i] <= 'z'))
                {
                    string tmp0;
                    tmp0.push_back(expression[i]);
                    tt.push_back(tmp0);
                }
                else if (expression[i] == '!')
                {
                    string tmp1 = "MN";
                    tt.push_back(tmp1);
                }
                else if (expression[i] == '(')
                {
                    string tmp2 = "MC";
                    tt.push_back(tmp2);
                }
                else if (expression[i] == '{')
                {
                    string tmp3 = "MD";
                    tt.push_back(tmp3);
                }
                else if (expression[i] == '[')
                {
                    string tmp4 = "ME";
                    tt.push_back(tmp4);
                }
                else if (expression[i] == '<')
                {
                    string tmp5 = "MI";
                    tt.push_back(tmp5);
                }
            }
        }

        bool matrix_mapping(vector<string> &tt, vector<MatrixXi> &mtxvec)
        {
            for (int ix = 0; ix < tt.size(); ix++)
            {
                if (tt[ix] == "MN")
                {
                    MatrixXi mtxtmp1(2, 2);
                    mtxtmp1 << 0, 1,
                        1, 0;
                    mtxvec.push_back(mtxtmp1);
                }
                else if (tt[ix] == "MC")
                {
                    MatrixXi mtxtmp2(2, 4);
                    mtxtmp2 << 1, 0, 0, 0,
                        0, 1, 1, 1;
                    mtxvec.push_back(mtxtmp2);
                }
                else if (tt[ix] == "MD")
                {
                    MatrixXi mtxtmp3(2, 4);
                    mtxtmp3 << 1, 1, 1, 0,
                        0, 0, 0, 1;
                    mtxvec.push_back(mtxtmp3);
                }
                else if (tt[ix] == "ME")
                {
                    MatrixXi mtxtmp4(2, 4);
                    mtxtmp4 << 1, 0, 0, 1,
                        0, 1, 1, 0;
                    mtxvec.push_back(mtxtmp4);
                }
                else if (tt[ix] == "MI")
                {
                    MatrixXi mtxtmp5(2, 4);
                    mtxtmp5 << 1, 0, 1, 1,
                        0, 1, 0, 0;
                    mtxvec.push_back(mtxtmp5);
                }
                else
                {
                    MatrixXi mtxtmp6(2, 1);
                    mtxtmp6 << 1,
                        0;
                    mtxvec.push_back(mtxtmp6);
                }
            }
            return true;
        }

        bool stp_exchange_judge(vector<string> &tt, vector<MatrixXi> &mtxvec)
        {
            for (int i = tt.size(); i > 0; i--)
            {
                for (int j = tt.size(); j > 1; j--)
                {
                    if (((tt[j - 1] != "MN") && (tt[j - 1] != "MC") && (tt[j - 1] != "MD") && (tt[j - 1] != "ME") && (tt[j - 1] != "MI") && (tt[j - 1] != "MR") && (tt[j - 1] != "MW")) && ((tt[j - 2] != "MN") && (tt[j - 2] != "MC") && (tt[j - 2] != "MD") && (tt[j - 2] != "ME") && (tt[j - 2] != "MI") && (tt[j - 2] != "MR") && (tt[j - 2] != "MW")))
                    {
                        string tmp1_tt;
                        string tmp2_tt;
                        tmp1_tt += tt[j - 2];
                        tmp2_tt += tt[j - 1];
                        if (tmp1_tt[0] > tmp2_tt[0])
                        {
                            MatrixXi matrix_w2(4, 4);
                            matrix_w2 << 1, 0, 0, 0,
                                0, 0, 1, 0,
                                0, 1, 0, 0,
                                0, 0, 0, 1;
                            string tmp3_tt = "MW";
                            string tmp4_tt = tt[j - 2];
                            tt.insert(tt.begin() + (j - 2), tt[j - 1]);
                            tt.erase(tt.begin() + (j - 1));
                            tt.insert(tt.begin() + (j - 1), tmp4_tt);
                            tt.erase(tt.begin() + j);
                            tt.insert(tt.begin() + (j - 2), tmp3_tt);
                            MatrixXi tmp_mtx = mtxvec[j - 2];
                            mtxvec.insert(mtxvec.begin() + (j - 2), mtxvec[j - 1]);
                            mtxvec.erase(mtxvec.begin() + (j - 1));
                            mtxvec.insert(mtxvec.begin() + (j - 1), tmp_mtx);
                            mtxvec.erase(mtxvec.begin() + j);
                            mtxvec.insert(mtxvec.begin() + (j - 2), matrix_w2);
                        }
                    }
                    if (((tt[j - 2] != "MN") && (tt[j - 2] != "MC") && (tt[j - 2] != "MD") && (tt[j - 2] != "ME") && (tt[j - 2] != "MI") && (tt[j - 2] != "MR") && (tt[j - 2] != "MW")) && ((tt[j - 1] == "MN") || (tt[j - 1] == "MC") || (tt[j - 1] == "MD") || (tt[j - 1] == "ME") || (tt[j - 1] == "MI") || (tt[j - 1] == "MR") || (tt[j - 1] == "MW")))
                    {
                        vector<MatrixXi> tmp1;
                        tmp1.insert(tmp1.begin(), mtxvec[j - 2]);
                        vector<MatrixXi> tmp2;
                        tmp2.insert(tmp2.begin(), mtxvec[j - 1]);
                        stpm_exchange(tmp1, tmp2);
                        string tmp_tt = tt[j - 2];
                        tt.insert(tt.begin() + (j - 2), tt[j - 1]);
                        tt.erase(tt.begin() + (j - 1));
                        tt.insert(tt.begin() + (j - 1), tmp_tt);
                        tt.erase(tt.begin() + j);
                        mtxvec.insert(mtxvec.begin() + (j - 2), tmp1[0]);
                        mtxvec.erase(mtxvec.begin() + (j - 1));
                        mtxvec.insert(mtxvec.begin() + (j - 1), tmp2[0]);
                        mtxvec.erase(mtxvec.begin() + j);
                    }
                    if ((tt[j - 2] == tt[j - 1]) && ((tt[j - 2] != "MN") && (tt[j - 2] != "MC") && (tt[j - 2] != "MD") && (tt[j - 2] != "ME") && (tt[j - 2] != "MI") && (tt[j - 2] != "MR") && (tt[j - 2] != "MW")))
                    {
                        MatrixXi temp_mtx(4, 2);
                        temp_mtx << 1, 0,
                            0, 0,
                            0, 0,
                            0, 1;
                        mtxvec.insert(mtxvec.begin() + (j - 2), temp_mtx);
                        mtxvec.erase(mtxvec.begin() + (j - 1));
                        tt.insert(tt.begin() + (j - 2), "MR");
                        tt.erase(tt.begin() + (j - 1));
                    }
                }
            }
            return true;
        }

        bool stpm_exchange(vector<MatrixXi> &matrix_f, vector<MatrixXi> &matrix_b)
        {
            vector<MatrixXi> exchange_matrix;
            vector<MatrixXi> matrix_i;
            exchange_matrix.insert(exchange_matrix.begin(), matrix_b[0]);
            MatrixXi matrix_tmp(2, 2);
            matrix_tmp << 1, 0,
                0, 1;
            matrix_i.insert(matrix_i.begin(), matrix_tmp);
            matrix_b[0] = matrix_f[0];
            matrix_f[0] = stp_kron_product(matrix_i, exchange_matrix);
            return true;
        }

        bool stp_product_judge(vector<string> &tt, vector<MatrixXi> &mtxvec)
        {
            vector<MatrixXi> temp0;
            vector<MatrixXi> temp1;
            for (int ix = 1; ix < tt.size(); ix++)
            {
                if ((tt[ix] == "MW") || (tt[ix] == "MN") || (tt[ix] == "MC") || (tt[ix] == "MD") || (tt[ix] == "ME") || (tt[ix] == "MI") || (tt[ix] == "MR"))
                {
                    temp0.insert(temp0.begin(), mtxvec[0]);
                    temp1.insert(temp1.begin(), mtxvec[ix]);
                    mtxvec[0] = stpm_basic_product(temp0, temp1);
                }
            }
            return true;
        }

        MatrixXi stpm_basic_product(vector<MatrixXi> matrix_f, vector<MatrixXi> matrix_b)
        {
            int z;
            MatrixXi result_matrix;
            int n_col = matrix_f[0].cols();
            int p_row = matrix_b[0].rows();
            if (n_col % p_row == 0)
            {
                z = n_col / p_row;
                vector<MatrixXi> matrix_i1;
                matrix_i1.insert(matrix_i1.begin(), (MatrixXi::Identity(z, z)));
                result_matrix = matrix_f[0] * stp_kron_product(matrix_b, matrix_i1);
            }
            else if (p_row % n_col == 0)
            {
                z = p_row / n_col;
                vector<MatrixXi> matrix_i2;
                matrix_i2.insert(matrix_i2.begin(), (MatrixXi::Identity(z, z)));
                result_matrix = stp_kron_product(matrix_f, matrix_i2) * matrix_b[0];
            }
            return result_matrix;
        }

        MatrixXi stp_kron_product(vector<MatrixXi> matrix_f, vector<MatrixXi> matrix_b)
        {
            int m = matrix_f[0].rows();
            int n = matrix_f[0].cols();
            int p = matrix_b[0].rows();
            int q = matrix_b[0].cols();
            MatrixXi dynamic_matrix(m * p, n * q);
            for (int i = 0; i < m * p; i++)
            {
                for (int j = 0; j < n * q; j++)
                {
                    dynamic_matrix(i, j) = matrix_f[0](i / p, j / q) * matrix_b[0](i % p, j % q);
                }
            }
            return dynamic_matrix;
        }

    private:
        string &expression;
        vector<string> &tt;
        vector<MatrixXi> &mtxvec;
    };

    void stp_calc(string &expression, vector<string> &tt, vector<MatrixXi> &mtxvec)
    {
        stp_calc_impl p(expression, tt, mtxvec);
        p.run();
    }
}
