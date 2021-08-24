/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file stp_cnf.cpp
 *
 * @brief Semi-Tensor Product based SAT for CNF
 *
 * @author Hongyang Pan
 * @since  0.1
 */

#include "stp_cnf.hpp"

using namespace std;
using namespace Eigen;
using Eigen::MatrixXi;

namespace also
{
    class stp_cnf_impl
    {
    public:
        stp_cnf_impl(vector<int> &expression, vector<string> &tt, vector<MatrixXi> &mtxvec)
            : expression(expression), tt(tt), mtxvec(mtxvec)
        {
        }

        void run()
        {
            parser_from_expression(tt, expression);
            matrix_mapping(tt, mtxvec);
            stp_cnf(tt, mtxvec, expression);
            stp_result(tt);
        }

    private:
        void parser_from_expression(vector<string> &tt, vector<int> &expression)
        {
            string tmp0 = "MN";
            string tmp1 = "END";
            string tmp2 = "MP";
            string tmp3 = "MD";
            string tmp4 = "A";
            for (int i = 2; i < expression.size(); i++)
            {
                if (expression[i] == 0)
                {
                    tt.push_back(tmp1);
                }
                else
                {
                    if ((expression[i - 1] == 0) && (expression[i + 1] == 0))
                    {
                        if (expression[i] < 0)
                        {
                            tt.push_back(tmp0);
                            tt.push_back(tmp4);
                        }
                        else
                        {
                            tt.push_back(tmp2);
                            tt.push_back(tmp4);
                        }
                    }
                    else if (expression[i + 1] == 0)
                    {
                        if (expression[i] < 0)
                        {
                            tt.push_back(tmp0);
                            tt.push_back(tmp4);
                        }
                        else
                        {
                            tt.push_back(tmp4);
                        }
                    }
                    else
                    {
                        tt.push_back(tmp3);
                        if (expression[i] < 0)
                        {
                            tt.push_back(tmp0);
                        }
                        tt.push_back(tmp4);
                    }
                }
                expression[i] = abs(expression[i]);
            }
        }

