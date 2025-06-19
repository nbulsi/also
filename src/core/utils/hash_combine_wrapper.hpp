// hash_combine_wrapper.hpp
#ifndef HASH_COMBINE_WRAPPER_HPP
#define HASH_COMBINE_WRAPPER_HPP

#include <boost/container_hash/hash.hpp>

namespace custom
{
template<typename T>
inline void hash_combine( std::size_t& seed, T const& v )
{
  boost::hash_combine( seed, v );
}
} // namespace custom

#endif // HASH_COMBINE_WRAPPER_HPP