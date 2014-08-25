#ifndef EQ_VAR_HPP
#define EQ_VAR_HPP

#include "basevar.hpp"
#include "domain.hpp"

namespace eq {
namespace detail {

template <typename T>
struct ChooseSolver { static_assert(!sizeof(T), "Type not supported"); };
template <>
struct ChooseSolver<int> { using Type= ConstraintSolver; };
template <>
struct ChooseSolver<double> { using Type= LinearSolver; };

} // detail

template <typename T>
using ChooseSolver= typename detail::ChooseSolver<T>::Type;

/// Variable that has value determined by constraints
/// Currently supported types are int and double
/// Var<int> uses ConstraintSolver and Var<double> uses LinearSolver
template <typename T, VarType type= VarType::normal>
class Var : public BaseVar {

public:
	/// @todo Maybe solver shouldn't be chosen implicitly
	using Domain= eq::Domain<ChooseSolver<T>>;

	Var()
	{
		/// @todo Create domain as late as possible
		setDomainPtr(std::make_shared<Domain>());
		getDomain().addVar(*this);
	}

	~Var()
	{
		if (getDomainPtr())
			getDomain().removeVar(*this);
	}

	Var(const Var&)= default;
	Var(Var&&)= default;
	Var& operator=(const Var&)= default;
	Var& operator=(Var&&)= default;

	operator const T&() const
	{
		getDomain().solve();
		return value;
	}

	void clear()
	{
		getDomain().removeVar(*this);
		getDomain().addVar(*this);
	}

	/// @todo Could be private
	T& get() { return value; }

private:
	Domain& getDomain() const
	{ return static_cast<Domain&>(BaseVar::getDomain()); }

	T value= 0;
};

} // eq

#endif // EQ_VAR_HPP
