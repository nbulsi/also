/* also: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file img_ff_rewriting.hpp
 *
 * @brief imgplication logic network rewriting for fanout-conflict
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef IMG_FF_REWRITING_HPP
#define IMG_FF_REWRITING_HPP

#include <mockturtle/mockturtle.hpp>
#include "../networks/img/img.hpp"

namespace also
{

  struct img_ff_rewriting_params
  {
    bool allow_area_increase{true};
    bool verbose{false};
  };

  namespace detail
  {
    template<class Ntk>
    class img_ff_rewriting_impl
    {
      public:
        img_ff_rewriting_impl( Ntk& ntk, img_ff_rewriting_params const& ps )
          : ntk( ntk ), ps( ps )
        {
        }

        void run()
        {
          std::cout << "Begin fanout-free rewriting" << std::endl;
        };
      
      private:
        Ntk& ntk;
        img_ff_rewriting_params const& ps;
    };
  }; /* namespace detail*/
  
  template<class Ntk>
  void img_ff_rewriting( Ntk& ntk, img_ff_rewriting_params const& ps = {} )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
    static_assert( has_level_v<Ntk>, "Ntk does not implement the level method" );
    static_assert( has_substitute_node_v<Ntk>, "Ntk does not implement the substitute_node method" );
    static_assert( has_update_levels_v<Ntk>, "Ntk does not implement the update_levels method" );
    static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
    static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
    static_assert( has_clear_values_v<Ntk>, "Ntk does not implement the clear_values method" );
    static_assert( has_set_value_v<Ntk>, "Ntk does not implement the set_value method" );
    static_assert( has_value_v<Ntk>, "Ntk does not implement the value method" );
    static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );

    detail::img_ff_rewriting_impl<Ntk> p( ntk, ps );
    p.run();
  }


}

#endif
