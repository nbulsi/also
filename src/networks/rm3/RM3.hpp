/*!
  \file RM3.hpp
  \brief RM3 logic network implementation
*/

#pragma once

#include <memory>
#include <optional>
#include <stack>
#include <string>

#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operators.hpp>
#include <kitty/partial_truth_table.hpp>

#include <mockturtle/traits.hpp>
#include <mockturtle/utils/algorithm.hpp>
#include <mockturtle/networks/detail/foreach.hpp>
#include <mockturtle/networks/events.hpp>
#include <mockturtle/networks/storage.hpp>
namespace mockturtle
{

struct rm3_storage_data
{
  uint32_t num_pis = 0u;
  uint32_t num_pos = 0u;
  std::vector<int8_t> latches;
  uint32_t trav_id = 0u;
};

/*! \brief RM3 storage container

  RM3 have nodes with fan-in 3.  We split of one bit of the index pointer to
  store a complemented attribute.  Every node has 64-bit of additional data
  used for the following purposes:

  `data[0].h1`: Fan-out size (we use MSB to indicate whether a node is dead)
  `data[0].h2`: Application-specific value
  `data[1].h1`: Visited flag
*/

using rm3_node = regular_node<3, 2, 1>;
using rm3_storage = storage<rm3_node,
                            rm3_storage_data>;

class rm3_network
{
public:
#pragma region Types and constructors
  static constexpr auto min_fanin_size = 3u;
  static constexpr auto max_fanin_size = 3u;

  using base_type = rm3_network;
  using storage = std::shared_ptr<rm3_storage>;
  using node = uint64_t;

  struct signal
  {
    signal() = default;

    signal( uint64_t index, uint64_t complement )
        : complement( complement ), index( index )
    // signal( uint64_t index )
    //     : index( index )    
    {
    }

    explicit signal( uint64_t data )
        : data( data )
    {
    }

    signal( rm3_storage::node_type::pointer_type const& p )
        : complement( p.weight ), index( p.index )
    //: index( p.index )
    {
    }

    union
    {
      struct
      {
        uint64_t complement : 1;
        uint64_t index : 63;
        // uint64_t index : 64;
      };
      uint64_t data;
    };

    signal operator!() const
    {
      return signal( data ^ 1 );
    }

    signal operator+() const
    {
      return { index, 0 };
    }

    signal operator-() const
    {
      return { index, 1 };
    }

    signal operator^( bool complement ) const
    {
      return signal( data ^ ( complement ? 1 : 0 ) );
    }

    bool operator==( signal const& other ) const
    {
      return data == other.data;
    }

    bool operator!=( signal const& other ) const
    {
      return data != other.data;
    }

    bool operator<( signal const& other ) const
    {
      return data < other.data;
    }

    operator rm3_storage::node_type::pointer_type() const
    {
      return { index, complement };
      // return index;
    }

    // std::ostream& operator<<( std::ostream& os, const signal& sig )
    // {
    //   // 在这里定义输出运算符的实现
    //   os << "signal content"; // 例如，输出 signal 的内容
    //   return os;
    // }

#if __cplusplus > 201703L
    bool operator==( rm3_storage::node_type::pointer_type const& other ) const
    {
      return data == other.data;
    }
#endif
  };

  rm3_network()
      : _storage( std::make_shared<rm3_storage>() ),
        _events( std::make_shared<decltype( _events )::element_type>() )
  {
  }

  rm3_network( std::shared_ptr<rm3_storage> storage )
      : _storage( storage ),
        _events( std::make_shared<decltype( _events )::element_type>() )
  {
  }

#pragma endregion

#pragma region Primary I / O and constants
  signal get_constant( bool value ) const
  {
    return { 0, static_cast<uint64_t>( value ? 1 : 0 ) };
  }

  // signal get_constant( bool value ) const
  // {
  //   if ( value )
  //   {
  //     return { 1, 0 };
  //   }
  //   else
  //   {
  //     return { 0, 0 };
  //   }
  // }

  signal create_pi( std::string const& name = std::string() )
  {
    (void)name;
    //std::cout << "create_pi前节点大小： " << _storage->nodes.size() << std::endl;
    const auto index = _storage->nodes.size() ; // 获取当前存储中节点的数量作为新节点的索引
    auto& node = _storage->nodes.emplace_back(); // 在存储中新增一个节点，返回对该节点的引用
    node.children[0].data = node.children[1].data = node.children[2].data = ~static_cast<uint64_t>( 0 ); // 设置节点的子节点数据（此处使用了存储的输入数量）
    //node.children[0].data = node.children[1].data = node.children[2].data = _storage->inputs.size();
    _storage->inputs.emplace_back( index );                                                              // 将新创建的节点索引添加到存储的输入列表中
    ++_storage->data.num_pis;
    return { index, 0 }; // 创建一个信号对象，使用新节点的索引和补码为0
  }

  uint32_t create_po( signal const& f, std::string const& name = std::string() )
  {
    (void)name;

    /* increase ref-count to children */
    _storage->nodes[f.index].data[0].h1++;
    auto const po_index = static_cast<uint32_t>( _storage->outputs.size() );
    _storage->outputs.emplace_back( f.index, f.complement );
    ++_storage->data.num_pos;
    return po_index;
  }

  signal create_ro( std::string const& name = std::string() )
  {
    (void)name;
    //std::cout << "create_ro时节点大小： " << _storage->nodes.size() << std::endl;
    auto const index = _storage->nodes.size();
    auto& node = _storage->nodes.emplace_back();
    node.children[0].data = node.children[1].data = node.children[2].data = _storage->inputs.size();
    _storage->inputs.emplace_back( index );
    return { index, 0 };
  }

  uint32_t create_ri( signal const& f, int8_t reset = 0, std::string const& name = std::string() )
  {
    (void)name;

    /* increase ref-count to children */
    _storage->nodes[f.index].data[0].h1++;
    auto const ri_index = static_cast<uint32_t>( _storage->outputs.size() );
    _storage->outputs.emplace_back( f.index, f.complement );
    _storage->data.latches.emplace_back( reset );
    return ri_index;
  }

  int8_t latch_reset( uint32_t index ) const
  {
    assert( index < _storage->data.latches.size() );
    return _storage->data.latches[index];
  }

  bool is_combinational() const
  {
    return ( static_cast<uint32_t>( _storage->inputs.size() ) == _storage->data.num_pis &&
             static_cast<uint32_t>( _storage->outputs.size() ) == _storage->data.num_pos );
  }

