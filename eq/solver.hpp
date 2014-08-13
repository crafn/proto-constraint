#ifndef EQ_SOLVER_HPP
#define EQ_SOLVER_HPP

#include "expr.hpp"
#include "util.hpp"

#include <stdexcept>

#if defined(__DEPRECATED)
#define EQ_DEPRECATED
#undef __DEPRECATED // Hack to silence gcc
#endif

// or-tools
#include <constraint_solver/constraint_solver.h>

// Restore
#if defined(EQ_DEPRECATED)
#undef EQ_DEPRECATED
#define __DEPRECATED
#endif

namespace eq {
namespace op= operations_research;
class Solver;

namespace detail {

template <typename T>
struct MakeRel {
	static_assert(!sizeof(T), "Solving for particular expr not implemented");
};

/// @todo Simplify expression trees so that unsupported operations vanish
/// @todo Check if IntExpr::Var() calls could be omitted
/// Creates solver constraints matching to expression
template <typename E>
static auto makeRel(Solver& self, E&& t)
-> Return<decltype(&MakeRel<RemoveRef<E>>::eval)>
{
	return detail::MakeRel<RemoveRef<E>>::eval(self, t);
}

} // detail

class Solver {
public:

	void addVar(int& ref)
	{
		intVars.emplace_back(&ref, solver.MakeIntVar(minInt, maxInt));
	}

	template <typename T>
	void addRelation(Expr<T> rel)
	{
		detail::makeRel(*this, rel);
	}

	/// Solve and apply results
	void apply()
	{
		std::vector<op::IntVar*> ints;
		for (auto&& m : intVars) {
			ints.push_back(m.model);
		}
		auto db= solver.MakePhase(ints, op::Solver::CHOOSE_FIRST_UNBOUND, op::Solver::ASSIGN_MIN_VALUE);
		solver.NewSearch(db);

		if (solver.NextSolution()) {
			for (auto&& var : intVars) {
				assert(var.actual && var.model);
				*var.actual= var.model->Value();
			}
		} else {
			std::cout << "Solving error, failure count: " << solver.failures() << std::endl;
		}
		
		solver.EndSearch();
	}

private:
	template <typename T>
	friend class detail::MakeRel;
	
	template <typename T>
	struct VarInfo {
		VarInfo()= default;
		VarInfo(T* actual, op::IntVar* model)
			: actual(actual)
			, model(model)
		{ }
		VarInfo(const VarInfo&)= default;
		VarInfo(VarInfo&& other) : VarInfo(other) {}

		VarInfo& operator=(const VarInfo&)= default;
		VarInfo& operator=(VarInfo&& other){
			// std::vector insists using move assign even when its deleted... (gcc 4.8.1)
			return operator=(other); 
		}

		T* actual= nullptr;
		op::IntVar* model= nullptr;
	};

	VarInfo<int>& getVarInfo(const int& ref)
	{
		for (auto&& m : intVars) {
			if (m.actual == &ref)
				return m;
		}

		throw std::runtime_error{"var not found"};
	}

	// Completely arbitrary
	/// @todo Remove limits
	static constexpr int minInt= -9999;
	static constexpr int maxInt= 9999;

