/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2023- Ningbo University, Ningbo, China */

/**
 * @file imgf.hpp
 *
 * @brief construct a fanout-free implication logic network by logic restructuring
 * 
 */

#ifndef IMGF_HPP
#define IMGF_HPP

#include <array>
#include <vector>
#include <mockturtle/mockturtle.hpp>
#include "../networks/img/img.hpp"

namespace mockturtle
{
    struct imgf_params
     {
  	                int normalCount = 0;
					int oneCount = 0;
					int bnCount = 0;
					int fnCount = 0;
                    int xnorCount = 0;
                    int secondCount = 0;
                    int one_secondCount = 0;
					int bn_xnorCount = 0;
					int bn_size = 0;
					
   //开关，计数器（几个结点有什么类型扇出），计时器（一个函数运行了多长时间）
    };
    imgf_params params;
	enum class FPType
	{
		perfect,   //完全正常，输出全部连接到了输入忆阻器
		normal,    //正常，输出只有一个连接到了工作忆阻器
		fp_1,      //第一类扇出故障，输出端连接了两个及以上工作忆阻器
		fp_2,    //第二类扇出
		fp_bn,   //bn
		fp_xnor,      //xnor型扇出,冲突扇出的第二种类型
		fp_fn     //FN型扇出
		// fp_xnor_bn  //xnor、bn混合       
	};
  bool use_deal_fn_2018 = false; // 默认不开启
  namespace detail
  {
    template<class Ntk>
    class imgf
    {
			public:
				using node = typename Ntk::node;
      public:
        imgf( Ntk& ntk, imgf_params const& ps )
          : ntk( ntk ), ps( ps )
        {
        }

        void run( )
        {     
			//  if ( countfanout( [&]( const node& n ))> 0)
		// {
		
					// 1: 处理所有的所有的冲突故障，先xnor,再常规
					ntk.foreach_node( [&]( const node& n ) 
														{
														//  std::cout << "全部有问题节点" << countfanout( n ) << std::endl;
															if (ntk.is_constant(n))
															{
                                                            //   std::cout << n <<  "是常数节点"<< std::endl;
                                                            }
                                                            else
                                                            {
                                                              get_works( n );
															  FP_infor fp = get_fanout_info(n);
													
															  if( fp.type == FPType::fp_xnor)
															  {
															    // std::cout << "处理xnor"<< std::endl;
                                                                deal_xnor( fp );
															  }
															  if( fp.type == FPType::fp_2 )
															  {
																// std::cout << "处理2"<< std::endl;
                                                                 deal_fp2( fp );
															  }
															//  if( fp.type == FPType::fp_xnor_bn )
															//   {
															// 	std::cout << "处理xnor、bn混合"<< std::endl;
                                                            //      deal_fpbx( fp );
															//   }

															  if( fp.type == FPType::fp_fn)
															   {
															//   std::cout << "处理FN" << std::endl;
                                                                if (use_deal_fn_2018) {
																		deal_fn_2018(fp);
																	} else {
																		deal_fn(fp);
																	}
															   }
                                                            }
															  
															  return true;
														});

				    // 2: 处理所有的所有的BN扇出									
                    ntk.foreach_node( [&]( const node& n ) 
														{
                                                            if (ntk.is_constant(n))
															{
                                                            //   std::cout << n <<  "是常数节点"<< std::endl;
                                                            }
                                                            else
                                                            {
		                                                      get_works( n );
															  FP_infor fp = get_fanout_info(n);
                                                         
															 if( fp.type == FPType::fp_1 )
															   {
                                                                //  std::cout << "处理1" << std::endl;
																deal_fp1(fp);
															   }
                                                              }
															  return true;
															
														}); 	
                //    3: 处理所有的所有的FN扇出									
                    ntk.foreach_node( [&]( const node& n ) 
														{
                                                            if (ntk.is_constant(n))
															{
                                                            //   std::cout << n <<  "是常数节点"<< std::endl;
                                                            }
                                                            else
                                                            {
															  get_works( n );
															  FP_infor fp = get_fanout_info(n);
                                                              if( fp.type == FPType::fp_fn)
															  {
															    //std::cout << "处理FN" << std::endl;
                                                                 if (use_deal_fn_2018) {
																		deal_fn_2018(fp);
																	} else {
																		deal_fn(fp);
																	}
															  }
													
                                                            }
															  return true;
															
														}); 
		}    
    