  //判断常数节点
  bool is_constant( node const& n ) const
  {
    //return n == 0 || n ==1;
    return n == 0 ;
  }

  bool is_ci( node const& n ) const
  {
    return _storage->nodes[n].children[0].data == _storage->nodes[n].children[1].data && _storage->nodes[n].children[0].data == _storage->nodes[n].children[2].data;
  }

  bool is_pi( node const& n ) const
  {
    return _storage->nodes[n].children[0].data == ~static_cast<uint64_t>( 0 ) && _storage->nodes[n].children[1].data == ~static_cast<uint64_t>( 0 ) && _storage->nodes[n].children[2].data == ~static_cast<uint64_t>( 0 );
  }

  bool is_ro( node const& n ) const
  {
    return _storage->nodes[n].children[0].data == _storage->nodes[n].children[1].data && _storage->nodes[n].children[0].data == _storage->nodes[n].children[2].data && _storage->nodes[n].children[0].data >= static_cast<uint64_t>( _storage->data.num_pis );
  }

  bool constant_value( node const& n ) const
  {
    (void)n;
    return false;
  }

#pragma endregion

#pragma region Create unary functions
  signal create_buf( signal const& a )
  {
    return a;
  }

  signal create_not( signal const& a )
  {
    return !a;
  }
#pragma endregion

#pragma region Create binary / ternary functions
  signal create_rm3( signal a, signal b, signal c )
  {
    /* order inputs */
    if ( a.index > c.index )
    {
      std::swap( a, c );
    }
    // if ( a.index > b.index )
    // {
    //   std::swap( a, b );
    //   if ( b.index > c.index )
    //     std::swap( b, c );
    //   if ( a.index > b.index )
    //     std::swap( a, b );
    // }
    // else
    // {
    //   if ( b.index > c.index )
    //     std::swap( b, c );
    //   if ( a.index > b.index )
    //     std::swap( a, b );
    // }

    /* trivial cases */
    if ( a.index == b.index )
    {
      return ( a.complement == b.complement ) ? c : (a);
      //return c;
    }
    else if ( a.index == c.index || b.index == c.index )
    {
      return ( a.complement == c.complement ) ? a : (!b);
      //return a;
    }
 
    /*  complemented edges minimization */
    auto node_complement = false;
    if ( static_cast<unsigned>( a.complement ) + static_cast<unsigned>( b.complement ) +
             static_cast<unsigned>( c.complement ) >=
         2u )
    {
      node_complement = true;
      a.complement = !a.complement;
      b.complement = !b.complement;
      c.complement = !c.complement;
    }

    storage::element_type::node_type node;
    node.children[0] = a;
    node.children[1] = b;
    node.children[2] = c;

    /* structural hashing */
    const auto it = _storage->hash.find( node );
    if ( it != _storage->hash.end() )
    {
      return { it->second, node_complement };
      //return it->second;
    }
    //std::cout << "create_rm3前节点大小： " << _storage->nodes.size() << std::endl;
    const auto index = _storage->nodes.size();
    //const auto index = _storage->nodes.size() + 1;

    if ( index >= .9 * _storage->nodes.capacity() )
    {
      _storage->nodes.reserve( static_cast<uint64_t>( 3.1415f * index ) );
      _storage->hash.reserve( static_cast<uint64_t>( 3.1415f * index ) );
    }

    _storage->nodes.push_back( node );

    _storage->hash[node] = index;

    /* increase ref-count to children */
    _storage->nodes[a.index].data[0].h1++;
    _storage->nodes[b.index].data[0].h1++;
    _storage->nodes[c.index].data[0].h1++;

    for ( auto const& fn : _events->on_add )
    {
      ( *fn )( index );
    }

    return { index, node_complement };
    //return index;
  }

  // signal create_not( signal const& b )
  // {
  //   return create_rm3( get_constant( false ), b, get_constant( true ) );
  // }

  signal create_rm3_without_complement_opt( signal a, signal b, signal c )
  {
    /* order inputs */
    if ( a.index > c.index )
    {
      std::swap( a, c );
    }
  
    /* trivial cases */
    if ( a.index == b.index )
    {
      return ( a.complement == b.complement ) ? c : ( !b );
      // return c;
    }
    else if ( a.index == c.index || b.index == c.index )
    {
      return ( a.complement == c.complement ) ? a : ( !b );
      // return a;
    }

    /*  complemented edges minimization */
    auto node_complement = false;

    storage::element_type::node_type node;
    node.children[0] = a;
    node.children[1] = b;
    node.children[2] = c;

    /* structural hashing */
    const auto it = _storage->hash.find( node );
    if ( it != _storage->hash.end() )
    {
      return { it->second, node_complement };
      // return it->second;
    }
    // std::cout << "create_rm3前节点大小： " << _storage->nodes.size() << std::endl;
    const auto index = _storage->nodes.size();
    // const auto index = _storage->nodes.size() + 1;

    if ( index >= .9 * _storage->nodes.capacity() )
    {
      _storage->nodes.reserve( static_cast<uint64_t>( 3.1415f * index ) );
      _storage->hash.reserve( static_cast<uint64_t>( 3.1415f * index ) );
    }

    _storage->nodes.push_back( node );

    _storage->hash[node] = index;

    /* increase ref-count to children */
    _storage->nodes[a.index].data[0].h1++;
    _storage->nodes[b.index].data[0].h1++;
    _storage->nodes[c.index].data[0].h1++;

    for ( auto const& fn : _events->on_add )
    {
      ( *fn )( index );
    }

    return { index, node_complement };
    // return index;
  }

  signal create_and( signal const& a, signal const& b )
  {
    return create_rm3( a, get_constant( true ), b );
  }

  signal create_nand( signal const& a, signal const& b )
  {
    return create_not(create_and( a, b ));
  }

  signal create_or( signal const& a, signal const& b )
  {
    return create_rm3( a, get_constant( false ), b );
  }

  signal create_nor( signal const& a, signal const& b )
  {
    return create_not( create_or( a, b ) );
  }

  signal create_lt( signal const& a, signal const& b )
  {
    return create_rm3( get_constant( false ), a, b );
  }

  signal create_le( signal const& a, signal const& b )
  {
    return create_rm3( get_constant( true ), a, b );
  }