	op::Solver solver{"solver"};
	DynArray<VarInfo<int>> intVars;
};


namespace detail {

// Expressions to constraints -conversions

template <typename T>
struct MakeRel<Expr<T>> {
	static auto eval(Solver& self, Expr<T> e)
	-> decltype(makeRel(self, e.get()))
	{
		return makeRel(self, e.get());
	}
};

template <typename T>
struct MakeRel<Var<T>> {
	static op::IntVar* eval(Solver& self, Var<T>& v)
	{
		return self.getVarInfo(v.get()).model;
	}
};

template <typename T>
struct MakeRel<Constant<T>> {
	static op::IntVar* eval(Solver& self, Constant<T> v)
	{
		/// @todo Not sure if leaks
		return self.solver.MakeIntConst(v.get());
	}
};

template <typename T1, typename T2>
struct MakeRel<BiOp<T1, T2, Add>> {
	static op::IntVar* eval(Solver& self, BiOp<T1, T2, Add> e)
	{
		return self.solver.MakeSum(	makeRel(self, e.lhs),
									makeRel(self, e.rhs))->Var();
	}
};

template <typename T1, typename T2>
struct MakeRel<BiOp<T1, T2, Sub>> {
	static op::IntVar* eval(Solver& self, BiOp<T1, T2, Sub> e)
	{
		return self.solver.MakeDifference(	makeRel(self, e.lhs),
											makeRel(self, e.rhs))->Var();
	}
};

template <typename T1, typename T2>
struct MakeRel<BiOp<T1, T2, Mul>> {
	static op::IntVar* eval(Solver& self, BiOp<T1, T2, Mul> e)
	{
		return self.solver.MakeProd(makeRel(self, e.lhs),
									makeRel(self, e.rhs))->Var();
	}
};

template <typename T1, typename T2>
struct MakeRel<BiOp<T1, T2, Div>> {
	static op::IntVar* eval(Solver& self, BiOp<T1, T2, Div> e)
	{
		return self.solver.MakeDiv(	makeRel(self, e.lhs),
									makeRel(self, e.rhs))->Var();
	}
};

template <typename T>
struct MakeRel<UOp<T, Pos>> {
	static op::IntVar* eval(Solver& self, UOp<T, Pos> op)
	{
		return makeRel(self, op.e);
	}
};

template <typename T>
struct MakeRel<UOp<T, Neg>> {
	static op::IntVar* eval(Solver& self, UOp<T, Neg> op)
	{
		return self.solver.MakeOpposite(makeRel(self, op.e))->Var();
	}
};

template <typename T1, typename T2>
struct MakeRel<BiOp<T1, T2, Eq>> {
	static void eval(Solver& self, BiOp<T1, T2, Eq> e)
	{
		auto constraint= self.solver.MakeEquality(	makeRel(self, e.lhs),
													makeRel(self, e.rhs));
		self.solver.AddConstraint(constraint);
	}
};

template <typename T1, typename T2>
struct MakeRel<BiOp<T1, T2, Neq>> {
	static void eval(Solver& self, BiOp<T1, T2, Neq> e)
	{
		auto constraint= self.solver.MakeNonEquality(	makeRel(self, e.lhs),
														makeRel(self, e.rhs));
		self.solver.AddConstraint(constraint);
	}
};

template <typename T1, typename T2>
struct MakeRel<BiOp<T1, T2, Gr>> {
	static void eval(Solver& self, BiOp<T1, T2, Gr> e)
	{
		auto constraint= self.solver.MakeGreater(	makeRel(self, e.lhs),
													makeRel(self, e.rhs));
		self.solver.AddConstraint(constraint);
	}
};

template <typename T1, typename T2>
struct MakeRel<BiOp<T1, T2, Ls>> {
	static void eval(Solver& self, BiOp<T1, T2, Ls> e)
	{
		auto constraint= self.solver.MakeLess(	makeRel(self, e.lhs),
												makeRel(self, e.rhs));
		self.solver.AddConstraint(constraint);
	}
};

template <typename T1, typename T2>
struct MakeRel<BiOp<T1, T2, Geq>> {
	static void eval(Solver& self, BiOp<T1, T2, Geq> e)
	{
		auto constraint= self.solver.MakeGreaterOrEqual(makeRel(self, e.lhs),
														makeRel(self, e.rhs));
		self.solver.AddConstraint(constraint);
	}
};

template <typename T1, typename T2>
struct MakeRel<BiOp<T1, T2, Leq>> {
	static void eval(Solver& self, BiOp<T1, T2, Leq> e)
	{
		auto constraint= self.solver.MakeLessOrEqual(	makeRel(self, e.lhs),
														makeRel(self, e.rhs));
		self.solver.AddConstraint(constraint);
	}
};

template <typename T1, typename T2>
struct MakeRel<BiOp<T1, T2, And>> {
	static void eval(Solver& self, BiOp<T1, T2, And> e)
	{
		static_assert(isRelation<T1>() && isRelation<T2>(),
				"Invalid exprs for && operator");
		MakeRel<T1>::eval(self, e.lhs);
		MakeRel<T2>::eval(self, e.rhs);
	}
};

template <typename T1, typename T2>
struct MakeRel<BiOp<T1, T2, Or>> {
	static void eval(Solver& self, BiOp<T1, T2, Or> e)
	{
		static_assert(!sizeof(T1), "@todo ||");
	}
};

template <typename T>
struct MakeRel<UOp<T, Not>> {
	static op::IntVar* eval(Solver& self, UOp<T, Not> e)
	{
		static_assert(!sizeof(T), "@todo Negation");
	}
};

} // detail
} // eq

#endif // EQ_SOLVER_HPP
