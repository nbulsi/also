
#pragma once
#include <array>
#include <utility>
#include "../networks/img/img.hpp"
#include <mockturtle/mockturtle.hpp>

namespace mockturtle
{

namespace detail
{

	template<class Ntk>
	class img_fanout_rewriting_impl
	{
	public:
		img_fanout_rewriting_impl(Ntk& ntk)
			: ntk(ntk)
		{
		}

		void run()
		{
			topo_view topo(ntk);
			topo.foreach_node([&](auto n){
				if (!ntk.is_constant(n)&& !ntk.is_pi(n) && !is_not(n))
				{
					deal_with(ntk,n);
				}
			});
		}

		void deal_with(Ntk& ntk, node<Ntk> const& n)
		{
			auto cs = get_children(n);
                        auto n1 = ntk.get_node(cs[1]);

			std::vector<node<Ntk>> parents_right, parents_left;
			if (ntk.fanout_size(n1) >= 2u )
			{
				ntk.foreach_node([&](auto p) {
					auto ocs = get_children(p);
					if(ntk.get_node(ocs[0])==n1)
					{
						parents_right.push_back(p);
					}
				        else if (ntk.get_node(ocs[1])==n1)
					{
						parents_left.push_back(p);
					}
				});
			

			if (parents_left.size() == 1u )
			{
				auto p = parents_left[0];
				assert(p == n);
			
				for (auto i : parents_right)
				{
					auto ics = get_children(i);
					if (ics[0] == cs[1] && ics[1] == cs[0]) //confliting fanout
					{
						if (is_not(n1))
						{
							fanout_bn(n);
						}
						fanout(n);
					}
				}
			}
			else if (parents_left.size() > 1u)
			{
		//		if (is_not(n1))
		//		{
		//			fanout_bn(n);
		//		}
		//		else
		//		{
					fanout(n);
		//		}
			}
            }

		}

		void fanout(node<Ntk> const& n)
		{
			const auto cs = get_children(n);

			auto opt = ntk.create_imp(ntk.create_not(cs[1]), ntk.create_not(cs[0]));
			ntk.substitute_node(n, opt);
			ntk.update_levels();

		}

		void fanout_bn(node<Ntk> const& n)
		{
			auto cs = get_children(n);
			auto cs1 = get_children(ntk.get_node(cs[1]));
			assert(cs1[1].index == 0);

			auto opt = ntk.create_imp(cs1[0], ntk.create_not(cs[0]));
			ntk.substitute_node(n, opt);
			ntk.update_levels();
		}

		std::array<signal<Ntk>, 2> get_children(node<Ntk> const& n) const
		{
			std::array<signal<Ntk>, 2> children;
			ntk.foreach_fanin(n, [&children](auto const& f, auto i) { children[i] = f; });
			return children;
		}

		bool is_not(node<Ntk> const& n)
		{
			auto cs = get_children(n);
			if (cs[1].index == 0) return true;
			return false;
		}

	private:
		Ntk& ntk;
		std::pair<node<Ntk>,node<Ntk>> conflicting_node;
	};


}
	template<class Ntk>
	void img_fanout_rewriting(Ntk& ntk)
	{
		static_assert(is_network_type_v<Ntk>, "Ntk is not a network type");
		static_assert(has_get_node_v<Ntk>, "Ntk does not implement the get_node method");
		static_assert(has_level_v<Ntk>, "Ntk does not implement the level method");
		static_assert(has_substitute_node_v<Ntk>, "Ntk does not implement the substitute_node method");
		static_assert(has_update_levels_v<Ntk>, "Ntk does not implement the update_levels method");
		static_assert(has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method");
		static_assert(has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method");
		static_assert(has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method");
		static_assert(has_clear_values_v<Ntk>, "Ntk does not implement the clear_values method");
		static_assert(has_set_value_v<Ntk>, "Ntk does not implement the set_value method");
		static_assert(has_value_v<Ntk>, "Ntk does not implement the value method");
		static_assert(has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method");

		detail::img_fanout_rewriting_impl<Ntk> p(ntk);
		p.run();
	}
}

