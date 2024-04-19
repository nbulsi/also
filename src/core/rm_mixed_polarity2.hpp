/* also: Advanced Logic Synthesis and Optimization tool*/
/* Copyright (C) 2022- Ningbo University, Ningbo, China */

/*!
  \file rm_mixed_polarity2.hpp
  \brief Multilevel RM logic optimization
  \author Hongwei Zhou
*/

#pragma once

#include <algorithm>
#include <cmath>
#include <iostream>
#include <mockturtle/mockturtle.hpp>
#include <string>
#include <vector>
using namespace std;

namespace mockturtle
{

/*! \brief Parameters for rm_logic_optimization.
 *
 * The data structure `rm_logic_optimization` holds configurable
 * parameters with default arguments for `rm_logic_optimization`.
 */
struct rm_rewriting_params2
{
  /*! \brief Rewriting strategy. */
  enum strategy_t
  {
    /*! \brief Cut is used to divide the network. */
    cut,
    /*! \brief mffc is used to divide the network. */
    mffc,
  } strategy = cut;

  /*! \brief minimum multiplicative complexity in XAG. */
  bool multiplicative_complexity{ false };
};

namespace detail
{

template<class Ntk,class NodeCostFn>
class rm_mixed_polarity_impl2
{
public:
  using node_t = node<Ntk>;
  using signal_t = signal<Ntk>;

  rm_mixed_polarity_impl2( Ntk& ntk, rm_rewriting_params2 const& ps_ntk, cut_rewriting_params const& ps )
      : ntk( ntk ), ps_ntk( ps_ntk ),ps( ps ) {}

