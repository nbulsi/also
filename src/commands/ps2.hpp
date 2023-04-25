/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file fanin_histogram.hpp
 *
 * @brief statistic fanin_histogram
 *
 * @author Chunliu Liao
 * @since  0.1
 */

#ifndef PS2
#define PS2
#include <../core/ps2_implemented.hpp>
#include <alice/alice.hpp>
#include <mockturtle/mockturtle.hpp>
namespace alice
{
using namespace mockturtle;
class ps2_command : public command
{
public:
  explicit ps2_command( const environment::ptr& env )
      : command( env, "Show statistics in the stored network." )
  {
    add_flag( "--aig,-a", "Display stats for stored AIG" );
    add_flag( "--mig,-m", "Display stats for stored MIG" );
    add_flag( "--xag,-x", "Display stats for stored XAG" );
    add_flag( "--xmg,-g", "Display stats for stored XMG" );
    add_flag( "--criti,-c", "Display critical path node index and gate" );
    add_flag( "--fanout,-f", "Display fanout histogram" );
    add_flag( "--cone,-o", "Dispaly logic cones from PO to PI(only for AIG)" );
    add_flag( "--level,-l", "Display level histigram" );
    add_flag( "--skip,-k", "Display skip histogram" );
  }

  template<typename network>
  void dump_stats( std::string name )
  {
    if ( store<network>().empty() )
    {
      env->err() << name << " network not stored\n";
      return;
    }
    auto ntk = store<network>().current();
    mockturtle::depth_view dag_depth{ ntk };

    /*critical_path*/
    also::critical_node_view<mockturtle::names_view<network>> critical{ ntk };
    auto critical_path = critical.get_critical_path();
    auto critical_nums = critical.get_critical_nums();
    also::function_counts counts;
    for ( auto curr_node : critical_path )
    {
      update_counts( counts, ntk, curr_node );
    }

    /*fanin_histogram*/
    mockturtle::fanout_view<network> fanout{ ntk };
    uint32_t times = 0;
    fanout.foreach_node( [&]( auto node ) {
                bool pure = true;
                fanout.foreach_fanin(node,[&](auto fi){
                    uint32_t focount = 0;
                    auto node = fanout.get_node(fi);
                    fanout.foreach_fanout(node,[&](auto fo){
                        focount++;
                    });
                    bool isolated_fanin = (focount == 1)? 1 : 0;
                    pure = pure && (fanout.is_constant(node) || isolated_fanin);
                });
                if (!pure)
                    times++; } );

    /*fanout_histogram*/
    //mockturtle::fanout_view<network> fanout{ ntk };
    std::vector<uint32_t> fanout_histogram( 33, 0 );
    std::vector<int> fanout_vec;
    int max_fanout = 0;
    double average_fanout = 0;
    fanout.foreach_node( [&]( auto node ) {
                uint32_t foc = 0;
                fanout.foreach_fanout(node,[&](auto fo){
                    foc++;
                });
                fanout_vec.emplace_back(foc);
                if (foc > max_fanout){
                    max_fanout = foc;
                }
                if (foc >= fanout_histogram.size() - 1){
                    fanout_histogram[fanout_histogram.size() - 1]++;
                }
                else {
                    fanout_histogram[foc]++;
                } } );
    average_fanout = std::accumulate( fanout_vec.begin(), fanout_vec.end(), 0.0, []( double acc, int i ) { return acc + static_cast<double>( i ); } ) / fanout_vec.size();

    /*function_stats*/
    also::function_counts counts1 = also::node_functions( ntk );

    /*skip_histogram*/
    std::vector<uint32_t> skip_histogram( dag_depth.depth(), 0 );
    uint32_t mismatch_levels = 0;
    fanout.foreach_node( [&]( auto node ) {
                uint32_t last_level = 0;
                bool mismatched = false;
                fanout.foreach_fanout(node,[&](auto f){
                    if (last_level != 0 && last_level != dag_depth.level(f)){
                        mismatched = true;
                    }
                    last_level = dag_depth.level(f);
                    uint32_t skip = dag_depth.level(f) - dag_depth.level(node);
                    skip_histogram[skip]++;
                });
                if (mismatched){
                    mismatch_levels++;
                } } );
    int max_skip = 0;
    double average_skip = 0.0;
    double dividend = 0.0;
    double divisor = 0.0;
    for ( int i = 0; i < skip_histogram.size(); i++ )
    {
      dividend += i * skip_histogram[i];
      divisor += skip_histogram[i];
    }
    average_skip = dividend / divisor;
    max_skip = skip_histogram.size() - 1;

    env->out() << "*******************************************************************************\n"
               << "*                            Statistic information                             *\n"
               << "*******************************************************************************\n"
               << std::setw( 2 ) << std::left << " " << std::setw( 70 ) << std::left << "Total size of critical path nodes : " << std::setw( 5 ) << std::left << critical_path.size() << "\n"
               << std::setw( 2 ) << std::left << " " << std::setw( 70 ) << std::left << "Number of critical paths : " << std::setw( 5 ) << std::left << critical_nums << "\n"
               << std::setw( 2 ) << std::left << " " << std::setw( 70 ) << std::left << "Number of pure nodes (at least one child node has multiple fanouts) : " << std::setw( 5 ) << std::left << times << "\n"
               << "-------------------------------------------------------------------------------\n"
               << "  +Node fanout : "
               << "\n"
               << std::setw( 3 ) << std::left << " "
               << "| Max Fanout | = " << max_fanout << std::setw( 10 ) << " "
               << "| Average Fanout | = " << average_fanout << "\n"
               << "-------------------------------------------------------------------------------\n"
               << "  +Function_statistic : "
               << "\n";

    std::cout << std::setw( 3 ) << " " << std::setfill( '-' ) << std::setw( 61 ) << "" << std::setfill( ' ' ) << std::endl;
    std::cout << std::setw( 3 ) << " "
              << "| " << std::left << std::setw( 4 ) << "Name"
              << " |" << std::setw( 4 ) << "MAJ"
              << " |" << std::setw( 4 ) << "AND"
              << " |" << std::setw( 4 ) << "OR"
              << " |" << std::setw( 4 ) << "XOR3"
              << " |" << std::setw( 4 ) << "XOR"
              << " |" << std::setw( 4 ) << "XNOR"
              << " |" << std::setw( 7 ) << "Unknown"
              << " |" << std::setw( 7 ) << "Input"
              << " |" << std::endl;
    std::cout << std::setw( 3 ) << " " << std::setfill( '-' ) << std::setw( 61 ) << "" << std::setfill( ' ' ) << std::endl;
    std::cout << std::setw( 3 ) << " "
              << "| " << std::left << std::setw( 4 ) << "nums"
              << " |" << std::setw( 4 ) << ( counts1.maj_num == 0 ? " / " : std::to_string( counts1.maj_num ) ) 
              << " |" << std::setw( 4 ) << ( counts1.and_num == 0 ? " / " : std::to_string( counts1.and_num ) ) 
              << " |" << std::setw( 4 ) << ( counts1.or_num == 0 ? " / " : std::to_string( counts1.or_num ) ) 
              << " |" << std::setw( 4 ) << ( counts1.xor3_num == 0 ? " / " : std::to_string( counts1.xor3_num ) ) 
              << " |" << std::setw( 4 ) << ( counts1.xor_num == 0 ? " / " : std::to_string( counts1.xor_num ) ) 
              << " |" << std::setw( 4 ) << ( counts1.xnor_num == 0 ? " / " : std::to_string( counts1.xnor_num ) ) 
              << " |" << std::setw( 7 ) << ( counts1.unknown_num == 0 ? " / " : std::to_string( counts1.unknown_num ) ) 
              << " |" << std::setw( 7 ) << ( counts1.input_num == 0 ? " / " : std::to_string( counts1.input_num ) ) << " |" << std::endl;

    std::cout << std::setw( 3 ) << " " << std::setfill( '-' ) << std::setw( 61 ) << "" << std::setfill( ' ' ) << std::endl;
    std::cout << "-------------------------------------------------------------------------------\n";
    std::cout << "  +Skip level : "
              << "\n"
              << "   | Number of nodes with inconsistent levels of fanout nodes (skip) : " << mismatch_levels << "\n"
              << std::setw( 3 ) << std::left << " "
              << "| Max skip Level | = " << max_skip << std::setw( 10 ) << " "
              << "| Average skip Level | = " << average_skip << "\n";
  }
  