  // signal create_xor( signal const& a, signal const& b )
  // {
  //   const auto fcompl = a.complement ^ b.complement;
  //   const auto c1 = create_and( +a, -b );
  //   const auto c2 = create_and( +b, -a );
  //   return create_and( create_not( c1 ), create_not( c2 ) ) ^ !fcompl;
  // }

  signal create_dir_not( signal const& b )
  {
    return !b;
  }

  signal create_dir_nor( signal const& a, signal const& b )
  {
    return !create_or( a, b );
  }

  signal create_dir_nand( signal const& a, signal const& b )
  {
    return !create_and( a, b );
  }

  signal create_ite( signal cond, signal f_then, signal f_else )
  {
    bool f_compl{ false };
    if ( f_then.index < f_else.index )
    {
      std::swap( f_then, f_else );
      cond.complement ^= 1;
    }
    if ( f_then.complement )
    {
      f_then.complement = 0;
      f_else.complement ^= 1;
      f_compl = true;
    }

    return create_and( !create_and( !cond, f_else ), !create_and( cond, f_then ) ) ^ !f_compl;
  }

  signal create_xor( signal const& a, signal const& b )
  {
    const auto f = create_rm3( get_constant( false ), a, b );
    const auto g = create_rm3( a, b, get_constant( false ) );
    return create_rm3( f, get_constant( false ), g );
  }

  signal create_xor3( signal const& a, signal const& b, signal const& c )
  {
    const auto f = create_rm3( b, a, c );
    const auto g = create_rm3( a, c, b );
    return create_rm3( f, b, g );
  }

  // derive rm3 operations from AIG nodes
  // op1: f =  a & !b = rm3(0!yx)
  // op2: f =  (a + b) = rm3(x!0y)
  // op3: f =  a & b = rm3(x!1y)
  // op4: f =  !a & b = rm3(0!xy)
  // op5: f =  (a + !b) = rm3(1!yx)
  // op6: f =  (!a + b) = rm3(1!xy)
  signal create_op1( signal const& a, signal const& b )
  {    
    return create_rm3( get_constant( false ), b, a );
  }

  signal create_op2( signal const& a, signal const& b )
  {
    return create_rm3( a, get_constant( false ), b );
  }

  signal create_op3( signal const& a, signal const& b )
  {
    return create_rm3( a, get_constant( true ), b );
  }

  signal create_op4( signal const& a, signal const& b )
  {
    return create_rm3( get_constant( false ), a, b );
  }

  signal create_op5( signal const& a, signal const& b )
  {
    return create_rm3( get_constant( true ), b, a );
  }

  signal create_op6( signal const& a, signal const& b )
  {
    return create_rm3( get_constant( true ), a, b );
  }

  // derive rm3 operations from MIG nodes
  // op7: f =  <a!bc> = rm3(abc)
  // op8: f =  <ab!c> = rm3(acb)
  // op9: f =  <!abc> = rm3(bac)
  signal create_maj( signal const& a, signal const& b, signal const& c )
  {
    return create_rm3( a, create_not( b ), c );
  }

  signal create_op7( signal const& a, signal const& b, signal const& c )
  {
    return create_rm3( a, b, c );
  }

  signal create_op8( signal const& a, signal const& b, signal const& c )
  {
    return create_rm3( a, c, b );
  }

  signal create_op9( signal const& a, signal const& b , signal const& c )
  {
    return create_rm3( b, a, c );
  }

#pragma endregion

#pragma region Create nary functions
  signal create_nary_and( std::vector<signal> const& fs )
  {
    return tree_reduce( fs.begin(), fs.end(), get_constant( true ), [this]( auto const& a, auto const& b )
                        { return create_and( a, b ); } );
  }

  signal create_nary_or( std::vector<signal> const& fs )
  {
    return tree_reduce( fs.begin(), fs.end(), get_constant( false ), [this]( auto const& a, auto const& b )
                        { return create_or( a, b ); } );
  }

  signal create_nary_xor( std::vector<signal> const& fs )
  {
    return tree_reduce( fs.begin(), fs.end(), get_constant( false ), [this]( auto const& a, auto const& b )
                        { return create_xor( a, b ); } );
  }
#pragma endregion

#pragma region Create arbitrary functions
  signal clone_node( rm3_network const& other, node const& source, std::vector<signal> const& children )
  {
    (void)other;
    (void)source;
    assert( children.size() == 3u );
    return create_rm3( children[0u], children[1u], children[2u] );
  }
#pragma endregion

#pragma region Restructuring
  std::optional<std::pair<node, signal>> replace_in_node( node const& n, node const& old_node, signal new_signal )
  {
    auto& node = _storage->nodes[n];

    uint32_t fanin = 0u;
    for ( auto i = 0u; i < 4u; ++i )
    {
      if ( i == 3u )
      {
        return std::nullopt;
      }

      if ( node.children[i].index == old_node )
      {
        fanin = i;
        new_signal.complement ^= node.children[i].weight;
        break;
      }
    }

    // determine potential new children of node n
    signal child2 = new_signal;
    signal child1 = node.children[( fanin + 1 ) % 3];
    signal child0 = node.children[( fanin + 2 ) % 3];


    if ( child0.index > child2.index )
    {
      std::swap( child0, child2 );
    }

    assert( child0.index <= child2.index );
    //assert( child1.index <= child2.index );

    // check for trivial cases?
    if ( child0.index == child1.index )
    {
      const auto diff_pol = child0.complement != child1.complement;
      return std::make_pair( n, diff_pol ? (!child1) : child2 );
      //return std::make_pair( n, child2 );
    }
    else if ( child1.index == child2.index  )
    {
      const auto diff_pol = child1.complement != child2.complement;
      return std::make_pair( n, diff_pol ? ( !child1 ) : child0 );
      //return std::make_pair( n, child0 );
    }
    else if ( child0.index == child2.index )
    {
      const auto diff_pol = child0.complement != child2.complement;
      return std::make_pair( n, diff_pol ? ( !child1 ) : child0 );
      //return std::make_pair( n, child0 );
    }

    // node already in hash table
    storage::element_type::node_type _hash_obj;
    _hash_obj.children[0] = child0;
    _hash_obj.children[1] = child1;
    _hash_obj.children[2] = child2;
    if ( const auto it = _storage->hash.find( _hash_obj ); it != _storage->hash.end() && it->second != old_node )
    {
      return std::make_pair( n, signal( it->second, 0 ) );
    }

    // remember before
    const auto old_child0 = signal{ node.children[0] };
    const auto old_child1 = signal{ node.children[1] };
    const auto old_child2 = signal{ node.children[2] };

    // erase old node in hash table
    _storage->hash.erase( node );

    // insert updated node into hash table
    node.children[0] = child0;
    node.children[1] = child1;
    node.children[2] = child2;
    _storage->hash[node] = n;

    // update the reference counter of the new signal
    _storage->nodes[new_signal.index].data[0].h1++;

    for ( auto const& fn : _events->on_modified )
    {
      ( *fn )( n, { old_child0, old_child1, old_child2 } );
    }

    return std::nullopt;
  }

