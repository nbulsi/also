/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * @file plim_compiler.cpp
 *
 * @brief PLiM compiler from MIGs
 *
 * @author YanMing
 * @since  0.1
 */

#include "plim_compiler.hpp"

#include <map>
#include <queue>
#include <stack>
#include <unordered_map>

//#include <boost/dynamic_bitset.hpp>

#include <mockturtle/networks/mig.hpp>
#include <mockturtle/properties/migcost.hpp>
#include <mockturtle/views/depth_view.hpp>
#include <mockturtle/views/topo_view.hpp>
#include <mockturtle/views/fanout_view.hpp>

#define L( x )                   \
  if ( verbose )                 \
  {                              \
    std::cout << x << std::endl; \
  }

namespace also
{

/******************************************************************************
 * Types                                                                      *
 ******************************************************************************/

/******************************************************************************
 * Private functions                                                          *
 ******************************************************************************/
template<typename Ntk>
std::unordered_map<typename Ntk::node, std::vector<typename Ntk::node>> mig_precompute_ingoing_edges(Ntk& ntk)
{
  mockturtle::fanout_view<Ntk> fanout_ntk( ntk );
  std::unordered_map<typename Ntk::node, std::vector<typename Ntk::node>> fanouts;
  fanout_ntk.foreach_node([&](auto const& n) {
    std::vector<typename Ntk::node> ingoing_edges;
    fanout_ntk.foreach_fanout(n, [&](auto const& fanout_node) {
      ingoing_edges.push_back(fanout_node);
      return true; // 继续迭代
    });
    fanouts[n] = ingoing_edges;
  });
  return fanouts;
}

template<typename Ntk>
std::unordered_map<typename Ntk::node, std::vector<typename Ntk::node>> precompute_ingoing_edges_and_degrees( const Ntk& ntk )
{
  // 创建 fanout_view 实例
  mockturtle::fanout_view<Ntk> fanout_ntk( ntk );

  // 获取每个节点的入边
  std::unordered_map<typename Ntk::node, std::vector<typename Ntk::node>> fanouts;
  fanout_ntk.foreach_node( [&]( auto const& n )
                           {
        std::vector<typename Ntk::node> ingoing_edges;
        fanout_ntk.foreach_fanout(n, [&](auto const& fanout_node) {
            ingoing_edges.push_back(fanout_node);
            return true; // 继续迭代
        });
        fanouts[n] = ingoing_edges; } );

  // 计算每个节点的入度
  std::unordered_map<typename Ntk::node, size_t> fanout_count;
  for ( const auto& [node, edges] : fanouts )
  {
    fanout_count[node] = edges.size();
  }

  return fanouts;
}

template<typename Ntk>
std::unordered_map<typename Ntk::node, size_t> mig_precompute_in_degrees( Ntk& ntk )
{
    fanout_view<Ntk> fanout_ntk( ntk );

    std::unordered_map<typename Ntk::node, size_t> fanout_count;
    fanout_ntk.foreach_node( [&]( auto const& n )
                      {
    size_t out_degree = 0;
    fanout_ntk.foreach_fanout( n, [&]( auto const& fanout )
                               {
      (void)fanout; // 只增加计数，不使用变量
      out_degree++; } );
    fanout_count[n] = out_degree;
    } );
    return fanout_count;
}

template<typename Ntk>
size_t mig_num_vertices( Ntk& ntk )
{
  size_t count = 0;
  ntk.foreach_node( [&]( auto const& )
                    { count++; } );
  return count;
}

template<typename IndexType>
class auto_index_generator
{
  /*用于生成索引值，并根据指定的策略进行请求和释放索引*/
public:
  enum class request_strategy
  {
    lifo,
    fifo
  };

  /*该构造函数接受一个参数 strategy，表示请求策略（后进先出或先进先出）。构造函数用于初始化 auto_index_generator 对象，设置请求策略。*/
  auto_index_generator( request_strategy strategy )
      : strategy( strategy )
  {
  }