  template<typename network>
  void critical_path( std::string name )
  {
    if ( store<network>().empty() )
      return;
    auto ntk = store<network>().current();
    also::critical_node_view<mockturtle::names_view<network>> c{ ntk };
    auto cp = c.get_critical_path();
    also::function_counts cou;
    for ( auto it : cp )
    {
      update_counts( cou, ntk, it );
    }
    std::cout << "  +Node in critical paths : "
              << "\n";
    for ( auto it : cp )
    {
      std::cout << std::setw( 3 ) << " ";
      std::cout << ntk.node_to_index( it ) << " ";
    }
    std::cout << std::endl;
    std::cout << "  +Node type in critical paths : "
              << "\n";
    std::cout << std::setw( 3 ) << " " << std::setfill( '-' ) << std::setw( 61 ) << "" << std::setfill( ' ' ) << std::endl;
    std::cout << std::setw( 3 ) << " "
              << "| " << std::left << std::setw( 4 ) << "Name"
              << " |" << std::setw( 4 ) << "MAJ"
              << " |" << std::setw( 4 ) << "AND"
              << " |" << std::setw( 4 ) << "OR"
              << " |" << std::setw( 4 ) << "XOR3"
              << " |" << std::setw( 4 ) << "XOR"
              << " |" << std::setw( 4 ) << "XNOR"
              << " |" << std::setw( 7 ) << "Unknown"
              << " |" << std::setw( 7 ) << "Input"
              << " |" << std::endl;
    std::cout << std::setw( 3 ) << " " << std::setfill( '-' ) << std::setw( 61 ) << "" << std::setfill( ' ' ) << std::endl;
    std::cout << std::setw( 3 ) << " "
              << "| " << std::left << std::setw( 4 ) << "nums"
              << " |" << std::setw( 4 ) << ( cou.maj_num == 0 ? " / " : std::to_string( cou.maj_num ) ) << " |" << std::setw( 4 ) << ( cou.and_num == 0 ? " /" : std::to_string( cou.and_num ) ) << " |" << std::setw( 4 ) << ( cou.or_num == 0 ? " /" : std::to_string( cou.or_num ) ) << " |" << std::setw( 4 ) << ( cou.xor3_num == 0 ? " /" : std::to_string( cou.xor3_num ) ) << " |" << std::setw( 4 ) << ( cou.xor_num == 0 ? " /" : std::to_string( cou.xor_num ) ) << " |" << std::setw( 4 ) << ( cou.xnor_num == 0 ? " /" : std::to_string( cou.xnor_num ) ) << " |" << std::setw( 7 ) << ( cou.unknown_num == 0 ? " /" : std::to_string( cou.unknown_num ) ) << " |" << std::setw( 7 ) << ( cou.input_num == 0 ? " /" : std::to_string( cou.input_num ) ) << " |" << std::endl;
    std::cout << std::setw( 3 ) << " " << std::setfill( '-' ) << std::setw( 61 ) << "" << std::setfill( ' ' ) << std::endl;
  }