  void run()
  {
    switch ( ps_ntk.strategy )
    {
    case rm_rewriting_params2::cut:
      ntk_cut();
      break;
    case rm_rewriting_params2::mffc:
      ntk_mffc();
      break;
    }
  }

private:
/**************************************************************************************************************/
  uint32_t recursive_deref1(Ntk& ntk1, node<Ntk> const& n )
  {
    /* terminate? */
    if ( ntk1.is_constant( n ) || ntk1.is_pi( n ) )
      return 0;

    /* recursively collect nodes */
    uint32_t value = NodeCostFn{}( ntk1, n );
    if ( ntk1.is_and( n ) )
      and_node_deref++;
    ntk1.foreach_fanin( n, [&]( auto const& s ) {
      if ( ntk1.decr_value( ntk1.get_node( s ) ) == 0 )
      {
        value += recursive_deref1( ntk1,ntk1.get_node( s ) );
      }
    } );
    return value;
  }
/**************************************************************************************************************/
  uint32_t recursive_ref1(Ntk& ntk1, node<Ntk> const& n )
  {
    /* terminate? */
    if ( ntk1.is_constant( n ) || ntk1.is_pi( n ) )
      return 0;

    /* recursively collect nodes */
    uint32_t value = NodeCostFn{}( ntk1, n );
    if ( ntk1.is_and( n ) )
      and_node_ref++;
    ntk1.foreach_fanin( n, [&]( auto const& s ) {
      if ( ntk1.incr_value( ntk1.get_node( s ) ) == 0 )
      {
        value += recursive_ref1( ntk1,ntk1.get_node( s ) );
      }
    } );
    return value;
  }
/**************************************************************************************************************/
  std::pair<int32_t, bool> recursive_ref_contains(Ntk& ntk1, node<Ntk> const& n, node<Ntk> const& repl )
  {
    /* terminate? */
    if ( ntk1.is_constant( n ) || ntk1.is_pi( n ) )
      return { 0, false };

    /* recursively collect nodes */
    int32_t value = NodeCostFn{}( ntk1, n );
    if ( ntk1.is_and( n ) )
      and_node_ref++;
    bool contains = ( n == repl );
    ntk1.foreach_fanin( n, [&]( auto const& s ) {
      contains = contains || ( ntk1.get_node( s ) == repl );
      if ( ntk1.incr_value( ntk1.get_node( s ) ) == 0 )
      {
        const auto [v, c] = recursive_ref_contains(ntk1, ntk1.get_node( s ), repl );
        value += v;
        contains = contains || c;
      }
    } );
    return { value, contains };
  }
/**************************************************************************************************************/
void pw_opt_by_onset(std::map<int, std::map<int, int>> onset)//on_set optimization
{

    int x = 0;
    int i, j;
    vector<int> decimalism;
    set<int> decimalism1;                          //Check to see if the elements are all equal
    int L = onset.size(), C = (onset[0]).size(); // L is the length and C is the width
    /*cout << L << " " << C << endl;*/
    if (L == 2)
        return;

    // cout << "------------------------------" << endl;
    // for (int i = 0; i < L; i++)
    //{
    //     for (int j = 0; j <C; j++)
    //     {
    //         cout << onset[i][j] << " ";
    //     }
    //     cout << endl;
    // }
    // cout << "------------------------------" << endl;

    //Delete columns with all zeros
    for (i = 0; i < C; i++)
    {
        for (j = 2; j < L; j++)
        {
            if (onset[j][i] != 0)
                break;
        }
        if (j == L)
        {
            C--;
            for (int k = i; k < C; k++)
            { //Delete column i
                for (int m = 0; m < L; m++)
                {
                    onset[m][k] = onset[m][k + 1];
                }
            }
            i--;
        }
    }

    // cout << "------------------------------" << endl;
    // for (int i = 0; i < L; i++)
    //{
    //     for (int j = 0; j <C; j++)
    //     {
    //         cout << onset[i][j] << " ";
    //     }
    //     cout << endl;
    // }
    // cout << "------------------------------" << endl;

    if (C == 0)
        return;

    // step2
    if (L == 3)
    {
        int count_and = 0;
        for (i = 0; i < C; i++)
        {
            string str = "";
            if (onset[2][i] == 1)
            {
                str = st[onset[1][i]];
                if (onset[0][i] == 0)
                    expression_new += str;
                else
                {
                    expression_new += "!";
                    expression_new += str;
                }
            }
            if (i < C - 1)
            {
                if (expression_new.size() >= 2 && expression_new[expression_new.size() - 2] == '!')
                {
                    expression_new.insert(expression_new.size() - 2, "(");
                    expression_new += "*";
                }
                else
                {
                    expression_new.insert(expression_new.size() - 1, "(");
                    expression_new += "*";
                }
                count_and++;
            }
        }
        if (count_and > 0)
        {
            for (i = 0; i < count_and; i++)
            {
                expression_new += ")";
            }
        }
        return;
    }

    for (i = 2; i < L; i++)
    {
        for (j = 0; j < C; j++)
        {
            if (onset[i][j] == 1)
                x += pow(2, onset[1][j]);
        }
        if (x != 0)
            decimalism1.insert(x);
        if (x == 1)
            decimalism.push_back(0);
        else if (!(x & (x - 1)) && x > 0)
            decimalism.push_back(0);
        else
            decimalism.push_back(1);
        x = 0;
    }
    if (find(decimalism.begin(), decimalism.end(), 1) == decimalism.end() && decimalism1.size() > 1)
    {
        int cout_row_0 = 0;
        int count_xor = 0;

        for (i = 2; i < L; i++)
        {
            int flag = 0;
            for (j = 0; j < C; j++)
            {
                string str = "";
                if (onset[i][j] == 1)
                {
                    str = st[onset[1][j]];
                    if (onset[0][j] == 0)
                        expression_new += str;
                    else
                    {
                        expression_new += "!";
                        expression_new += str;
                    }
                }
            }
            if (i + 1 < L)
            {
                for (j = 0; j < C; j++)
                {
                    if (onset[i + 1][j] != 0)
                        break;
                }

                if (j == C)
                {
                    cout_row_0++;
                    flag = 1;
                }
            }

            if (i < L - 1 && flag == 0)
            {
                if (expression_new.size() >= 2 && expression_new[expression_new.size() - 2] == '!')
                {
                    expression_new.insert(expression_new.size() - 2, "[");
                    expression_new += "+";
                }
                else
                {
                    expression_new.insert(expression_new.size() - 1, "[");
                    expression_new += "+";
                }
                count_xor++;
            }
        }
        if (cout_row_0 % 2 != 0)
        {
            // cout << expression_new[expression_new.size() - 2] << endl;
            if (expression_new[expression_new.size() - 2] == '+')
                expression_new.insert(expression_new.size() - 1, "!");
            else if (expression_new[expression_new.size() - 2] == '!')
            {
                string str = "";
                for (int jj = 0; jj < expression_new.size(); jj++)
                {
                    if (jj != expression_new.size() - 2)
                        str += expression_new[jj];
                }
                expression_new = str;
            }
        }
        if (count_xor > 0)
        {
            for (i = 0; i < count_xor; i++)
            {
                expression_new += "]";
            }
        }

        return;
    }

    // for (int i = 0; i < decimalism.size(); i++)
    //     cout << decimalism[i] << endl;

    //Extract all 1 columns
    int count_and = 0;
    for (i = 0; i < C; i++)
    {
        for (j = 2; j < L; j++)
        {
            if (onset[j][i] != 1)
                break;
        }
        string str = "";
        if (j == L)
        {
            str = st[onset[1][i]];
            if (onset[0][i] == 0)
                expression_new += str;
            else
            {
                expression_new += "!";
                expression_new += str;
            }
            C--;

            if (C > 0)
            {
                if (expression_new.size() >= 2 && expression_new[expression_new.size() - 2] == '!')
                {
                    expression_new.insert(expression_new.size() - 2, "(");
                    expression_new += "*";
                }
                else
                {
                    expression_new.insert(expression_new.size() - 1, "(");
                    expression_new += "*";
                }
                count_and++;
            }

            for (int k = i; k < C; k++)
            { //Delete column i
                for (int m = 0; m < L; m++)
                {
                    onset[m][k] = onset[m][k + 1];
                }
            }
            i--;
        }
    }
    if (C == 0 && count_and > 0)
    {
        for (i = 0; i < count_and; i++)
        {
            expression_new += ")";
        }
        return;
    }
    else if (C == 0 && count_and == 0)
        return;

    int cout_1 = 0, cout_0 = 0;
    if (C == 1)
    {
        for (i = 2; i < L; i++)
        {
            if (onset[i][0] == 0)
            {
                cout_0++;
            }
            else
                cout_1++;
        }

        if (cout_0 % 2 != 0 && onset[0][0] == 0 && cout_1 % 2 != 0)
        {
            expression_new += "!";
            expression_new += st[onset[1][0]];
        }
        else if (cout_0 % 2 != 0 && onset[0][0] == 0 && cout_1 % 2 == 0)
            expression_new += "1";
        else if (cout_0 % 2 == 0 && onset[0][0] == 0 && cout_1 % 2 != 0)
            expression_new += st[onset[1][0]];
        else if (cout_0 % 2 == 0 && onset[0][0] == 0 && cout_1 % 2 == 0)
            expression_new += "";
        else if (cout_0 % 2 != 0 && onset[0][0] == 1 && cout_1 % 2 != 0)
            expression_new += st[onset[1][0]];
        else if (cout_0 % 2 != 0 && onset[0][0] == 1 && cout_1 % 2 == 0)
            expression_new += "";
        else if (cout_0 % 2 == 0 && onset[0][0] == 1 && cout_1 % 2 != 0)
        {
            expression_new += "!";
            expression_new += st[onset[1][0]];
        }
        else if (cout_0 % 2 == 0 && onset[0][0] == 1 && cout_1 % 2 == 0)
            expression_new += "1";

        if (count_and == 0)
            return;
        else if (count_and > 0)
        {
            for (i = 0; i < count_and; i++)
            {
                expression_new += ")";
            }
            return;
        }
    }

    int count_row_0 = 0; //Delete all 0 lines and invert any xor items
    std::map<int, std::map<int, int>> temporary;
    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < C; j++)
        {
            temporary[i][j] = onset[i][j];
        }
    }