        // }
       
     
		void run_1()
         {
          // 1: 处理所有的所有的冲突故障，先xnor,再常规
					ntk.foreach_node( [&]( const node& n ) 
														  {
															  
															 if (ntk.is_constant(n))
															{
                                                              std::cout << n <<  "是常数节点"<< std::endl;
                                                            }
                                                            else
                                                            {
                                                          	 get_works( n );
															  FP_infor fp = get_fanout_info(n);
															
															  if( fp.type == FPType::fp_xnor)
															  {
															    std::cout << "处理xnor"<< std::endl;
                                                                deal_xnor( fp );
																}
															  if( fp.type == FPType::fp_2 ){
																std::cout << "处理2"<< std::endl;
                                                                 deal_fp2( fp );
															  }
															//   if( fp.type == FPType::fp_xnor_bn )
															//   {
															// 	std::cout << "处理xnor、bn混合"<< std::endl;
                                                            //      deal_fpbx( fp );
															//   }
															   if( fp.type == FPType::fp_1 )
															   {
																std::cout << "处理1" << std::endl;
																deal_fp1(fp);
															   }
															  if( fp.type == FPType::fp_fn)
															   {
															  std::cout << "处理FN" << std::endl;
                                                              deal_fn( fp );
															  }
															}  
															  return true;
															
													
														});

		 }
        void run_2()
		 {

           // 2: 处理所有的所有的BN扇出或第一类扇出									
                    ntk.foreach_node( [&]( const node& n ) 
														{
															 if (ntk.is_constant(n))
															{ 
															 
                                                              std::cout << n <<  "是常数节点"<< std::endl;
                                                            }
                                                            else
                                                            {
															  get_works( n );
															  FP_infor fp = get_fanout_info(n);
	                                                          if( fp.type == FPType::fp_fn)
															  {
															  std::cout << "处理FN" << std::endl;
                                                              deal_fn_2018( fp );
															  }
															//   	if( fp.type == FPType::fp_1 )
															//    {
															// 	std::cout << "处理1" << std::endl;
															// 	deal_fp1(fp);
															//    }

                                                            }
															  return true;
															
														}); 
		 }
		 void run_3()
		 {
															int real_size = 0;
															int flag = 0;
															int bn_not_gate = 0;
															int num_not_afterbn = 0;
															int num_not_before = 0;
			// 3: 处理所有的所有的FN扇出									
                    ntk.foreach_node( [&]( const node& n ) 
														{   
															num_not_before = not_gate( ntk ) ;
															if ( ntk.is_constant(n) )
															{
															//   constant_num++;
                                                              std::cout << n <<  "是常数节点"<< std::endl;
															//   std::cout << "常数节点数量：" << constant_num << std::endl;
                                                            }else
                                                            {
                                                              get_works( n );
	 														  FP_infor fp = get_fanout_info(n);
	                                                    
															   if( fp.type == FPType::fp_bn )
																{
																real_size = (ntk.size()-ntk.num_pis()-1) - flag;//常数节点
					                                            std::cout << "gate_real_size: "<< real_size << std::endl;
																std::cout << "size: "<< ntk.size() << std::endl;	
																std::cout << "处理BN" << std::endl;
                                                                deal_bn( fp );
																
                                                                std::cout << "处理bn后" << std::endl;
																real_size = (ntk.size()-ntk.num_pis()-1) -1 - flag;//dead节点
																num_not_afterbn = not_gate( ntk ) ;
					                                            std::cout << "gate_real_size: "<< real_size << std::endl;
																std::cout << "size: "<< ntk.size() << std::endl;
																flag ++;
																}
															// 	if( fp.type == FPType::fp_1 )
															//    {
															// 	std::cout << "处理1" << std::endl;
															// 	deal_fp1(fp);
															//    }
                                                            }
															  return true;
															
														});
		   											 
		   
		   bn_not_gate = (real_size - ntk.num_gates());
		   std::cout << "not_gate: "<< num_not_before  << std::endl;
		   std::cout << "not_gate_afterbn: "<< num_not_afterbn  << std::endl;
		   std::cout << "afterbn_memristors: " << num_not_afterbn + ntk.num_pis()  << std::endl;
		   std::cout << "gate_real_size: "<< real_size << std::endl;
		   std::cout << "old pulse: " << ntk.num_gates() + 1 << std::endl;
		   std::cout << "[cost_function] bn pulse: " << real_size + 1 << " memristors: " << num_not_afterbn + ntk.num_pis() << "\n";
		   std::cout << "[cost_function] real_pulse: " << ntk.num_gates() + params.bn_size + 1 << " memristors: " <<  num_not_afterbn + ntk.num_pis() << "\n";
		 }
		 		//检测扇出
		void run_11()
         {
             
					ntk.foreach_node( [&]( const node& n ) 
														  {
															if ( ntk.is_constant(n) )
															{
                                                              std::cout << n <<  "是常数节点"<< std::endl;
															
                                                            }else
                                                            {
                                                              get_works( n );
															  FP_infor fp = get_fanout_info(n);
                                                             }
															  return true;
														});
				std::cout << "正常节点数量：" << params.normalCount << std::endl;
                std::cout << "xnor扇出节点数量: " << params.xnorCount << std::endl;
				std::cout << "第二类扇出节点数量：" <<  params.secondCount << std::endl;
				std::cout << "第一类扇出节点数量：" << params.oneCount << std::endl;
				std::cout << "bn类扇出节点数量: " << params.bnCount << std::endl;
				std::cout << "fn类扇出节点数量: " << params.fnCount << std::endl;
				std::cout << "第一、二类混合扇出节点数量：" << params.one_secondCount << std::endl;
				std::cout << "bn、xnor混合扇出节点数量: " << params.bn_xnorCount << std::endl;
				std::cout << "bn扇出总数量: " << params.bn_size<< std::endl;
		 }