  std::optional<std::pair<node, signal>> replace_in_node_without_complement_opt( node const& n, node const& old_node, signal new_signal )
  {
    auto& node = _storage->nodes[n];

    uint32_t fanin = 0u;
    for ( auto i = 0u; i < 4u; ++i )
    {
      if ( i == 3u )
      {
        return std::nullopt;
      }

      if ( node.children[i].index == old_node )
      {
        fanin = i;
        new_signal.complement ^= node.children[i].weight;
        break;
      }
    }

    // determine potential new children of node n
    signal child2 = new_signal;
    signal child1 = node.children[( fanin + 1 ) % 3];
    signal child0 = node.children[( fanin + 2 ) % 3];

    if ( child0.index > child2.index )
    {
      std::swap( child0, child2 );
    }

    assert( child0.index <= child2.index );
    // assert( child1.index <= child2.index );

    // normalize complemented edges
    auto node_complement = false;

    // check for trivial cases?
    if ( child0.index == child1.index )
    {
      const auto diff_pol = child0.complement != child1.complement;
      return std::make_pair( n, diff_pol ? ( !child1 ) : child2 );
      // return std::make_pair( n, child2 );
    }
    else if ( child1.index == child2.index )
    {
      const auto diff_pol = child1.complement != child2.complement;
      return std::make_pair( n, diff_pol ? ( !child1 ) : child0 );
      // return std::make_pair( n, child0 );
    }
    else if ( child0.index == child2.index )
    {
      const auto diff_pol = child0.complement != child2.complement;
      return std::make_pair( n, diff_pol ? ( !child1 ) : child0 );
      // return std::make_pair( n, child0 );
    }

    // node already in hash table
    storage::element_type::node_type _hash_obj;
    _hash_obj.children[0] = child0;
    _hash_obj.children[1] = child1;
    _hash_obj.children[2] = child2;
    if ( const auto it = _storage->hash.find( _hash_obj ); it != _storage->hash.end() && it->second != old_node )
    {
      return std::make_pair( n, signal( it->second, 0 ) );
    }

    // remember before
    const auto old_child0 = signal{ node.children[0] };
    const auto old_child1 = signal{ node.children[1] };
    const auto old_child2 = signal{ node.children[2] };

    // erase old node in hash table
    _storage->hash.erase( node );

    // insert updated node into hash table
    node.children[0] = child0;
    node.children[1] = child1;
    node.children[2] = child2;
    _storage->hash[node] = n;

    // update the reference counter of the new signal
    _storage->nodes[new_signal.index].data[0].h1++;

    for ( auto const& fn : _events->on_modified )
    {
      ( *fn )( n, { old_child0, old_child1, old_child2 } );
    }

    return std::nullopt;
  }

  void replace_in_outputs( node const& old_node, signal const& new_signal )
  {
    if ( is_dead( old_node ) )
      return;

    for ( auto& output : _storage->outputs )
    {
      if ( output.index == old_node )
      {
        output.index = new_signal.index;
        output.weight ^= new_signal.complement;

        if ( old_node != new_signal.index )
        {
          // increment fan-in of new node
          _storage->nodes[new_signal.index].data[0].h1++;
        }
      }
    }
  }

  void take_out_node( node const& n )
  {
    /* we cannot delete CIs, constants, or already dead nodes */
    if ( n == 0 || is_ci( n ) || is_dead( n ) )
      return;

    auto& nobj = _storage->nodes[n];
    nobj.data[0].h1 = UINT32_C( 0x80000000 ); /* fanout size 0, but dead */
    _storage->hash.erase( nobj );

    for ( auto const& fn : _events->on_delete )
    {
      ( *fn )( n );
    }

    for ( auto i = 0u; i < 3u; ++i )
    {
      if ( fanout_size( nobj.children[i].index ) == 0 )
      {
        continue;
      }
      if ( decr_fanout_size( nobj.children[i].index ) == 0 )
      {
        take_out_node( nobj.children[i].index );
      }
    }
  }

  inline bool is_dead( node const& n ) const
  {
    return ( _storage->nodes[n].data[0].h1 >> 31 ) & 1;
  }

  void substitute_node( node const& old_node, signal const& new_signal )
  {
    std::stack<std::pair<node, signal>> to_substitute;
    to_substitute.push( { old_node, new_signal } );

    while ( !to_substitute.empty() )
    {
      const auto [_old, _new] = to_substitute.top();
      to_substitute.pop();
      //std::cout << "substitute_node前节点大小： " << _storage->nodes.size() << std::endl;
      for ( auto idx = 1u; idx < _storage->nodes.size(); ++idx )
      //for ( auto idx = 2u; idx < _storage->nodes.size(); ++idx )
      {
        if ( is_ci( idx ) || is_dead( idx ) )
          continue; /* ignore CIs */

        if ( const auto repl = replace_in_node( idx, _old, _new ); repl )
        {
          to_substitute.push( *repl );
        }
      }
      /* check outputs */
      replace_in_outputs( _old, _new );

      // reset fan-in of old node
      take_out_node( _old );
    }
  }

  void substitute_node_without_complement_opt( node const& old_node, signal const& new_signal )
  {
    std::stack<std::pair<node, signal>> to_substitute;
    to_substitute.push( { old_node, new_signal } );

    while ( !to_substitute.empty() )
    {
      const auto [_old, _new] = to_substitute.top();
      to_substitute.pop();
      // std::cout << "substitute_node前节点大小： " << _storage->nodes.size() << std::endl;
      for ( auto idx = 1u; idx < _storage->nodes.size(); ++idx )
      // for ( auto idx = 2u; idx < _storage->nodes.size(); ++idx )
      {
        if ( is_ci( idx ) || is_dead( idx ) )
          continue; /* ignore CIs */

        if ( const auto repl = replace_in_node_without_complement_opt( idx, _old, _new ); repl )
        {
          to_substitute.push( *repl );
        }
      }
      /* check outputs */
      replace_in_outputs( _old, _new );

      // reset fan-in of old node
      take_out_node( _old );
    }
  }

