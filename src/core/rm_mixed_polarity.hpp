#include <mockturtle/mockturtle.hpp>
#include<string>
#include<iostream>
#include<string>
#include<vector>
#include<cmath>
#include<fstream>
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
			xag_cut();
		}


	private:
		int count_node_num(vector<string>& minterm, int variate_num, string polarity)//统计与门个数
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
		int atoint(string s, int radix)    //将3进制转化为10进制
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
		string intToA(int n, int radix)    //将10进制转化为3进制
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
    //create signal by expression
    signal_t create_ntk_from_str( std::string const& s, std::vector<node_t> const&  leaves )
    {
      std::vector<signal_t> pis;

      for( const auto& l : leaves )
      {
         pis.push_back( ntk.make_signal( l ) );
      }
      
       std::stack<signal_t> inputs;
       int flag=0;
       int flag1=0;

        for ( auto i = 0; i < s.size(); i++ )
        {
          if(s[i] == '[')
          {
            continue;
          }
          else if(s[i] == '(')
          {
            continue;
          }
          else if(s[i] == '1')
          {
            flag1=1;
            continue;
          }
          else if(s[i] >= 'a')
          {
            if(flag==1)
            {
              inputs.push(ntk.create_not(pis[ s[i] - 'a'] ));
              flag=0;
            }
            else 
              inputs.push(pis[ s[i] - 'a'] );
            
          }
          else if(s[i] == ')')
          {
            auto x1 = inputs.top();
            inputs.pop();
              
            auto x2 = inputs.top();
            inputs.pop();

            inputs.push(ntk.create_and(x1,x2));
          }
          else if(s[i] == ']')
          {
            if(flag1==1)
            {
              auto x1 = inputs.top();
              inputs.pop();
              inputs.push(ntk.create_not(x1));
              flag1=0;
            }
            else
            {
              auto x1 = inputs.top();
              inputs.pop();
              
              auto x2 = inputs.top();
              inputs.pop();
              inputs.push(ntk.create_xor(x1,x2));
            }
          }
          else if(s[i] == '!')
          {
            flag=1;
          }
          
        }
         return inputs.top();
    }
   	/**************************************************************************************************************/
		void xag_cut()
		{
     int v=0;
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
          int flag1=0;

					if (str10.size() > 4)
					{
						string str = str10;
						int len = str.length();
						int variate_num;//存储输入变量的个数;
						variate_num = log(len) / log(2);

						//列出所有的变量取值可能//列出真值表
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


						vector<char> c_out;
						for (int i = len - 1; i >= 0; i--)
						{
							c_out.push_back(str[i]);
						}




						vector<char> c_out1;
						vector<string> minterm;//存储二进制的最小项
						for (int i = 0; i < binary.size(); i++)
						{
							if (c_out[i] != '0')
							{
								minterm.push_back(binary[i]);
								c_out1.push_back(c_out[i]);
							}
						}


						//列出所有的极性取值可能
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

						vector<string> polarity;//存储所有极性
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
						//统计最初的成本大小;
						int current_size = INT_MAX;
						vector <string> min_product;
						string  optimal_polarity;//存储最优极性
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
									//删除数量为偶数个的最小项
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
									vector<string> final_term;//删除数组中为'0'的项
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
									//删除数量为偶数个的最小项
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
									vector<string> final_term;//删除数组中为'0'的项
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

						int node_num = min_product.size() - 1;//统计节点数
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

            //遍历每一个cut的门节点，判断其门节点是否都是单输出  
            dcut.foreach_gate( [&]( auto const& n2) { 
            if(ntk.fanout_size(ntk.get_node( ntk.make_signal(n2))) != 1)
            {
              flag1=1;
            }
            } );
						if (node_num < dcut.num_gates()&&flag1==0)
						{

							/*cout << "Cut " << *cut
								<< " with truth table " << to_hex(cuts.truth_table(*cut))
								<< "  ";*/

							string st = "abcdefghijklmnopqrstuvwxyz";
							string expression = "";
							//cout << "The RM expression is: ";
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

							//cout << expression;
							//cout << " " << "the new node_num: " << node_num << " " << "the old node_num: " << dcut.num_gates();
							//cout << endl;

                auto opt =create_ntk_from_str( expression, leaves);
                ntk.substitute_node(n, opt);
                ntk.update_levels();
                           
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