    //   private: 
	            imgf_params params;
				struct FP_infor
				{
					FP_infor() {}
					FP_infor(FPType _type, std::vector<node>& f_info) : type(_type)
					{
						switch (type)
						{
						case FPType::perfect:
						assert(f_info.size() == 0);
							break;
						case FPType::normal:
						assert(f_info.size() == 2);
							break;
						case FPType::fp_1:
						assert(f_info.size() > 2);
							break;
						case FPType::fp_bn:
						assert(f_info.size() > 2);
						    break;			
						case FPType::fp_2:
						assert(f_info.size() == 4);
							break;
						case FPType::fp_xnor:
						assert(f_info.size() == 4);
						    break;
						case FPType::fp_fn:
						assert(f_info.size() > 3);
							break;	
						// case FPType::fp_xnor_bn:
						// assert(f_info.size() > 2);
						// 	break;		 		
						default:
							break;
						}
						fanout_info = f_info;
					}
					FPType type = FPType::perfect;
					std::vector<node> fanout_info;

                    bool operator==(const FP_infor& fp) const
				     {
                       return (type == fp.type) && (fanout_info == fp.fanout_info);
                     }
               
				};

              // 计算not蕴含门的数量
              int not_gate( const Ntk& ntk )
              {
                 int num = 0;
                 ntk.foreach_gate( [&]( auto n )
                 {
                   auto nd = get_child_node(n);
                   if(nd[1] == 0 ) num++; } );
                    return num;
             }
				//按顺序获取子节点(img节点第一个子节点为输入忆阻器，第二个子节点为工作忆阻器)
				std::array<node, 2> get_child_node( const node& n )
				{
					std::array<node, 2> children;
					ntk.foreach_fanin( n, [&]( const auto& f, auto i ) { children[i] = ntk.get_node(f); } );//get_node(f) 用于获取输入端口的值，并将其存储在 children[i] 中
					
					return children;
				}
		
				//蕴含输入，工作忆阻器
               node get_imp_work( const node& n )
			   {
				return get_child_node( n )[1];
			   }

               node get_imp_input( const node& n )
			   {
				 return get_child_node( n )[0];
			   }
				//front连接了back的工作忆阻器吗
				bool is_work( const node& front, const node& back )
				{
					return get_child_node( back )[1] == front;
				}
				//front连接了back的输入忆阻器吗
				bool is_input( const node& front, const node& back )
				{
					return get_child_node( back )[0] == front;
				}
				//判断节点n本身是否是not
				bool is_not(const node& n)
		        {
			        return get_imp_work( n ) == 0 && !ntk.is_pi( n );
		        }
				//front连接的back是否是not
				bool back_is_not( const node& front, const node& back )
				{
					return get_child_node( back )[0] == front && is_not( back );
				}
				//获取节点n后面的所有工作忆阻器
				std::vector<node> get_works( const node& n )
				{
					// std::cout <<"节点"<< n << "后面的工作忆阻器：";
					std::vector<node> works;
					auto fn = [&]( const node& p ) { if( is_work(n, p) ) works.push_back( p ); };
					ntk.foreach_fanout( n, fn );
                    // for(const node& work : works)
					// std::cout << work << " ";
					// std::cout << std::endl;
					return works;
				}

				//不在哈希表里面的节点数量
                int real_size()
				{
				    int num = 0;
				    ntk.foreach_gate( [&]( auto n ) 
								{
									if( ntk.is_dead(n) )
									num ++;						
								});
				    return (ntk.size() - num);				
				}