  void substitute_node_of_parents( std::vector<node> const& parents, node const& old_node, signal const& new_signal )
  {
    for ( auto& p : parents )
    {
      auto& n = _storage->nodes[p];
      for ( auto& child : n.children )
      {
        if ( child.index == old_node )
        {
          child.index = new_signal.index;
          child.weight ^= new_signal.complement;

          // increment fan-in of new node
          _storage->nodes[new_signal.index].data[0].h1++;

          // decrement fan-in of old node
          _storage->nodes[old_node].data[0].h1--;
        }
      }
    }
    /* check outputs */
    for ( auto& output : _storage->outputs )
    {
      if ( output.index == old_node )
      {
        output.index = new_signal.index;
        output.weight ^= new_signal.complement;

        // increment fan-in of new node
        _storage->nodes[new_signal.index].data[0].h1++;

        // decrement fan-in of old node
        _storage->nodes[old_node].data[0].h1--;
      }
    }
  }
#pragma endregion

#pragma region Structural properties
  auto size() const
  {
    //std::cout << "size节点大小： " << _storage->nodes.size() << std::endl;
    return static_cast<uint32_t>( _storage->nodes.size() );
  }

  auto num_cis() const
  {
    return static_cast<uint32_t>( _storage->inputs.size() );
  }

  auto num_cos() const
  {
    return static_cast<uint32_t>( _storage->outputs.size() );
  }

  uint32_t num_latches() const
  {
    return static_cast<uint32_t>( _storage->data.latches.size() );
  }

  auto num_pis() const
  {
    return _storage->data.num_pis;
  }

  auto num_pos() const
  {
    return _storage->data.num_pos;
  }

  // auto num_registers() const
  // {
  //   assert( static_cast<uint32_t>( _storage->inputs.size() - _storage->data.num_pis ) == static_cast<uint32_t>( _storage->outputs.size() - _storage->data.num_pos ) );
  //   return static_cast<uint32_t>( _storage->inputs.size() - _storage->data.num_pis );
  // }

  auto num_gates() const
  {
    return static_cast<uint32_t>( _storage->hash.size() );
  }

  uint32_t fanin_size( node const& n ) const
  {
    if ( is_constant( n ) || is_ci( n ) )
      return 0;
    return 3;
  }

  uint32_t fanout_size( node const& n ) const
  {
    return _storage->nodes[n].data[0].h1 & UINT32_C( 0x7FFFFFFF );
  }

  uint32_t incr_fanout_size( node const& n ) const
  {
    return _storage->nodes[n].data[0].h1++ & UINT32_C( 0x7FFFFFFF );
  }

  uint32_t decr_fanout_size( node const& n ) const
  {
    return --_storage->nodes[n].data[0].h1 & UINT32_C( 0x7FFFFFFF );
  }