  template<typename network>
  void fanout_histogram( std::string name )
  {
    if ( store<network>().empty() )
      return;
    auto ntk = store<network>().current();
    mockturtle::fanout_view<network> fanoutP{ ntk };
    int max_fanout = 0;
    std::vector<uint32_t> fanout_histogram( 33, 0 );
    fanoutP.foreach_node( [&]( auto node ) {
                uint32_t foc = 0;
                fanoutP.foreach_fanout(node,[&](auto fo){
                    foc++;
                });
                if ( foc > max_fanout )
                {
                  max_fanout = foc;
                }
                if (foc >= fanout_histogram.size() - 1){
                    fanout_histogram[fanout_histogram.size() - 1]++;
                }
                else {
                    fanout_histogram[foc]++;
                } } );
    std::vector<uint32_t> fanout_histogram1( fanout_histogram.begin(), fanout_histogram.begin() + max_fanout + 1 );
    std::cout << "  +Node counts by number of fanouts." << std::endl;
    std::cout << std::setw( 3 ) << " " << std::setfill( '-' ) << std::setw( 22 ) << "" << std::setfill( ' ' ) << std::endl;
    std::cout << std::setw( 3 ) << " "
              << "| " << std::left << std::setw( 10 ) << "Fanout"
              << std::right << " | " << std::setw( 5 ) << "Nodes"
              << " |" << std::endl;
    std::cout << std::setw( 3 ) << " " << std::setfill( '-' ) << std::setw( 22 ) << "" << std::setfill( ' ' ) << std::endl;
    for ( int i = 0; i < fanout_histogram1.size(); i++ )
    {
      std::cout << std::setw( 3 ) << " "
                << "| " << std::left << std::setw( 10 ) << i
                << std::right << " | " << std::setw( 5 ) << fanout_histogram1[i]
                << " |" << std::endl;
    }
    std::cout << std::setw( 3 ) << " " << std::setfill( '-' ) << std::setw( 22 ) << "" << std::setfill( ' ' ) << std::endl;
  }