				//获取节点n后面的所有输入忆阻器
				std::vector<node> get_inputs( const node& n )
				{
					// std::cout <<"节点"<< n << "后面的输入忆阻器：";
					std::vector<node> inputs;
					auto fn = [&]( const node& p ) { if( is_input(n, p) ) inputs.push_back( p ); };
					ntk.foreach_fanout( n, fn );
                    // for(const node& work : works)
					// std::cout << work << " ";
					// std::cout << std::endl;
					return inputs;
				}

               //获取节点n后面的not门
                std::vector<node> get_not( const node& n )
				{
					std::vector<node> nots;
					auto fn = [&]( const node& p ) { if( back_is_not(n, p) ) nots.push_back( p ); };
					ntk.foreach_fanin( n, fn );
                    return nots;
				}
                //定义第一类扇出或BN类扇出
				FP_infor get_fp1_bn( const node& G1 )
                {   
					std::vector<node> fp1_bn_info;
                     fp1_bn_info = get_works( G1 );
				     fp1_bn_info.insert(fp1_bn_info.begin(), G1);
				     assert(fp1_bn_info.size() > 2);
					 if( is_not( G1 )  )
					 {
				      //  fp1_bn_info.insert(fp1_bn_info.begin(), get_child_node( G1 )[0]);
					   std::cout << "节点 " << G1 << " 具有BN类型扇出" << std::endl;
				       return FP_infor(FPType::fp_bn, fp1_bn_info);
					 }
				     else
					 {
                       std::cout << " 节点 " << G1 << " 具有第一种类型扇出" << std::endl;
				       return FP_infor(FPType::fp_1, fp1_bn_info);
					 }	
				 }	
				

				//定义xnor类扇出
				FP_infor get_fp2_xnor( const node& x, const node& G2 )
				{
				if( is_not( x) )
				{
					std::vector<node> xnor_info;
				
                    xnor_info.push_back(x);
					xnor_info.push_back(G2);

					assert( is_work( x, G2 )  );
                  
                    const node y = get_child_node( G2 )[0];
					std::vector<node> works_y = get_works( y );
					node G1 = 0xffffffff;
					
					for(const node& nd : works_y)
					{
						// if(get_child_node( nd )[0] == x)
						if(get_imp_input( nd ) == x)
						{
							G1 = nd;
							break;
						}
					}
					if( G1 == 0xffffffff )
						return FP_infor(FPType::normal, xnor_info);
                 
					xnor_info.push_back(y);
					xnor_info.push_back(G1);

                    std::cout <<"具有xnor类扇出的节点: "<< x << " " ;
					std::cout << G2 << " ";
					std::cout << y << " ";
					std::cout << G1 << " ";
					std::cout << std::endl;
					return FP_infor(FPType::fp_xnor, xnor_info);
				}
				else
				{
					std::vector<node> info;
				    std::vector<node> xnor_info;
                    info.push_back(x);
					info.push_back(G2);
				
					assert( is_work( x, G2 ) );

                    const node y = get_child_node( G2 )[0];
					std::vector<node> works_y = get_works( y );
					node G1 = 0xffffffff;
					
					for(const node& nd : works_y)
					{
						// if(get_child_node( nd )[0] == x)
						if(get_imp_input( nd ) == x)
						{
							G1 = nd;
							break;
						}
					}
					if( G1 == 0xffffffff )
						return FP_infor(FPType::normal, info);
            
					info.push_back(y);
					info.push_back(G1);
					
                    if( is_not( y ) )
					{
					xnor_info.push_back(y);
					xnor_info.push_back(G1);
					xnor_info.push_back(x);
					xnor_info.push_back(G2);	
					 std::cout <<"具有xnor类扇出的节点: "<< y << " " ;
					 std::cout << G1 << " ";
					 std::cout << x << " ";
					 std::cout << G2 << " ";
					 std::cout << std::endl;
                    return FP_infor(FPType::fp_xnor, xnor_info);
					}
					
                    std::cout <<"具有第二类扇出的节点："<< x << " " ;
					std::cout << G2 << " ";
					std::cout << y << " ";
					std::cout << G1 << " ";
					std::cout << std::endl;
					return FP_infor(FPType::fp_2, info);	

				}
				}