    // cout << "temporary: " << endl;
    // for (i = 0; i < temporary.size(); i++)
    //{
    //     for (j = 0; j < temporary[i].size(); j++)
    //     {
    //         cout << temporary[i][j] << " ";
    //     }
    //     cout << endl;
    // }

    int a = 2;
    int flag_row_0 = 0;
    for (i = 2; i < L; i++)
    {
        for (j = 0; j < C; j++)
        {
            if (onset[i][j] != 0)
                break;
        }

        if (j == C)
        {
            count_row_0++;
        }
        else if (j != C)
        {
            for (j = 0; j < C; j++)
            {
                temporary[a][j] = onset[i][j];
            }
            a++;
        }
    }

    if (count_row_0 % 2 != 0)
    {
        expression_new += "[";
        expression_new += "1";
        flag_row_0 = 1;
    }

    onset.clear();
    onset = temporary;
    L = onset.size();
    C = (onset[0]).size();

    // cout << "temporary: " << endl;
    // for (i = 0; i < temporary.size(); i++)
    //{
    //     for (j = 0; j < temporary[i].size(); j++)
    //     {
    //         cout << temporary[i][j] << " ";
    //     }
    //     cout << endl;
    // }

    //Extraction of local public variables
    std::map<int, int> maxi;
    maxi.clear();
    for (i = 0; i < C; i++)
    {
        //Statistical maxi
        maxi[i] = 0;
        vector<string> row(L - 2, "");
        int n = 0;
        for (j = 2; j < L; j++)
        {
            for (int k = 0; k < C; k++)
            {
                if (k != i)
                    row[n] += onset[j][k] + '0';
            }
            n++;
        }
        for (int m = 0; m < row.size(); m++)
        {
            int cnt = count(row.begin(), row.end(), row[m]);
            int count_1 = count(row[m].begin(), row[m].end(), '1');
            int sum = cnt * count_1;
            if (sum > maxi[i])
                maxi[i] = sum;
        }

         //cout << "-----------------------" << endl;
         //for (int m = 0; m< row.size(); m++)
         //    cout << row[m] << endl;
         //cout << maxi[i] << endl;
         //cout << "-----------------------" << endl;
    }

    // for (i = 0; i < C; i++)
    //     cout << maxi[i] << " ";
    // cout << endl;

    int k = C / 2;
    vector<int> CK(k, 0);
    for (i = 0; i < k; i++)
    { //Find CK
        int max = maxi[0];
        for (j = 1; j < C; j++)
        {
            if (maxi[j] > max)
            {
                max = maxi[j];
                CK[i] = j;
            }
        }
        maxi[CK[i]] = -1;
    }

     //for (i = 0; i < CK.size(); i++)
     //    cout << "CK: " << CK[i] << endl;

    int temp;
    for (i = 0; i < k; i++)
    { //Swap CK to onset table behind
        for (j = 0; j < L; j++)
        {
            temp = onset[j][CK[i]];
            onset[j][CK[i]] = onset[j][C - i - 1];
            onset[j][C - i - 1] = temp;
        }
    }

    //Looking for LK
    vector<string> onset_string(L, ""); //Convert the onset table to a CK string stripping table
    int nn = 0;
    for (i = 0; i < L; i++)
    {
        for (j = 0; j < C - k; j++)
        {
            onset_string[nn] += onset[i][j] + '0';
        }
        nn++;
    }

     //for (i = 0; i < onset_string.size(); i++)
     //   cout << onset_string[i] << endl;
     //cout << endl;

    int max = 0, index = 0;
    int LK_count = 0;
    for (i = 2; i < onset_string.size(); i++)
    {
        int cnt = count(onset_string.begin()+2, onset_string.end(), onset_string[i]);
        int count_1 = count(onset_string[i].begin(), onset_string[i].end(), '1');
        int sum = cnt * count_1;
        if (sum > max)
        {
            index = i;
            max = sum;
            LK_count = cnt;
        }
    }

    /*cout << "index: " << index << endl;*/

    vector<string> onset_string1(L, ""); //Convert the onset table to the onset string table
    for (i = 0; i < L; i++)
    {
        for (j = 0; j < C; j++)
        {
            onset_string1[i] += onset[i][j] + '0';
        }
    }

    // for (i = 0; i < onset_string1.size(); i++)
    //     cout << onset_string1[i] << endl;
    // cout << endl;

    string str = onset_string1[index].substr(0, C - k);
    // cout << "str: " << str << endl;
    vector<string> temp1; //Storage LK
    temp1.clear();
    for (i = 2; i < onset_string1.size(); i++)
    {
        if (str == onset_string1[i].substr(0, C - k))
        {
            temp1.push_back(onset_string1[i]);
        }
    }
    /*cout << "temp1:" << endl;
     for (i = 0; i < temp1.size(); i++)
         cout << temp1[i] << endl;
     cout << endl;*/

    vector<string> temp2; //Stores other child tables
    temp2.clear();
    for (i = 2; i < onset_string1.size(); i++)
    {
        if (str != onset_string1[i].substr(0, C - k))
        {
            temp2.push_back(onset_string1[i]);
        }
    }

   /* cout << "temp2:" << endl;
    for (i = 0; i < temp2.size(); i++)
        cout << temp2[i] << endl;
    cout << endl;*/

    vector<string> onset_string_new; //Stores the onset table after it has been transformed
    onset_string_new.push_back(onset_string1[0]);
    onset_string_new.push_back(onset_string1[1]);
    for (i = 0; i < temp2.size(); i++)
    {
        onset_string_new.push_back(temp2[i]);
    }
    for (i = 0; i < temp1.size(); i++)
    {
        onset_string_new.push_back(temp1[i]);
    }