  /*用于请求索引*/
  IndexType request()
  {
    /*如果存在空闲的索引（free 不为空），则从空闲索引中获取一个索引*/
    if ( !free.empty() )
    {
      IndexType index; // 声明一个变量index，用于存储生成的索引

      /*根据策略（strategy）的不同，可以选择从后进先出（LIFO）或先进先出（FIFO）的方式获取索引*/
      if ( strategy == request_strategy::lifo )
      {
        index = free.back(); // 如果策略是后进先出（lifo）,将free容器中的最后一个元素赋值给index
        free.pop_back();     // 移除free容器中的最后一个元素
      }
      else
      {
        index = free.front(); // 如果策略是先进先出（fifo）,将free容器中的第一个元素赋值给index
        free.pop_front();     // 移除free容器中的第一个元素
      }

      return index;
    }
    /*如果没有空闲索引，则创建一个新的索引，并返回。这是通过 IndexType::from_index 来实现的，每次请求时将 max 值递增*/
    else
    {
      return IndexType::from_index( ++max );
    }
  }

  /*成员函数 release：
  release 函数用于释放索引，将索引放回到空闲索引队列 free 容器的末尾。
  这样可以确保之前请求的索引可以被再次利用*/
  void release( IndexType i )
  {
    free.push_back( i );
  }

private:
  request_strategy strategy;  // 存储请求策略，可以是 LIFO 或 FIFO
  unsigned max = 0u;          // 存储当前最大的索引值，初始为 0
  std::deque<IndexType> free; // 一个双端队列,用于存储可用的索引，根据请求策略不同，在前端或后端进行添加和移除
};

/*C++结构体 compilation_compare，它定义了一个比较器，用于比较MIG的节点*/
struct compilation_compare
{
  compilation_compare( const mockturtle::mig_network& mig, bool enable = true )
      : mig( mig ),
        _fanout_levels( mig_num_vertices( mig ) ), // 用于获取图中的顶点数
        enable( enable )
  {

    fanouts = precompute_ingoing_edges_and_degrees( mig ); // 用于预先计算图中每个顶点的入边（ingoing edges）.
    _fanout_count = mig_precompute_in_degrees( mig ); // 计算图中每个顶点的入度，入度表示有多少条边指向该顶点
    // levels = compute_levels( mig, max_level );    // 计算多输入图（MIG）中各个节点的级别（level）

    // 创建 depth_view 实例
    mockturtle::depth_view<mockturtle::mig_network> mig_depth{ mig };
    // 获取网络的最大深度
    auto max_level = mig_depth.depth();
    // 获取每个节点的级别
    std::map<mockturtle::mig_network::node, unsigned> levels;
    mig.foreach_node( [&]( auto const& n )
                      { levels[n] = mig_depth.level( n ); } );

    // 收集所有节点
    std::vector<mockturtle::mig_network::node> nodes;
    mig.foreach_node( [&]( auto const& n )
                      { nodes.push_back( n ); } );

    // 计算 fanout_levels
    std::transform(
        nodes.begin(), nodes.end(), _fanout_levels.begin(),
        std::bind( &compilation_compare::fanout_levels, this, std::placeholders::_1 ) );
  }

  bool operator()( mockturtle::mig_network::node a, mockturtle::mig_network::node b ) const
  {
    if ( !enable )
    {
      return a > b;
    }

    /* return "false" means a is preferred over b */
    /* return "true" means a is worse than b */
    const auto nfa = number_of_releasing_fanins( a ); // 获取 a 和 b 的释放输入数量，并将结果分别存储在 nfa 和 nfb 中。
    //std::cout << "nfa:  " << nfa << std::endl;
    const auto nfb = number_of_releasing_fanins( b );
    //std::cout << "nfb:  " << nfb << std::endl;
    if ( nfa != nfb )
    {
      return nfa < nfb;
    }

    const auto fla = _fanout_levels[a]; // 获取节点 a 的扇出级别范围, fla.first 和 fla.second 分别访问节点 a 的最小和最大扇出级别
    const auto flb = _fanout_levels[b];

    /* last resort, nodes are incomparable, just pick id */
    return a > b;
  }
  inline unsigned fanout_count( mockturtle::mig_network::node a ) const { return _fanout_count.at( a ); } // 用于获取节点 a 的扇出数量
  // inline unsigned fanout_count( mig_network::node a ) const { return _fanout_count[a]; } // 用于获取节点 a 的扇出数量
  inline unsigned remove_fanout( mockturtle::mig_network::node a ) { return --_fanout_count[a]; } // 用于减少节点 a 的扇出数量，并返回减少后的值