  template<typename network>
  void get_cones( std::string name )
  {
    if ( !store<network>().empty() )
    {
      auto aig = store<network>().current();
      // map with number of nodes in each logical cone
      std::unordered_map<int, int> po_nodes;
      // number of inputs for each cone
      std::unordered_map<int, int> po_ins;
      // first processing logical cones for POs
      std::cout << "  +Logic cones from PO2PI." << std::endl;
      std::cout << std::setw( 3 ) << " " << std::setfill( '-' ) << std::setw( 44 ) << "" << std::setfill( ' ' ) << std::endl;
      std::cout << std::setw( 3 ) << " "
                << "| " << std::left << std::setw( 10 ) << "Name"
                << std::right << " | " << std::setw( 5 ) << "Index"
                << " |" << std::setw( 5 ) << "Nodes"
                << " |" << std::setw( 5 ) << "Level"
                << " |" << std::setw( 6 ) << "Inputs"
                << " |" << std::endl;
      std::cout << std::setw( 3 ) << " " << std::setfill( '-' ) << std::setw( 44 ) << "" << std::setfill( ' ' ) << std::endl;
      for ( int outIndex = 0; outIndex < aig.num_pos(); outIndex++ )
      {
        aig.foreach_node( [&]( auto node ) {
                    //set all nodes as not visited
                    aig._storage->nodes[node].data[1].h1 = 0; } );
        // start counter for a given output index
        po_nodes.insert( std::make_pair( outIndex, 0 ) );
        // starting the counter of inputs
        po_ins.insert( std::make_pair( outIndex, 0 ) );
        // calculate the index of the node driving the output
        auto inIdx = aig._storage->outputs[outIndex].data;
        if ( aig._storage->outputs[outIndex].data & 1 )
        {
          inIdx = aig._storage->outputs[outIndex].data - 1;
        }
        inIdx = inIdx >> 1;
        // call DFS
        also::compute_cone( aig, inIdx, po_nodes, outIndex, po_ins );
        aig.foreach_node( [&]( auto node ) {
                //set all nodes as not visited
                aig._storage->nodes[node].data[1].h1 = 0; } );

        int level = also::computeLevel<network>( aig, inIdx );
        int nodes = 0;
        int inputs = 0;

        // for each output prints index, nodes, depth and number of inputs, respectively
        std::unordered_map<int, int>::iterator it;
        it = po_nodes.find( outIndex );

        if ( it != po_nodes.end() )
          nodes = it->second;

        std::unordered_map<int, int>::iterator init;
        init = po_ins.find( outIndex );

        if ( it != po_ins.end() )
          inputs = init->second;

        std::cout << std::setw( 3 ) << " "
                  << "| " << std::left << std::setw( 10 ) << "Output"
                  << std::right << " | " << std::setw( 5 ) << outIndex
                  << " |" << std::setw( 5 ) << nodes
                  << " |" << std::setw( 5 ) << level
                  << " |" << std::setw( 6 ) << inputs
                  << " |" << std::endl;
      }
      std::cout << std::setw( 3 ) << " " << std::setfill( '-' ) << std::setw( 44 ) << "" << std::setfill( ' ' ) << std::endl;
    }
    else
    {
      env->err() << "There is not an AIG network stored.\n";
    }
  }