     //for (i = 0; i < onset_string_new.size(); i++)
     //    cout << onset_string_new[i] << endl;
    for (i = 0; i < L; i++)
    {
        for (j = 0; j < C; j++)
        {
            onset[i][j] = onset_string_new[i][j] - '0';
        }
    }
    // cout << endl;
    //cout <<"LK_count: " << LK_count << endl;
    std::map<int, std::map<int, int>> onset_st1;
    std::map<int, std::map<int, int>> onset_st2;
    onset_st1.clear();
    onset_st2.clear();

    for (i = 0; i < L - LK_count; i++) //Store ST2
    {
        for (j = 0; j < C; j++)
        {
            onset_st2[i][j] = onset[i][j];
        }
    }
    // cout << "ST2: " << endl;
    // for (i = 0; i < onset_st2.size(); i++)
    //{
    //     for (j = 0; j < onset_st2[i].size(); j++)
    //     {
    //         cout << onset_st2[i][j] << " ";
    //     }
    //     cout << endl;
    // }

    // cout << endl;

    //Store ST1
    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < C; j++)
        {
            onset_st1[i][j] = onset[i][j];
        }
    }

    int n = 2;
    for (i = L - LK_count; i < L; i++)
    {
        for (j = 0; j < C; j++)
        {
            onset_st1[n][j] = onset[i][j];
        }
        n++;
    }

    // cout << "st1: " << endl;
    // for (i = 0; i < onset_st1.size(); i++)
    //{
    //     for (j = 0; j < onset_st1[i].size(); j++)
    //     {
    //         cout << onset_st1[i][j] << " ";
    //     }
    //     cout << endl;
    // }

    // cout << endl;

    int flag1 = 0;
    for (i = 2; i < L; i++)
    {
        for (j = 0; j < C; j++)
        {
            if (onset_st1[i][j] == 1)
            {
                flag1 = 1;
                break;
            }
        }
        if (flag1 == 1)
            break;
    }
    int flag2 = 0;
    for (i = 2; i < L; i++)
    {
        for (j = 0; j < C; j++)
        {
            if (onset_st2[i][j] == 1)
            {
                flag2 = 1;
                break;
            }
        }
        if (flag2 == 1)
            break;
    }

    if (flag1 == 1 && flag2 == 1 && onset_st1.size() > 2 && onset_st2.size() > 2)
        expression_new += "[";
    if (flag1 == 1)
        pw_opt_by_onset(onset_st1);
    if (flag1 == 1 && flag2 == 1 && onset_st1.size() > 2 && onset_st2.size() > 2)
        expression_new += "+";
    if (flag2 == 1)
        pw_opt_by_onset(onset_st2);
    if (flag1 == 1 && flag2 == 1 && onset_st1.size() > 2 && onset_st2.size() > 2)
        expression_new += "]";

    if (flag_row_0 == 1)
        expression_new += "]";

    if (count_and > 0)
    {
        for (i = 0; i < count_and; i++)
        {
            expression_new += ")";
        }
    }

    // cout << "------------------------------" << endl;
    // for (int i = 0; i < L; i++)
    //{
    //     for (int j = 0; j < C; j++)
    //     {
    //         cout << onset[i][j] << " ";
    //     }
    //     cout << endl;
    // }
    // cout << "------------------------------" << endl;
}
  /**************************************************************************************************************/
  void adjust_the_expression()
  {
    vector<string> symbol;
    for ( int i = 0; i < expression_new.size(); i++ )
    {
      if ( expression_new[i] == '*' )
      {
        continue;
      }
      else if ( expression_new[i] == '+' )
      {
        continue;
      }
      else if ( expression_new[i] == '-' )
      {
        continue;
      }
      else
      {
        expression_adjusted += expression_new[i];
      }
    }
  }
  /**************************************************************************************************************/
  /* for cost estimation we use reference counters initialized by the fanout
   * size. */
  void initialize_refs( Ntk& ntk1 )
  {
    ntk1.clear_values();
    ntk1.foreach_node(
        [&]( auto const& n ) { ntk1.set_value( n, ntk1.fanout_size( n ) ); } );
  }
  /**************************************************************************************************************/
 /* Count the number of nodes in the new network. */
  int count_the_number_of_nodes( int& variate_num, vector<string>& RM_product,
                                 string& optimal_polarity )
  {
    /* Count the number of nodes. */
    int node_num = 0;
    if ( RM_product.size() > 1 )
      node_num = RM_product.size() - 1;

    if ( variate_num > 1 )
    {
      for ( int i = 0; i < RM_product.size(); i++ )
      {
        int n = 0;
        for ( int j = variate_num - 1; j >= 0; j-- )
        {
          if ( RM_product[i][j] == '1' || optimal_polarity[j] == '2' )
            n++;
        }
        if ( n > 0 )
          node_num = node_num + n - 1;
      }
    }
    return node_num;
  }
  /**************************************************************************************************************/
  /* Convert ternary to decimal. */
  int atoint( string s, int radix )
  {
    int ans = 0;
    for ( int i = 0; i < s.size(); i++ )
    {
      char t = s[i];
      if ( t >= '0' && t <= '9' )
        ans = ans * radix + t - '0';
      else
        ans = ans * radix + t - 'a' + 10;
    }
    return ans;
  }
  /**************************************************************************************************************/
  /* Convert decimal to ternary. */
  string intToA( int n, int radix )
  {
    string ans = "";
    do
    {
      int t = n % radix;
      if ( t >= 0 && t <= 9 )
        ans += t + '0';
      else
        ans += t - 10 + 'a';
      n /= radix;
    } while ( n != 0 );
    reverse( ans.begin(), ans.end() );
    return ans;
  }
  /**************************************************************************************************************/
  /* create signal by expression. */
  signal_t create_ntk_from_str( std::string const& s,std::vector<signal<Ntk>>const& children )
  {
    std::vector<signal_t> pis;
    pis.clear();
    pis.push_back( ntk.get_constant( true ) );
    for ( const auto& l : children )
    {
      pis.push_back( l );
    }

    std::stack<signal_t> inputs;
    int flag = 0;

    for ( auto i = 0; i < s.size(); i++ )
    {
      if ( s[i] == '[' )
      {
        continue;
      }
      else if ( s[i] == '(' )
      {
        continue;
      }
      else if ( s[i] == '1' )
      {
        inputs.push( pis[0] );
      }
      else if ( s[i] >= 'a' )
      {
        if ( flag == 1 )
        {
          inputs.push( ntk.create_not( pis[s[i] - 'a'+1] ) );
          flag = 0;
        }
        else
          inputs.push( pis[s[i] - 'a'+1] );
      }
      else if ( s[i] == ')' )
      {
        auto x1 = inputs.top();
        inputs.pop();

        auto x2 = inputs.top();
        inputs.pop();

        inputs.push( ntk.create_and( x1, x2 ) );
      }
      else if ( s[i] == ']' )
      {
          auto x1 = inputs.top();
          inputs.pop();

          auto x2 = inputs.top();
          inputs.pop();
          inputs.push( ntk.create_xor( x1, x2 ) );
      }
      else if ( s[i] == '!' )
      {
        flag = 1;
      }
    }
    if(s.size()==0)
      inputs.push(ntk.get_constant( false ));
    assert( inputs.size() == 1u );
    return inputs.top();
  }
  /**************************************************************************************************************/
  /* List all possible variable values/List truth table. */
  vector<string> list_truth_table( int& variate_num )
  {
    int a = pow( 2, variate_num );
    vector<int> b;
    for ( int i = 0; i < a; i++ )
    {
      b.push_back( i );
    }
    vector<string> binary;
    string binary1;
    for ( int i = 0; i < a; i++ )
    {
      binary1 = { "" };
      for ( int j = variate_num - 1; j >= 0; j-- )
      {
        int d = ( ( b[i] >> j ) & 1 );
        binary1 += d + 48;
      }
      binary.push_back( binary1 );
    }
    return binary;
  }
  /**************************************************************************************************************/
  /* List all possible polarity values. */
  vector<string> list_all_polarities( int& variate_num )
  {
    int a1 = pow( 2, variate_num );
    vector<int> c;
    for ( int i = 0; i < a1; i++ )
    {
      c.push_back( i );
    }
    vector<string> polarity1;
    string polarity2;
    for ( int i = 0; i < a1; i++ )
    {
      polarity2 = { "" };
      for ( int j = variate_num - 1; j >= 0; j-- )
      {
        int d = ( ( c[i] >> j ) & 1 );
        polarity2 += d + 48;
      }
      polarity1.push_back( polarity2 );
    }

    /* Store all polarities. */
    vector<string> polarity;
    polarity.push_back( polarity1[0] );
    int a2 = pow( 3, variate_num );
    string str1 = polarity1[0];
    string str2 = polarity1[1];
    for ( int i = 0; i < a2 - 1; i++ )
    {
      int num1 = atoint( str1, 3 );
      int num2 = atoint( str2, 3 );

      int sum = num1 + num2;
      string str3 = intToA( sum, 3 );

      str1 = "0";
      if ( str3.size() < variate_num )
      {
        for ( int i = 0; i < variate_num - str3.size() - 1; i++ )
          str1 += '0';

        str1 += str3;
      }
      else
        str1 = str3;

      polarity.push_back( str1 );
    }
    return polarity;
  }
  /**************************************************************************************************************/
  /* Mixed polarity conversion algorithm based on list technique. */
  vector<string> polarity_conversion( int& variate_num, vector<string>& minterm,
                                      string& polarity )
  {
    vector<string> RM_product = minterm;
    string str;
    for ( int i = 0; i < variate_num; i++ )
    {
      for ( int j = 0; j < RM_product.size(); j++ )
      {
        if ( polarity[i] == '0' )
        {
          if ( RM_product[j][i] == '0' )
          {
            str = RM_product[j];
            str[i] = '1';
            RM_product.push_back( str );
          }
        }
        else if ( polarity[i] == '1' )
        {
          if ( RM_product[j][i] == '1' )
          {
            str = RM_product[j];
            str[i] = '0';
            RM_product.push_back( str );
          }
        }
      }
      if ( polarity[i] == '0' )
      {
        /* Delete the minimum number of even items. */
        if ( RM_product.size() > 0 )
        {
          sort( RM_product.begin(), RM_product.end() );
          for ( int m = 0; m < RM_product.size() - 1; m++ )
          {
            if ( RM_product[m] == RM_product[m + 1] )
            {
              RM_product[m] = "0";
              RM_product[m + 1] = "0";
            }
          }
        }
        /* Delete item with '0' in array. */
        vector<string> final_term;
        final_term.clear();
        for ( int m = 0; m < RM_product.size(); m++ )
        {
          if ( RM_product[m] != "0" )
            final_term.push_back( RM_product[m] );
        }
        RM_product.clear();
        RM_product = final_term;
      }
      else if ( polarity[i] == '1' )
      {
        /* Delete the minimum number of even items. */
        if ( RM_product.size() > 0 )
        {
          sort( RM_product.begin(), RM_product.end() );
          for ( int m = 0; m < RM_product.size() - 1; m++ )
          {
            if ( RM_product[m] == RM_product[m + 1] )
            {
              RM_product[m] = "0";
              RM_product[m + 1] = "0";
            }
          }
        }
        /* Delete item with '0' in array. */
        vector<string> final_term;
        final_term.clear();
        for ( int m = 0; m < RM_product.size(); m++ )
        {
          if ( RM_product[m] != "0" )
          {
            if ( RM_product[m][i] == '0' )
              RM_product[m][i] = '1';
            else if ( RM_product[m][i] == '1' )
              RM_product[m][i] = '0';
            final_term.push_back( RM_product[m] );
          }
        }
        RM_product.clear();
        RM_product = final_term;
      }
    }
    return RM_product;
  }
  /**************************************************************************************************************/
  /* Search for the optimal polarity and the corresponding product term. */
  void search_for_optimal_polarity( vector<string>& minterm,
                                    vector<string>& polarity, int& variate_num,
                                    vector<string>& RM_product,
                                    string& optimal_polarity )
  {
    int minimum = INT_MAX;
    vector<string> RM_product_initial;
    for ( int l = 0; l < polarity.size(); l++ )
    {
      RM_product_initial =
          polarity_conversion( variate_num, minterm, polarity[l] );

      int count = 0;
      count = count_the_number_of_nodes( variate_num, RM_product_initial, polarity[l] );

      if ( count < minimum )
      {
        optimal_polarity = polarity[l];
        RM_product.clear();
        RM_product = RM_product_initial;
        RM_product_initial.clear();
        minimum = count;
      }
      else
      {
        RM_product_initial.clear();
      }
    }
  }
  /**************************************************************************************************************/
  /* Create expression. */
  string create_expression( vector<string>& RM_product, int& variate_num,
                            string& optimal_polarity, vector<string>& binary )
  {
    string st = "abcdefghijklmnopqrstuvwxyz";
    string expression = "";
    if ( RM_product.size() > 1 )
      for ( int i = 0; i < RM_product.size() - 1; i++ )
        expression += '[';
    for ( int i = 0; i < RM_product.size(); i++ )
    {
      int k = 0;
      int l = 0;
      int m = 0;
      for ( int j = variate_num - 1; j >= 0; j-- )
      {
        if ( RM_product[i][j] == '1' || optimal_polarity[j] == '2' )
          l++;
      }
      for ( int j = 0; j < l - 1; j++ )
        expression += '(';
      for ( int j = variate_num - 1; j >= 0; j-- )
      {
        if ( optimal_polarity[j] == '0' && RM_product[i][j] == '1' )
        {
          expression += st[k];
          m++;
        }
        else if ( optimal_polarity[j] == '1' && RM_product[i][j] == '1' )
        {
          expression += '!';
          expression += st[k];
          m++;
        }
        else if ( optimal_polarity[j] == '2' && RM_product[i][j] == '1' )
        {
          expression += st[k];
          m++;
        }
        else if ( optimal_polarity[j] == '2' && RM_product[i][j] == '0' )
        {
          expression += '!';
          expression += st[k];
          m++;
        }

        if ( ( m > 1 && RM_product[i][j] == '1' ) ||
             ( m > 1 && optimal_polarity[j] == '2' ) )
          expression += ')';
        k++;
      }
      if ( RM_product[i] == binary[0] )
      {
        int flag = 0;
        for ( int m = variate_num - 1; m >= 0; m-- )
        {
          if ( optimal_polarity[m] == '2' )
            flag = 1;
        }
        if ( flag == 0 )
          expression += '1';
      }
      if ( i > 0 && RM_product.size() > 1 )
        expression += ']';
    }
    return expression;
  }
  /**************************************************************************************************************/
  /* Cut is used to divide the network. */
  void ntk_cut()
  {
     /* enumerate cuts */
     const auto cuts =  cut_enumeration<Ntk, true, cut_enumeration_cut_rewriting_cut>( ntk, ps.cut_enumeration_ps );

     /* for cost estimation we use reference counters initialized by the fanout size */
     ntk.clear_values();
     ntk.foreach_node( [&]( auto const& n ) {
       ntk.set_value( n, ntk.fanout_size( n ) );
     } );

     /* store best replacement for each cut */
     node_map<std::vector<signal<Ntk>>, Ntk> best_replacements( ntk );

     /* iterate over all original nodes in the network */
     const auto size = ntk.size();
     auto max_total_gain = 0u;
     progress_bar pbar{ ntk.size(), "rm_optimization_by_cut |{0}| node = {1:>4}@{2:>2} / " + std::to_string( size ) + "   comm. gain = {3}", ps.progress };
     ntk.foreach_node( [&]( auto const& n, auto index ) {
      /* stop once all original nodes were visited */
      if ( index >= size )
        return false;

      /* do not iterate over constants or PIs */
      if ( ntk.is_constant( n ) || ntk.is_pi( n ) )
        return true;

      /* skip cuts with small MFFC */
      if ( mffc_size( ntk, n ) == 1 )
        return true;

      /* foreach cut */
      int a=0;
      int32_t best_gain=-1;
      int32_t best_cut=0;
      signal<Ntk> best_signal;
      for ( auto& cut : cuts.cuts( ntk.node_to_index( n ) ) )
      {
        a++;
        /* skip trivial cuts */
        if ( cut->size() < ps.min_cand_cut_size )
          continue;

        pbar( index, ntk.node_to_index( n ), best_replacements[n].size(), max_total_gain );

        std::vector<signal<Ntk>> children;
        for ( auto l : *cut )
        {
          children.push_back( ntk.make_signal( ntk.index_to_node( l ) ) );
        }

        and_node_deref=0;
        int32_t value =recursive_deref1( ntk, n );
        {
          string tt = to_binary( cuts.truth_table( *cut ) );

          int len = tt.length();
          /* Stores the number of input variables. */
          int variate_num = log( len ) / log( 2 );

          /* List all possible variable values/List truth table. */
          vector<string> binary = list_truth_table( variate_num );

          /* Stores the output of the truth table. */
          vector<char> c_out;
          for ( int i = len - 1; i >= 0; i-- )
          {
            c_out.push_back( tt[i] );
          }

          vector<char> c_out1;
          /* Minimum item for storing binary. */
          vector<string> minterm;
          for ( int i = 0; i < binary.size(); i++ )
          {
            if ( c_out[i] != '0' )
            {
              minterm.push_back( binary[i] );
              c_out1.push_back( c_out[i] );
            }
          }

          /* Store all polarities. */
          vector<string> polarity = list_all_polarities( variate_num );

          /* Storage optimal polarity. */
          string optimal_polarity;
          /* Mixed polarity conversion algorithm based on list technique. */
          vector<string> RM_product;

          /* Search for the optimal polarity and the corresponding product term.*/
          search_for_optimal_polarity( minterm, polarity, variate_num, RM_product,
                                     optimal_polarity);
          
          int pos = optimal_polarity.find("2");
          string expression="";
          if (pos != optimal_polarity.npos)
          {
            expression = create_expression( RM_product, variate_num,optimal_polarity, binary);
          }
          else
          {
            onset.clear();//初始化onset表
            for(int i=0;i<optimal_polarity.size();i++)
              onset[0][i]=optimal_polarity[i]-'0';

            for(int i=0;i<variate_num;i++)
              onset[1][i]=variate_num-i-1;

            for(int i=0;i<RM_product.size();i++)
            {
              for(int j=0;j<variate_num;j++)
              {
                onset[i+2][j]=RM_product[i][j]-'0';
              }
            } 


          // cout<<"-----------------------------------"<<endl;
          //   for(int i=0;i<onset.size();i++)
          //   {
          //     for(int j=0;j<onset[i].size();j++)
          //       cout<<onset[i][j]<<" ";
          //     cout<<endl;
          //   }
          //   cout<<endl;
          // cout<<"-----------------------------------"<<endl;
            expression_new= "";
            pw_opt_by_onset(onset);
            expression_adjusted="";
            adjust_the_expression();
            expression=expression_adjusted;
          }

          auto f_new = create_ntk_from_str(expression, children );

          and_node_ref=0;
          auto [v, contains] = recursive_ref_contains(ntk, ntk.get_node( f_new ), n );
          recursive_deref<Ntk, NodeCostFn>( ntk, ntk.get_node( f_new ) );

          int32_t gain = contains ? -1 : value - v;
          int32_t and_node_num=and_node_deref-and_node_ref;

          if (ps_ntk.multiplicative_complexity == true)
          {
            if(((gain>=0)||(ps.allow_zero_gain && gain == 0))&&gain>best_gain&&and_node_num>=0)
            {
              best_gain= gain;
              best_signal=f_new;
              best_cut=a;
              //best_replacements[n].push_back( f_new );
            }
          }
          else
          {
            if(((gain>0)||(ps.allow_zero_gain && gain == 0))&&gain>best_gain)
            {
              best_gain= gain;
              best_signal=f_new;
              best_cut=a;
              //best_replacements[n].push_back( f_new );
            }
          }

         

          if ( best_gain > 0 )
          {
            max_total_gain += best_gain;
          }
        }

        recursive_ref<Ntk, NodeCostFn>( ntk, n );
      }

      if(best_gain!=-1)
      {
        int b=0;
        for ( auto& cut : cuts.cuts( ntk.node_to_index( n ) ) )
        {
          b++;
          /* skip trivial cuts */
          if ( cut->size() < ps.min_cand_cut_size )
            continue;

          if(b==best_cut)
          {
            ( *cut )->data.gain = best_gain;
            best_replacements[n].push_back( best_signal );
          }

        }
        
      }
      

      return true;
    } );

    auto [g, map] = network_cuts_graph( ntk, cuts, ps );

    const auto is = ( ps.candidate_selection_strategy == cut_rewriting_params::minimize_weight ) ? maximum_weighted_independent_set_gwmin( g ) : maximal_weighted_independent_set( g );

    for ( const auto v : is )
    {
      const auto v_node = map[v].first;
      const auto v_cut = map[v].second;

      if ( best_replacements[v_node].empty() )
        continue;

      const auto replacement = best_replacements[v_node][v_cut];

      if ( ntk.is_constant( ntk.get_node( replacement ) ) || v_node == ntk.get_node( replacement ) )
        continue;

      if ( !ntk.is_dead( ntk.get_node( replacement ) ) )
      {
        ntk.substitute_node( v_node, replacement );
      }
    }

  }
  /**************************************************************************************************************/
  /* mffc is used to divide the network. */
  void ntk_mffc()
  {
    progress_bar pbar{ ntk.size(), "rm_optimization_by_mffc |{0}| node = {1:>4}   cand = {2:>4}   est. reduction = {3:>5}", ps.progress };

    ntk.clear_visited();
    ntk.clear_values();
    ntk.foreach_node( [&]( auto const& n ) {
      ntk.set_value( n, ntk.fanout_size( n ) );
    } );

    const auto size = ntk.num_gates();
    ntk.foreach_gate( [&]( auto const& n, auto i ) {
      if ( i >= size )
      {
        return false;
      }
      if ( ntk.fanout_size( n ) == 0u )
      {
        return true;
      }
      mffc_view mffc{ ntk, n };

      pbar( i, i, _candidates, _estimated_gain );

      if ( mffc.num_pos() == 0 || mffc.num_pis() > 6)
      {
        return true;
      }

      std::vector<signal<Ntk>> leaves( mffc.num_pis() );
      mffc.foreach_pi( [&]( auto const& m, auto j ) {
        leaves[j] = ntk.make_signal( m );
      } );

      default_simulator<kitty::dynamic_truth_table> sim( mffc.num_pis() );
      string tt = to_binary( simulate<kitty::dynamic_truth_table>( mffc, sim )[0] );

      int len = tt.length();
      /* Stores the number of input variables. */
      int variate_num = log( len ) / log( 2 );

      /* List all possible variable values/List truth table. */
      vector<string> binary = list_truth_table( variate_num );

      /* Stores the output of the truth table. */
      vector<char> c_out;
      for ( int i = len - 1; i >= 0; i-- )
      {
        c_out.push_back( tt[i] );
      }

      vector<char> c_out1;
      /* Minimum item for storing binary. */
      vector<string> minterm;
      for ( int i = 0; i < binary.size(); i++ )
      {
        if ( c_out[i] != '0' )
        {
          minterm.push_back( binary[i] );
          c_out1.push_back( c_out[i] );
        }
      }

      /* Store all polarities. */
      vector<string> polarity = list_all_polarities( variate_num );

      /* Count the initial cost. */
      /* Storage optimal polarity. */
      string optimal_polarity = polarity[0];
      /* Mixed polarity conversion algorithm based on list technique. */
      vector<string> RM_product =
          polarity_conversion( variate_num, minterm, optimal_polarity );

      /* Search for the optimal polarity and the corresponding product term. */
      search_for_optimal_polarity( minterm, polarity, variate_num, RM_product,
                                   optimal_polarity );


      int pos = optimal_polarity.find("2");
      string expression="";
      if (pos != optimal_polarity.npos)
      {
        expression = create_expression( RM_product, variate_num,optimal_polarity, binary);
      }
      else
      {
        onset.clear();//初始化onset表
        for(int i=0;i<optimal_polarity.size();i++)
          onset[0][i]=optimal_polarity[i]-'0';

        for(int i=0;i<variate_num;i++)
          onset[1][i]=variate_num-i-1;

        for(int i=0;i<RM_product.size();i++)
        {
          for(int j=0;j<variate_num;j++)
          {
            onset[i+2][j]=RM_product[i][j]-'0';
          }
        } 

        expression_new= "";
        pw_opt_by_onset(onset);
        expression_adjusted="";
        adjust_the_expression();
        expression=expression_adjusted;
      }

      auto f_new = create_ntk_from_str(expression, leaves ); 

      and_node_deref=0;
      int32_t value =recursive_deref1( ntk, n ); 

      and_node_ref=0;
      auto value2 = recursive_ref1( ntk, ntk.get_node( f_new ) );

      int32_t gain = value - value2;   
      int32_t and_node_num=and_node_deref-and_node_ref;

    if (ps_ntk.multiplicative_complexity == true)
    {
      if ( (gain > 0&&and_node_num>=0))
      {
        ++_candidates;
        _estimated_gain += gain;
        ntk.substitute_node( n, f_new );

        ntk.set_value( n, 0 );
        ntk.set_value( ntk.get_node( f_new ), ntk.fanout_size( ntk.get_node( f_new ) ) );
        for ( auto i = 0u; i < leaves.size(); i++ )
        {
          ntk.set_value( ntk.get_node( leaves[i] ), ntk.fanout_size( ntk.get_node( leaves[i] ) ) );
        }
      }
      else
      {
        recursive_deref( ntk, ntk.get_node( f_new ) );
        recursive_ref( ntk, n);
      }  
    }
    else
    {
      if ( (gain > 0))
      {
        ++_candidates;
        _estimated_gain += gain;
        ntk.substitute_node( n, f_new );

        ntk.set_value( n, 0 );
        ntk.set_value( ntk.get_node( f_new ), ntk.fanout_size( ntk.get_node( f_new ) ) );
        for ( auto i = 0u; i < leaves.size(); i++ )
        {
          ntk.set_value( ntk.get_node( leaves[i] ), ntk.fanout_size( ntk.get_node( leaves[i] ) ) );
        }
      }
      else
      {
        recursive_deref( ntk, ntk.get_node( f_new ) );
        recursive_ref( ntk, n);
      }  
    }
    
      return true;
    } );  
  }

  /**************************************************************************************************************/
