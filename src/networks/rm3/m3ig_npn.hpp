#pragma once

#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>

#include <kitty/dynamic_truth_table.hpp>
#include <kitty/npn.hpp>
#include <kitty/print.hpp>
#include "../../core/misc.hpp"
#include <mockturtle/mockturtle.hpp>

namespace mockturtle
{

/*! \brief Resynthesis function based on pre-computed size-optimum rm3igs.
 *
 * This resynthesis function can be passed to ``node_resynthesis``,
 * ``cut_rewriting``, and ``refactoring``.  It will produce an rm3ig based on
 * pre-computed size-optimum rm3igs with up to at most 4 variables.
 * Consequently, the nodes' fan-in sizes in the input network must not exceed
 * 4.
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      const klut_network klut = ...;
      rm3ig_npn_resynthesis resyn;
      const auto rm3ig = node_resynthesis<rm3ig_network>( klut, resyn );
   \endverbatim
 */

class m3ig_npn_resynthesis
{
public:
  // 默认构造函数，调用 build_db() 来构建内部的数据库
  m3ig_npn_resynthesis()
  {
    build_db();
  }

  // 运算符的重载，实现了将输入逻辑函数转换为 rm3ig 网络的功能
  template<typename LeavesIterator, typename Fn>
  void operator()( mig_network& m3ig,
                   kitty::dynamic_truth_table const& function,
                   LeavesIterator begin,
                   LeavesIterator end,
                   Fn&& fn ) const
  {
    assert( function.num_vars() <= 4 );
    // 断言：布尔函数的变量数不超过4
    const auto fe = kitty::extend_to( function, 4 );
    // 使用 kitty::exact_npn_canonization 函数对扩展后的逻辑函数进行精确的 NPN（等效）规范化，得到其等效配置信息
    const auto config = kitty::exact_npn_canonization( fe );

    // 从配置中获取信号信息
    auto func_str = "0x" + kitty::to_hex( std::get<0>( config ) );
    //std::cout << "func: " << func_str << std::endl;
    // 在 class2signal 中查找函数等效配置对应的 rm3ig 信号
    const auto it = class2signal.find( func_str );
    assert( it != class2signal.end() );

    // 构建输入信号向量
    std::vector<mig_network::signal> pis( 4, m3ig.get_constant( false ) );
    std::copy( begin, end, pis.begin() );

    // 对输入进行置换
    std::vector<mig_network::signal> pis_perm( 4 );
    auto perm = std::get<2>( config );
    for ( auto i = 0; i < 4; ++i )
    {
      pis_perm[i] = pis[perm[i]];
    }

    // 对输入进行相位调整
    const auto& phase = std::get<1>( config );
    for ( auto i = 0; i < 4; ++i )
    {
      if ( ( phase >> perm[i] ) & 1 )
      {
        pis_perm[i] = !pis_perm[i];
      }
    }

    // 针对每个输出信号进行处理
    for ( auto const& po : it->second )
    {
      topo_view topo{ db, po };
      auto f = cleanup_dangling( topo, m3ig, pis_perm.begin(), pis_perm.end() ).front();

      // 根据相位信息调整输出信号的值，并调用用户提供的处理函数
      if ( !fn( ( ( phase >> 4 ) & 1 ) ? !f : f ) )
      {
        return; /* quit */
      }
    }
  }

private:
  // 存储从文件加载的最优 rm3ig 结构
  std::unordered_map<std::string, std::vector<std::string>> opt_m3ig;

  // 从文件中加载预先计算好的最优 rm3ig 结构
  void load_optimal_m3ig()
  {
    std::ifstream infile( "/home/ym/also/src/networks/m5ig/opt_m3ig.txt" );
    if ( !infile )
    {
      std::cout << " Cannot open file " << std::endl;
      assert( false );
    }

    std::string line;
    std::vector<std::string> v;
    while ( std::getline( infile, line ) )
    {
      v.clear();
      auto strs = also::split_by_delim( line, ' ' );
      for ( auto i = 1u; i < strs.size(); i++ )
      {
        v.push_back( strs[i] );
      }
      opt_m3ig.insert( std::make_pair( strs[0], v ) );
    }
  }

  // 根据输入的字符串向量和信号向量创建 rm3ig 网络
  std::vector<mig_network::signal> creat_m3ig_from_str_vec( const std::vector<std::string> strs,
                                                             const std::vector<mig_network::signal>& signals )
  {
    auto sig = signals;
    auto size = strs.size();
    auto pol = std::stoi( strs[0] );

    std::vector<mig_network::signal> result;

    int a, b, c;

    /*for( const auto& s : strs )
    {
      std::cout << s << std::endl;
    }*/

    for ( auto i = 1; i < size; i++ )
    {
      const auto substrs = also::split_by_delim( strs[i], '-' );

      assert( substrs.size() == 3u );

      a = substrs[2][0] - '0';
      b = substrs[2][1] - '0';
      c = substrs[2][2] - '0';

#if 0
      std::cout << " a: " << a << " sig[a]: " << sig[a].index << std::endl
                << " b: " << b << " sig[b]: " << sig[b].index << std::endl
                << " c: " << c << " sig[c]: " << sig[c].index << std::endl;
#endif
      switch ( std::stoi( substrs[1] ) )
      {
      case 0:
        sig.push_back( db.create_maj( sig[a], sig[b], sig[c] ) );
        break;

      case 1:
        sig.push_back( db.create_maj( sig[a] ^ true, sig[b], sig[c] ) );
        break;

      case 2:
        sig.push_back( db.create_maj( sig[a], sig[b] ^ true, sig[c] ) );
        break;

      case 3:
        sig.push_back( db.create_maj( sig[a], sig[b], sig[c] ^ true ) );
        break;

      default:
        assert( false );
        break;
      }
    }

    const auto driver = sig[sig.size() - 1] ^ ( pol ? true : false );
    db.create_po( driver );
    result.push_back( driver );
    return result;
  }

  // 构建了一个包含预先计算的最优 rm3ig 结构的数据库
  void build_db()
  {
    std::vector<mig_network::signal> signals;
    signals.push_back( db.get_constant( false ) );

    for ( auto i = 0u; i < 4; ++i )
    {
      signals.push_back( db.create_pi() );
    }

    load_optimal_m3ig();

    for ( const auto e : opt_m3ig )
    {
      //std::cout << "tt" << e.first << std::endl;
      if ( e.first == "0x0000" )
      {
        std::vector<mig_network::signal> tmp{ signals[0] };
        class2signal.insert( std::make_pair( e.first, tmp ) );
      }
      else if ( e.first == "0x00ff" )
      {
        std::vector<mig_network::signal> tmp{ signals[4] ^ true };
        class2signal.insert( std::make_pair( e.first, tmp ) );
      }
      else
      {
        // 根据最优 rm3ig 结构的字符串信息和 signals 向量创建 rm3ig 网络
        // 将创建的 rm3ig 网络的信号向量作为值，插入到 class2signal 中，键为当前最优 rm3ig 结构的字符串表示
        class2signal.insert( std::make_pair( e.first, creat_m3ig_from_str_vec( e.second, signals ) ) );
      }
    }
  }

  // rm3ig网络的实例
  mig_network db;
  // 将字符串表示的函数映射到 rm3ig 网络的信号
  std::unordered_map<std::string, std::vector<mig_network::signal>> class2signal;
};
} // namespace mockturtle