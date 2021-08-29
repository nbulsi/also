/* also: Advanced Logic Synthesis and Optimization tool*/
/* Copyright (C) 2021- Ningbo University, Ningbo, China */

/*!
  \file rm_mixed_polarity.hpp
  \brief RM logic optimization
  \author hongwei zhou
*/

#pragma once

#include <mockturtle/mockturtle.hpp>
#include<iostream>
#include<string>
#include<vector>
#include<cmath>
#include <algorithm>
using namespace std;

namespace mockturtle
{
	template<class Ntk>
	class rm_mixed_polarity_impl
	{
	public:

		using node_t = node<Ntk>;
		using signal_t = signal<Ntk>;

		rm_mixed_polarity_impl(Ntk& ntk)
			: ntk(ntk)
		{
		}

		void run()
		{
			ntk_cut();
		}


	private:
		/**************************************************************************************************************/
		int count_node_num(vector<string>& minterm, int variate_num, string polarity)//Count the number of and gates
		{
			int count = 0;
			if (variate_num > 1)
			{
				for (int i = 0; i < minterm.size(); i++)
				{
					int count1 = 0;
					for (int j = 0; j < variate_num; j++)
					{
						if (minterm[i][j] == '1' || polarity[j] == '2')
							count1++;
					}
					count = count + count1 - 1;
				}
			}

			return count;
		}
		/**************************************************************************************************************/
		int atoint(string s, int radix)    //Convert ternary to decimal
		{
			int ans = 0;
			for (int i = 0; i < s.size(); i++)
			{
				char t = s[i];
				if (t >= '0' && t <= '9') ans = ans * radix + t - '0';
				else ans = ans * radix + t - 'a' + 10;
			}
			return ans;
		}
		/**************************************************************************************************************/
		string intToA(int n, int radix)    //Convert decimal to ternary
		{
			string ans = "";
			do {
				int t = n % radix;
				if (t >= 0 && t <= 9)    ans += t + '0';
				else ans += t - 10 + 'a';
				n /= radix;
			} while (n != 0);
			reverse(ans.begin(), ans.end());
			return ans;
		}
		/**************************************************************************************************************/
		signal_t create_ntk_from_str(std::string const& s, std::vector<node_t> const& leaves)//create signal by expression
		{
			std::vector<signal_t> pis;

			for (const auto& l : leaves)
			{
				pis.push_back(ntk.make_signal(l));
			}

			std::stack<signal_t> inputs;
			int flag = 0;
			int flag1 = 0;

			for (auto i = 0; i < s.size(); i++)
			{
				if (s[i] == '[')
				{
					continue;
				}
				else if (s[i] == '(')
				{
					continue;
				}
				else if (s[i] == '1')
				{
					flag1 = 1;
					continue;
				}
				else if (s[i] >= 'a')
				{
					if (flag == 1)
					{
						inputs.push(ntk.create_not(pis[s[i] - 'a']));
						flag = 0;
					}
					else
						inputs.push(pis[s[i] - 'a']);

				}
				else if (s[i] == ')')
				{
					auto x1 = inputs.top();
					inputs.pop();

					auto x2 = inputs.top();
					inputs.pop();

					inputs.push(ntk.create_and(x1, x2));
				}
				else if (s[i] == ']')
				{
					if (flag1 == 1)
					{
						auto x1 = inputs.top();
						inputs.pop();
						inputs.push(ntk.create_not(x1));
						flag1 = 0;
					}
					else
					{
						auto x1 = inputs.top();
						inputs.pop();

						auto x2 = inputs.top();
						inputs.pop();
						inputs.push(ntk.create_xor(x1, x2));
					}
				}
				else if (s[i] == '!')
				{
					flag = 1;
				}

			}
			return inputs.top();
		}
		/**************************************************************************************************************/
		vector <string> list_truth_table(int& variate_num)//List all possible variable values/List truth table
		{
			int a = pow(2, variate_num);
			vector<int> b;
			for (int i = 0; i < a; i++)
			{
				b.push_back(i);
			}
			vector <string> binary;
			string binary1;
			for (int i = 0; i < a; i++)
			{
				binary1 = { "" };
				for (int j = variate_num - 1; j >= 0; j--)
				{
					int d = ((b[i] >> j) & 1);
					binary1 += d + 48;
				}
				binary.push_back(binary1);
			}
			return binary;
		}
		/**************************************************************************************************************/
		vector<string> list_all_polarities(int& variate_num)//List all possible polarity values
		{
			int a1 = pow(2, variate_num);
			vector<int> c;
			for (int i = 0; i < a1; i++)
			{
				c.push_back(i);
			}
			vector <string> polarity1;
			string polarity2;
			for (int i = 0; i < a1; i++)
			{
				polarity2 = { "" };
				for (int j = variate_num - 1; j >= 0; j--)
				{
					int d = ((c[i] >> j) & 1);
					polarity2 += d + 48;
				}
				polarity1.push_back(polarity2);
			}

			vector<string> polarity;//Store all polarities
			polarity.push_back(polarity1[0]);
			int a2 = pow(3, variate_num);
			string str1 = polarity1[0];
			string str2 = polarity1[1];
			for (int i = 0; i < a2 - 1; i++)
			{
				int num1 = atoint(str1, 3);
				int num2 = atoint(str2, 3);

				int sum = num1 + num2;
				string str3 = intToA(sum, 3);

				str1 = "0";
				if (str3.size() < variate_num)
				{

					for (int i = 0; i < variate_num - str3.size() - 1; i++)
						str1 += '0';

					str1 += str3;
				}
				else
					str1 = str3;

				polarity.push_back(str1);
			}
			return polarity;
		}
		/**************************************************************************************************************/
		void search_for_optimal_polarity(vector<string>& minterm, vector<string>& polarity,int& variate_num, int &current_size, vector <string>& min_product, string& optimal_polarity)//Search for the optimal polarity and the corresponding product term
		{
			vector<string> original_minterm = minterm;
			for (int l = 0; l < polarity.size(); l++)
			{
				string str;
				minterm = original_minterm;
				for (int i = 0; i < variate_num; i++)
				{
					for (int j = 0; j < minterm.size(); j++)
					{
						if (polarity[l][i] == '0')
						{
							if (minterm[j][i] == '0')
							{
								str = minterm[j];
								str[i] = '1';
								minterm.push_back(str);
							}

						}
						else if (polarity[l][i] == '1')
						{
							if (minterm[j][i] == '1')
							{
								str = minterm[j];
								str[i] = '0';
								minterm.push_back(str);
							}
						}
					}
					if (polarity[l][i] == '0')
					{
						//Delete the minimum number of even items
						if (minterm.size() > 0)
						{
							sort(minterm.begin(), minterm.end());
							for (int m = 0; m < minterm.size() - 1; m++)
							{
								if (minterm[m] == minterm[m + 1])
								{
									minterm[m] = "0";
									minterm[m + 1] = "0";
								}
							}
						}
						vector<string> final_term;//Delete item with '0' in array
						final_term.clear();
						for (int m = 0; m < minterm.size(); m++)
						{
							if (minterm[m] != "0")
								final_term.push_back(minterm[m]);
						}
						minterm.clear();
						minterm = final_term;
					}
					else if (polarity[l][i] == '1')
					{
						//Delete the minimum number of even items
						if (minterm.size() > 0)
						{
							sort(minterm.begin(), minterm.end());
							for (int m = 0; m < minterm.size() - 1; m++)
							{
								if (minterm[m] == minterm[m + 1])
								{
									minterm[m] = "0";
									minterm[m + 1] = "0";
								}
							}
						}
						vector<string> final_term;//Delete item with '0' in array
						final_term.clear();
						for (int m = 0; m < minterm.size(); m++)
						{
							if (minterm[m] != "0")
							{
								if (minterm[m][i] == '0')
									minterm[m][i] = '1';
								else if (minterm[m][i] == '1')
									minterm[m][i] = '0';
								final_term.push_back(minterm[m]);
							}
						}
						minterm.clear();
						minterm = final_term;
					}
				}

				int count1, count2;
				count1 = count_node_num(minterm, variate_num, polarity[l]);
				count2 = count_node_num(min_product, variate_num, optimal_polarity);
				if ((minterm.size() == current_size && count1 < count2) || (minterm.size() < current_size))
				{
					optimal_polarity = polarity[l];
					current_size = minterm.size();
					min_product.clear();
					min_product = minterm;
					minterm.clear();
				}
				else
				{
					minterm.clear();
				}

			}
		}
		/**************************************************************************************************************/
		int count_the_number_of_nodes(int& variate_num, vector <string>& min_product, string& optimal_polarity)//Count the number of nodes in the new network
		{
			int node_num = min_product.size() - 1;//Count the number of nodes
			for (int i = 0; i < min_product.size(); i++)
			{
				int n = 0;
				for (int j = variate_num - 1; j >= 0; j--)
				{
					if (min_product[i][j] == '1' || optimal_polarity[j] == '2')
						n++;

				}
				if (n != 0)
					node_num = node_num + n - 1;
			}
			return node_num;
		}
		/**************************************************************************************************************/
		string create_expression(vector <string>& min_product,int& variate_num, string& optimal_polarity, vector <string>& binary)//Create expression
		{
			string st = "abcdefghijklmnopqrstuvwxyz";
			string expression = "";
			if (min_product.size() > 1)
				for (int i = 0; i < min_product.size() - 1; i++)
					expression += '[';
			for (int i = 0; i < min_product.size(); i++)
			{
				int k = 0;
				int l = 0;
				int m = 0;
				for (int j = variate_num - 1; j >= 0; j--)
				{
					if (min_product[i][j] == '1' || optimal_polarity[j] == '2')
						l++;
				}
				for (int j = 0; j < l - 1; j++)
					expression += '(';
				for (int j = variate_num - 1; j >= 0; j--)
				{
					if (optimal_polarity[j] == '0' && min_product[i][j] == '1')
					{
						expression += st[k];
						m++;
					}
					else if (optimal_polarity[j] == '1' && min_product[i][j] == '1')
					{
						expression += '!';
						expression += st[k];
						m++;
					}
					else if (optimal_polarity[j] == '2' && min_product[i][j] == '1')
					{
						expression += st[k];
						m++;
					}
					else if (optimal_polarity[j] == '2' && min_product[i][j] == '0')
					{
						expression += '!';
						expression += st[k];
						m++;
					}

					if ((m > 1 && min_product[i][j] == '1') || (m > 1 && optimal_polarity[j] == '2'))
						expression += ')';
					k++;

				}
				if (min_product[i] == binary[0])
				{
					int flag = 0;
					for (int m = variate_num - 1; m >= 0; m--)
					{
						if (optimal_polarity[m] == '2')
							flag = 1;
					}
					if (flag == 0)
						expression += '1';
				}
				if (i > 0 && min_product.size() > 1)
					expression += ']';

			}
			return expression;
		}
		/**************************************************************************************************************/
		void substitute_network(string& expression, std::vector<node_t>& leaves, node_t& n)//Replace network
		{
			auto opt = create_ntk_from_str(expression, leaves);
			ntk.substitute_node(n, opt);
			ntk.update_levels();
		}
		/**************************************************************************************************************/
		void ntk_cut()
		{
			int v = 0;
			cut_enumeration_params ps;
			ps.cut_size = 4;
			ps.cut_limit = 8;
			auto cuts = cut_enumeration<Ntk, true>(ntk, ps);   /* true enables truth table computation */
			ntk.foreach_node([&](auto n) {
				const auto index = ntk.node_to_index(n);


				for (auto const& cut : cuts.cuts(index))
				{

					std::vector<node<Ntk>> leaves;
					for (auto leaf_index : *cut)
					{
						leaves.push_back(ntk.index_to_node(leaf_index));
					}
					cut_view<Ntk> dcut(ntk, leaves, ntk.make_signal(n));

					string str10 = to_binary(cuts.truth_table(*cut));
					int flag1 = 0;

					if (str10.size() > 2)
					{
						string str = str10;
						int len = str.length();
						int variate_num;//Stores the number of input variables
						variate_num = log(len) / log(2);

						vector <string> binary;//List all possible variable values/List truth table
						binary = list_truth_table(variate_num);


						vector<char> c_out;//Stores the output of the truth table
						for (int i = len - 1; i >= 0; i--)
						{
							c_out.push_back(str[i]);
						}

						vector<char> c_out1;
						vector<string> minterm;//Minimum item for storing binary
						for (int i = 0; i < binary.size(); i++)
						{
							if (c_out[i] != '0')
							{
								minterm.push_back(binary[i]);
								c_out1.push_back(c_out[i]);
							}
						}

						vector<string> polarity;//Store all polarities
						polarity=list_all_polarities(variate_num);//List all polarities


						
						//Count the initial cost
						int current_size = INT_MAX;
						vector <string> min_product;
						string  optimal_polarity;//Storage optimal polarity
						search_for_optimal_polarity(minterm, polarity, variate_num, current_size, min_product, optimal_polarity);//Search for the optimal polarity and the corresponding product term
						

						int node_num; //Count the number of nodes in the new network
						node_num = count_the_number_of_nodes(variate_num, min_product, optimal_polarity);
 
						
						if (node_num < dcut.num_gates())
						{
               
							int optimization_nodes=dcut.num_gates()-node_num;
               
							dcut.foreach_gate([&](auto const& n2) {
  								if (ntk.fanout_size(ntk.get_node(ntk.make_signal(n2))) != 1&&n2!=n)
								{
									if(dcut.num_gates()<4)
			    						  optimization_nodes=optimization_nodes-1;
									else
									  optimization_nodes=optimization_nodes-2;
								 }
							});
               
               
							 if(optimization_nodes>0&&dcut.num_gates()<10)
							 {
								 /*cout << "Cut " << *cut
									 << " with truth table " << to_hex(cuts.truth_table(*cut))
									<< "  ";*/

								 string expression = create_expression(min_product,variate_num,optimal_polarity,binary);//Create expression

								//cout << "The RM expression is: ";
								//cout << expression;
								//cout << " " << "the new node_num: " << node_num << " " << "the old node_num: " << dcut.num_gates();
								//cout << endl;

								substitute_network(expression, leaves, n);//Replace network
							 }
						}
					}
				}
			});
		}
	private:
		Ntk& ntk;

	};

	template<class Ntk>
	void rm_mixed_polarity(Ntk& ntk)
	{
		rm_mixed_polarity_impl p(ntk);
		p.run();
	}


}/* namespace mockturtle */
