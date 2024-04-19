/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file xag_lut_dec.hpp
 *
 * @brief resynthesis using xag decomposition
 *
 * @author Zhufei
 * @since  0.1
 */

#ifndef XAG_LUT_DEC_HPP
#define XAG_LUT_DEC_HPP

#include "xag_dec.hpp"
#include "build_xag_db.hpp"

namespace mockturtle
{

  template<class Ntk>
  class xag_lut_dec_resynthesis
  {
    public:
      explicit xag_lut_dec_resynthesis( std::unordered_map<std::string, std::string>& opt_xags )
        : opt_xags( opt_xags )
      {
      }
      
      template<typename LeavesIterator, typename Fn>
      void operator()( Ntk& ntk, kitty::dynamic_truth_table const& function, LeavesIterator begin, LeavesIterator end, Fn&& fn ) const
      {
        const auto f = also::xag_dec( ntk, function, std::vector<signal<Ntk>>( begin, end ), opt_xags );
        fn( f );
      }

    private:
      std::unordered_map<std::string, std::string>& opt_xags;
  };


}

#endif
