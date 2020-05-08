/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2020- Ningbo University, Ningbo, China */
 // hmtian

#include <mockturtle/mockturtle.hpp>
#include <mockturtle/properties/migcost.hpp>
#include <mockturtle/utils/stopwatch.hpp>
#include <mockturtle/networks/xmg.hpp>

using namespace mockturtle;

namespace also
{
	using ntk = xmg_network;
	using nd_t = node<ntk>;

	class nniinv_manager
	{
	public:
		nniinv_manager(ntk xmg);

		int num_invs_fanins(nd_t n);
		int num_maj_remain(ntk const& xmg);
		int num_xors(ntk const& xmg);
		int num_constant_fanins(nd_t n);
		int num_invs_reduced(ntk const& xmg);

		void run();

	private:
		ntk& xmg;
		unsigned num_nni_opt{ 0u }, num_maj_opt{ 0u }, num_maj_ori{ 0u }, num_xor_ori{ 0u }, num_xor_opt{ 0u }, num_inv_ori{0u}, num_inv_opt{0u};
		double qca_energy_ori, qca_energy_opt;
	};

	nniinv_manager::nniinv_manager(ntk xmg)
		: xmg(xmg)
	{
	}
	/**************************************************************************************************************/
	  /* Compute the total bumber of INV of all fanins of a node*/
	int nniinv_manager::num_invs_fanins(nd_t n)
	{
		int cost = 0;

		xmg.foreach_fanin(n, [&](auto s) { if (xmg.is_complemented(s)) { cost++; } });
		return cost;
	}

	int nniinv_manager::num_constant_fanins(nd_t n)
	{
		int num_con = 0;

		xmg.foreach_fanin(n, [&](auto s){ if (s == 0 || s == 1) { num_con++; } });
		return num_con;
	}

        int nniinv_manager::num_invs_reduced(ntk const& xmg)
	{
		int cost1 = 0;
		int cost2 = 0;
		xmg.foreach_gate([&] (auto n)
		{
			auto tmp = num_invs_fanins(n);
			auto tmp1 = num_constant_fanins(n);
			if(xmg.is_maj(n))
			{
			if(tmp == 2)
			{
				cost1++;
			}
			else if(tmp == 1 && tmp1 == 1)
			{
				cost2++;
			}
			}
		});
		return cost1 * 2 + cost2;
	}
	/**************************************************************************************************************/
  /* The number of NNI gates increased to substitute MAJ, for eliminating INVs. */
	int nniinv_manager::num_maj_remain(ntk const& xmg)
	{
		int num_maj = 0;

		xmg.foreach_gate([&](auto n)
		{
			auto tmp = num_invs_fanins(n);
			auto tmp1 = num_constant_fanins(n);
			if(xmg.is_maj(n))
			{
			if (tmp == 0 || tmp == 3)
			{
				num_maj++;
			}
			else if(tmp == 1 && tmp1 == 0)
			{
				num_maj++;
			}
			}
					
		}
		);

		return num_maj;
	}

	int nniinv_manager::num_xors(ntk const& xmg)
	{
		int num_xor = 0;
		xmg.foreach_gate([&](auto n)
		{	
			if(xmg.is_xor3(n))
			{
				num_xor++;
			}
			
		});
		
		return num_xor;
	}
	/**************************************************************************************************************/
	void nniinv_manager::run()
	{
		num_xor_ori = num_xors(xmg);
		num_maj_ori = xmg.num_gates() - num_xor_ori;
		num_inv_ori = num_inverters(xmg);

		num_maj_opt = num_maj_remain(xmg);
		num_xor_opt = num_xors(xmg);
		num_nni_opt = num_maj_ori - num_maj_opt;
		num_inv_opt = num_invs_reduced(xmg);

		qca_energy_ori = num_xor_ori * 6.62 + num_maj_ori *2.94 + num_inv_ori * 9.80;
		qca_energy_opt = num_maj_opt * 2.94 + num_xor_ori * 6.615 + num_nni_opt * 3.24 + num_inv_opt * 9.80;

		std::cout << "*****************************************************" << std::endl;
		std::cout << "num maj ori                             = " << num_maj_ori << std::endl;
		std::cout << "num xor ori                             = " << num_xor_ori << std::endl;
		std::cout << "num inv ori                             = " << num_inv_ori << std::endl << std::endl;
		std::cout << "num maj remaining                       = " << num_maj_opt << std::endl;
		std::cout << "num nni increasing                      = " << num_nni_opt << std::endl;
		std::cout << "num inv opt                             = " << num_inv_opt << std::endl;
		std::cout << "num of qca cells reduced by nni         = " << num_nni_opt << std::endl;
		std::cout << "[QCA energy]\n";
		std::cout << "energy ori                              = "<< qca_energy_ori << std::endl;
		std::cout << "energy opt                              = "<< qca_energy_opt << std::endl;
		std::cout << "*****************************************************" << std::endl;
	}
	/**************************************************************************************************************/
	  /* public function */
	void xmg2nni_inv_optimization(xmg_network& xmg)
	{
		nniinv_manager mgr(xmg);
		mgr.run();
	}
	/********************************************Functions used end*****************************************************************/
}