				//定义FN类扇出
                FP_infor get_fp_fn( const node& n )
                {
                    std::vector<node> inputs_n = get_inputs( n );
					std::vector<node> fn_info = get_works( n );
                    fn_info.insert( fn_info.begin(), n );
                    node G1 = 0xffffffff;

					 for(const node& nd : inputs_n)
					 {
						// if(get_child_node( nd )[0] == x)
						if( is_not( nd ))
						{
							G1 = nd;
							break;
						}
					 }
					if( G1 == 0xffffffff )
					{
                       if( fn_info.size() == 2)
					   return FP_infor(FPType::normal, fn_info);
					   if( fn_info.size() > 2)
					   return FP_infor(FPType::fp_1, fn_info);
					}
					assert( is_not( G1 ) );
					fn_info.insert(fn_info.begin() + 1, G1);  // 在第二个位置插入元素G1
					assert(fn_info.size() > 3);
				    //std::cout << "节点" << x << "属于FN类扇出" << std::endl;
					return FP_infor(FPType::fp_fn, fn_info);
    
				}
                
				// 获取 works 的大小
				int get_works_size(const std::vector<node>& works) 
				{
					return static_cast<int>(works.size()) - 2;
				}
				//判断一个节点的fanout是哪一类  
				FP_infor get_fanout_info(const node& n)
				{   
				
					std::vector<node> works = get_works( n );
					works.insert(works.begin(), n);
                    std::cout <<"节点"<< n << "及其工作忆阻器：";
                    for(const node& work : works)
					std::cout << work << " ";
				    std::cout << std::endl;

					if(works.size() == 1)
					{
					    std::cout << "扇出正常" << std::endl;
						params.normalCount++;
						return FP_infor();//默认初始化为FPType::perfect;
					}

				    if(works.size() == 2)
					{
                       FP_infor fp = get_fp2_xnor( works[0], works[1] );
					   if( fp.type == FPType::normal)
					   {
						    std::cout << "扇出正常" << std::endl;
							params.normalCount++;
						    return FP_infor(FPType::normal, works);
					   }
					   else if( fp.type == FPType::fp_xnor )
					   {
						    std::cout << "属于xnor类扇出类型" << std::endl;
							params.xnorCount++;
					        return get_fp2_xnor( works[0], works[1] );
					   }
                       else 
					   {
						   std::cout << "属于第二类扇出类型" << std::endl;
						   params.secondCount++;
						   return get_fp2_xnor( works[0], works[1] );//第二类
					   }

					}
					if(works.size() > 2)//bn(is_not(n))、fn、第一类或混合！！！
					{
						
					    std::vector<node> node_fp2;
						std::vector<node> node_fp_x;
				      for(int i = 1; i < works.size(); i++)
					    {
                          if( get_fp2_xnor(works[0], works[i]).type == FPType::fp_2 )
						  {
							node_fp2.push_back(works[i]);
						  }
						  if( get_fp2_xnor(works[0], works[i]).type == FPType::fp_xnor )
						  {
						 	node_fp_x.push_back(works[i]);
						  }
						}
                        if(node_fp2.size() > 0  )//第一、第二混合扇出按第二类处理
						{
							std::cout << "属于第一、第二混合扇出类型" << std::endl;
							params.one_secondCount++;
							return get_fp2_xnor( n , node_fp2[0] );
						}
                        if(node_fp_x.size() > 0 )//bn、xnor混合混合扇出按xnor类处理
						{
							std::cout << "属于bn、xnor混合扇出类型" << std::endl;
							params.bn_xnorCount++;
							return get_fp2_xnor( n , node_fp_x[0] );
							// FP_infor fp_x_bn =  get_fp2_xnor( n , node_fp_x[0] );
							// return FP_infor(FPType::fp_xnor_bn, fp_x_bn.fanout_info);
						}

					//    FP_infor fn = get_fp_fn( n );
					  if( is_not( n ) )//bn
					   {
						    std::cout << "属于bn扇出类型" << std::endl;
							params.bnCount++;
							int size = get_works_size(works);
							std::cout << "bn扇出个数: " << size << std::endl;
							params.bn_size += size;
							return get_fp1_bn( n );//bn扇出故障
					   }

                        //  if( fn.type == FPType::fp_fn )
						//     {
						// 	  std::cout << "属于fn类扇出类型" << std::endl;
						// 	  params.fnCount++;
						// 	  return fn;
					   	//     }
						//   else
						//     {
						// 	 std::cout << "属于第一种扇出类型" << std::endl;
						// 	 params.oneCount++;
						// 	 return get_fp1_bn( n );  
						//     }
				     	//  }
					     else//fn、fp_1
					     {
							 FP_infor fn = get_fp_fn( n );
							 if( fn.type == FPType::fp_fn )
						    {
							  std::cout << "属于fn类扇出类型" << std::endl;
							  params.fnCount++;
							  return fn;
					   	    }
							else
							{
                        	 std::cout << "属于第一种扇出类型" << std::endl;
							 params.oneCount++;
							 return get_fp1_bn( n );  
							}
						  }
					 
					}

					//不可能执行到下面一行
					return FP_infor();
				} 