  std::vector<mockturtle::mig_network::signal> mig_get_children( const mockturtle::mig_network& mig, mockturtle::mig_network::node const& n ) const
  {
    std::vector<mockturtle::mig_network::signal> children;
    mig.foreach_fanin( n, [&children]( auto const& f )
                       { children.push_back( f ); } );
    return children;
  }

private:
  mockturtle::mig_network::node get_node3( mockturtle::mig_network::signal const& f ) const
  {
    return f.index;
  }

  /* number_of_releasing_fanins 的成员函数，其目的是计算MIG（多输入图）中指定节点 a 的释放输入数（releasing fan-ins）的数量*/
  unsigned number_of_releasing_fanins( mockturtle::mig_network::node a ) const
  {
    auto sum = 0u; // 计算释放输入数
    auto children3 = mig_get_children( mig, a );
    /*用于遍历节点 a 的子节点。get_children(mig, a) 函数用于获取节点 a 的子节点列表，并循环遍历这些子节点*/
    for ( const auto& c : children3 )
    {
      /*对每个子节点 c 执行条件判断。这里检查 _fanout_count 数组中与子节点的 node 相关联的值是否等于1。如果等于1，表示该子节点只有一个输出引用，这意味着它是释放输入*/
      if (_fanout_count.at(get_node3(c)) == 1u)
      //if ( _fanout_count[c.node] == 1u )
      {
        ++sum;
      }
    }
    return sum;
  }

  /* 计算MIG中指定节点 a 的输出引用的层级范围 */
  std::pair<unsigned, unsigned> fanout_levels( mockturtle::mig_network::node a ) const
  {
    auto min = 0u;
    auto max = max_level;

    // 在 fanouts 的数据结构中查找与节点 a 相关的输出引用
    const auto it = fanouts.find( a );

    if ( it != fanouts.end() )
    {
      for ( const auto& e : it->second )
      {
        // 查找父节点的层级信息
        const auto it2 = levels.find( e );
        if ( it2 == levels.end() )
          continue;

        const auto level = it2->second;
        min = std::min( min, level );
        max = std::max( max, level );
      }
    }

    return std::make_pair( min, max );
  }

private:
  const mockturtle::mig_network& mig;
  std::unordered_map<mockturtle::mig_network::node, std::vector<mockturtle::mig_network::node>> fanouts;
  std::vector<std::pair<unsigned, unsigned>> _fanout_levels;
  std::unordered_map<mockturtle::mig_network::node, size_t> _fanout_count;
  unsigned max_level;
  std::map<mockturtle::mig_network::node, unsigned> levels;
  bool enable = true;
};

template<typename Ntk>
bool all_children_computed( typename Ntk::node n, const Ntk& ntk, const std::vector<bool>& computed )
{
  bool all_computed = true;
  ntk.foreach_fanin( n, [&]( const auto& fanin )
                     {
    auto child = ntk.get_node(fanin);
    if (!computed[child])
    {
      all_computed = false;
      return false; // 终止循环
    }
     return true; } );
  return all_computed;
}

/*根据 x 的值，返回一个二元组，其中第一个元素表示在 0 和 2 之间除了 x 以外的另一个数字，第二个元素表示在 0 和 2 之间除了 x 和第一个元素以外的另一个数字*/
inline std::pair<unsigned, unsigned> three_without( unsigned x )
{
  return std::make_pair( x == 0u ? 1u : 0u, x == 2u ? 1u : 2u );
}

} // namespace cirkit

namespace std
{

template<>
struct hash<mockturtle::mig_network::signal>
{
  std::size_t operator()( mockturtle::mig_network::signal const& f ) const
  {
    return std::hash<unsigned>()( ( f.index << 1u ) | f.complement );
  }
}; /* hash */

} // namespace std