  template<typename ntk>
  void level_size( std::string name )
  {
    if ( store<ntk>().empty() )
    {
      env->err() << name << " network not stored\n";
      return;
    }
    auto dag = store<ntk>().current();
    mockturtle::depth_view dag_view( dag );
    std::vector<uint32_t> levels( dag_view.depth() + 1, 0 );
    dag_view.foreach_node( [&]( auto node ) { levels[dag_view.level( node )]++; } );
    std::cout << "  +Nodes per level." << std::endl;
    std::cout << std::setw( 3 ) << " " << std::setfill( '-' ) << std::setw( 22 ) << "" << std::setfill( ' ' ) << std::endl;
    std::cout << std::setw( 3 ) << " "
              << "| " << std::left << std::setw( 10 ) << "Level"
              << std::right << " | " << std::setw( 5 ) << "Nodes"
              << " |" << std::endl;
    std::cout << std::setw( 3 ) << " " << std::setfill( '-' ) << std::setw( 22 ) << "" << std::setfill( ' ' ) << std::endl;
    for ( int i = 0; i < levels.size(); i++ )
    {
      std::cout << std::setw( 3 ) << " "
                << "| " << std::left << std::setw( 10 ) << i
                << std::right << " | " << std::setw( 5 ) << levels[i]
                << " |" << std::endl;
    }
    std::cout << std::setw( 3 ) << " " << std::setfill( '-' ) << std::setw( 22 ) << "" << std::setfill( ' ' ) << std::endl;
  }