               int countfanout(const node& n)
				{   
					int totalnum = 0;
					std::vector<node> works = get_works( n );
					works.insert(works.begin(), n);
                    // std::cout <<"节点"<< n << "及其工作忆阻器：";
                    // for(const node& work : works)
					// std::cout << work << " ";
				    // std::cout << std::endl;

					if(works.size() == 1)
					{
						params.normalCount++;
					}

				    if(works.size() == 2)
					{
                       FP_infor fp = get_fp2_xnor( works[0], works[1] );
					   if( fp.type == FPType::normal)
					   {
							params.normalCount++;
					   }
					   else if( fp.type == FPType::fp_xnor )
					   {
							params.xnorCount++;
					   }
                       else 
					   {
						   params.secondCount++;
					   }

					}
					if(works.size() > 2)//bn(is_not(n))、fn、第一类或混合！！！
					{
						
					    std::vector<node> node_fp2;
						std::vector<node> node_fp_x;
				      for(int i = 1; i < works.size(); i++)
					    {
                          if( get_fp2_xnor(works[0], works[i]).type == FPType::fp_2 )
						  {
							node_fp2.push_back(works[i]);
						  }
						  if( get_fp2_xnor(works[0], works[i]).type == FPType::fp_xnor )
						  {
						 	node_fp_x.push_back(works[i]);
						  }
						}
                        if(node_fp2.size() > 0  )//第一、第二混合扇出按第二类处理
						{
							params.one_secondCount++;
						}
                        if(node_fp_x.size() > 0 )//bn、xnor混合混合扇出按xnor类处理
						{
							params.bn_xnorCount++;
						}

					   FP_infor fn = get_fp_fn( n );
					  if( !is_not( n ) )//fn、第一类
					   {
						
                         if( fn.type == FPType::fp_fn )
						    {
							  params.fnCount++;
					   	    }
						  else
						    {
							 params.oneCount++;
						    }
				     	 }
					     else//bn
					     {
							 if( fn.type == FPType::fp_fn )
						    {
							  params.fnCount++;
					   	    }
							else
							{
						      params.bnCount++;
							}
						  }
					 
					}
                totalnum = params.xnorCount + params.secondCount + params.oneCount + params.bnCount + params.fnCount + params.one_secondCount + params.bn_xnorCount;
				return totalnum;
				
				}

				//deal way1
				void d_fp1(const node& G1, const node& G3)
				{
					auto s1 = ntk.make_signal( G1 );
					auto s3 = ntk.make_signal( G3 );

					auto x = ntk.create_not( s1 );
					auto y = ntk.create_not( ntk.make_signal(get_child_node(G3)[0]) );
					auto opt = ntk.create_imp( x , y );

					ntk.substitute_node( G3, opt );
				
				}
                //处理fp_1扇出
				void deal_fp1(const FP_infor& fp1)
				{
					assert(fp1.type == FPType::fp_1 );
					assert(fp1.fanout_info.size() > 2);
					d_fp1(fp1.fanout_info[0], fp1.fanout_info[1]);	
				}
				//处理fp_2故障
				void deal_fp2(const FP_infor& fp2)
				{
					assert(fp2.type == FPType::fp_2);
					assert(fp2.fanout_info.size() == 4);

					auto singal_x = ntk.make_signal( fp2.fanout_info[0] );
					auto f1 = ntk.create_not( singal_x );

					auto singal_y = ntk.make_signal( fp2.fanout_info[2] );
					auto f2 = ntk.create_not( singal_y );

					auto singal_opt = ntk.create_imp( f1,  f2 );
                   
					ntk.substitute_node( fp2.fanout_info[1], singal_opt );
					
				}

				//处理FN的方法
				void d_fn( const node& G1, const node& G2)
				{
					auto s1 = ntk.make_signal( G1 );
					auto s2 = ntk.make_signal( G2 );
					auto f1 = ntk.create_not( s1 );
                    auto f4 = ntk.make_signal( get_child_node( G2 )[0] );
					auto opt = ntk.create_imp(f4 , f1);
					ntk.substitute_node( G2, opt);
				}
				//处理FN类型扇出
				void deal_fn(const FP_infor& fpfn)
				{
					assert(fpfn.type == FPType::fp_fn);
					assert(fpfn.fanout_info.size() > 3);
					for(int i = 2; i < (fpfn.fanout_info.size()-1); i++)
					d_fn( fpfn.fanout_info[1] , fpfn.fanout_info[i]);
				}
				//2018TVLSI
				void deal_fn_2018(const FP_infor& fpfn)
				{
					assert(fpfn.type == FPType::fp_fn);
					assert(fpfn.fanout_info.size() > 3);
					for(int i = 2; i < fpfn.fanout_info.size(); i++)
					{
						d_fn( fpfn.fanout_info[1] , fpfn.fanout_info[i]);
					}	
				}