namespace also
{

/******************************************************************************
 * Public functions                                                           *
 ******************************************************************************/
std::vector<mockturtle::mig_network::signal> mig_get_children3( const mockturtle::mig_network& mig, mockturtle::mig_network::node const& n )
{
  std::vector<mockturtle::mig_network::signal> children;
  mig.foreach_fanin( n, [&]( auto const& f )
                     { 
                       children.push_back( f );
                       //std::cout << "Child signal: " << f.index << " complemented: " << mig.is_complemented(f) << std::endl;
                     } );
  //std::cout << "Children size: " << children.size() << std::endl;
  return children;
}

//inline const mig_graph_info& mig_info( const mig_network& mig ) { return boost::get_property( mig, boost::graph_name ); }


// 结构体，用于存储 MIG 网络的信息
struct mig_network_info
{
  mockturtle::mig_network::node constant;
  std::vector<mockturtle::mig_network::node> inputs;
};

// 函数，提取 MIG 网络的信息
mig_network_info mig_info( const mockturtle::mig_network& mig )
{
  mig_network_info info;

  // 获取常数节点
  info.constant = mig.get_constant( false ).index; // Extract the node from the signal

  // 获取所有输入节点
  mig.foreach_pi( [&]( auto node )
                  { info.inputs.push_back( node ); } );

  return info;
}

void set_bit( std::vector<bool>& bitset, size_t pos, bool val = true )
{
  assert( pos < bitset.size() );
  bitset[pos] = val;
}

plim_program
compile_for_plim( const mockturtle::mig_network& mig,
                  const properties::ptr& settings,
                  const properties::ptr& statistics )
{
  /* settings */
  const auto verbose = get( settings, "verbose", false );
  //const auto progress = get( settings, "progress", false );
  const auto enable_cost_function = get( settings, "enable_cost_function", true );
  const auto generator_strategy = get( settings, "generator_strategy", 0u ); /* 0u: LIFO, 1u: FIFO */

  /* timing */
  //properties_timer t( statistics ); // 统计编译过程的时间

  plim_program program; // 存储编译后的 pliM 程序

  const auto& info = mig_info( mig ); // 调用 mig_info 函数获取了 mig 网络的信息，并将其保存到 info 变量中

  std::vector<bool> computed( mig_num_vertices( mig ), false );       // 记录哪些节点已经计算过
  //boost::dynamic_bitset<> computed( mig_num_vertices( mig ) );        // 记录哪些节点已经计算过。它的大小与 mig 网络中的顶点数量相同
  std::unordered_map<mockturtle::mig_network::signal, memristor_index> func_to_rram; // 用于将每个 mig_function 对象映射到一个 memristor_index 对象。这个映射关系将在编译过程中使用
  auto_index_generator<memristor_index> memristor_generator(
      generator_strategy == 0u
          ? auto_index_generator<memristor_index>::request_strategy::lifo
          : auto_index_generator<memristor_index>::request_strategy::fifo ); // 创建了一个名为 memristor_generator 的 auto_index_generator<memristor_index> 对象，用于生成 memristor_index 类型的索引。这个对象的构造函数根据 generator_strategy 的值决定使用 LIFO 还是 FIFO 策略

  /* constant and all PIs are computed */
  /*标记常数节点和输入节点已经计算过，并为每个输入节点创建一个映射关系*/
  set_bit( computed, info.constant ); // 将 computed 中的第 info.constant 位设置为 1，表示常数节点已经计算过

  for ( const auto& input : info.inputs )
  {
    set_bit( computed, input );                                                 // 将 computed 中的第 info.input 位设置为 1，表示输入节点已经计算过
    func_to_rram.insert( { { input, false }, memristor_generator.request() } ); // 将每个输入节点映射到一个唯一的 memristor_index 对象
  }

  /* keep a priority queue for candidates
     invariant: candidates elements' children are all computed */
  compilation_compare cmp( mig, enable_cost_function ); // 定义了一个名为compilation_compare的比较函数对象cmp，用于比较两个mig_node对象的优先级

  std::priority_queue<mockturtle::mig_network::node, std::vector<mockturtle::mig_network::node>, compilation_compare> candidates( cmp );

  //std::cerr << "Generated candidate nodes: " << std::endl;
  mig.foreach_node( [&]( auto node )
                    {
    if (mig.is_pi(node)||mig.is_constant(node)) {
      //std::cerr << "Skipping PI or constant node. Node index: " << node << std::endl;
      return; // Skip nodes with no outgoing edges
    }
    if (all_children_computed(node, mig, computed)) {
        candidates.push(node);
        //std::cerr << "Candidate node: " << node << std::endl;
    } } );

  const auto parent_edges = precompute_ingoing_edges_and_degrees( mig ); // 保存每个节点的所有父节点
  /* synthesis loop */
  while ( !candidates.empty() )
  {
    /* pick the best candidate */
    auto candidate = candidates.top(); // 获取队列中优先级最高的候选节点，并将其从队列中弹出
    candidates.pop();

    auto children = mig_get_children3( mig, candidate );

    if ( children.empty() )
    {
      continue;
    }

    // 输出子节点的索引
    for ( size_t i = 0; i < children.size(); ++i )
    {
      auto child_node = mig.get_node( children[i] );
      auto child_index = mig.node_to_index( child_node );
      //std::cerr << "Child " << i << " node: " << child_node << " index: " << child_index << std::endl;
    }

    //std::cerr << "Children of node " << candidate << ": ";
    for ( const auto& child : children )
    {
      auto child_node = mig.get_node( child );
      bool complemented = mig.is_complemented( child );
      //std::cerr << child_node << ( complemented ? " (complemented) " : " " ) << " ";
    }
    //std::cerr << std::endl;

    L( "[i] compute node " << candidate ); // 输出一条日志信息，表示正在计算该节点
    std::bitset<3> children_compl;

    for ( size_t i = 0; i < children.size(); ++i )
    {
      children_compl.set( i, mig.is_complemented( children[i] ) );
    }

    // 检查节点是否有反向子节点
    if ( children_compl.count() > 0 )
    {
      //std::cerr << "Node " << candidate << " has complemented children." << std::endl;
      auto complemented_child_index = children_compl._Find_first();
      //std::cerr << "First complemented child index: " << complemented_child_index << std::endl;
      auto result = mig.node_to_index( mig.get_node( children[complemented_child_index] ) );
      //std::cerr << "Result of mig.node_to_index(mig.get_node(children[children_compl._Find_first()])): " << result << std::endl;
    }
    else
    {
      //std::cerr << "Node " << candidate << " has no complemented children." << std::endl;
    }

    /* indexes and registers */
    auto i_src_pos = 3u, i_src_neg = 3u, i_dst = 3u;
    plim_program::operand_t src_pos; // 操作数A
    plim_program::operand_t src_neg; // 操作数B
    memristor_index dst;             // 操作数Z

    /* find the inverter */
    /* if there is one inverter */
    if ( children_compl.count() == 1u )
    {
      i_src_neg = children_compl._Find_first(); // 将i_src_neg设置为补码子节点的索引，通过children_compl.find_first()找到第一个补码子节点的索引

      // 检查补码子节点对应的节点是否为常量0
      if ( mig.node_to_index( mig.get_node( children[i_src_neg] ) ) == 0u )
      //if ( mig.get_node( children[i_src_neg] ).index == 0u )
      {
        src_neg = false;
      }
      else
      {
        src_neg = func_to_rram.at( { mig.node_to_index( mig.get_node( children[i_src_neg] ) ), false } ); // 如果补码子节点不是常量0，则通过func_to_rram映射表，使用补码子节点的索引和false（表示非补码）作为键值，找到对应的RRAM，并将其赋值给src_neg
      }
    }

    else if ( children_compl.count() > 1u && mig.node_to_index( mig.get_node( children[children_compl._Find_first()] ) ) == 0u )
    {
      i_src_neg = children_compl._Find_next( children_compl._Find_first() );             // 如果第一个补码子节点是常量0，则通过children_compl.find_next(children_compl.find_first())找到下一个非常量的补码子节点的索引，并将其赋值给i_src_neg
      src_neg = func_to_rram.at( { mig.node_to_index( mig.get_node( children[i_src_neg] ) ), false } ); // 通过func_to_rram映射表，使用非常量补码子节点的索引和false（表示非补码）作为键值，找到对应的RRAM，并将其赋值给src_neg
    }
    /* if there is no inverter but a constant */
    else if ( children_compl.count() == 0u && mig.node_to_index( mig.get_node( children[0u] ) ) == 0u )
    {
      i_src_neg = 0u;                                 // 操作数B选择常量子节点
      src_neg = !mig.is_complemented( children[0u] ); // 将src_neg设置为常量子节点的反向值
    }
    /* if there are more than one inverters */
    else if ( children_compl.count() > 1u )
    {
      do /* in order to escape early */
      {
        /* pick an input that has multiple fanout */
        /*遍历children_compl数组的前3个元素（假设只有3个输入），找到具有多个扇出的子节点*/
        for ( auto i = 0u; i < 3u; ++i )
        {
          if ( !children_compl[i] )
            continue; // 检查是否为补码子节点

          /*计算该子节点对应的节点的扇出数量，如果大于1，则选择该子节点作为操作数B*/
          if ( cmp.fanout_count( mig.get_node( children[i] ) ) > 1u )
          {
            i_src_neg = i;                                                               // 将其索引赋值给i_src_neg
            src_neg = func_to_rram.at( { mig.get_node( children[i_src_neg] ), false } ); // 通过func_to_rram映射表找到对应的RRAM，并将其赋值给src_neg
            break;
          }
        }

        /*如果在遍历完前3个元素后仍未找到满足条件的子节点，则通过children_compl.find_first()找到第一个补码子节点的索引，并将其赋值给i_src_neg*/
        if ( i_src_neg < 3u )
        {
          break;
        }

        i_src_neg = children_compl._Find_first();
        src_neg = func_to_rram.at( { mig.get_node( children[i_src_neg] ), false } );
      } while ( false );
    }

    /* if there is no inverter */
    else
    {
      do /* in order to escape early */
      {
        /* pick an input that has multiple fanout */
        for ( auto i = 0u; i < 3u; ++i )
        {
          const auto it_reg = func_to_rram.find( { mig.get_node( children[i] ), true } );
          if ( it_reg != func_to_rram.end() )
          {
            i_src_neg = i;
            src_neg = it_reg->second;
            break;
          }
        }

        if ( i_src_neg < 3u )
        {
          break;
        }

        /* pick an input that has multiple fanout */
        for ( auto i = 0u; i < 3u; ++i )
        {
          if ( cmp.fanout_count( mig.get_node( children[i] ) ) > 1u )
          {
            i_src_neg = i;
            break;
          }
        }

        /* or pick the first one */
        if ( i_src_neg == 3u )
        {
          i_src_neg = 0u;
        }

        /* create new register for inversion */
        const auto inv_result = memristor_generator.request();

        program.invert( inv_result, func_to_rram.at( { mig.get_node( children[i_src_neg] ), false } ) );
        func_to_rram.insert( { { mig.get_node( children[i_src_neg] ), true }, inv_result } );
        src_neg = inv_result;
      } while ( false );
    }
    children_compl.reset( i_src_neg );

    /* find the destination */
    unsigned oa, ob;
    std::tie( oa, ob ) = three_without( i_src_neg );

    /* if there is a child with one fan-out */
    /* check whether they fulfill the requirements (non-constant and one fan-out) */
    const auto oa_c = children[oa].index != 0u && cmp.fanout_count( mig.get_node( children[oa] ) ) == 1u;
    const auto ob_c = children[ob].index != 0u && cmp.fanout_count( mig.get_node( children[ob] ) ) == 1u;

    if ( oa_c || ob_c )
    {
      /* first check for complemented cases (to avoid them for last operand) */
      std::unordered_map<mockturtle::mig_network::signal, memristor_index>::const_iterator it;
      if ( oa_c && children[oa].complement && ( it = func_to_rram.find( { mig.get_node( children[oa] ), true } ) ) != func_to_rram.end() )
      {
        i_dst = oa;
        dst = it->second;
      }
      else if ( ob_c && children[ob].complement && ( it = func_to_rram.find( { mig.get_node( children[ob] ), true } ) ) != func_to_rram.end() )
      {
        i_dst = ob;
        dst = it->second;
      }
      else if ( oa_c && !children[oa].complement )
      {
        i_dst = oa;
        dst = func_to_rram.at( { mig.get_node( children[oa] ), false } );
      }
      else if ( ob_c && !children[ob].complement )
      {
        i_dst = ob;
        dst = func_to_rram.at( { mig.get_node( children[ob] ), false } );
      }
    }

    /* no destination found yet? */
    if ( i_dst == 3u )
    {
      /* create new work RRAM */
      dst = memristor_generator.request();

      /* is there a constant (if, then it's the first one) */
      if ( children[oa].index == 0u )
      {
        i_dst = oa;
        program.read_constant( dst, children[oa].complement );
      }
      /* is there another inverter, then load it with that one? */
      else if ( children_compl.count() > 0u )
      {
        i_dst = children_compl._Find_first();
        program.invert( dst, func_to_rram.at( { mig.get_node( children[i_dst] ), false } ) );
      }
      /* otherwise, pick first one */
      else
      {
        i_dst = oa;
        program.assign( dst, func_to_rram.at( { mig.get_node( children[i_dst] ), false } ) );
      }
    }

    /* positive operand */
    i_src_pos = 3u - i_src_neg - i_dst;
    const auto node = mig.get_node( children[i_src_pos] );

    if ( node == 0u )
    {
      if ( mig.is_complemented( children[i_src_pos] ) )
      {
        src_pos = true; // 设置为布尔值 true
      }
      else
      {
        //src_pos = also::plim_program::create_memristor_index( children[i_src_pos].index ); // 设置为 memristor_index
        src_pos = false;
      }
    }
    else if ( children[i_src_pos].complement )
    {
      const auto it_reg = func_to_rram.find( { node, true } );
      if ( it_reg == func_to_rram.end() )
      {
        /* create new register for inversion */
        const auto inv_result = memristor_generator.request();

        program.invert( inv_result, func_to_rram.at( { node, false } ) );
        func_to_rram.insert( { { node, true }, inv_result } );
        src_pos = inv_result;
      }
      else
      {
        src_pos = it_reg->second;
      }
    }
    else
    {
      src_pos = func_to_rram.at( { node, false } );
    }

    program.compute( dst, src_pos, src_neg );
    func_to_rram.insert( { { candidate, false }, dst } );

    /* free free registers */
    for ( const auto& c : children )
    {
      if ( cmp.remove_fanout( mig.get_node( c ) ) == 0u && mig.get_node( c ) != 0u )
      {
        const auto reg = func_to_rram.at( { mig.get_node( c ), false } );
        if ( reg != dst )
        {
          memristor_generator.release( reg );
        }

        const auto it_reg = func_to_rram.find( { mig.get_node( c ), true } );
        if ( it_reg != func_to_rram.end() && it_reg->second != dst )
        {
          memristor_generator.release( it_reg->second );
        }
      }
    }

    /* update computed and find new candidates */
    set_bit( computed, candidate );
    const auto it = parent_edges.find( candidate );
    if ( it != parent_edges.end() ) /* if it has parents */
    {
      //std::cerr << "Candidate node: " << candidate << " has parents: ";
      for ( const auto& parent : it->second )
      {
        //std::cerr << parent << " ";
      }
      //std::cerr << std::endl;

      for ( const auto& parent : it->second )
      {
        //std::cerr << "Parent node: " << parent << std::endl;

        if ( !computed[parent] && all_children_computed( parent, mig, computed ) )
        {
          candidates.push( parent );
        }
      }
    }

    L( "    - src_pos: " << i_src_pos << std::endl
                         << "    - src_neg: " << i_src_neg << std::endl
                         << "    - dst:     " << i_dst << std::endl );

 }
 set( statistics, "step_count", (int)program.step_count() );
 set( statistics, "rram_count", (int)program.rram_count() );

 std::vector<int> write_counts( program.write_counts().begin(), program.write_counts().end() );
 set( statistics, "write_counts", write_counts );

 return program;
}

} // namespace also

