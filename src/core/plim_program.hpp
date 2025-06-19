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
 * @file plim_program.hpp
 *
 * @brief PLiM program
 *
 * @author YanMing
 * @since  0.1
 */

#ifndef PLIM_PROGRAM_HPP
#define PLIM_PROGRAM_HPP

#include <iostream>

//#include <boost/variant.hpp>
#include "utils/index.hpp"
#include <variant>
#include <vector>

namespace also
{

struct memristor_index_tag;

class memristor_index : public base_index<memristor_index_tag>{
public:
  using base_index<memristor_index_tag>::base_index;
  static memristor_index from_index(unsigned i)
  {
    return memristor_index( i );
  }

protected:
  //using base_index::base_index;
  friend class plim_program;
};

//using memristor_index = base_index<memristor_index_tag>;

class plim_program
{
public:

  using operand_t = std::variant<memristor_index, bool>;
  static memristor_index create_memristor_index(unsigned i){
    return memristor_index( i );
  }
  using instruction_t = std::tuple<operand_t, operand_t, memristor_index>; // 表示一条指令，包括操作数和目标索引

public:
  void read_constant( memristor_index dest, bool value ); // 用于将常量值写入到指定的目标索引位置
  void invert( memristor_index dest, memristor_index src ); // 用于执行取反操作，将源索引位置的值取反，并写入到目标索引位置
  void assign( memristor_index dest, memristor_index src ); // 用于执行赋值操作，将源索引位置的值直接写入到目标索引位置
  void compute( memristor_index dest, operand_t src_pos, operand_t src_neg ); // 用于执行计算操作，根据给定的操作数，对源索引位置的值进行计算，并写入到目标索引位置

  const std::vector<instruction_t>& instructions() const; // 返回存储的指令向量

  unsigned step_count() const; //返回指令的数量
  unsigned rram_count() const; // 返回RRAM的数量
  const std::vector<unsigned>& write_counts() const; // 返回每个目标索引位置的写入次数向量

private:
  std::vector<instruction_t> _instructions; // 存储指令的向量
  std::vector<unsigned>      _write_counts; //存储每个目标索引位置的写入次数的向量
};

std::ostream& operator<<( std::ostream& os, const plim_program& program );

}

#endif

// Local Variables:
// c-basic-offset: 2
// eval: (c-set-offset 'substatement-open 0)
// eval: (c-set-offset 'innamespace 0)
// End:
