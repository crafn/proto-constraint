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

class Priority {
public:
	static Priority makeHard() { return Priority{hardPriority}; }

	Priority(int p)
		: p(p) { }
	
	bool hard() const { return p == hardPriority; }
	int getValue() const { return p; }

private:
	static constexpr int hardPriority= -10000;

	/// @todo Replace with op::IntVar
	int p= hardPriority;
};

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
		makeRel(rel, detail::Priority::makeHard());
	}

	template <typename T>
	void addRelation(Expr<T> rel, int priority)
	{
		makeRel(rel, detail::Priority{priority});
	}

	/// Solve and apply results
	/// @todo Make safe for sequential calls
	void apply()
	{
		/// @todo Should undo this after solving
		auto success_amount= solver.MakeSum(successAmounts)->Var();
		auto optimizer= solver.MakeMaximize(success_amount, 1);

		std::vector<op::IntVar*> vars;
		for (auto&& m : intVars) {
			vars.push_back(m.model);
		}
		auto db= solver.MakePhase(vars, op::Solver::CHOOSE_FIRST_UNBOUND, op::Solver::ASSIGN_CENTER_VALUE);
		solver.NewSearch(db, optimizer);

		if (solver.NextSolution()) {
			// Apparently last solution is the one which has the best success amount
			do {
				// Update solution to actual variables
				for (auto&& var : intVars) {
					assert(var.actual && var.model);
					*var.actual= var.model->Value();
					//std::cout << "Solution: " << var.model->Value() << std::endl;
				}
			} while (solver.NextSolution());
		} else {
			/// @todo Throw
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

	/// @todo Simplify expression trees so that unsupported operations vanish
	/// @todo Check if IntExpr::Var() calls could be omitted
	/// Creates solver constraints matching to expression
	template <typename E>
	auto makeRel(E&& t, detail::Priority p)
	-> Return<decltype(&detail::MakeRel<RemoveRef<E>>::eval)>
	{
		return detail::MakeRel<RemoveRef<E>>::eval(*this, t, p);
	}

	void addSuccessVar(op::IntVar* success, detail::Priority p)
	{
		assert(!p.hard());
		assert(success);
		auto prod= solver.MakeProd(success, p.getValue())->Var();
		successAmounts.push_back(prod);
	}

	// Completely arbitrary
	/// @todo Remove limits
	static constexpr int minInt= -9999;
	static constexpr int maxInt= 9999;

	op::Solver solver{"solver"};
	DynArray<VarInfo<int>> intVars;
	/// Priorization is implemented by maximizing success of constraints
	DynArray<op::IntVar*> successAmounts;
};


namespace detail {

// Expressions to constraints -conversions

template <typename T>
struct MakeRel<Expr<T>> {
	static auto eval(Solver& self, Expr<T> e, Priority p)
	-> decltype(self.makeRel(e.get(), p))
	{
		return self.makeRel(e.get(), p);
	}
};

template <typename T>
struct MakeRel<Var<T>> {
	static op::IntVar* eval(Solver& self, Var<T>& v, Priority p)
	{
		return self.getVarInfo(v.get()).model;
	}
};

template <typename T>
struct MakeRel<Constant<T>> {
	static op::IntVar* eval(Solver& self, Constant<T> v, Priority p)
	{
		/// @todo Not sure if leaks
		return self.solver.MakeIntConst(v.get());
	}
};

template <typename T1, typename T2>
struct MakeRel<BiOp<T1, T2, Add>> {
	static op::IntVar* eval(Solver& self, BiOp<T1, T2, Add> op, Priority p)
	{
		return self.solver.MakeSum(	self.makeRel(op.lhs, p),
									self.makeRel(op.rhs, p))->Var();
	}
};

template <typename T1, typename T2>
struct MakeRel<BiOp<T1, T2, Sub>> {
	static op::IntVar* eval(Solver& self, BiOp<T1, T2, Sub> op, Priority p)
	{
		return self.solver.MakeDifference(	self.makeRel(op.lhs, p),
											self.makeRel(op.rhs, p))->Var();
	}
};

template <typename T1, typename T2>
struct MakeRel<BiOp<T1, T2, Mul>> {
	static op::IntVar* eval(Solver& self, BiOp<T1, T2, Mul> op, Priority p)
	{
		return self.solver.MakeProd(self.makeRel(op.lhs, p),
									self.makeRel(op.rhs, p))->Var();
	}
};

template <typename T1, typename T2>
struct MakeRel<BiOp<T1, T2, Div>> {
	static op::IntVar* eval(Solver& self, BiOp<T1, T2, Div> op, Priority p)
	{
		return self.solver.MakeDiv(	self.makeRel(op.lhs, p),
									self.makeRel(op.rhs, p))->Var();
	}
};

template <typename T>
struct MakeRel<UOp<T, Pos>> {
	static op::IntVar* eval(Solver& self, UOp<T, Pos> op, Priority p)
	{
		return self.makeRel(op.e, p);
	}
};

template <typename T>
struct MakeRel<UOp<T, Neg>> {
	static op::IntVar* eval(Solver& self, UOp<T, Neg> op, Priority p)
	{
		return self.solver.MakeOpposite(self.makeRel(op.e, p))->Var();
	}
};

template <typename T1, typename T2>
struct MakeRel<BiOp<T1, T2, Eq>> {
	static void eval(Solver& self, BiOp<T1, T2, Eq> op, Priority p)
	{
		if (p.hard()) {
			auto cst= self.solver.MakeEquality(	self.makeRel(op.lhs, p),
												self.makeRel(op.rhs, p));
			self.solver.AddConstraint(cst);
		} else {
			auto success=
				self.solver.MakeIsEqualVar(	self.makeRel(op.lhs, p),
											self.makeRel(op.rhs, p));
			self.addSuccessVar(success, p);
		}
	}
};

template <typename T1, typename T2>
struct MakeRel<BiOp<T1, T2, Neq>> {
	static void eval(Solver& self, BiOp<T1, T2, Neq> op, Priority p)
	{
		if (p.hard()) {
			auto constraint= self.solver.MakeNonEquality(	self.makeRel(op.lhs, p),
															self.makeRel(op.rhs, p));
			self.solver.AddConstraint(constraint);
		} else {
			auto success=
				self.solver.MakeIsDifferentVar(	self.makeRel(op.lhs, p),
												self.makeRel(op.rhs, p));
			self.addSuccessVar(success, p);
		}
	}
};

template <typename T1, typename T2>
struct MakeRel<BiOp<T1, T2, Gr>> {
	static void eval(Solver& self, BiOp<T1, T2, Gr> op, Priority p)
	{
		if (p.hard()) {
			auto constraint= self.solver.MakeGreater(	self.makeRel(op.lhs, p),
														self.makeRel(op.rhs, p));
			self.solver.AddConstraint(constraint);
		} else {
			auto success=
				self.solver.MakeIsGreaterVar(	self.makeRel(op.lhs, p),
												self.makeRel(op.rhs, p));
			self.addSuccessVar(success, p);
		}
	}
};

template <typename T1, typename T2>
struct MakeRel<BiOp<T1, T2, Ls>> {
	static void eval(Solver& self, BiOp<T1, T2, Ls> op, Priority p)
	{
		if (p.hard()) {
			auto constraint= self.solver.MakeLess(	self.makeRel(op.lhs, p),
													self.makeRel(op.rhs, p));
			self.solver.AddConstraint(constraint);
		} else {
			auto success=
				self.solver.MakeIsLessVar(	self.makeRel(op.lhs, p),
											self.makeRel(op.rhs, p));
			self.addSuccessVar(success, p);
		}
	}
};

template <typename T1, typename T2>
struct MakeRel<BiOp<T1, T2, Geq>> {
	static void eval(Solver& self, BiOp<T1, T2, Geq> op, Priority p)
	{
		if (p.hard()) {
			auto constraint= self.solver.MakeGreaterOrEqual(self.makeRel(op.lhs, p),
															self.makeRel(op.rhs, p));
			self.solver.AddConstraint(constraint);
		} else {
			auto success=
				self.solver.MakeIsGreaterOrEqualVar(self.makeRel(op.lhs, p),
													self.makeRel(op.rhs, p));
			self.addSuccessVar(success, p);
		}
	}
};

template <typename T1, typename T2>
struct MakeRel<BiOp<T1, T2, Leq>> {
	static void eval(Solver& self, BiOp<T1, T2, Leq> op, Priority p)
	{
		if (p.hard()) {
			auto constraint= self.solver.MakeLessOrEqual(	self.makeRel(op.lhs, p),
															self.makeRel(op.rhs, p));
			self.solver.AddConstraint(constraint);
		} else {
			auto success=
				self.solver.MakeIsLessOrEqualVar(	self.makeRel(op.lhs, p),
													self.makeRel(op.rhs, p));
			self.addSuccessVar(success, p);
		}
	}
};

template <typename T1, typename T2>
struct MakeRel<BiOp<T1, T2, And>> {
	static void eval(Solver& self, BiOp<T1, T2, And> op, Priority p)
	{
		static_assert(isRelation<T1>() && isRelation<T2>(),
				"Invalid exprs for && operator");
		self.makeRel(op.lhs, p);
		self.makeRel(op.rhs, p);
	}
};

template <typename T1, typename T2>
struct MakeRel<BiOp<T1, T2, Or>> {
	static void eval(Solver& self, BiOp<T1, T2, Or> op, Priority p)
	{
		static_assert(!sizeof(T1), "@todo ||");
	}
};

template <typename T>
struct MakeRel<UOp<T, Not>> {
	static op::IntVar* eval(Solver& self, UOp<T, Not> op, Priority p)
	{
		static_assert(!sizeof(T), "@todo Negation");
	}
};

} // detail
} // eq

#endif // EQ_SOLVER_HPP
