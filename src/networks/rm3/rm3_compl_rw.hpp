#ifndef RM3_COMPL_RW_HPP
#define RM3_COMPL_RW_HPP

#include "RM3.hpp"
using namespace mockturtle;

namespace also
{
rm3_network rm3_rewriting( rm3_network& rm3 );
void step_to_expression( const rm3_network& rm3, std::ostream& s, int index );
void rm3_to_expression( std::ostream& s, const rm3_network& rm3 );
std::array<rm3_network::signal, 3> get_children( const rm3_network& rm3, rm3_network::node const& n );
} // namespace also

#endif
