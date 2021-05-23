#pragma once
#ifndef  IMG_FANOUT_HPP
#define  IMG_FANOUT_HPP

#include <mockturtle/mockturtle.hpp>
#include "../networks/img/img.hpp"
#include "../core/fanout.hpp"
#include "../networks/img/fc_cost.hpp"

using namespace also;

namespace alice
{

	class img_fanout_command : public command
	{
	public:
		explicit img_fanout_command(const environment::ptr& env) : command(env, "Performs IMG fanout")
		{
		}

  	        std::array<img_network::signal, 2> get_children(img_network img, img_network::node const& n ) const
                {
                	std::array<img_network::signal, 2> children;
                	img.foreach_fanin( n, [&children]( auto const& f, auto i ) { children[i] = f; } );
                	return children;
                }


                int not_imply_gate(const img_network& img )
                {
			int num=0;
                	img.foreach_node([&](auto n){
                 	auto cs = get_children(img,n);
                        if(cs[1].index == 0 ) num++;
                        });
             		return num;
         	}

	protected:
		void execute()
		{
	        	img_network img = store<img_network>().current();

	        	while(total_fc_cost(img)!=0)
	        	{
                		img = cleanup_dangling(img);
	            		depth_view img11{img};
	            		img_fanout_rewriting(img11);
	         	}

            		std::cout<<"[cost_function]  pulse: "<<img.num_gates()+1<<" memristors: "<<not_imply_gate(img)+img.num_pis()<<"\n";
	        	std::cout<<"[img_fanout] ";
	        	also::print_stats(img);
            		store<img_network>().extend();
            		store<img_network>().current() = img;
	    	}

	};
	ALICE_ADD_COMMAND(img_fanout, "Rewriting")
}
#endif