                //  处理bn、xnor混合类型的方法 
				// void deal_fpbx(const FP_infor& fpbx)
				// {
				// 	assert(fpbx.type == FPType::fp_xnor_bn);
				// 	assert(fpbx.fanout_info.size() == 4);

				// 	auto singalx = ntk.make_signal( fpbx.fanout_info[0] );
				// 	auto f1 = ntk.create_not( singalx );

				// 	auto singaly = ntk.make_signal( fpbx.fanout_info[2] );
				// 	auto f2 = ntk.create_not( singaly );

				// 	auto singal_opt = ntk.create_imp( f1,  f2 );
                   
				// 	ntk.substitute_node( fpbx.fanout_info[1], singal_opt );
					
				// }

                 //处理BN类型的方法
				 void dl_bn( const node& G1, const node& G2 )
				 {
					auto f1 = ntk.make_signal( get_child_node( G1 )[0] );
					std::cout << "num_gates: "<< ntk.num_gates() << std::endl;
					std::cout << "size: "<< ntk.size() << std::endl;	
					// std::cout << "real_size: "<< real_size( n ) << std::endl;
					auto f2 = ntk.create_not_without_hash( f1 );
					auto f3 = ntk.make_signal( get_child_node( G2 )[0] );
					std::cout << "num_gates: "<< ntk.num_gates() << std::endl;
					std::cout << "size: "<< ntk.size() << std::endl;	

					// std::cout << "real_size: "<< real_size( n ) << std::endl;
					auto opt = ntk.create_imp_without_hash( f3, f2);
					std::cout << "num_gates: "<< ntk.num_gates() << std::endl;
					std::cout << "size: "<< ntk.size() << std::endl;	

					// std::cout << "real_size: "<< real_size( n ) << std::endl;
                    ntk.substitute_node( G2, opt);
					std::cout << "num_gates: "<< ntk.num_gates() << std::endl;
					std::cout << "size: "<< ntk.size() << std::endl;	

					// std::cout << "real_size: "<< real_size( n ) << std::endl;
				 }
                //处理BN类型扇出
				 void deal_bn( const FP_infor& fpbn)
				 {
					assert(fpbn.type == FPType::fp_bn);
					assert(fpbn.fanout_info.size() > 2);
                    for(int i = 1; i < fpbn.fanout_info.size()-1; i++)
					dl_bn(fpbn.fanout_info[0],  fpbn.fanout_info[i]);
				 }
            