  bool is_and( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_or( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_xor( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_rm3( node const& n ) const
  {
    // 判断节点 n 是否为三输入多数逻辑（RM3）类型的节点
    // n > 0,检查节点 n 的索引值是否大于0
    // is_ci(n) 函数来判断节点 n 是否为常量输入节点（Constant Input）
    //return n > 1 && !is_ci( n );
    return n > 0 && !is_ci( n );
  }

  bool is_maj( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_ite( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_xor3( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_nary_and( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_nary_or( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_nary_xor( node const& n ) const
  {
    (void)n;
    return false;
  }
#pragma endregion

#pragma region Functional properties
  kitty::dynamic_truth_table node_function( const node& n ) const
  {
    (void)n;
    kitty::dynamic_truth_table _rm3( 3 );
    _rm3._bits[0] = 0xb2;
    return _rm3;
  }
#pragma endregion

#pragma region Nodes and signals
  node get_node( signal const& f ) const
  {
    return f.index;
  }

  signal make_signal( node const& n ) const
  {
    return signal( n, 0 );
  }

  bool is_complemented( signal const& f ) const
  {
    return f.complement;
  }

  uint32_t node_to_index( node const& n ) const
  {
    return static_cast<uint32_t>( n );
  }

  node index_to_node( uint32_t index ) const
  {
    return index;
  }

  node ci_at( uint32_t index ) const
  {
    assert( index < _storage->inputs.size() );
    return *( _storage->inputs.begin() + index );
  }

  signal co_at( uint32_t index ) const
  {
    assert( index < _storage->outputs.size() );
    return *( _storage->outputs.begin() + index );
  }

  node pi_at( uint32_t index ) const
  {
    assert( index < _storage->data.num_pis );
    return *( _storage->inputs.begin() + index );
  }

  signal po_at( uint32_t index ) const
  {
    assert( index < _storage->data.num_pos );
    return *( _storage->outputs.begin() + index );
  }

  node ro_at( uint32_t index ) const
  {
    assert( index < _storage->inputs.size() - _storage->data.num_pis );
    return *( _storage->inputs.begin() + _storage->data.num_pis + index );
  }

  signal ri_at( uint32_t index ) const
  {
    assert( index < _storage->outputs.size() - _storage->data.num_pos );
    return *( _storage->outputs.begin() + _storage->data.num_pos + index );
  }

  uint32_t ci_index( node const& n ) const
  {
    assert( _storage->nodes[n].children[0].data == _storage->nodes[n].children[1].data &&
            _storage->nodes[n].children[0].data == _storage->nodes[n].children[2].data );
    return static_cast<uint32_t>( _storage->nodes[n].children[0].data );
  }

  uint32_t co_index( signal const& s ) const
  {
    uint32_t i = -1;
    foreach_co( [&]( const auto& x, auto index )
                {
      if ( x == s )
      {
        i = index;
        return false;
      }
      return true; } );
    return i;
  }

  uint32_t pi_index( node const& n ) const
  {
    assert( _storage->nodes[n].children[0].data == _storage->nodes[n].children[1].data &&
            _storage->nodes[n].children[0].data == _storage->nodes[n].children[2].data );
    assert( _storage->nodes[n].children[0].data < _storage->data.num_pis );

    return static_cast<uint32_t>( _storage->nodes[n].children[0].data );
  }

  uint32_t po_index( signal const& s ) const
  {
    uint32_t i = -1;
    foreach_po( [&]( const auto& x, auto index )
                {
      if ( x == s )
      {
        i = index;
        return false;
      }
      return true; } );
    return i;
  }

  uint32_t ro_index( node const& n ) const
  {
    assert( _storage->nodes[n].children[0].data == _storage->nodes[n].children[1].data &&
            _storage->nodes[n].children[0].data == _storage->nodes[n].children[2].data );
    assert( _storage->nodes[n].children[0].data >= _storage->data.num_pis );

    return static_cast<uint32_t>( _storage->nodes[n].children[0].data - _storage->data.num_pis );
  }

  uint32_t ri_index( signal const& s ) const
  {
    uint32_t i = -1;
    foreach_ri( [&]( const auto& x, auto index )
                {
      if ( x == s )
      {
        i = index;
        return false;
      }
      return true; } );
    return i;
  }

  signal ro_to_ri( signal const& s ) const
  {
    return *( _storage->outputs.begin() + _storage->data.num_pos + _storage->nodes[s.index].children[0].data - _storage->data.num_pis );
  }

  node ri_to_ro( signal const& s ) const
  {
    return *( _storage->inputs.begin() + _storage->data.num_pis + ri_index( s ) );
  }
#pragma endregion

#pragma region Node and signal iterators
  template<typename Fn>
  void foreach_node( Fn&& fn ) const
  {
    //std::cout << "foreach_node前节点大小： " << _storage->nodes.size() << std::endl;
    auto r = range<uint64_t>( _storage->nodes.size() );
    detail::foreach_element_if(
        r.begin(), r.end(),
        [this]( auto n )
        { return !is_dead( n ); },
        fn );
  }

  template<typename Fn>
  void foreach_ci( Fn&& fn ) const
  {
    detail::foreach_element( _storage->inputs.begin(), _storage->inputs.end(), fn );
  }

  template<typename Fn>
  void foreach_co( Fn&& fn ) const
  {
    detail::foreach_element( _storage->outputs.begin(), _storage->outputs.end(), fn );
  }

  template<typename Fn>
  void foreach_pi( Fn&& fn ) const
  {
    detail::foreach_element( _storage->inputs.begin(), _storage->inputs.begin() + _storage->data.num_pis, fn );
  }

  template<typename Fn>
  void foreach_po( Fn&& fn ) const
  {
    detail::foreach_element( _storage->outputs.begin(), _storage->outputs.begin() + _storage->data.num_pos, fn );
  }

  template<typename Fn>
  void foreach_ro( Fn&& fn ) const
  {
    detail::foreach_element( _storage->inputs.begin() + _storage->data.num_pis, _storage->inputs.end(), fn );
  }

  template<typename Fn>
  void foreach_ri( Fn&& fn ) const
  {
    detail::foreach_element( _storage->outputs.begin() + _storage->data.num_pos, _storage->outputs.end(), fn );
  }

  template<typename Fn>
  void foreach_gate( Fn&& fn ) const
  {
    //std::cout << "foreach_gate前节点大小： " << _storage->nodes.size() << std::endl;
    //auto r = range<uint64_t>( 2u, _storage->nodes.size() );
    auto r = range<uint64_t>( 1u, _storage->nodes.size() ); // start from 1 to avoid constant
    detail::foreach_element_if(
        r.begin(), r.end(),
        [this]( auto n )
        { return !is_ci( n ) && !is_dead( n ); },
        fn );
  }

  template<typename Fn>
  void foreach_fanin( node const& n, Fn&& fn ) const
  {
    if ( n == 0 || is_ci( n ) )
      return;

    static_assert( detail::is_callable_without_index_v<Fn, signal, bool> ||
                   detail::is_callable_with_index_v<Fn, signal, bool> ||
                   detail::is_callable_without_index_v<Fn, signal, void> ||
                   detail::is_callable_with_index_v<Fn, signal, void> );

    // we don't use foreach_element here to have better performance
    if constexpr ( detail::is_callable_without_index_v<Fn, signal, bool> )
    {
      if ( !fn( signal{ _storage->nodes[n].children[0] } ) )
        return;
      if ( !fn( signal{ _storage->nodes[n].children[1] } ) )
        return;
      fn( signal{ _storage->nodes[n].children[2] } );
    }
    else if constexpr ( detail::is_callable_with_index_v<Fn, signal, bool> )
    {
      if ( !fn( signal{ _storage->nodes[n].children[0] }, 0 ) )
        return;
      if ( !fn( signal{ _storage->nodes[n].children[1] }, 1 ) )
        return;
      fn( signal{ _storage->nodes[n].children[2] }, 2 );
    }
    else if constexpr ( detail::is_callable_without_index_v<Fn, signal, void> )
    {
      fn( signal{ _storage->nodes[n].children[0] } );
      fn( signal{ _storage->nodes[n].children[1] } );
      fn( signal{ _storage->nodes[n].children[2] } );
    }
    else if constexpr ( detail::is_callable_with_index_v<Fn, signal, void> )
    {
      fn( signal{ _storage->nodes[n].children[0] }, 0 );
      fn( signal{ _storage->nodes[n].children[1] }, 1 );
      fn( signal{ _storage->nodes[n].children[2] }, 2 );
    }
  }
#pragma endregion

#pragma region Value simulation
  template<typename Iterator>
  iterates_over_t<Iterator, bool>
  compute( node const& n, Iterator begin, Iterator end ) const
  {
    (void)end;

    assert( n != 0 && !is_ci( n ) );
    //assert( n != 0 && n != 1 && !is_ci( n ) );

    auto const& c1 = _storage->nodes[n].children[0];
    auto const& c2 = _storage->nodes[n].children[1];
    auto const& c3 = _storage->nodes[n].children[2];

    auto v1 = *begin++;
    auto v2 = *begin++;
    auto v3 = *begin++;
    // 返回值是一个 bool 类型，并且与输入迭代器范围相同。
    return ( ( v1 ^ c1.weight ) && ( ( ~v2 ) ^ c2.weight ) ) || ( ( v3 ^ c3.weight ) && ( v1 ^ c1.weight ) ) || ( ( v3 ^ c3.weight ) && ( ( ~v2 ) ^ c2.weight ) );
  }

  template<typename Iterator>
  iterates_over_truth_table_t<Iterator>
  compute( node const& n, Iterator begin, Iterator end ) const
  {
    (void)end;

    assert( n != 0 && !is_ci( n ) );
    //assert( n != 0 && n != 1 && !is_ci( n ) );

    auto const& c1 = _storage->nodes[n].children[0];
    auto const& c2 = _storage->nodes[n].children[1];
    auto const& c3 = _storage->nodes[n].children[2];

    auto tt1 = *begin++;
    auto tt2 = *begin++;
    auto tt3 = *begin++;
    // 返回值是一个 truth table 类型，并且与输入迭代器范围相同
    // return kitty::ternary_majority( c1.weight ? ~tt1 : tt1, c2.weight ? tt2 : ~tt2, c3.weight ? ~tt3 : tt3 );
    return ( ( c1.weight ? ~tt1 : tt1 ) & ( c2.weight ? tt2 : ~tt2 ) ) | ( ( c3.weight ? ~tt3 : tt3 ) & ( c1.weight ? ~tt1 : tt1 ) ) | ( ( c3.weight ? ~tt3 : tt3 ) & ( c2.weight ? tt2 : ~tt2 ) );
  }

  /*! \brief Re-compute the last block. */
  template<typename Iterator>
  void compute( node const& n, kitty::partial_truth_table& result, Iterator begin, Iterator end ) const
  {
    static_assert( iterates_over_v<Iterator, kitty::partial_truth_table>, "begin and end have to iterate over partial_truth_tables" );

    (void)end;
    assert( n != 0 && !is_ci( n ) );
    //assert( n != 0 && n != 1 && !is_ci( n ) );

    auto const& c1 = _storage->nodes[n].children[0];
    auto const& c2 = _storage->nodes[n].children[1];
    auto const& c3 = _storage->nodes[n].children[2];

    auto tt1 = *begin++;
    auto tt2 = *begin++;
    auto tt3 = *begin++;

    // 断言检查 tt1 是否非空
    assert( tt1.num_bits() > 0 && "truth tables must not be empty" );
    // 断言检查 tt1、tt2 和 tt3 的位数是否相等
    assert( tt1.num_bits() == tt2.num_bits() );
    assert( tt1.num_bits() == tt3.num_bits() );
    // 断言检查 tt1 的位数是否大于等于 result 的位数
    assert( tt1.num_bits() >= result.num_bits() );
    // 断言检查 result 的块数是否与 tt1 的块数相等，或者 result 的块数是否比 tt1 的块数少1，并且 result 的位数是否为64的倍数。这是因为 kitty::partial_truth_table 使用64位块存储部分真值表的数据
    assert( result.num_blocks() == tt1.num_blocks() || ( result.num_blocks() == tt1.num_blocks() - 1 && result.num_bits() % 64 == 0 ) );

    // 调整 result 的大小，使其与输入的部分真值表 tt1 具有相同的位数
    result.resize( tt1.num_bits() );
    result._bits.back() =
        ( ( c1.weight ? ~tt1._bits.back() : tt1._bits.back() ) & ( c2.weight ? tt2._bits.back() : ~tt2._bits.back() ) ) |
        ( ( c1.weight ? ~tt1._bits.back() : tt1._bits.back() ) & ( c3.weight ? ~tt3._bits.back() : tt3._bits.back() ) ) |
        ( ( c2.weight ? tt2._bits.back() : ~tt2._bits.back() ) & ( c3.weight ? ~tt3._bits.back() : tt3._bits.back() ) );
    result.mask_bits();
  }
#pragma endregion

#pragma region Inverters propagations
  void complement_node( node const& n, std::vector<node> const& parents )
  {
    assert( n != 0 && !is_pi( n ) );

    auto& c1 = _storage->nodes[n].children[0];
    auto& c2 = _storage->nodes[n].children[1];
    auto& c3 = _storage->nodes[n].children[2];

    if ( is_rm3( n ) )
    {
      const auto& xx = c1.index;
      const auto& yy = c2.index;
      const auto& zz = c3.index;
      const auto cx = c1.weight;
      const auto cy = c2.weight;
      const auto cz = c3.weight;

      if ( cx || cz )
      {
        c1.weight = !c1.weight;
        c2.weight = !c2.weight;
        c3.weight = !c3.weight;

        for ( auto& p : parents )
        {
          for ( auto i = 0; i < 3; i++ )
          {
            auto& c = _storage->nodes[p].children[i];
            if ( c.index == n )
            {
              c.weight = !c.weight;
            }
          }
        }

        /* pos */
        for ( auto& output : _storage->outputs )
        {
          if ( output.index == n )
          {
            output.weight = !output.weight;
          }
        }
      }
      else
      {
        c1.weight = c1.weight;
        c2.weight = c2.weight;
        c3.weight = c3.weight;

        for ( auto& p : parents )
        {
          for ( auto i = 0; i < 3; i++ )
          {
            auto& c = _storage->nodes[p].children[i];
            if ( c.index == n )
            {
              c.weight = c.weight;
            }
          }
        }

        /* pos */
        for ( auto& output : _storage->outputs )
        {
          if ( output.index == n )
          {
            output.weight = output.weight;
          }
        }
      }
    }
  }

  std::array<rm3_network::signal, 3> get_children3( rm3_network const& rm3ig, rm3_network::node const& n )
  {
    std::array<rm3_network::signal, 3> children;
    rm3ig.foreach_fanin( n, [&children]( auto const& f, auto i )
                         { children[i] = f; } );
    return children;
  }

  void node_complement( node const& n, std::vector<node> const& parents )
  {
    assert( n != 0 && !is_pi( n ) );

    auto& c1 = _storage->nodes[n].children[0];
    auto& c2 = _storage->nodes[n].children[1];
    auto& c3 = _storage->nodes[n].children[2];

    if ( is_rm3( n ) )
    {
      const auto& xx = c1.index;
      const auto& yy = c2.index;
      const auto& zz = c3.index;
      const auto cx = c1.weight;
      const auto cy = c2.weight;
      const auto cz = c3.weight;
      // if(cx && cy && !cz)
      if ( cx && cy )
      {
        c1.weight = !c1.weight;
        c2.weight = !c2.weight;
        c3.weight = c3.weight;
        std::swap( c1, c2 );
      }
      // else if (!cx && cy && cz)
      else if ( cy && cz )
      {
        c1.weight = c1.weight;
        c2.weight = !c2.weight;
        c3.weight = !c3.weight;
        std::swap( c2, c3 );
      }
      else
      {
        c1.weight = c1.weight;
        c2.weight = c2.weight;
        c3.weight = c3.weight;
      }
    }

    for ( auto& p : parents )
    {
      for ( auto i = 0; i < 3; i++ )
      {
        auto& c = _storage->nodes[p].children[i];
        if ( c.index == n )
        {
          c.weight = c.weight;
        }
      }
    }

    /* pos */
    for ( auto& output : _storage->outputs )
    {
      if ( output.index == n )
      {
        output.weight = output.weight;
      }
    }
  }

  void node_jump_complement( node const& n, std::vector<node> const& parents )
  {
    assert( n != 0 && !is_pi( n ) );

    auto& c1 = _storage->nodes[n].children[0];
    auto& c2 = _storage->nodes[n].children[1];
    auto& c3 = _storage->nodes[n].children[2];

    if ( is_rm3( n ) )
    {
      const auto& xx = c1.index;
      const auto& yy = c2.index;
      const auto& zz = c3.index;
      const auto cx = c1.weight;
      const auto cy = c2.weight;
      const auto cz = c3.weight;
      if ( cz && !cy && !cx )
      //if ( cz && !cy )
      {
        c1.weight = c1.weight;
        c2.weight = c2.weight;
        c3.weight = !c3.weight;
        std::swap( c1, c2 );

        for ( auto& p : parents )
        {
          for ( auto i = 0; i < 3; i++ )
          {
            auto& c = _storage->nodes[p].children[i];
            if ( c.index == n )
            {
              c.weight = !c.weight;
            }
          }
        }

        /* pos */
        for ( auto& output : _storage->outputs )
        {
          if ( output.index == n )
          {
            output.weight = !output.weight;
          }
        }
      }
      else if ( !cz && !cy && cx )
      //else if ( !cy && cx )
      {
        c1.weight = !c1.weight;
        c2.weight = c2.weight;
        c3.weight = c3.weight;
        std::swap( c2, c3 );

        for ( auto& p : parents )
        {
          for ( auto i = 0; i < 3; i++ )
          {
            auto& c = _storage->nodes[p].children[i];
            if ( c.index == n )
            {
              c.weight = !c.weight;
            }
          }
        }

        /* pos */
        for ( auto& output : _storage->outputs )
        {
          if ( output.index == n )
          {
            output.weight = !output.weight;
          }
        }
      }
      else
      {
        c1.weight = c1.weight;
        c2.weight = c2.weight;
        c3.weight = c3.weight;

        for ( auto& p : parents )
        {
          for ( auto i = 0; i < 3; i++ )
          {
            auto& c = _storage->nodes[p].children[i];
            if ( c.index == n )
            {
              c.weight = c.weight;
            }
          }
        }

        /* pos */
        for ( auto& output : _storage->outputs )
        {
          if ( output.index == n )
          {
            output.weight = output.weight;
          }
        }
      }
    }
  }

  void and_inv(node const& n)
  {
    assert( n != 0 && !is_pi( n ) );
    auto& c1 = _storage->nodes[n].children[0];
    auto& c2 = _storage->nodes[n].children[1];
    auto& c3 = _storage->nodes[n].children[2];
    const auto& xx = c1.index;
    const auto& yy = c2.index;
    const auto& zz = c3.index;
    const auto cx = c1.weight;
    const auto cy = c2.weight;
    const auto cz = c3.weight;
    auto tmp = static_cast<int>( c1.weight ) + static_cast<int>( c2.weight ) + static_cast<int>( c3.weight );

    if ( tmp == 1 || tmp == 0)
    {
      if ( !cz && !cy && cx )
      {
        if( yy == 0 )
        {
          c1.weight = !c1.weight;
          c2.weight = !c2.weight;
          c3.weight = c3.weight;
          std::swap( c1, c2 );
        }
      }
      else if ( !cz && cy )
      {
        if ( xx == 0 )
        {
          c1.weight = !c1.weight;
          c2.weight = !c2.weight;
          c3.weight = c3.weight;
          std::swap( c1, c2 );
        }
      }
      else if ( cz && !cy && !cx )
      {
        if ( yy == 0 )
        {
          c1.weight = c1.weight;
          c2.weight = !c2.weight;
          c3.weight = !c3.weight;
          std::swap( c1, c2 );
          std::swap( c2, c3 );
        }
      }
    }
    else
    {
      return;
    }
  }

#pragma endregion

#pragma region Custom node values
  void clear_values() const
  {
    std::for_each( _storage->nodes.begin(), _storage->nodes.end(), []( auto& n )
                   { n.data[0].h2 = 0; } );
  }

  auto value( node const& n ) const
  {
    return _storage->nodes[n].data[0].h2;
  }

  void set_value( node const& n, uint32_t v ) const
  {
    _storage->nodes[n].data[0].h2 = v;
  }

  auto incr_value( node const& n ) const
  {
    return _storage->nodes[n].data[0].h2++;
  }

  auto decr_value( node const& n ) const
  {
    return --_storage->nodes[n].data[0].h2;
  }
#pragma endregion

#pragma region Visited flags
  void clear_visited() const
  {
    std::for_each( _storage->nodes.begin(), _storage->nodes.end(), []( auto& n )
                   { n.data[1].h1 = 0; } );
  }

  auto visited( node const& n ) const
  {
    return _storage->nodes[n].data[1].h1;
  }

  void set_visited( node const& n, uint32_t v ) const
  {
    _storage->nodes[n].data[1].h1 = v;
  }

  uint32_t trav_id() const
  {
    return _storage->data.trav_id;
  }

  void incr_trav_id() const
  {
    ++_storage->data.trav_id;
  }
#pragma endregion

#pragma region General methods
  auto& events() const
  {
    return *_events;
  }
#pragma endregion

public:
  std::shared_ptr<rm3_storage> _storage;
  std::shared_ptr<network_events<base_type>> _events;
  };

  } // namespace mockturtle

namespace std
{

template<>
struct hash<mockturtle::rm3_network::signal>
{
  uint64_t operator()( mockturtle::rm3_network::signal const& s ) const noexcept
  {
    uint64_t k = s.data;
    k ^= k >> 33;
    k *= 0xff51afd7ed558ccd;
    k ^= k >> 33;
    k *= 0xc4ceb9fe1a85ec53;
    k ^= k >> 33;
    return k;
  }
}; /* hash */
} // namespace std