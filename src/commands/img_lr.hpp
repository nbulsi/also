/* also: Advanced Logic Synthesis and Optimization tool
     * Copyright (C) 2023- Ningbo University, Ningbo, China */

/**
 * @file img_lr.hpp
 *
 * @brief construct a fanout-free implication logic network by logic restructuring
 */

#ifndef DEMO_HPP
#define DEMO_HPP

#include "../core/imgf.hpp"
using namespace mockturtle;
    namespace alice
{

  class imglr_command : public command
  {
  public:
    explicit imglr_command( const environment::ptr& env ) : command( env, "Constructing a fanout-free implication logic network by logic restructuring " )
    {
      add_flag( "--pm_xnor_fp2, -a", "Solving the type of xnor and fp2 problem" );
      add_flag( "--pm_bn_fp1, -b", "Solving the type of BN and fp1 problem" );
      add_flag( "--pm_fn, -c", "Solving the type of FN problem" );
      add_flag( "--pm_all, -d", "Solving all the fanout problem" );
      add_flag( "--pm_test, -t", "Analyze fanout information for each node in the circuit" );
    }

    rules validity_rules() const
    {
      return { has_store_element<img_network>( env ) };
    }
    std::array<img_network::signal, 2> get_children( img_network img, img_network::node const& n ) const
    {
      std::array<img_network::signal, 2> children;
      img.foreach_fanin( n, [&children]( auto const& f, auto i )
                         { children[i] = f; } );
      return children;
    }
    // 计算not蕴含门的数量
    int not_imply_gate( const img_network& img )
    {
      int num = 0;
      img.foreach_gate( [&]( auto n )
          {
            auto cs = get_children(img,n);
            if(cs[1].index  == 0 ) num++; } );
            return num;
    }
    int real_size(  const img_network& img  ) 
    {
      int number =0;
      img.foreach_node( [&]( auto n )
          {
						if( img.is_dead( n ) )
							number ++; } );
              std::cout <<" 死节点数量：" << number << std::endl;
              std::cout <<" size数量: " << img.size() << std::endl;
              return (img.size()- number);
    }
  protected:
    void execute()
    {
      img_network img = store<img_network>().current();

      stopwatch<>::duration time{ 0 };
      
      // 选择不同扇出情况
      if ( is_set( "pm_xnor_fp2" ) )
      {
        call_with_stopwatch( time, [&]()
                             {
							fanout_view fanout_img( img );   //分析fanout故障肯定要fanout的连接信息
						  imgf_1(fanout_img);             //主要函数
              img = cleanup_dangling( img ); } );
      }

      else if ( is_set( "pm_bn_fp1" ) )
      { 
        call_with_stopwatch( time, [&]()
                             {
							fanout_view fanout_img( img );   //分析fanout故障肯定要fanout的连接信息
						  imgf_2(fanout_img);             //主要函数
              img = cleanup_dangling( img ); } );
      }

      else if ( is_set( "pm_fn" ) )
      {
        call_with_stopwatch( time, [&]()
                             {
							fanout_view fanout_img( img );   //分析fanout故障肯定要fanout的连接信息
						  imgf_3(fanout_img);             //主要函数
              img = cleanup_dangling( img ); } );
      }

      else if ( is_set( "pm_all" ) )
      {
        // use_deal_fn_2018 = true;
        call_with_stopwatch( time, [&]()
                             {
							fanout_view fanout_img( img );   //分析fanout故障肯定要fanout的连接信息
              imgf(fanout_img);      //主要函数
              img = cleanup_dangling( img ); } );
      }
     else if ( is_set( "pm_test" ) )
      {
        call_with_stopwatch( time, [&]()
                             {
							fanout_view fanout_img( img );   //分析fanout故障肯定要fanout的连接信息
						  imgf_11(fanout_img);             //主要函数
              img = cleanup_dangling( img ); } );
      }

      /* print information */
      // std::cout << "real_size" ;
      // real_size( img );
      // std::cout << " real_size: " << real << std::endl;
      // std::cout<< "params.bn_size: " << params.bn_size << std::endl;
      std::cout << "[cost_function]  pulse: " << img.num_gates() + 1 << " memristors: " << not_imply_gate( img ) + img.num_pis() + params.bn_size << "\n";
      std::cout << "[imglr] ";
      also::print_stats( img );
      store<img_network>().extend();
      store<img_network>().current() = img;

      std::cout << fmt::format( "[time]: {:5.2f} ms\n", to_seconds( time ) * 1000 );
    }

  private:
    imgf_params ps{};
    // int real;
  };

  ALICE_ADD_COMMAND( imglr, "Rewriting" )
}

#endif
