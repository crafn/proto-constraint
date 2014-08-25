#ifndef EQ_LINEARSOLVER_HPP
#define EQ_LINEARSOLVER_HPP

#include "expr.hpp"
#include "util.hpp"
#include "varstorage.hpp"

#include <stdexcept>

#if defined(__DEPRECATED)
#define EQ_DEPRECATED
#undef __DEPRECATED // Hack to silence gcc
#endif

// Enable underlying solver
#define USE_CLP
// or-tools
#include <linear_solver/linear_solver.h>
#undef USE_CLP

// Restore
#if defined(EQ_DEPRECATED)
#undef EQ_DEPRECATED
#define __DEPRECATED
#endif

namespace eq {
namespace op= operations_research;
namespace detail {

template <typename T>
struct MakeLinRel {
	static_assert(!sizeof(T), "Solving for particular expr not implemented");
};

} // detail

/// Drawbacks using LinearSolver
///   - handles only linear equations
///   - no priority support
///   - no integer support (yet)
class LinearSolver {
public:
	static constexpr bool hasPrioritySupport= false;

	LinearSolver()= default;

	void addVar(double& ref);

	/// @todo Normalize relation before calling makeRel
	template <typename T>
	void addRelation(Expr<T> rel)
	{ makeRel(rel); }

	template <typename T>
	void addRelation(Expr<T> rel, int priority)
	{ static_assert(!sizeof(T), "LinearSolver has no priority support"); }

	/// Solve and apply results
	/// @todo Make safe for sequential calls
	void apply();

private:
	template <typename T>
	friend class detail::MakeLinRel;
	
	/// Creates solver constraints matching to expression
	template <typename E, typename... Args>
	auto makeRel(E&& t, Args&&... args)
	-> decltype(detail::MakeLinRel<RemoveRef<E>>::eval(*this, t, std::declval<Args>()...))
	{ return detail::MakeLinRel<RemoveRef<E>>::eval(*this, t, args...); }

	op::MPSolver solver{"solver", op::MPSolver::CLP_LINEAR_PROGRAMMING};
	VarStorage<double, op::MPVariable> vars;
};

namespace detail {

/// @todo Rest of expressions

template <typename T>
struct MakeLinRel<Expr<T>> {
	template <typename... Args>
	static auto eval(LinearSolver& self, Expr<T> e, Args&&... args)
	-> decltype(self.makeRel(e.get(), std::forward<Args>(args)...))
	{
		return self.makeRel(e.get(), std::forward<Args>(args)...);
	}
};

template <typename T, VarType type>
struct MakeLinRel<Var<T, type>> {
	static void eval(
		LinearSolver& self,
		Var<T, type>& v,
		op::MPConstraint* c,
		double coeff= 1.0)
	{
		auto&& model= self.vars.getInfo(v.get()).model;
		c->SetCoefficient(model, coeff);
	}
};

template <typename T1, typename T2>
struct MakeLinRel<BiOp<T1, T2, Add>> {
	static void eval(
		LinearSolver& self,
		BiOp<T1, T2, Add> op,
		op::MPConstraint* c)
	{
		self.makeRel(op.lhs, c);
		self.makeRel(op.rhs, c);
	}
};

template <typename E1>
struct MakeLinRel<BiOp<E1, Expr<Constant<double>>, Eq>> {
	static void eval(LinearSolver& self, BiOp<E1, Expr<Constant<double>>, Eq> op)
	{
		double rhs= op.rhs.get().get();
		auto c= self.solver.MakeRowConstraint(rhs, rhs);
		self.makeRel(op.lhs, c);
	}
};

} // detail
} // eq

#endif // EQ_LINEARSOLVER_HPP