        void matrix_mapping(vector<string> &tt, vector<MatrixXi> &mtxvec)
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
                else if (tt[ix] == "MP")
                {
                    MatrixXi mtxtmp4(2, 2);
                    mtxtmp4 << 1, 0,
                        0, 1;
                    mtxvec.push_back(mtxtmp4);
                }
                else if (tt[ix] == "MD")
                {
                    MatrixXi mtxtmp2(2, 4);
                    mtxtmp2 << 1, 1, 1, 0,
                        0, 0, 0, 1;
                    mtxvec.push_back(mtxtmp2);
                }
                else
                {
                    MatrixXi mtxtmp3(2, 1);
                    mtxtmp3 << 1,
                        0;
                    mtxvec.push_back(mtxtmp3);
                }
            }
        }

        void stp_cnf(vector<string> &tt, vector<MatrixXi> &mtxvec, vector<int> &expression)
        {
            vector<string> tt_tmp;
            vector<MatrixXi> mtx_tmp;
            vector<int> exp_tmp;
            vector<string> result;
            int v = expression[0];
            string tmp0(v, '2');
            string tmp1 = tmp0;
            string tmp2 = "END";
            expression.erase(expression.begin(), expression.begin() + 2);
            for (int i = 0; i < tt.size(); i++)
            {
                if (tt[i] == "END")
                {
                    stp_cut(tt_tmp, mtx_tmp);
                    for (int j = 0; j < expression.size(); j++)
                    {
                        if (expression[0] == 0)
                        {
                            expression.erase(expression.begin());
                            break;
                        }
                        else
                        {
                            exp_tmp.push_back(expression[0]);
                            expression.erase(expression.begin());
                        }
                    }
                    for (int m = 0; m < tt_tmp.size(); m++)
                    {
                        for (int n = 0; n < exp_tmp.size(); n++)
                        {
                            tmp1[exp_tmp[n] - 1] = tt_tmp[m][n];
                        }
                        result.push_back(tmp1);
                    }
                    result.push_back(tmp2);
                    tmp1 = tmp0;
                    tt_tmp.clear();
                    mtx_tmp.clear();
                    exp_tmp.clear();
                }
                else
                {
                    tt_tmp.push_back(tt[i]);
                    mtx_tmp.push_back(mtxvec[i]);
                }
            }
            tt.clear();
            tt.assign(result.begin(), result.end());
        }

        void stp_result(vector<string> &tt)
        {
            vector<string> result_tmp;
            vector<string> result;
            int length = tt.size();
            int count = 0;
            for (int i = 0; i < length; i++)
            {
                if (tt[0] != "END")
                {
                    result_tmp.push_back(tt[0]);
                    tt.erase(tt.begin());
                }
                else
                {
                    tt.erase(tt.begin());
                    break;
                }
            }
            int v = result_tmp[0].size();
            for (int i = 0; i < tt.size(); i++)
            {
                if (tt[i] != "END")
                {
                    for (int j = 0; j < result_tmp.size(); j++)
                    {
                        count = 0;
                        string tmp0 = result_tmp[j];
                        for (int k = 0; k < v; k++)
                        {
                            if ((tt[i][k] == result_tmp[j][k]) || (result_tmp[j][k] == '2'))
                            {
                                tmp0[k] = tt[i][k];
                                count += 1;
                            }
                            else if (tt[i][k] == '2')
                            {
                                count += 1;
                            }
                            else
                            {
                                break;
                            }
                        }
                        if (count == v)
                        {
                            result.push_back(tmp0);
                        }
                    }
                }
                else
                {
                    result_tmp.clear();
                    result_tmp.assign(result.begin(), result.end());
                    result.clear();
                }
            }
            tt.clear();
            tt.assign(result_tmp.begin(), result_tmp.end());
        }

        void stp_cut(vector<string> &tt, vector<MatrixXi> &mtxvec)
        {
            vector<MatrixXi> mtx_tmp;
            vector<string> tt_tmp;
            vector<MatrixXi> result_b;
            string stp_result;
            vector<string> result;
            int count = 0;
            int length = tt.size();
            for (int i = 0; i < length; i++) //计算每个子式的变量个数
            {
                if (tt[i] == "A")
                {
                    count += 1;
                }
            }
            for (int i = 0; i < length; i++) //CUT计算
            {
                tt_tmp.push_back(tt[0]);
                mtx_tmp.push_back(mtxvec[0]);
                if ((tt[0] != "MN") && (tt[0] != "MD") && (tt[0] != "MP"))
                {
                    tt_tmp.pop_back();
                    mtx_tmp.pop_back();
                    if (tt_tmp.size() >= 1)
                    {
                        stp_exchange_judge(tt_tmp, mtx_tmp);
                        stp_product_judge(tt_tmp, mtx_tmp);
                        result_b.push_back(mtx_tmp[0]);
                    }
                    tt_tmp.clear();
                    mtx_tmp.clear();
                }
                tt.erase(tt.begin());
                mtxvec.erase(mtxvec.begin());
            }
            mtxvec.clear();
            mtxvec.assign(result_b.begin(), result_b.end());
            int target = 1;
            stp_result_enumeration(mtxvec, target, stp_result);
            string tmp;
            for (int j = 0; j < stp_result.size(); j++)
            {
                if (stp_result[j] == '\n')
                {
                    if (tmp.size() == count)
                    {
                        result.push_back(tmp);
                    }
                    if (tmp.size() > count)
                    {
                        int tmp0 = tmp.size() - count - 1;
                        tmp.erase((count - tmp0), tmp0 + 1);
                        result.push_back(tmp);
                    }
                }
                tmp += stp_result[j];
            }
            tt.clear();
            tt.assign(result.begin(), result.end());
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
                if ((tt[ix] == "MW") || (tt[ix] == "MN") || (tt[ix] == "MC") || (tt[ix] == "MD") || (tt[ix] == "ME") || (tt[ix] == "MI") || (tt[ix] == "MR") || (tt[ix] == "MM"))
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

        void stp_result_enumeration(vector<MatrixXi> &mtxvec, int &target, string &stp_result)
        {
            int target_tmp;
            int n = mtxvec[0].cols();
            MatrixXi R = mtxvec[0].topRows(1);
            target_tmp = target;
            if (n == 4)
            {
                for (int j = 0; j < n; j++)
                {
                    if (R(j) == target_tmp)
                    {
                        if (j < (n / 2))
                        {
                            string tmp = "1";
                            stp_result += tmp;
                            if (j < (n / 4))
                            {
                                target = 1;
                                if (mtxvec.size() > 1)
                                {
                                    vector<MatrixXi> temp;
                                    temp.assign(mtxvec.begin() + 1, mtxvec.end());
                                    stp_result_enumeration(temp, target, stp_result);
                                }
                                if (mtxvec.size() == 1)
                                {
                                    string tmp = "1";
                                    stp_result += tmp;
                                    string tmp1 = "\n";
                                    stp_result += tmp1;
                                }
                            }
                            if (j >= (n / 4))
                            {
                                target = 0;
                                if (mtxvec.size() > 1)
                                {
                                    vector<MatrixXi> temp;
                                    temp.assign(mtxvec.begin() + 1, mtxvec.end());
                                    stp_result_enumeration(temp, target, stp_result);
                                }
                                if (mtxvec.size() == 1)
                                {
                                    string tmp = "0";
                                    stp_result += tmp;
                                    string tmp1 = "\n";
                                    stp_result += tmp1;
                                }
                            }
                        }
                        if (j >= (n / 2))
                        {
                            string tmp = "0";
                            stp_result += tmp;
                            if (j < ((3 * n) / 4))
                            {
                                target = 1;
                                if (mtxvec.size() > 1)
                                {
                                    vector<MatrixXi> temp;
                                    temp.assign(mtxvec.begin() + 1, mtxvec.end());
                                    stp_result_enumeration(temp, target, stp_result);
                                }
                                if (mtxvec.size() == 1)
                                {
                                    string tmp = "1";
                                    stp_result += tmp;
                                    string tmp1 = "\n";
                                    stp_result += tmp1;
                                }
                            }
                            if (j >= ((3 * n) / 4))
                            {
                                target = 0;
                                if (mtxvec.size() > 1)
                                {
                                    vector<MatrixXi> temp;
                                    temp.assign(mtxvec.begin() + 1, mtxvec.end());
                                    stp_result_enumeration(temp, target, stp_result);
                                }
                                if (mtxvec.size() == 1)
                                {
                                    string tmp = "0";
                                    stp_result += tmp;
                                    string tmp1 = "\n";
                                    stp_result += tmp1;
                                }
                            }
                        }
                    }
                }
            }
            else if (n == 2)
            {
                for (int j = 0; j < n; j++)
                {
                    if (R(j) == target_tmp)
                    {
                        if (j < (n / 2))
                        {
                            string tmp = "1";
                            stp_result += tmp;
                            string tmp1 = "\n";
                            stp_result += tmp1;
                        }
                        if (j >= (n / 2))
                        {
                            string tmp = "0";
                            stp_result += tmp;
                            string tmp1 = "\n";
                            stp_result += tmp1;
                        }
                    }
                }
            }
        }

    private:
        vector<int> &expression;
        vector<string> &tt;
        vector<MatrixXi> &mtxvec;
    };

    void stp_cnf(vector<int> &expression, vector<string> &tt, vector<MatrixXi> &mtxvec)
    {
        stp_cnf_impl p(expression, tt, mtxvec);
        p.run();
    }
}