				 //处理xnor类型扇出
				 void deal_xnor( const FP_infor& fpxnor)
				 {
                    assert(fpxnor.type == FPType::fp_xnor);
					assert(fpxnor.fanout_info.size() == 4);

                    auto xnor_1 = ntk.make_signal( fpxnor.fanout_info[2] );
					auto f1 = ntk.create_not( xnor_1 );

					auto f2 = ntk.make_signal( get_child_node( fpxnor.fanout_info[0] )[0] );
					auto opt1 = ntk.create_imp( f1,  f2 );

                    auto f3 = ntk.make_signal( fpxnor.fanout_info[2] );
					auto xnor_2 = ntk.make_signal( fpxnor.fanout_info[0] );
                    auto opt2 = ntk.create_imp( f3, xnor_2 );

                    ntk.substitute_node( fpxnor.fanout_info[3], opt1);
					ntk.substitute_node( fpxnor.fanout_info[1], opt2);
				 }
      private:
        Ntk& ntk;
        imgf_params const& ps;
    };

  }; /* namespace detail*/


/*! \brief construct a fanout-free implication logic network
 *
 * This algorithm tries to rewrite a network with 2-input
 * implication gates for depth
 * optimization using the  identities in
 * implication logic.  
 *
 * **Required network functions:**
 * - `get_node`
 * - `level`
 * - `update_levels`
 * - `create_imp`
 * - `substitute_node`
 * - `foreach_node`
 * - `foreach_po`
 * - `foreach_fanin`
 * - `is_imp`
 * - `clear_values`
 * - `set_value`
 * - `value`
 * - `fanout_size`
 *
   \verbatim embed:rst

   \endverbatim
 */
  template<class Ntk>
  void imgf( Ntk& ntk, imgf_params const& ps = {} )
  {
	// detail::imgf<Ntk> info;
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
    // static_assert( has_level_v<Ntk>, "Ntk does not implement the level method" );
    static_assert( has_substitute_node_v<Ntk>, "Ntk does not implement the substitute_node method" );
    // static_assert( has_update_levels_v<Ntk>, "Ntk does not implement the update_levels method" );
    static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
    static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
    static_assert( has_clear_values_v<Ntk>, "Ntk does not implement the clear_values method" );
    static_assert( has_set_value_v<Ntk>, "Ntk does not implement the set_value method" );
    static_assert( has_value_v<Ntk>, "Ntk does not implement the value method" );
    static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );
	detail::imgf<Ntk> p( ntk, ps );
	p.run();
	// ntk.foreach_node( [&]( const auto &n )
	// {
	// 	p.run_11();
	// });
	// while( params.xnorCount + params.secondCount + params.oneCount + params.bnCount + params.fnCount + params.one_secondCount + params.bn_xnorCount > 0)
	// p.run();
	// std::cout << "xnor扇出节点数量：" << params.xnorCount << std::endl;
	// std::cout << "第二类扇出节点数量：" <<  params.secondCount << std::endl;
	// std::cout << "第一类扇出节点数量：" << params.oneCount << std::endl;
	// std::cout << "bn类扇出节点数量：" << params.bnCount << std::endl;
	// std::cout << "fn类扇出节点数量：" << params.fnCount << std::endl;
    // std::cout << "第一、二类混合扇出节点数量：" << params.one_secondCount << std::endl;
	// std::cout << "bn、xnor混合扇出节点数量：" << params.bn_xnorCount << std::endl;
  }

  template<class Ntk>
  void imgf_1( Ntk& ntk, imgf_params const& ps = {} )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
    // static_assert( has_level_v<Ntk>, "Ntk does not implement the level method" );
    static_assert( has_substitute_node_v<Ntk>, "Ntk does not implement the substitute_node method" );
    // static_assert( has_update_levels_v<Ntk>, "Ntk does not implement the update_levels method" );
    static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
    static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
    static_assert( has_clear_values_v<Ntk>, "Ntk does not implement the clear_values method" );
    static_assert( has_set_value_v<Ntk>, "Ntk does not implement the set_value method" );
    static_assert( has_value_v<Ntk>, "Ntk does not implement the value method" );
    static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );

    detail::imgf<Ntk> p( ntk, ps );
    p.run_1();
  }

  template<class Ntk>
  void imgf_2( Ntk& ntk, imgf_params const& ps = {} )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
    // static_assert( has_level_v<Ntk>, "Ntk does not implement the level method" );
    static_assert( has_substitute_node_v<Ntk>, "Ntk does not implement the substitute_node method" );
    // static_assert( has_update_levels_v<Ntk>, "Ntk does not implement the update_levels method" );
    static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
    static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
    static_assert( has_clear_values_v<Ntk>, "Ntk does not implement the clear_values method" );
    static_assert( has_set_value_v<Ntk>, "Ntk does not implement the set_value method" );
    static_assert( has_value_v<Ntk>, "Ntk does not implement the value method" );
    static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );

    detail::imgf<Ntk> p( ntk, ps );
    p.run_2();
  }

  template<class Ntk>
  void imgf_3( Ntk& ntk, imgf_params const& ps = {} )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
    // static_assert( has_level_v<Ntk>, "Ntk does not implement the level method" );
    static_assert( has_substitute_node_v<Ntk>, "Ntk does not implement the substitute_node method" );
    // static_assert( has_update_levels_v<Ntk>, "Ntk does not implement the update_levels method" );
    static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
    static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
    static_assert( has_clear_values_v<Ntk>, "Ntk does not implement the clear_values method" );
    static_assert( has_set_value_v<Ntk>, "Ntk does not implement the set_value method" );
    static_assert( has_value_v<Ntk>, "Ntk does not implement the value method" );
    static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );

    detail::imgf<Ntk> p( ntk, ps );
    p.run_3();

  }
  template<class Ntk>
  void imgf_11( Ntk& ntk, imgf_params const& ps = {} )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
    // static_assert( has_level_v<Ntk>, "Ntk does not implement the level method" );
    static_assert( has_substitute_node_v<Ntk>, "Ntk does not implement the substitute_node method" );
    // static_assert( has_update_levels_v<Ntk>, "Ntk does not implement the update_levels method" );
    static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
    static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
    static_assert( has_clear_values_v<Ntk>, "Ntk does not implement the clear_values method" );
    static_assert( has_set_value_v<Ntk>, "Ntk does not implement the set_value method" );
    static_assert( has_value_v<Ntk>, "Ntk does not implement the value method" );
    static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );

    detail::imgf<Ntk> p( ntk, ps );
    p.run_11();
  }

} /* namespace also */

#endif
