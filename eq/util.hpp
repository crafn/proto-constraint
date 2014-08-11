#ifndef EQ_UTIL_HPP
#define EQ_UTIL_HPP

#include <algorithm>
#include <cassert>
#include <map>
#include <memory>
#include <set>
#include <vector>

namespace eq {
namespace detail {

template <bool B>
struct EnableIf {};

template <>
struct EnableIf<true> { using Type= void; };

} // detail

/// Temp typedefs until using clover-util
template <typename... Ts>
using Set= std::set<Ts...>;
template <typename... Ts>
using DynArray= std::vector<Ts...>;
template <typename... Ts>
using Map= std::map<Ts...>;
template <typename T>
using UniquePtr= std::unique_ptr<T>;
template <typename T>
using SharedPtr= std::shared_ptr<T>;
#define ensure assert

template <typename... Ts>
Set<Ts...> operator+(Set<Ts...> lhs, const Set<Ts...>& rhs)
{
	lhs.insert(rhs.begin(), rhs.end());
	return lhs;
}

template <typename... Ts>
DynArray<Ts...> operator+(DynArray<Ts...> lhs, const DynArray<Ts...>& rhs)
{
	lhs.insert(lhs.end(), rhs.begin(), rhs.end());
	return lhs;
}

template <typename... Ts>
Map<Ts...> operator+(Map<Ts...> lhs, const Map<Ts...>& rhs)
{
	lhs.insert(rhs.begin(), rhs.end());
	return lhs;
}
template <typename C, typename E>
void eraseFrom(C& c, E&& e)
{
	auto it= find(c.begin(), c.end(), e);
	assert(it != c.end());
	c.erase(it);
}

template <typename... Ts, typename E>
void eraseFrom(std::map<Ts...>& c, E&& e)
{
	auto it= c.find(e);
	assert(it != c.end());
	c.erase(it);
}

template <typename C, typename F>
void eraseIf(C& c, F&& f)
{
	c.erase(std::remove_if(c.begin(), c.end(), f), c.end());
}

template <bool B>
using EnableIf= typename detail::EnableIf<B>::Type;

template <typename T>
using RemoveRef= typename std::remove_reference<T>::type;

template <typename T>
using RemoveConst= typename std::remove_const<T>::type;


} // eq

#endif // EQ_UTIL_HPP
