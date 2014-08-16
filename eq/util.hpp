#ifndef EQ_UTIL_HPP
#define EQ_UTIL_HPP

#include <algorithm>
#include <cassert>
#include <list>
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
using LinkedList= std::list<Ts...>;
template <typename... Ts>
using Map= std::map<Ts...>;
template <typename T>
using UniquePtr= std::unique_ptr<T>;
template <typename T>
using SharedPtr= std::shared_ptr<T>;
template <typename T>
using WeakPtr= std::weak_ptr<T>;
template <typename T1, typename T2>
constexpr bool isSame() { return std::is_same<T1, T2>::value; }
#define ensure assert

template <typename T>
struct FuncPtr;

template <typename R, typename C, typename... Args>
struct FuncPtr<R (C::*)(Args...)> {
	using Return= R;
};

template <typename R, typename C, typename... Args>
struct FuncPtr<R (C::*)(Args...) const> {
	using Return= R;
};

template <typename R, typename... Args>
struct FuncPtr<R (*)(Args...)> {
	using Return= R;
};
template <typename T>
using Return= typename FuncPtr<T>::Return;

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
LinkedList<Ts...> operator+(LinkedList<Ts...> lhs, const LinkedList<Ts...>& rhs)
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
	ensure(it != c.end());
	c.erase(it);
}

template <typename... Ts, typename E>
void eraseFrom(std::map<Ts...>& c, E&& e)
{
	auto it= c.find(e);
	ensure(it != c.end());
	c.erase(it);
}

template <typename C, typename F>
void eraseIf(C& c, F&& f)
{
	c.erase(std::remove_if(c.begin(), c.end(), f), c.end());
}

template <typename C>
auto backIt(C&& c)
-> decltype(c.begin())
{
	auto it= c.end();
	--it;
	return it;
}

template <bool B>
using EnableIf= typename detail::EnableIf<B>::Type;

template <typename T>
using RemoveRef= typename std::remove_reference<T>::type;

template <typename T>
using RemoveConst= typename std::remove_const<T>::type;


} // eq

#endif // EQ_UTIL_HPP