  template<typename ntk>
  void skip_histogram( std::string name )
  {
    if ( !store<ntk>().empty() )
    {
      auto dag = store<ntk>().current();
      mockturtle::depth_view dag_depth{ dag };
      mockturtle::fanout_view dag_fanout{ dag };
      std::vector<uint32_t> skip_histogram( dag_depth.depth(), 0 );
      uint32_t mismatch_levels = 0;
      dag_fanout.foreach_node( [&]( auto node ) {
                uint32_t last_level = 0;
                bool mismatched = false;
                dag_fanout.foreach_fanout(node,[&](auto f){
                    if (last_level != 0 && last_level != dag_depth.level(f)){
                        mismatched = true;
                    }
                    last_level = dag_depth.level(f);
                    uint32_t skip = dag_depth.level(f) - dag_depth.level(node);
                    skip_histogram[skip]++;
                });
                if (mismatched){
                    mismatch_levels++;
                } } );

      std::cout << "  +Nodes whose fanout skip levels : " << mismatch_levels << std::endl;
      std::cout << std::setw( 3 ) << " " << std::setfill( '-' ) << std::setw( 22 ) << "" << std::setfill( ' ' ) << std::endl;
      std::cout << std::setw( 3 ) << " "
                << "| " << std::left << std::setw( 11 ) << "Skip_levels"
                << std::right << " | " << std::setw( 5 ) << "Nodes"
                << " |" << std::endl;
      std::cout << std::setw( 3 ) << " " << std::setfill( '-' ) << std::setw( 22 ) << "" << std::setfill( ' ' ) << std::endl;
      for ( int i = 0; i < skip_histogram.size(); i++ )
      {
        std::cout << std::setw( 3 ) << " "
                  << "| " << std::left << std::setw( 11 ) << i
                  << std::right << " | " << std::setw( 5 ) << skip_histogram[i]
                  << " |" << std::endl;
      }
      std::cout << std::setw( 3 ) << " " << std::setfill( '-' ) << std::setw( 22 ) << "" << std::setfill( ' ' ) << std::endl;
    }
    else
    {
      env->err() << "There is not an " << name << " network stored.\n";
    }
  }

protected:
  void execute()
  {
    if ( is_set( "mig" ) )
    {
      dump_stats<mockturtle::mig_network>( "MIG" );
      if ( is_set( "criti" ) )
      {
        critical_path<mockturtle::mig_network>( "MIG" );
      }
      if ( is_set( "fanout" ) )
      {
        fanout_histogram<mockturtle::mig_network>( "MIG" );
      }
      if ( is_set( "level" ) )
      {
        level_size<mockturtle::mig_network>( "MIG" );
      }
      if ( is_set( "skip" ) )
      {
        skip_histogram<mockturtle::mig_network>( "MIG" );
      }
      return;
    }
    else if ( is_set( "xag" ) )
    {
      dump_stats<mockturtle::xag_network>( "XAG" );
      if ( is_set( "criti" ) )
      {
        critical_path<mockturtle::xag_network>( "XAG" );
      }
      if ( is_set( "fanout" ) )
      {
        fanout_histogram<mockturtle::xag_network>( "XAG" );
      }
      if ( is_set( "level" ) )
      {
        level_size<mockturtle::xag_network>( "XAG" );
      }
      if ( is_set( "skip" ) )
      {
        skip_histogram<mockturtle::xag_network>( "XAG" );
      }
      return;
    }
    else if ( is_set( "xmg" ) )
    {
      dump_stats<mockturtle::xmg_network>( "XMG" );
      if ( is_set( "criti" ) )
      {
        critical_path<mockturtle::xmg_network>( "XMG" );
      }
      if ( is_set( "fanout" ) )
      {
        fanout_histogram<mockturtle::xmg_network>( "XMG" );
      }
      if ( is_set( "level" ) )
      {
        level_size<mockturtle::xmg_network>( "XMG" );
      }
      if ( is_set( "skip" ) )
      {
        skip_histogram<mockturtle::xmg_network>( "XMG" );
      }
      return;
    }
    else if ( is_set( "aig" ) )
    {
      dump_stats<mockturtle::aig_network>( "AIG" );
    }
    else
    {
      //default
      dump_stats<mockturtle::aig_network>( "AIG" );
    }

    if ( is_set( "criti" ) )
    {
      critical_path<mockturtle::aig_network>( "AIG" );
    }
    if ( is_set( "fanout" ) )
    {
      fanout_histogram<mockturtle::aig_network>( "AIG" );
    }
    if ( is_set( "cone" ) )
    {
      get_cones<mockturtle::aig_network>( "AIG" );
    }
    if ( is_set( "level" ) )
    {
      level_size<mockturtle::aig_network>( "AIG" );
    }
    if ( is_set( "skip" ) )
    {
      skip_histogram<mockturtle::aig_network>( "AIG" );
    }
  }

private:
};

ALICE_ADD_COMMAND( ps2, "General" );
} // namespace alice
#endif