private:
  int and_node_deref=0;
  int and_node_ref=0;
  string expression_new = "";
  string expression_adjusted = "";
  string st = "abcdefghijklmnopqrstuvwxyz";
  std::map<int, std::map<int, int>> onset;
  Ntk& ntk;
  rm_rewriting_params2 const& ps_ntk;
  cut_rewriting_params const& ps;

  uint32_t _candidates{ 0 };
  uint32_t _estimated_gain{ 0 };
};
} // namespace detail

/*! \brief RM logic optimization.
 *
 * This algorithm divides the network and then attempts to rewrite the subnetwork by RM logic
 * in terms of gates of the same network.  The rewritten structures are added
 * to the network, and if they lead to area improvement, will be used as new
 * parts of the logic.
 *
 * **Required network functions:**
 * - `get_node`
 * - `level`
 * - `update_levels`
 * - `create_and`
 * - `create_not`
 * - `create_xor`
 * - `substitute_node`
 * - `foreach_node`
 * - `foreach_po`
 * - `foreach_fanin`
 * - `is_and`
 * - `clear_values`
 * - `set_value`
 * - `value`
 * - `fanout_size`
 *
   \verbatim embed:rst

  .. note::

   \endverbatim
 */
template<class Ntk, class NodeCostFn = unit_cost<Ntk>>
void rm_mixed_polarity2( Ntk& ntk, rm_rewriting_params2 const& ps_ntk = {},cut_rewriting_params const& ps = {} )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );
  static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
  static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  static_assert( has_clear_values_v<Ntk>, "Ntk does not implement the clear_values method" );
  static_assert( has_incr_value_v<Ntk>, "Ntk does not implement the incr_value method" );
  static_assert( has_decr_value_v<Ntk>, "Ntk does not implement the decr_value method" );
  static_assert( has_set_value_v<Ntk>, "Ntk does not implement the set_value method" );
  static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
  static_assert( has_index_to_node_v<Ntk>, "Ntk does not implement the index_to_node method" );
  static_assert( has_substitute_node_v<Ntk>, "Ntk does not implement the substitute_node method" );
  static_assert( has_make_signal_v<Ntk>, "Ntk does not implement the make_signal method" );

  detail::rm_mixed_polarity_impl2<Ntk,NodeCostFn> p( ntk, ps_ntk, ps);
  p.run();
}

} /* namespace mockturtle */
